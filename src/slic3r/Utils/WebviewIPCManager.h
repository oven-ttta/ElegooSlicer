#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <wx/webview.h>
#include <nlohmann/json.hpp>

namespace webviewIpc {
using json = nlohmann::json;

/**
 * IPC request structure
 */
struct IPCRequest {
    std::string id;
    std::string method;
    json params;
    
    IPCRequest() = default;
    IPCRequest(const std::string& id, const std::string& method, const json& params)
        : id(id), method(method), params(params) {}
};

struct IPCResult {
    int code = 0;
    std::string message;
    json data;

    IPCResult() = default;
    IPCResult(int code, const std::string& message="", const json& data=json::object())
        : code(code), message(message), data(data) {}
    IPCResult(const json& data) : code(0), data(data) {}
    
    // Create success response
    static IPCResult success(const json& data = json::object()) {
        return IPCResult(0, "success", data);
    }

    // Create error response
    static IPCResult error(int code, const std::string& message) {
        return IPCResult(code, message, json::object());
    }

    static IPCResult error(const std::string& message="error") {
        return IPCResult(1, message);
    }
};

/**
 * IPC response structure
 */
struct IPCResponse {
    std::string id;
    std::string method;
    int code = 0;
    std::string message;
    json data;
    
    IPCResponse() : code(0) {}
    IPCResponse(int code, const std::string& message="") : code(code), message(message) {}
    IPCResponse(const json& data) : code(0), data(data) {}
    IPCResponse(const std::string& id, const std::string& method, int code, 
                const std::string& message, const json& data = json::object())
        : id(id), method(method), code(code), message(message), data(data) {}
    
    // Create success response
    static IPCResponse success(const std::string& id, const std::string& method, const json& data = json::object()) {
        return IPCResponse(id, method, 0, "success", data);
    }

    static IPCResponse success(const json& data = json::object()) {
        return IPCResponse(data);
    }

    // Create error response
    static IPCResponse error(const std::string& id, const std::string& method, int code, const std::string& message) {
        return IPCResponse(id, method, code, message, json::object());
    }

    static IPCResponse error(const std::string& message="") {
        return IPCResponse(1, message);
    }
};

/**
 * IPC event structure
 */
struct IPCEvent {
    std::string id;  // Optional, associated request ID
    std::string method;
    json data;
    
    IPCEvent() = default;
    IPCEvent(const std::string& method, const json& data, const std::string& id = "")
        : id(id), method(method), data(data) {}
};

/**
 * Request handler type definitions
 */
using RequestHandler = std::function<IPCResult(const IPCRequest&)>;
using AsyncRequestHandler = std::function<void(const IPCRequest&, std::function<void(const IPCResult&)>)>;
using AsyncRequestHandlerWithEvents = std::function<void(const IPCRequest&, std::function<void(const IPCResult&)>, std::function<void(const std::string&, const json&)>)>;

using ResponseHandler = std::function<void(const IPCResult&)>;
using EventHandler = std::function<void(const IPCEvent&)>;
using RequestEventHandler = std::function<void(const IPCEvent&)>;

/**
 * Pending request information
 */
struct PendingRequest {
    std::string method;
    ResponseHandler callback;
    RequestEventHandler eventCallback;  // Added: event callback
    std::chrono::steady_clock::time_point timestamp;
    int timeout;
    bool hasEventCallback;  // Added: flag for event callback
    
    PendingRequest(const std::string& method, ResponseHandler callback, int timeout)
        : method(method), callback(std::move(callback)), 
          timestamp(std::chrono::steady_clock::now()), timeout(timeout), hasEventCallback(false) {}
    
    // Added: constructor with event callback
    PendingRequest(const std::string& method, ResponseHandler callback, RequestEventHandler eventCallback, int timeout)
        : method(method), callback(std::move(callback)), eventCallback(std::move(eventCallback)),
          timestamp(std::chrono::steady_clock::now()), timeout(timeout), hasEventCallback(true) {}
};

/**
 * wxWebView IPC Manager
 */
class WebviewIPCManager {
private:
    wxWebView* m_webView;
    std::atomic<int> m_requestIdCounter;
    
    // Store pending requests
    std::map<std::string, std::unique_ptr<PendingRequest>> m_pendingRequests;
    std::mutex m_pendingRequestsMutex;
    
    // Request handlers
    std::map<std::string, RequestHandler> m_requestHandlers;
    std::map<std::string, AsyncRequestHandler> m_asyncRequestHandlers;
    std::map<std::string, AsyncRequestHandlerWithEvents> m_asyncRequestHandlersWithEvents;
    std::mutex m_requestHandlersMutex;
    
    // Event handlers
    std::map<std::string, std::vector<EventHandler>> m_eventHandlers;
    std::mutex m_eventHandlersMutex;
    
    // Timeout check thread
    std::thread m_timeoutThread;
    std::atomic<bool> m_running;
    
    // Message serialization/deserialization
    std::string serializeMessage(const json& message);
    json parseMessage(const std::string& jsonStr);
    
    // JSON parsing helper functions for robustness
    std::string safeGetString(const json& j, const std::string& key, const std::string& defaultValue = "");
    int safeGetInt(const json& j, const std::string& key, int defaultValue = 0);
    json safeGetObject(const json& j, const std::string& key, const json& defaultValue = json::object());
    
    // Internal processing methods
    void handleMessage(const json& message);
    void handleRequest(const json& message);
    void handleResponse(const json& message);
    void handleEvent(const json& message);
    
    // Timeout checking
    void checkTimeouts();
    void startTimeoutChecker();
    void stopTimeoutChecker();

    void onScriptMessage(wxWebViewEvent& event);
public:
    explicit WebviewIPCManager(wxWebView* webView);
    ~WebviewIPCManager();
    
    // Disable copy constructor and assignment
    WebviewIPCManager(const WebviewIPCManager&) = delete;
    WebviewIPCManager& operator=(const WebviewIPCManager&) = delete;
    
    /**
     * Initialize IPC manager
     */
    void initialize();
    
    /**
     * Clean up resources
     */
    void cleanup();
    
    /**
     * Handle messages from JavaScript
     * @param message JSON format message string
     */
    void onMessageReceived(const std::string& message);
    
    /**
     * Send asynchronous request
     * @param method Method name
     * @param params Parameters
     * @param callback Response callback function
     * @param timeout Timeout in milliseconds, default 10 seconds
     */
    void request(const std::string& method, const json& params, 
                ResponseHandler callback, int timeout = 10000);
    
    /**
     * Send asynchronous request with event callback
     * @param method Method name
     * @param params Parameters
     * @param responseCallback Response callback function
     * @param eventCallback Event callback function, receives events related to this request
     * @param timeout Timeout in milliseconds, default 10 seconds
     */
    void requestWithEvents(const std::string& method, const json& params,
                          ResponseHandler responseCallback, RequestEventHandler eventCallback,
                          int timeout = 10000);
    
    /**
     * Send synchronous request (blocking version, not recommended for main thread)
     * @param method Method name
     * @param params Parameters
     * @param timeout Timeout in milliseconds, default 10 seconds
     * @return Response result
     */
    IPCResult requestSync(const std::string& method, const json& params, int timeout = 10000);
    
    /**
     * Send response
     * @param response Response object
     */
    void sendResponse(const IPCResponse& response);
    
    /**
     * Send event
     * @param event Event object
     */
    void sendEvent(const IPCEvent& event);
    
    /**
     * Send event (convenience method)
     * @param method Event name
     * @param data Event data
     * @param requestId Associated request ID (optional)
     */
    void sendEvent(const std::string& method, const json& data, const std::string& requestId = "");
    
    /**
     * Register request handler (synchronous)
     * @param method Method name
     * @param handler Handler function
     */
    void onRequest(const std::string& method, RequestHandler handler);
    
    /**
     * Register request handler (asynchronous)
     * @param method Method name
     * @param handler Asynchronous handler function
     */
    void onRequestAsync(const std::string& method, AsyncRequestHandler handler);
    
    /**
     * Register request handler (asynchronous, with event sending callback)
     * @param method Method name
     * @param handler Asynchronous handler function with event sending callback
     */
    void onRequestAsyncWithEvents(const std::string& method, AsyncRequestHandlerWithEvents handler);
    
    /**
     * Remove request handler
     * @param method Method name
     */
    void offRequest(const std::string& method);
    
    /**
     * Register event handler
     * @param method Event name
     * @param handler Handler function
     */
    void onEvent(const std::string& method, EventHandler handler);
    
    /**
     * Remove event handler
     * @param method Event name
     * @param handler Handler function (must be the same function object)
     */
    void offEvent(const std::string& method, const EventHandler& handler);
    
    /**
     * Generate unique request ID
     */
    std::string generateRequestId();
    
private:
    /**
     * Send message to JavaScript side
     * @param message JSON message object
     */
    void sendMessage(const json& message);
};
}