#include "WebviewIPCManager.h"
#include <chrono>
#include <thread>
#include <condition_variable>
#include <wx/log.h>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include "slic3r/GUI/GUI_App.hpp"
namespace webviewIpc {
WebviewIPCManager::WebviewIPCManager(wxWebView* webView)
    : m_webView(webView), m_requestIdCounter(1000), m_running(false) {
        initialize();
}

WebviewIPCManager::~WebviewIPCManager() {
    cleanup();
}

void WebviewIPCManager::initialize() {
    if (m_running) {
        return;
    }
    if(m_webView){
        m_webView->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &WebviewIPCManager::onScriptMessage, this);
    }
    m_running = true;
    startTimeoutChecker();
    
    wxLogMessage("IPC: C++ side initialization complete");
}

void WebviewIPCManager::cleanup() {
    if (!m_running) {
        return;
    }
    m_running = false;
    stopTimeoutChecker();
    
    // Clear pending requests
    std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
    m_pendingRequests.clear();
    
    wxLogMessage("IPC: C++ side cleanup complete");
}
void WebviewIPCManager::onScriptMessage(wxWebViewEvent& event){
    wxString message = event.GetString();
    wxLogMessage("Received message: %s", message);
    onMessageReceived(message.ToUTF8().data());
}
void WebviewIPCManager::onMessageReceived(const std::string& message) {
    try {
        json jsonMessage = parseMessage(message);
        handleMessage(jsonMessage);
    } catch (const std::exception& e) {
        wxLogError("IPC: Failed to parse message: %s", e.what());
    }
}

std::string WebviewIPCManager::serializeMessage(const json& message) {
    return message.dump();
}

json WebviewIPCManager::parseMessage(const std::string& jsonStr) {
    return json::parse(jsonStr);
}

void WebviewIPCManager::handleMessage(const json& message) {
     std::string type;
    if (!message.contains("type")) {
        // wxLogError("IPC: Message missing type field");
        // return;
        type = "request";
    }else{
        type = message["type"];
    }
    
    if (type == "request") {
        handleRequest(message);
    } else if (type == "response") {
        handleResponse(message);
    } else if (type == "event") {
        handleEvent(message);
    } else {
        wxLogWarning("IPC: Unknown message type: %s", type.c_str());
    }
}

void WebviewIPCManager::handleRequest(const json& message) {
    if (!message.contains("id") || !message.contains("method")) {
        wxLogError("IPC: Request message format error");
        return;
    }
    
    std::string id = message["id"];
    std::string method = message["method"];
    json params = message.contains("params") ? message["params"] : json::object();
    
    IPCRequest request(id, method, params);
    
    std::lock_guard<std::mutex> lock(m_requestHandlersMutex);
    
    // Check asynchronous handlers with event sending
    auto asyncWithEventsIt = m_asyncRequestHandlersWithEvents.find(method);
    if (asyncWithEventsIt != m_asyncRequestHandlersWithEvents.end()) {
        asyncWithEventsIt->second(request, 
            [this, id, method](const IPCResult& result) {
                // Convert IPCResult to IPCResponse for sending
                IPCResponse response(id, method, result.code, result.message, result.data);
                sendResponse(response);
            },
            [this, id](const std::string& eventMethod, const json& eventData) {
                sendEvent(eventMethod, eventData, id);
            });
        return;
    }
    
    // Check regular asynchronous handlers
    auto asyncIt = m_asyncRequestHandlers.find(method);
    if (asyncIt != m_asyncRequestHandlers.end()) {
        asyncIt->second(request, [this, id, method](const IPCResult& result) {
            // Convert IPCResult to IPCResponse for sending
            IPCResponse response(id, method, result.code, result.message, result.data);
            sendResponse(response);
        });
        return;
    }
    
    // Check synchronous handlers
    auto syncIt = m_requestHandlers.find(method);
    if (syncIt != m_requestHandlers.end()) {
        IPCResult result = syncIt->second(request);
        // Convert IPCResult to IPCResponse for sending
        IPCResponse response(request.id, request.method, result.code, result.message, result.data);
        sendResponse(response);
        return;
    }
    
    // Handler not found
    IPCResponse errorResponse = IPCResponse::error(id, method, 404, "Method not found");
    sendResponse(errorResponse);
}

void WebviewIPCManager::handleResponse(const json& message) {
    if (!message.contains("id")) {
        wxLogError("IPC: Response message missing id field");
        return;
    }
    
    std::string id = message["id"];
    
    std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
    auto it = m_pendingRequests.find(id);
    if (it != m_pendingRequests.end()) {
        auto& pendingRequest = it->second;
        
        // Create IPCResult from response message
        int code = message.contains("code") ? message["code"].get<int>() : 0;
        std::string msg = message.contains("message") ? message["message"].get<std::string>() : "";
        json data = message.contains("data") ? message["data"] : json::object();
        IPCResult result(code, msg, data);
        
        // Execute callback
        if (pendingRequest->callback) {
            pendingRequest->callback(result);
        }
        
        // Remove request
        m_pendingRequests.erase(it);
    }
}

void WebviewIPCManager::handleEvent(const json& message) {
    if (!message.contains("method")) {
        wxLogError("IPC: Event message missing method field");
        return;
    }
    
    std::string method = message["method"];
    std::string id = message.contains("id") ? message["id"].get<std::string>() : "";
    json data = message.contains("data") ? message["data"] : json::object();
    
    IPCEvent event(method, data, id);
    
    // First check if there's an associated request event callback
    if (!id.empty()) {
        std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
        auto it = m_pendingRequests.find(id);
        if (it != m_pendingRequests.end() && it->second->hasEventCallback) {
            try {
                it->second->eventCallback(event);
            } catch (const std::exception& e) {
                wxLogError("IPC: Request event callback execution failed %s: %s", method.c_str(), e.what());
            }
        }
    }
    
    // Then handle global event handlers
    std::lock_guard<std::mutex> lock(m_eventHandlersMutex);
    auto it = m_eventHandlers.find(method);
    if (it != m_eventHandlers.end()) {
        for (auto& handler : it->second) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                wxLogError("IPC: Global event handler execution failed %s: %s", method.c_str(), e.what());
            }
        }
    }
}

void WebviewIPCManager::request(const std::string& method, const json& params, 
                        ResponseHandler callback, int timeout) {
    std::string id = generateRequestId();
    
    // Store pending request
    {
        std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
        m_pendingRequests[id] = std::make_unique<PendingRequest>(method, std::move(callback), timeout);
    }
    
    // Build request message
    json message = {
        {"id", id},
        {"method", method},
        {"type", "request"},
        {"params", params}
    };
    
    sendMessage(message);
}

void WebviewIPCManager::requestWithEvents(const std::string& method, const json& params,
                                  ResponseHandler responseCallback, RequestEventHandler eventCallback,
                                  int timeout) {
    std::string id = generateRequestId();
    
    // Store pending request with event callback
    {
        std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
        m_pendingRequests[id] = std::make_unique<PendingRequest>(method, std::move(responseCallback), 
                                                                std::move(eventCallback), timeout);
    }
    
    // Build request message
    json message = {
        {"id", id},
        {"method", method},
        {"type", "request"},
        {"params", params}
    };
    
    sendMessage(message);
}

IPCResult WebviewIPCManager::requestSync(const std::string& method, const json& params, int timeout) {
    std::mutex responseMutex;
    std::condition_variable responseCondition;
    IPCResponse response;
    bool responseReceived = false;
    
    request(method, params, [&](const IPCResult& resp) {
        std::lock_guard<std::mutex> lock(responseMutex);
        // Convert IPCResult to IPCResponse for internal processing
        response = IPCResponse("", method, resp.code, resp.message, resp.data);
        responseReceived = true;
        responseCondition.notify_one();
    }, timeout);
    
    // Wait for response
    std::unique_lock<std::mutex> lock(responseMutex);
    if (!responseCondition.wait_for(lock, std::chrono::milliseconds(timeout), 
                                   [&]() { return responseReceived; })) {
        // Timeout
        return IPCResult::error(-1, "Request timeout");
    }
    
    // Convert back to IPCResult for return
    return IPCResult(response.code, response.message, response.data);
}

void WebviewIPCManager::sendResponse(const IPCResponse& response) {
    json message = {
        {"id", response.id},
        {"method", response.method},
        {"type", "response"},
        {"code", response.code},
        {"message", response.message},
        {"data", response.data}
    };
    
    sendMessage(message);
}

void WebviewIPCManager::sendEvent(const IPCEvent& event) {
    json message = {
        {"method", event.method},
        {"type", "event"},
        {"data", event.data}
    };
    
    if (!event.id.empty()) {
        message["id"] = event.id;
    }
    
    sendMessage(message);
}

void WebviewIPCManager::sendEvent(const std::string& method, const json& data, const std::string& requestId) {
    IPCEvent event(method, data, requestId);
    sendEvent(event);
}

void WebviewIPCManager::onRequest(const std::string& method, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(m_requestHandlersMutex);
    m_requestHandlers[method] = std::move(handler);
}

void WebviewIPCManager::onRequestAsync(const std::string& method, AsyncRequestHandler handler) {
    std::lock_guard<std::mutex> lock(m_requestHandlersMutex);
    m_asyncRequestHandlers[method] = std::move(handler);
}

void WebviewIPCManager::onRequestAsyncWithEvents(const std::string& method, AsyncRequestHandlerWithEvents handler) {
    std::lock_guard<std::mutex> lock(m_requestHandlersMutex);
    m_asyncRequestHandlersWithEvents[method] = std::move(handler);
}

void WebviewIPCManager::offRequest(const std::string& method) {
    std::lock_guard<std::mutex> lock(m_requestHandlersMutex);
    m_requestHandlers.erase(method);
    m_asyncRequestHandlers.erase(method);
    m_asyncRequestHandlersWithEvents.erase(method);
}

void WebviewIPCManager::onEvent(const std::string& method, EventHandler handler) {
    std::lock_guard<std::mutex> lock(m_eventHandlersMutex);
    m_eventHandlers[method].push_back(std::move(handler));
}

void WebviewIPCManager::offEvent(const std::string& method, const EventHandler& handler) {
    std::lock_guard<std::mutex> lock(m_eventHandlersMutex);
    auto it = m_eventHandlers.find(method);
    if (it != m_eventHandlers.end()) {
        auto& handlers = it->second;
        // Note: This requires function object comparison, may need other identification methods in actual use
        handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
            [&handler](const EventHandler& h) {
                // Comparison implementation needed based on actual situation
                return false; // Simplified implementation
            }), handlers.end());
    }
}

std::string WebviewIPCManager::generateRequestId() {
    return "req-" + std::to_string(++m_requestIdCounter);
}

void WebviewIPCManager::sendMessage(const json& message) {
    if (!m_webView) {
        wxLogError("IPC: WebView not initialized");
        return;
    }
    
    // std::string jsonStr = serializeMessage(message);
    wxString strJS = wxString::Format("HandleStudio(%s)", message.dump(-1, ' ', true));
    Slic3r::GUI::wxGetApp().CallAfter([this, strJS] { WebView::RunScript(m_webView, strJS); });
}

void WebviewIPCManager::checkTimeouts() {
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto now = std::chrono::steady_clock::now();
        std::vector<std::string> expiredIds;
        
        {
            std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
            for (const auto& pair : m_pendingRequests) {
                const auto& request = pair.second;
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - request->timestamp).count();
                
                if (elapsed >= request->timeout) {
                    expiredIds.push_back(pair.first);
                }
            }
        }
        
        // Handle timeout requests
        for (const std::string& id : expiredIds) {
            std::lock_guard<std::mutex> lock(m_pendingRequestsMutex);
            auto it = m_pendingRequests.find(id);
            if (it != m_pendingRequests.end()) {
                IPCResult timeoutResult = IPCResult::error(-1, "Request timeout");
                if (it->second->callback) {
                    it->second->callback(timeoutResult);
                }
                m_pendingRequests.erase(it);
            }
        }
    }
}

void WebviewIPCManager::startTimeoutChecker() {
    m_timeoutThread = std::thread(&WebviewIPCManager::checkTimeouts, this);
}

void WebviewIPCManager::stopTimeoutChecker() {
    if (m_timeoutThread.joinable()) {
        m_timeoutThread.join();
    }
}
}