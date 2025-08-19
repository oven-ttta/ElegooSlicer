#include "PrinterCache.hpp"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>
#include <wx/log.h>
#include <chrono>
#include <exception>
#include <cstdint>
#include "libslic3r/PrinterNetworkInfo.hpp"
#include "libslic3r/Utils.hpp"

namespace Slic3r {

namespace fs = boost::filesystem;

PrinterCache::PrinterCache() {
  
}

PrinterCache::~PrinterCache() {
}

void PrinterCache::init() {
    loadPrinterList();
}

bool PrinterCache::loadPrinterList() {
    std::lock_guard<std::mutex> lock(mMutex);
    fs::path printerListPath = fs::path(Slic3r::data_dir()) / "user" / "printer_list.json";
    // read printer list from file
    std::ifstream ifs(printerListPath.string());
    if (!ifs.is_open()) {
        wxLogError("Failed to open printer list file for reading: %s", printerListPath.string().c_str());
        return false;
    }
    
    try {
        nlohmann::json jsonData;
        ifs >> jsonData;
        
        mPrinters.clear();
    
        for (const auto& [printerId, printerJson] : jsonData.items()) {
            PrinterNetworkInfo printerInfo = convertJsonToPrinterNetworkInfo(printerJson);
            mPrinters[printerId] = printerInfo;
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to load printer list from JSON: %s", e.what());
    }
    return true;
}

bool PrinterCache::savePrinterList() {
    fs::path printerListPath = fs::path(Slic3r::data_dir()) / "user" / "printer_list.json";
    nlohmann::json jsonData;
    for (const auto& [printerId, printerInfo] : mPrinters) {
        jsonData[printerId] = convertPrinterNetworkInfoToJson(printerInfo);
    }
    std::ofstream ofs(printerListPath.string());
    ofs << jsonData.dump(4);
    return true;
}

std::optional<PrinterNetworkInfo> PrinterCache::getPrinter(const std::string& printerId) const {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<PrinterNetworkInfo> PrinterCache::getPrinters() const {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<PrinterNetworkInfo> result;
    result.reserve(mPrinters.size());
    for (const auto& pair : mPrinters) {
        result.push_back(pair.second);
    }
    std::sort(result.begin(), result.end(),
    [](const PrinterNetworkInfo& a, const PrinterNetworkInfo& b) { return a.addTime < b.addTime; });
    return result;
}

bool PrinterCache::addPrinter(const PrinterNetworkInfo& printerInfo) {
    if (printerInfo.printerId.empty()) {
        return false;
    }   
    std::lock_guard<std::mutex> lock(mMutex);
    auto result = mPrinters.emplace(printerInfo.printerId, printerInfo);
    savePrinterList();
    return result.second;
}

bool PrinterCache::updatePrinter(const PrinterNetworkInfo& printerInfo) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerInfo.printerId);
    if (it != mPrinters.end()) {
        it->second = printerInfo;
        savePrinterList();
        return true;
    }
    return false;
}

bool PrinterCache::deletePrinter(const std::string& printerId) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        mPrinters.erase(it);
        savePrinterList();
        return true;
    }
    return false;
}

bool PrinterCache::updatePrinterConnectStatus(const std::string& printerId, const PrinterConnectStatus& status) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        it->second.connectStatus = status;
        return true;
    }
    return false;
}

bool PrinterCache::updatePrinterName(const std::string& printerId, const std::string& name) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        it->second.printerName = name;
        uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        it->second.modifyTime     = now;
        it->second.lastActiveTime = now;
        savePrinterList();
        return true;
    }
    return false;
}

bool PrinterCache::updatePrinterHost(const std::string& printerId, const std::string& host) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        it->second.host = host;
        savePrinterList();
        return true;
    }
    return false;
}

void PrinterCache::updatePrinterStatus(const std::string& printerId, const PrinterStatus& status) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        it->second.printerStatus = status;
        if(status == PrinterStatus::PRINTER_STATUS_IDLE) {
            it->second.printTask.taskId = "";
            it->second.printTask.fileName = "";
            it->second.printTask.totalTime = 0;
            it->second.printTask.currentTime = 0;
            it->second.printTask.estimatedTime = 0;
            it->second.printTask.progress = 0;
        } 
    }
}

void PrinterCache::updatePrinterPrintTask(const std::string& printerId, const PrinterPrintTask& task) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        if(task.taskId.empty() || it->second.printerStatus == PrinterStatus::PRINTER_STATUS_IDLE) {
            it->second.printTask.taskId = "";
            it->second.printTask.fileName = "";
            it->second.printTask.totalTime = 0;
            it->second.printTask.currentTime = 0;
            it->second.printTask.estimatedTime = 0;
            it->second.printTask.progress = 0;
        } else {
            it->second.printTask = task;
        }
    }
}

void PrinterCache::updatePrinterAttributes(const std::string& printerId, const PrinterNetworkInfo& printerInfo) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mPrinters.find(printerId);
    if (it != mPrinters.end()) {
        if(printerInfo.firmwareVersion.empty()) {
            it->second.firmwareVersion = printerInfo.firmwareVersion;
        }
    }
}

} // namespace Slic3r
