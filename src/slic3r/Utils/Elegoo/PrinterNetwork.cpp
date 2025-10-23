#include "PrinterNetwork.hpp"
#include "ElegooPrinterNetwork.hpp"
#include "ElegooUserNetwork.hpp"
#include "ElegooPluginNetwork.hpp"
#include "ElegooNetworkHelper.hpp"

#include <wx/log.h>
#include "PrintHost.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"


namespace Slic3r {

std::shared_ptr<IPrinterNetwork> NetworkFactory::createPrinterNetwork(const PrinterNetworkInfo& printerNetworkInfo)
{
    switch (PrintHost::get_print_host_type(printerNetworkInfo.hostType)) {
    case PrintHostType::htElegooLink: {
        return std::make_shared<ElegooPrinterNetwork>(printerNetworkInfo);
    }
    default: {
        wxLogWarning("Unsupported network type: %s", printerNetworkInfo.hostType.c_str());
        return nullptr;
    }
    }
    return nullptr;
}

std::shared_ptr<IUserNetwork> NetworkFactory::createUserNetwork(const UserNetworkInfo& userNetworkInfo)
{
    switch (PrintHost::get_print_host_type(userNetworkInfo.hostType)) {
    case PrintHostType::htElegooLink: {
        return std::make_shared<ElegooUserNetwork>(userNetworkInfo);
    }
    default: {
        wxLogWarning("Unsupported network type: %s", userNetworkInfo.hostType.c_str());
        return nullptr;
    }
    }
    return nullptr;
}

std::shared_ptr<IPluginNetwork> NetworkFactory::createPluginNetwork(const PluginNetworkInfo& pluginNetworkInfo)
{
    switch (PrintHost::get_print_host_type(pluginNetworkInfo.hostType)) {
    case PrintHostType::htElegooLink: {
        return std::make_shared<ElegooPluginNetwork>(pluginNetworkInfo);
    }
    default: {
        wxLogWarning("Unsupported network type: %s", pluginNetworkInfo.hostType.c_str());
        return nullptr;
    }
    }
    return nullptr;
}

std::shared_ptr<INetworkHelper> NetworkFactory::createNetworkHelper(PrintHostType hostType)
{
    switch (hostType) {
    case PrintHostType::htElegooLink: {
        return std::make_shared<ElegooNetworkHelper>();
    }
    default: {
        wxLogWarning("Unsupported network type: %s", PrintHost::get_print_host_type_str(hostType).c_str());
        return nullptr;
    }
    }
    return nullptr;
}


std::string INetworkHelper::getLanguage() {
    std::string language = wxGetApp().app_config->get_language_code();
    language = boost::to_upper_copy(language);
    if (language == "ZH-CN") {
        language = "zh-CN";
    } else if (language == "ZH-TW") {
        language = "zh-TW";
    } else if (language == "EN") {
        language = "en";
    } else if (language == "ES") {
        language = "es";
    } else if (language == "FR") {
        language = "fr";
    } else if (language == "DE") {
        language = "de";
    } else if (language == "JA") {
        language = "ja";
    } else if (language == "KO") {
        language = "ko";
    } else if (language == "RU") {
        language = "ru";
    } else if (language == "PT") {
        language = "pt";
    } else if (language == "IT") {
        language = "it";
    } else if (language == "NL") {
        language = "nl";
    } else if (language == "TR") {
        language = "tr";
    } else if (language == "CS") {
        language = "cs";
    } else {
        language = "en";
    }

   return language;
}

std::string INetworkHelper::getRegion() {
    std::string region = wxGetApp().app_config->get_region();
    region = boost::to_upper_copy(region);
    if (region == "CHN" || region == "CHINA") {
        region = "CN";
    } else if (region == "USA" || region == "NORTH AMERICA") {
        region = "US";
    } else if (region == "EUROPE") {
        region = "GB";
    } else if (region == "ASIA-PACIFIC") {
        region = "JP";
    } else {
        region = "other";
    }

    return region;
}


void IPrinterNetwork::init() { ElegooPrinterNetwork::init(); }

void IPrinterNetwork::uninit() { ElegooPrinterNetwork::uninit(); }

void IUserNetwork::init() { ElegooUserNetwork::init(); }

void IUserNetwork::uninit() { ElegooUserNetwork::uninit(); }

void IPluginNetwork::init() { ElegooPluginNetwork::init(); }

void IPluginNetwork::uninit() { ElegooPluginNetwork::uninit(); }
} // namespace Slic3r