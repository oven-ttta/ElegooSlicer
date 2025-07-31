#include "PrinterManager.hpp"
#include "slic3r/Utils/PrinterNetworkManager.hpp"
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include "libslic3r/PresetBundle.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace Slic3r {
  
VendorProfile getMachineProfile(const std::string& vendorName, const std::string& machineModel, VendorProfile::PrinterModel& printerModel)
{
    std::string profile_vendor_name;    
    VendorProfile machineProfile;
    PresetBundle bundle;
    auto [substitutions, errors] = bundle.load_system_models_from_json(ForwardCompatibilitySubstitutionRule::EnableSilent);
    for (const auto& vendor : bundle.vendors) {
        const auto& vendor_profile = vendor.second;
        if (boost::to_upper_copy(vendor_profile.name) == boost::to_upper_copy(vendorName)) {
            for (const auto& model : vendor_profile.models) {
                if (boost::to_upper_copy(model.name).find(boost::to_upper_copy(machineModel)) != std::string::npos) {
                    machineProfile = vendor_profile;
                    printerModel = model;
                    break;
                }
            }
            break;
        }
    }  
    return machineProfile;
}


PrinterManager::PrinterManager()
{
    loadPrinterList();
}

PrinterManager::~PrinterManager() {
    savePrinterList();
}

void PrinterManager::loadPrinterList()
{
    fs::path printerListPath = boost::filesystem::path(Slic3r::data_dir()) / "user" / "printer_list";
    if (!boost::filesystem::exists(printerListPath)) {
        mPrinterList.clear();
        return;
    }
    std::ifstream ifs(printerListPath.string(), std::ios::binary);
    if (!ifs.is_open()) {
        wxLogError("open printer list file failed: %s", printerListPath.string().c_str());
        mPrinterList.clear();
        return;
    }
    try {
        cereal::BinaryInputArchive iarchive(ifs);
        iarchive(mPrinterList);
    } catch (const std::exception& e) {
        wxLogError("printer list deserialize failed: %s", e.what());
        mPrinterList.clear();
    }
}
void PrinterManager::savePrinterList()
{
    fs::path printerListPath = boost::filesystem::path(Slic3r::data_dir()) / "user" / "printer_list";
    std::ofstream ofs(printerListPath.string(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(ofs);
    oarchive(mPrinterList);
}
bool PrinterManager::deletePrinter(const std::string& id)
{
    auto it = mPrinterList.find(id);
    if (it != mPrinterList.end()) {
        mPrinterList.erase(it);
        savePrinterList();
        return true;
    }
    return false;
}
bool PrinterManager::updatePrinterName(const std::string& id, const std::string& name)
{
    auto it = mPrinterList.find(id);
    if (it != mPrinterList.end()) {
        it->second.name = name;
        uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        it->second.modifyTime = now;
        it->second.lastActiveTime = now;
        savePrinterList();
        return true;
    }
    return false;
}
std::string PrinterManager::bindPrinter(PrinterNetworkInfo& printerNetworkInfo)
{
    for(auto& p : mPrinterList) {
        if (p.second.ip == printerNetworkInfo.ip || p.second.deviceId == printerNetworkInfo.deviceId) {
            return p.second.id;
        }
    }
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printerNetworkInfo.addTime = now;
    printerNetworkInfo.modifyTime = now;
    printerNetworkInfo.lastActiveTime = now;
    printerNetworkInfo.id = boost::uuids::to_string(boost::uuids::random_generator{}());
    mPrinterList[printerNetworkInfo.id] = printerNetworkInfo;
    savePrinterList();
    return printerNetworkInfo.id;
}
std::vector<PrinterNetworkInfo> PrinterManager::discoverPrinter()
{
    boost::filesystem::path resources_path(Slic3r::resources_dir());
    std::vector<PrinterNetworkInfo> printers;
    std::vector<PrinterNetworkInfo> discoverPrinterList = PrinterNetworkManager::getInstance()->discoverPrinters();
    for (auto& discoverPrinter : discoverPrinterList) {
        for (auto& p : mPrinterList) {
            if (p.second.ip == discoverPrinter.ip || p.second.deviceId == discoverPrinter.deviceId) {
                continue;
            }
        }
        PrinterNetworkInfo printerNetworkInfo = discoverPrinter;
        VendorProfile::PrinterModel printerModel;
        VendorProfile machineProfile = getMachineProfile(discoverPrinter.vendor, discoverPrinter.machineModel, printerModel);
        printerNetworkInfo.vendor = machineProfile.name;
        printerNetworkInfo.machineName = printerModel.name;
        printerNetworkInfo.machineModel = printerModel.name;
        printerNetworkInfo.isPhysicalPrinter = false;
        printers.push_back(printerNetworkInfo);
    }
    return printers;
}
std::vector<PrinterNetworkInfo> PrinterManager::getPrinterList()
{  
    std::vector<PrinterNetworkInfo> printers;
    for (auto& printerInfo : mPrinterList) {
        printers.push_back(printerInfo.second);
    }
    std::sort(printers.begin(), printers.end(), [](const PrinterNetworkInfo& a, const PrinterNetworkInfo& b) {
        return a.addTime < b.addTime;
    });
    return printers;
}


bool PrinterManager::upload(PrinterNetworkParams& params)
{
    std::string printerId = params.printerNetworkInfo.id;
    if (mPrinterList.find(printerId) == mPrinterList.end()) {
        return false;
    }
    PrinterNetworkInfo printerNetworkInfo = mPrinterList[printerId];

    if (!PrinterNetworkManager::getInstance()->isPrinterConnected(printerNetworkInfo)) {
        if (!PrinterNetworkManager::getInstance()->connectToPrinter(printerNetworkInfo)) {
            if (params.errorFn) {
                params.errorFn("connect to printer failed");
            }
            return false;
        }
    }

    if (PrinterNetworkManager::getInstance()->sendPrintFile(printerNetworkInfo, params)) {
        if (params.uploadAndStartPrint) {
            PrinterNetworkManager::getInstance()->sendPrintTask(printerNetworkInfo, params);
        }
        return true;
    }
    return false;
}
} // Slic3r
