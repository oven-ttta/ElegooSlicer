#ifndef slic3r_PrinterNetworkEvent_hpp_
#define slic3r_PrinterNetworkEvent_hpp_

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <variant>
#include <string>
#include "libslic3r/PrinterNetworkInfo.hpp"
#include "Singleton.hpp"

namespace Slic3r {

struct PrinterConnectStatusEvent {
    std::string printerId;
    PrinterConnectStatus status;
    std::chrono::system_clock::time_point timestamp;
    
    PrinterConnectStatusEvent(const std::string& id, const PrinterConnectStatus& s)
        : printerId(id), status(s), timestamp(std::chrono::system_clock::now()) {}
};

struct PrinterStatusEvent {
    std::string printerId;
    PrinterStatus status;
    std::chrono::system_clock::time_point timestamp;
    
    PrinterStatusEvent(const std::string& id, const PrinterStatus& s)
        : printerId(id), status(s), timestamp(std::chrono::system_clock::now()) {}
};

struct PrinterPrintTaskEvent {
    std::string printerId;
    PrinterPrintTask task;
    std::chrono::system_clock::time_point timestamp;
    
    PrinterPrintTaskEvent(const std::string& id, const PrinterPrintTask& t)
        : printerId(id), task(t), timestamp(std::chrono::system_clock::now()) {}
};

struct PrinterAttributesEvent {
    std::string printerId;    
    PrinterNetworkInfo printerInfo;
    std::chrono::system_clock::time_point timestamp;

    PrinterAttributesEvent(const std::string& id, const PrinterNetworkInfo& info)
        : printerId(id), printerInfo(info), timestamp(std::chrono::system_clock::now()) {}
};

using PrinterEvent = std::variant<PrinterConnectStatusEvent, PrinterStatusEvent, PrinterPrintTaskEvent, PrinterAttributesEvent>;

template<typename EventType>
class PrinterSignal {
private:
    std::vector<std::function<void(const EventType&)>> mHandlers;
    std::mutex mHandlersMutex;
    
public:
    // register handler
    void connect(std::function<void(const EventType&)> handler) {
        std::lock_guard<std::mutex> lock(mHandlersMutex);
        mHandlers.push_back(std::move(handler));
    }
    
    // emit event (non-blocking)
    void emit(const EventType& event) {
        std::lock_guard<std::mutex> lock(mHandlersMutex);
        for (auto& handler : mHandlers) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                wxLogError("Exception in event handler: %s", e.what());
            }
        }
    }
    
    // get handler count
    size_t handlerCount() const {
        std::lock_guard<std::mutex> lock(mHandlersMutex);
        return mHandlers.size();
    }
};

// printer network event system
class PrinterNetworkEvent : public Singleton<PrinterNetworkEvent> {
    friend class Singleton<PrinterNetworkEvent>;
public:
    PrinterNetworkEvent(const PrinterNetworkEvent&) = delete;
    PrinterNetworkEvent& operator=(const PrinterNetworkEvent&) = delete;
    
    // type-safe signal
    PrinterSignal<PrinterConnectStatusEvent> connectStatusChanged;
    PrinterSignal<PrinterStatusEvent> statusChanged;
    PrinterSignal<PrinterPrintTaskEvent> printTaskChanged;
    PrinterSignal<PrinterAttributesEvent> attributesChanged;
    
private:
    PrinterNetworkEvent() = default;
    ~PrinterNetworkEvent() = default;
};

} // namespace Slic3r

#endif 