#include "ElegooNetworkHelper.hpp"
#include "libslic3r/Utils.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"

namespace Slic3r {

std::string ElegooNetworkHelper::getOnlineModelsUrl() {
    std::string region = getRegion();
    std::string language = getLanguage();
    std::string onlineModelsUrl = ELEGOO_CHINA_ONLINE_MODEL_URL;
    if (region == "CN") {
        onlineModelsUrl = ELEGOO_CHINA_ONLINE_MODEL_URL;
    } else {
        onlineModelsUrl = ELEGOO_GLOBAL_ONLINE_MODEL_URL;
    }
    return onlineModelsUrl + "?language=" + language + "&region=" + region;
}

std::string ElegooNetworkHelper::getLoginUrl() {
    std::string region = getRegion();
    std::string language = getLanguage();
    std::string loginUrl = ELEGOO_CHINA_LOGIN_URL;
    if (region == "CN") {
        loginUrl = ELEGOO_CHINA_LOGIN_URL;
    } else {
        loginUrl = ELEGOO_GLOBAL_LOGIN_URL;
    }
    return loginUrl + "?language=" + language + "&region=" + region;
}

std::string ElegooNetworkHelper::getProfileUpdateUrl() {
    std::string region = getRegion();
    std::string language = getLanguage();
    std::string profileUpdateUrl = ELEGOO_CHINA_PROFILE_UPDATE_URL;
    if (region == "CN") {
        profileUpdateUrl = ELEGOO_CHINA_PROFILE_UPDATE_URL;
    } else {
        profileUpdateUrl = ELEGOO_GLOBAL_PROFILE_UPDATE_URL;
    }
    return profileUpdateUrl + "?language=" + language + "&region=" + region;
}

std::string ElegooNetworkHelper::getAppUpdateUrl() {
    std::string region = getRegion();
    std::string language = getLanguage();
    std::string appUpdateUrl = ELEGOO_CHINA_UPDATE_URL;
    if (region == "CN") {
        appUpdateUrl = ELEGOO_CHINA_UPDATE_URL;
    } else {
        appUpdateUrl = ELEGOO_GLOBAL_UPDATE_URL;
    }
    return appUpdateUrl + "?language=" + language + "&region=" + region;
}

std::string ElegooNetworkHelper::getPluginUpdateUrl() { 
    std::string region = getRegion();
    std::string language = getLanguage();
    std::string pluginUpdateUrl = ELEGOO_CHINA_PLUGIN_UPDATE_URL;
    if (region == "CN") {
        pluginUpdateUrl = ELEGOO_CHINA_PLUGIN_UPDATE_URL;
    } else {
        pluginUpdateUrl = ELEGOO_GLOBAL_PLUGIN_UPDATE_URL;
    }
    return pluginUpdateUrl + "?language=" + language + "&region=" + region;
}

std::string ElegooNetworkHelper::getUserAgent() {
    std::string userAgent = "";
    std::string theme = wxGetApp().dark_mode() ? "dark" : "light";
    std::string version = std::string(SLIC3R_VERSION);
    
    #ifdef __WIN32__
        userAgent = "ElegooSlicer/" + version + " (" + theme + ") Mozilla/5.0 (Windows NT 10.0; Win64; x64)";
    #elif defined(__WXMAC__)
        userAgent = "ElegooSlicer/" + version + " (" + theme + ") Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)";
    #elif defined(__linux__)
        userAgent = "ElegooSlicer/" + version + " (" + theme + ") Mozilla/5.0 (X11; Linux x86_64)";
    #else
        userAgent = "ElegooSlicer/" + version + " (" + theme + ") Mozilla/5.0 (compatible; ElegooSlicer)";
    #endif 
    return userAgent;
}

} // namespace Slic3r
