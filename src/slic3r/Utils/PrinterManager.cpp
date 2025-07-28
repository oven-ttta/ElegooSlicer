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
std::string PrinterManager::bindPrinter(PrinterInfo& printer)
{
    for(auto& p : mPrinterList) {
        if (p.second.ip == printer.ip || p.second.deviceId == printer.deviceId) {
            return p.second.id;
        }
    }
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    printer.addTime = now;
    printer.modifyTime = now;
    printer.lastActiveTime = now;
    printer.id = boost::uuids::to_string(boost::uuids::random_generator{}());
    mPrinterList[printer.id] = printer;
    savePrinterList();
    return printer.id;
}
std::vector<PrinterInfo> PrinterManager::discoverPrinter()
{
    boost::filesystem::path resources_path(Slic3r::resources_dir());
    std::vector<PrinterInfo> printers;
    std::vector<PrinterInfo> discoverPrinterList = PrinterNetworkManager::getInstance()->discoverPrinters();
    for (auto& discoverPrinter : discoverPrinterList) {
        for (auto& p : mPrinterList) {
            if (p.second.ip == discoverPrinter.ip || p.second.deviceId == discoverPrinter.deviceId) {
                continue;
            }
        }
        PrinterInfo printerInfo = discoverPrinter;
        VendorProfile::PrinterModel printerModel;
        VendorProfile machineProfile = getMachineProfile(discoverPrinter.vendor, discoverPrinter.machineModel, printerModel);
        printerInfo.vendor = machineProfile.name;
        printerInfo.machineName = printerModel.name;
        printerInfo.machineModel = printerModel.name;
        printerInfo.isPhysicalPrinter = false;
        printers.push_back(printerInfo);
    }
    return printers;
}
std::vector<PrinterInfo> PrinterManager::getPrinterList()
{  
    std::vector<PrinterInfo> printers;
    for (auto& printerInfo : mPrinterList) {
        printers.push_back(printerInfo.second);
    }
    std::sort(printers.begin(), printers.end(), [](const PrinterInfo& a, const PrinterInfo& b) {
        return a.addTime < b.addTime;
    });
    return printers;
}

void PrinterManager::notifyPrintInfo(PrinterInfo printerInfo, int printerStatus, int printProgress, int currentTicks, int totalTicks)
{

}

bool PrinterManager::upload(PrintHostUpload       upload_data,
                            PrintHost::ProgressFn prorgess_fn,
                            PrintHost::ErrorFn    error_fn,
                            PrintHost::InfoFn     info_fn)
{
    std::string printerId = upload_data.extended_info["selectedPrinterId"];
    if (mPrinterList.find(printerId) == mPrinterList.end()) {
        return false;
    }
    PrinterInfo printerInfo = mPrinterList[printerId];

    if (!PrinterNetworkManager::getInstance()->isPrinterConnected(printerInfo)) {
        if (!PrinterNetworkManager::getInstance()->connectToPrinter(printerInfo)) {
            error_fn(wxString::FromUTF8("connect to printer failed"));
            return false;
        }
    }

    PrinterNetworkParams params;
    params.uploadData = upload_data;
    params.progressFn = prorgess_fn;
    params.errorFn    = error_fn;
    params.infoFn     = info_fn;
    if (PrinterNetworkManager::getInstance()->sendPrintFile(printerInfo, params)) {
        if (upload_data.post_action == PrintHostPostUploadAction::StartPrint) {
            PrinterNetworkManager::getInstance()->sendPrintTask(printerInfo, params);
        }
        return true;
    }
    return false;
}
} // Slic3r
