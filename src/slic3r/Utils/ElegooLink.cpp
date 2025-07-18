#include "ElegooLink.hpp"

#include <algorithm>
#include <sstream>
#include <exception>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <curl/curl.h>

#include <wx/progdlg.h>

#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/format.hpp"
#include "Http.hpp"
#include "libslic3r/AppConfig.hpp"
#include "Bonjour.hpp"
#include "slic3r/GUI/BonjourDialog.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::property_tree;
#define MAX_UPLOAD_PACKAGE_LENGTH 1048576 //(1024*1024)

namespace Slic3r {

    enum ElegooLinkCommand {
        //get status
        ELEGOO_GET_STATUS = 0,
        //get properties
        ELEGOO_GET_PROPERTIES = 1,
        //start print
        ELEGOO_START_PRINT = 128,
    };

    typedef enum
    {
        SDCP_PRINT_CTRL_ACK_OK = 0,  // OK
        SDCP_PRINT_CTRL_ACK_BUSY = 1 , // Device is busy
        SDCP_PRINT_CTRL_ACK_NOT_FOUND = 2,  // File not found
        SDCP_PRINT_CTRL_ACK_MD5_FAILED = 3,  // MD5 check failed
        SDCP_PRINT_CTRL_ACK_FILEIO_FAILED = 4,  // File I/O error
        SDCP_PRINT_CTRL_ACK_INVLAID_RESOLUTION = 5, // File resolution is invalid
        SDCP_PRINT_CTRL_ACK_UNKNOW_FORMAT = 6,  // File format is invalid
        SDCP_PRINT_CTRL_ACK_UNKNOW_MODEL = 7, // File model is invalid
    } ElegooLinkStartPrintAck;


    namespace {

        std::string get_host_from_url(const std::string& url_in)
        {
            std::string url = url_in;
            // add http:// if there is no scheme
            size_t double_slash = url.find("//");
            if (double_slash == std::string::npos)
                url = "http://" + url;
            std::string out = url;
            CURLU* hurl = curl_url();
            if (hurl) {
                // Parse the input URL.
                CURLUcode rc = curl_url_set(hurl, CURLUPART_URL, url.c_str(), 0);
                if (rc == CURLUE_OK) {
                    // Replace the address.
                    char* host;
                    rc = curl_url_get(hurl, CURLUPART_HOST, &host, 0);
                    if (rc == CURLUE_OK) {
                        char* port;
                        rc = curl_url_get(hurl, CURLUPART_PORT, &port, 0);
                        if (rc == CURLUE_OK && port != nullptr) {
                            out = std::string(host) + ":" + port;
                            curl_free(port);
                        } else {
                            out = host;
                            curl_free(host);
                        }
                    }
                    else
                        BOOST_LOG_TRIVIAL(error) << "ElegooLink get_host_from_url: failed to get host form URL " << url;
                }
                else
                    BOOST_LOG_TRIVIAL(error) << "ElegooLink get_host_from_url: failed to parse URL " << url;
                curl_url_cleanup(hurl);
            }
            else
                BOOST_LOG_TRIVIAL(error) << "ElegooLink get_host_from_url: failed to allocate curl_url";
            return out;
        }
    
        std::string get_host_from_url_no_port(const std::string& url_in)
        {
            std::string url = url_in;
            // add http:// if there is no scheme
            size_t double_slash = url.find("//");
            if (double_slash == std::string::npos)
                url = "http://" + url;
            std::string out = url;
            CURLU* hurl = curl_url();
            if (hurl) {
                // Parse the input URL.
                CURLUcode rc = curl_url_set(hurl, CURLUPART_URL, url.c_str(), 0);
                if (rc == CURLUE_OK) {
                    // Replace the address.
                    char* host;
                    rc = curl_url_get(hurl, CURLUPART_HOST, &host, 0);
                    if (rc == CURLUE_OK) {
                        out = host;
                        curl_free(host);
                    }
                    else
                        BOOST_LOG_TRIVIAL(error) << "ElegooLink get_host_from_url: failed to get host form URL " << url;
                }
                else
                    BOOST_LOG_TRIVIAL(error) << "ElegooLink get_host_from_url: failed to parse URL " << url;
                curl_url_cleanup(hurl);
            }
            else
                BOOST_LOG_TRIVIAL(error) << "ElegooLink get_host_from_url: failed to allocate curl_url";
            return out;
        }

        #ifdef WIN32
            // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
        std::string substitute_host(const std::string& orig_addr, std::string sub_addr)
        {
            // put ipv6 into [] brackets 
            if (sub_addr.find(':') != std::string::npos && sub_addr.at(0) != '[')
                sub_addr = "[" + sub_addr + "]";

        #if 0
            //URI = scheme ":"["//"[userinfo "@"] host [":" port]] path["?" query]["#" fragment]
            std::string final_addr = orig_addr;
            //  http
            size_t double_dash = orig_addr.find("//");
            size_t host_start = (double_dash == std::string::npos ? 0 : double_dash + 2);
            // userinfo
            size_t at = orig_addr.find("@");
            host_start = (at != std::string::npos && at > host_start ? at + 1 : host_start);
            // end of host, could be port(:), subpath(/) (could be query(?) or fragment(#)?)
            // or it will be ']' if address is ipv6 )
            size_t potencial_host_end = orig_addr.find_first_of(":/", host_start); 
            // if there are more ':' it must be ipv6
            if (potencial_host_end != std::string::npos && orig_addr[potencial_host_end] == ':' && orig_addr.rfind(':') != potencial_host_end) {
                size_t ipv6_end = orig_addr.find(']', host_start);
                // DK: Uncomment and replace orig_addr.length() if we want to allow subpath after ipv6 without [] parentheses.
                potencial_host_end = (ipv6_end != std::string::npos ? ipv6_end + 1 : orig_addr.length()); //orig_addr.find('/', host_start));
            }
            size_t host_end = (potencial_host_end != std::string::npos ? potencial_host_end : orig_addr.length());
            // now host_start and host_end should mark where to put resolved addr
            // check host_start. if its nonsense, lets just use original addr (or  resolved addr?)
            if (host_start >= orig_addr.length()) {
                return final_addr;
            }
            final_addr.replace(host_start, host_end - host_start, sub_addr);
            return final_addr;
        #else
            // Using the new CURL API for handling URL. https://everything.curl.dev/libcurl/url
            // If anything fails, return the input unchanged.
            std::string out = orig_addr;
            CURLU *hurl = curl_url();
            if (hurl) {
                // Parse the input URL.
                CURLUcode rc = curl_url_set(hurl, CURLUPART_URL, orig_addr.c_str(), 0);
                if (rc == CURLUE_OK) {
                    // Replace the address.
                    rc = curl_url_set(hurl, CURLUPART_HOST, sub_addr.c_str(), 0);
                    if (rc == CURLUE_OK) {
                        // Extract a string fromt the CURL URL handle.
                        char *url;
                        rc = curl_url_get(hurl, CURLUPART_URL, &url, 0);
                        if (rc == CURLUE_OK) {
                            out = url;
                            curl_free(url);
                        } else
                            BOOST_LOG_TRIVIAL(error) << "ElegooLink substitute_host: failed to extract the URL after substitution";
                    } else
                        BOOST_LOG_TRIVIAL(error) << "ElegooLink substitute_host: failed to substitute host " << sub_addr << " in URL " << orig_addr;
                } else
                    BOOST_LOG_TRIVIAL(error) << "ElegooLink substitute_host: failed to parse URL " << orig_addr;
                curl_url_cleanup(hurl);
            } else
                BOOST_LOG_TRIVIAL(error) << "ElegooLink substitute_host: failed to allocate curl_url";
            return out;
        #endif
        }
        #endif // WIN32
        std::string escape_string(const std::string& unescaped)
        {
            std::string ret_val;
            CURL* curl = curl_easy_init();
            if (curl) {
                char* decoded = curl_easy_escape(curl, unescaped.c_str(), unescaped.size());
                if (decoded) {
                    ret_val = std::string(decoded);
                    curl_free(decoded);
                }
                curl_easy_cleanup(curl);
            }
            return ret_val;
        }
        std::string escape_path_by_element(const boost::filesystem::path& path)
        {
            std::string ret_val = escape_string(path.filename().string());
            boost::filesystem::path parent(path.parent_path());
            while (!parent.empty() && parent.string() != "/") // "/" check is for case "/file.gcode" was inserted. Then boost takes "/" as parent_path.
            {
                ret_val = escape_string(parent.filename().string()) + "/" + ret_val;
                parent = parent.parent_path();
            }
            return ret_val;
        }

    } //namespace


    ElegooLink::ElegooLink(DynamicPrintConfig *config):
    OctoPrint(config) {

    }

    const char* ElegooLink::get_name() const { return "ElegooLink"; }

    bool ElegooLink::elegoo_test(wxString& msg) const{

    const char *name = get_name();
    bool res = true;
    auto url = make_url("");
    // Here we do not have to add custom "Host" header - the url contains host filled by user and libCurl will set the header by itself.
    auto http = Http::get(std::move(url));
    set_auth(http);
    http.on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Error getting version: %2%, HTTP %3%, body: `%4%`") % name % error % status % body;
            res = false;
            msg = format_error(body, error, status);
        })
        .on_complete([&, this](std::string body, unsigned) {
            BOOST_LOG_TRIVIAL(debug) << boost::format("%1%: Got version: %2%") % name % body;
            // Check if the response contains "ELEGOO" in any case.
            // This is a simple check to see if the response is from an ElegooLink server.
            std::regex re("ELEGOO", std::regex::icase);
            std::smatch match;
            if (std::regex_search(body, match, re)) {
                res = true;
            } else {
                msg = format_error(body, "ElegooLink not detected", 0);
                res = false;
            }
        })
#ifdef WIN32
            .ssl_revoke_best_effort(m_ssl_revoke_best_effort)
            .on_ip_resolve([&](std::string address) {
            // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
            // Remember resolved address to be reused at successive REST API call.
            msg = GUI::from_u8(address);
        })
#endif // WIN32
        .perform_sync();
        return res;
    }
    bool ElegooLink::test(wxString &curl_msg) const{
        if(OctoPrint::test(curl_msg)){
            return true;
        }
        curl_msg="";
        return elegoo_test(curl_msg);
    }

#ifdef WIN32
    bool ElegooLink::elegoo_test_with_resolved_ip(wxString& msg) const
    {
        // Since the request is performed synchronously here,
        // it is ok to refer to `msg` from within the closure
        const char* name = get_name();
        bool        res  = true;
        // Msg contains ip string.
        auto url = substitute_host(make_url(""), GUI::into_u8(msg));
        msg.Clear();
        std::string host = get_host_from_url(m_host);
        auto        http = Http::get(url); // std::move(url));
        // "Host" header is necessary here. We have resolved IP address and subsituted it into "url" variable.
        // And when creating Http object above, libcurl automatically includes "Host" header from address it got.
        // Thus "Host" is set to the resolved IP instead of host filled by user. We need to change it back.
        // Not changing the host would work on the most cases (where there is 1 service on 1 hostname) but would break when f.e. reverse
        // proxy is used (issue #9734). Also when allow_ip_resolve = 0, this is not needed, but it should not break anything if it stays.
        // https://www.rfc-editor.org/rfc/rfc7230#section-5.4
        http.header("Host", host);
        set_auth(http);
        http.on_error([&](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Error getting version at %2% : %3%, HTTP %4%, body: `%5%`") % name % url %
                                                error % status % body;
                res = false;
                msg = format_error(body, error, status);
            })
            .on_complete([&, this](std::string body, unsigned) {
                // Check if the response contains "ELEGOO" in any case.
                // This is a simple check to see if the response is from an ElegooLink server.
                std::regex  re("ELEGOO", std::regex::icase);
                std::smatch match;
                if (std::regex_search(body, match, re)) {
                    res = true;
                } else {
                    msg = format_error(body, "ElegooLink not detected", 0);
                    res = false;
                }
            })
            .ssl_revoke_best_effort(m_ssl_revoke_best_effort)
            .perform_sync();

        return res;
    }
    bool ElegooLink::test_with_resolved_ip(wxString& msg) const
    {
        // Elegoo supports both otcoprint and Elegoo link
        if (OctoPrint::test_with_resolved_ip(msg)) {
            return true;
        }
        msg = "";
        return elegoo_test_with_resolved_ip(msg);
    }
#endif // WIN32


    wxString ElegooLink::get_test_ok_msg() const
    {
        return _L("Connection to ElegooLink is working correctly.");
    }

    wxString ElegooLink::get_test_failed_msg(wxString& msg) const
    {
        return GUI::format_wxstr("%s: %s", _L("Could not connect to ElegooLink"), msg);
    }

    #ifdef WIN32
    bool ElegooLink::upload_inner_with_resolved_ip(PrintHostUpload upload_data, ProgressFn prorgess_fn, ErrorFn error_fn, InfoFn info_fn, const boost::asio::ip::address& resolved_addr) const
    {
        wxString test_msg_or_host_ip = "";

        info_fn(L"resolve", boost::nowide::widen(resolved_addr.to_string()));
        // If test fails, test_msg_or_host_ip contains the error message.
        // Otherwise on Windows it contains the resolved IP address of the host.
        // Test_msg already contains resolved ip and will be cleared on start of test().
        test_msg_or_host_ip          = GUI::from_u8(resolved_addr.to_string());
        //Elegoo supports both otcoprint and Elegoo link
        if(OctoPrint::test_with_resolved_ip(test_msg_or_host_ip)){
            return OctoPrint::upload_inner_with_host(upload_data, prorgess_fn, error_fn, info_fn);
        }

        test_msg_or_host_ip = GUI::from_u8(resolved_addr.to_string());
        if(!elegoo_test_with_resolved_ip(test_msg_or_host_ip)){
            error_fn(std::move(test_msg_or_host_ip));
            return false;
        }


        std::string url = substitute_host(make_url("uploadFile/upload"), resolved_addr.to_string());
        info_fn(L"resolve", boost::nowide::widen(url));

        bool res = loopUpload(url, upload_data, prorgess_fn, error_fn, info_fn);
        return res;
    }
    #endif //WIN32

    bool ElegooLink::upload_inner_with_host(PrintHostUpload upload_data, ProgressFn prorgess_fn, ErrorFn error_fn, InfoFn info_fn) const
    {
        // If test fails, test_msg_or_host_ip contains the error message.
        // Otherwise on Windows it contains the resolved IP address of the host.
        wxString test_msg_or_host_ip;
        //Elegoo supports both otcoprint and Elegoo link
        if(OctoPrint::test(test_msg_or_host_ip)){
            return OctoPrint::upload_inner_with_host(upload_data, prorgess_fn, error_fn, info_fn);
        }
        test_msg_or_host_ip="";
        if(!elegoo_test(test_msg_or_host_ip)){
            error_fn(std::move(test_msg_or_host_ip));
            return false;
        }

        std::string url;
    #ifdef WIN32
        // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
        if (m_host.find("https://") == 0 || test_msg_or_host_ip.empty() || !GUI::get_app_config()->get_bool("allow_ip_resolve"))
    #endif // _WIN32
        {
            // If https is entered we assume signed ceritificate is being used
            // IP resolving will not happen - it could resolve into address not being specified in cert
            url = make_url("uploadFile/upload");
        }
    #ifdef WIN32
        else {
            // Workaround for Windows 10/11 mDNS resolve issue, where two mDNS resolves in succession fail.
            // Curl uses easy_getinfo to get ip address of last successful transaction.
            // If it got the address use it instead of the stored in "host" variable.
            // This new address returns in "test_msg_or_host_ip" variable.
            // Solves troubles of uploades failing with name address.
            // in original address (m_host) replace host for resolved ip 
            info_fn(L"resolve", test_msg_or_host_ip);
            url = substitute_host(make_url("uploadFile/upload"), GUI::into_u8(test_msg_or_host_ip));
            BOOST_LOG_TRIVIAL(info) << "Upload address after ip resolve: " << url;
        }
    #endif // _WIN32

        bool res = loopUpload(url, upload_data, prorgess_fn, error_fn, info_fn);
        return res;
    }

    bool ElegooLink::validate_version_text(const boost::optional<std::string> &version_text) const
    {
        return  true;
    }
    bool ElegooLink::uploadPart(Http &http,std::string md5,std::string uuid,std::string path,
                                std::string filename,size_t filesize,size_t offset,size_t length,
                                ProgressFn  prorgess_fn,ErrorFn error_fn,InfoFn info_fn)const
    {
        const char* name   = get_name();
        bool        result = false;
        http.form_clear();
        http.form_add("Check", "1")
            .form_add("S-File-MD5", md5)
            .form_add("Offset", std::to_string(offset))
            .form_add("Uuid", uuid)
            .form_add("TotalSize", std::to_string(filesize))
            .form_add_file("File", path, filename, offset, length)
            .on_complete([&](std::string body, unsigned status) {
                BOOST_LOG_TRIVIAL(debug) << boost::format("%1%: File uploaded: HTTP %2%: %3%") % name % status % body;
                if (status == 200) {
                    try {
                        pt::ptree          root;
                        std::istringstream is(body);
                        pt::read_json(is, root);
                        std::string code = root.get<std::string>("code");
                        if (code == "000000") {
                            // info_fn(L"complete", wxString());
                            result = true;
                        } else {
                            // get error messages
                            pt::ptree   messages      = root.get_child("messages");
                            std::string error_message = "ErrorCode : " + code + "\n";
                            for (pt::ptree::value_type& message : messages) {
                                std::string field = message.second.get<std::string>("field");
                                std::string msg   = message.second.get<std::string>("message");
                                error_message += field + ":" + msg + "\n";
                            }
                            error_fn(wxString::FromUTF8(error_message));
                        }
                    } catch (...) {
                        BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Error parsing response: %2%") % name % body;
                        error_fn(wxString::FromUTF8("Error parsing response"));
                    }
                } else {
                    error_fn(format_error(body, "upload failed", status));
                }
            })
            .on_error([&](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(error) << boost::format("%1%: Error uploading file: %2%, HTTP %3%, body: `%4%`") % name % error % status %
                                                body;
                error_fn(format_error(body, error, status));
            })
            .on_progress([&](Http::Progress progress, bool& cancel) {
                // If upload is finished, do not call progress_fn
                // on_complete will be called after some time, so we do not need to call it here
                // Because some devices will call on_complete after the upload progress reaches 100%,
                // so we need to process it here, based on on_complete
                if (progress.ultotal == progress.ulnow) {
                    // Upload is finished
                    return;
                }
                prorgess_fn(std::move(progress), cancel);
                if (cancel) {
                    // Upload was canceled
                    BOOST_LOG_TRIVIAL(info) << name << ": Upload canceled";
                }
            })
#ifdef WIN32
            .ssl_revoke_best_effort(m_ssl_revoke_best_effort)
#endif
            .perform_sync();
        return result;
    }

    bool ElegooLink::loopUpload(std::string url, PrintHostUpload upload_data, ProgressFn progress_fn, ErrorFn error_fn, InfoFn info_fn) const
    {
        const char* name               = get_name();
        const auto  upload_filename = upload_data.upload_path.filename().string();
        std::string source_path        = upload_data.source_path.string();

        // calc file size
        std::ifstream   file(source_path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.close();
        const std::string fileSize = std::to_string(size);

        // generate uuid
        boost::uuids::random_generator generator;
        boost::uuids::uuid             uuid        = generator();
        std::string                    uuid_string = to_string(uuid);

        std::string md5;
        bbl_calc_md5(source_path, md5);

        auto        http   = Http::post(url);
#ifdef WIN32
        // "Host" header is necessary here. In the workaround above (two mDNS..) we have got IP address from test connection and subsituted
        // it into "url" variable. And when creating Http object above, libcurl automatically includes "Host" header from address it got.
        // Thus "Host" is set to the resolved IP instead of host filled by user. We need to change it back. Not changing the host would work
        // on the most cases (where there is 1 service on 1 hostname) but would break when f.e. reverse proxy is used (issue #9734). Also
        // when allow_ip_resolve = 0, this is not needed, but it should not break anything if it stays.
        // https://www.rfc-editor.org/rfc/rfc7230#section-5.4
        std::string host = get_host_from_url(m_host);
        http.header("Host", host);
        http.header("Accept", "application/json, text/plain, */*");
#endif // _WIN32
        set_auth(http);

        bool      res          = false;
        const int packageCount = (size + MAX_UPLOAD_PACKAGE_LENGTH - 1) / MAX_UPLOAD_PACKAGE_LENGTH;

        for (size_t i = 0; i < packageCount; i++) {
            BOOST_LOG_TRIVIAL(info) << boost::format("%1%: Uploading file %2%/%3%") % name % (i + 1) % packageCount;
            const size_t offset = MAX_UPLOAD_PACKAGE_LENGTH * i;
            size_t       length = MAX_UPLOAD_PACKAGE_LENGTH;
            // if it is the last package, the length is the remainder of the file size divided by MAX_UPLOAD_PACKAGE_LENGTH
            if ((i == packageCount - 1) && (size % MAX_UPLOAD_PACKAGE_LENGTH > 0)) {
                length = size % MAX_UPLOAD_PACKAGE_LENGTH;
            }
            res = uploadPart(
                http, md5, uuid_string, source_path, upload_filename, size, offset, length,
                [size, i, progress_fn](Http::Progress progress, bool& cancel) {
                    Http::Progress p(0, 0, size, i * MAX_UPLOAD_PACKAGE_LENGTH + progress.ulnow, progress.buffer);
                    progress_fn(p, cancel);
                },
                error_fn, info_fn);
            if (!res) {
                break;
            }
        }

        if (res) {
            if (upload_data.post_action == PrintHostPostUploadAction::StartPrint) {
                // connect to websocket, since the upload is successful, the file will be printed
                std::string     wsUrl = get_host_from_url_no_port(m_host);
                WebSocketClient client;
                try {
                    client.connect(wsUrl, "3030", "/websocket");
                } catch (std::exception& e) {
                    const auto errorString = std::string(e.what());
                    if (errorString.find("The WebSocket handshake was declined by the remote peer") != std::string::npos) {
                        // error_fn(
                        //     _L("The number of printer connections has exceeded the limit. Please disconnect other connections, restart the "
                        //        "printer and slicer, and then try again."));
                        error_fn(_L("The file has been transferred, but some unknown errors occurred. Please check the device page for the file and try to start printing again."));
                    } else {
                        error_fn(std::string("\n") + wxString::FromUTF8(e.what()));
                    }
                    return false;
                }
                std::string timeLapse = "0";
                std::string heatedBedLeveling = "0";
                std::string bedType           = "0";
                timeLapse         = upload_data.extended_info["timeLapse"];
                heatedBedLeveling = upload_data.extended_info["heatedBedLeveling"];
                bedType           = upload_data.extended_info["bedType"];
                std::string filamentAmsMapping = "";
                if (upload_data.extended_info.find("filamentAmsMapping") != upload_data.extended_info.end()) {
                     filamentAmsMapping = upload_data.extended_info["filamentAmsMapping"];   
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (checkResult(client, error_fn)) {
                    // send print command
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    res = print(client, timeLapse, heatedBedLeveling, bedType, upload_filename, filamentAmsMapping, error_fn);
                }else{
                    res = false;
                }
            }
        }
        return res;
    }

    bool ElegooLink::upload(PrintHostUpload upload_data, ProgressFn prorgess_fn, ErrorFn error_fn, InfoFn info_fn) const
    {
    #ifndef WIN32
        return upload_inner_with_host(std::move(upload_data), prorgess_fn, error_fn, info_fn);
    #else
        std::string host = get_host_from_url(m_host);

        // decide what to do based on m_host - resolve hostname or upload to ip
        std::vector<boost::asio::ip::address> resolved_addr;
        boost::system::error_code ec;
        boost::asio::ip::address host_ip = boost::asio::ip::make_address(host, ec);
        if (!ec) {
            resolved_addr.push_back(host_ip);
        } else if ( GUI::get_app_config()->get_bool("allow_ip_resolve") && boost::algorithm::ends_with(host, ".local")){
            Bonjour("octoprint")
                .set_hostname(host)
                .set_retries(5) // number of rounds of queries send
                .set_timeout(1) // after each timeout, if there is any answer, the resolving will stop
                .on_resolve([&ra = resolved_addr](const std::vector<BonjourReply>& replies) {
                    for (const auto & rpl : replies) {
                        boost::asio::ip::address ip(rpl.ip);
                        ra.emplace_back(ip);
                        BOOST_LOG_TRIVIAL(info) << "Resolved IP address: " << rpl.ip;
                    }
                })
                .resolve_sync();
        }
        if (resolved_addr.empty()) {
            // no resolved addresses - try system resolving
            BOOST_LOG_TRIVIAL(error) << "ElegooLink failed to resolve hostname " << m_host << " into the IP address. Starting upload with system resolving.";
            return upload_inner_with_host(std::move(upload_data), prorgess_fn, error_fn, info_fn);
        } else if (resolved_addr.size() == 1) {
            // one address resolved - upload there
            return upload_inner_with_resolved_ip(std::move(upload_data), prorgess_fn, error_fn, info_fn, resolved_addr.front());
        }  else if (resolved_addr.size() == 2 && resolved_addr[0].is_v4() != resolved_addr[1].is_v4()) {
            // there are just 2 addresses and 1 is ip_v4 and other is ip_v6
            // try sending to both. (Then if both fail, show both error msg after second try)
            wxString error_message;
            if (!upload_inner_with_resolved_ip(std::move(upload_data), prorgess_fn
                , [&msg = error_message, resolved_addr](wxString error) { msg = GUI::format_wxstr("%1%: %2%", resolved_addr.front(), error); }
                , info_fn, resolved_addr.front())
                &&
                !upload_inner_with_resolved_ip(std::move(upload_data), prorgess_fn
                , [&msg = error_message, resolved_addr](wxString error) { msg += GUI::format_wxstr("\n%1%: %2%", resolved_addr.back(), error); }
                , info_fn, resolved_addr.back())
                ) {

                error_fn(error_message);
                return false;
            }
            return true;
        } else {
            // There are multiple addresses - user needs to choose which to use.
            size_t selected_index = resolved_addr.size(); 
            IPListDialog dialog(nullptr, boost::nowide::widen(m_host), resolved_addr, selected_index);
            if (dialog.ShowModal() == wxID_OK && selected_index < resolved_addr.size()) {    
                return upload_inner_with_resolved_ip(std::move(upload_data), prorgess_fn, error_fn, info_fn, resolved_addr[selected_index]);
            }
        }
        return false;
    #endif // WIN32
    }

    bool ElegooLink::print(WebSocketClient&  client,
                           std::string       timeLapse,
                           std::string       heatedBedLeveling,
                           std::string       bedType,
                           const std::string filename,
                           std::string       filamentAmsMapping,
                           ErrorFn           error_fn) const
    {

        // convert bedType to 0 or 1, 0 is PTE, 1 is PC
        if (bedType == std::to_string((int)BedType::btPC)){
            bedType = "1";
        }else{
            bedType = "0";
        }
        bool res = false;
        // create a random UUID generator
        boost::uuids::random_generator generator;
        // generate a UUID
        boost::uuids::uuid uuid = generator();
        std::string uuid_string = to_string(uuid);
        try {
            std::string requestID = uuid_string; 
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            std::string timestamp = std::to_string(milliseconds);   
            std::string amsData = buildAmsDataString(filamentAmsMapping);
            std::string cmdData = R"({"Filename":"/local/)" + filename + R"(",
                                                "StartLayer":0,
                                                "Calibration_switch":)" + heatedBedLeveling + R"(,
                                                "PrintPlatformType":)" + bedType + R"(,
                                                "Tlp_Switch":)" + timeLapse;
            if (!amsData.empty()) {
                cmdData += "," + amsData;
            }
            cmdData += R"(})";
            std::string jsonString = R"({
                                        "Id":"",
                                            "Data":{
                                                "Cmd":)"+std::to_string(ElegooLinkCommand::ELEGOO_START_PRINT)+R"(,
                                                "Data":)" + cmdData + R"(,
                                                "RequestID":")"+ uuid_string + R"(",
                                                "MainboardID":"",
                                                "TimeStamp":)" + timestamp + R"(,
                                                "From":1
                                        }
                                    })";
                    std::cout << "send: " << jsonString << std::endl;
                    BOOST_LOG_TRIVIAL(info) << "start print, param: " << jsonString;
                    client.send(jsonString);
                    // wait 30s
                    auto start_time = std::chrono::steady_clock::now();
                    do{
                        std::string response = client.receive();
                        std::cout << "Received: " << response << std::endl;
                        BOOST_LOG_TRIVIAL(info) << "Received: " << response;

                        //sample response
                        // {"Id":"979d4C788A4a78bC777A870F1A02867A","Data":{"Cmd":128,"Data":{"Ack":1},"RequestID":"5223de52cc7642ae8d7924f9dd46f6ad","MainboardID":"1c7319d30105041800009c0000000000","TimeStamp":1735032553},"Topic":"sdcp/response/1c7319d30105041800009c0000000000"}  
                        pt::ptree root;
                        std::istringstream is(response);
                        pt::read_json(is, root);

                        auto data = root.get_child_optional("Data");
                        if(!data){
                            BOOST_LOG_TRIVIAL(info) << "wait for start print response";
                            continue;
                        }
                        auto cmd = data->get_optional<int>("Cmd");
                        if(!cmd){
                            BOOST_LOG_TRIVIAL(info) << "wait for start print response";
                            continue;
                        }
                        if(*cmd == ElegooLinkCommand::ELEGOO_START_PRINT){
                            auto _ack = data->get_optional<int>("Data.Ack");
                            if(!_ack){
                                BOOST_LOG_TRIVIAL(error) << "start print failed, ack not found";
                                error_fn(_L("Error code not found"));
                                break;
                            }
                            auto ack = static_cast<ElegooLinkStartPrintAck>(*_ack);
                            if(ack == ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_OK){
                                res = true;
                            }else{
                                res = false;
                                BOOST_LOG_TRIVIAL(error) << "start print failed: ack: " << ack;
                                wxString error_message = "";
                                switch(ack){
                                    case ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_BUSY:
                                    {   
                                        error_message =_L("The printer is busy, Please check the device page for the file and try to start printing again.");
                                        break;
                                    }
                                    case ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_NOT_FOUND:
                                    {
                                        error_message =_(L("The file is lost, please check and try again."));
                                        break;
                                    }
                                    case ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_MD5_FAILED:
                                    {
                                        error_message =_(L("The file is corrupted, please check and try again."));
                                        break;
                                    }
                                    case ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_FILEIO_FAILED:
                                    case ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_INVLAID_RESOLUTION:
                                    case ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_UNKNOW_FORMAT:
                                    {
                                        error_message =_(L("Transmission abnormality, please check and try again."));
                                        break;
                                    }
                                    case ElegooLinkStartPrintAck::SDCP_PRINT_CTRL_ACK_UNKNOW_MODEL:
                                    {
                                        error_message =_(L("The file does not match the printer, please check and try again."));
                                        break;
                                    }
                                    default:
                                    {
                                        error_message =_L("Unknown error");
                                        break;
                                    }
                                }

                                error_message += " " + wxString::Format(_L("Error code: %d"),ack);
                                error_fn(error_message);
                            }
                            break;
                        }
                    } while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(30));
                    if (std::chrono::steady_clock::now() - start_time >= std::chrono::seconds(30)) {
                        res = false;
                        error_fn(_L("Start print timeout"));
                    }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                BOOST_LOG_TRIVIAL(error) << "start print error: " << e.what();
                error_fn(_L("Start print failed") +"\n" +GUI::from_u8(e.what()));
                res=false;
            }
        return res;
    }

    bool ElegooLink::checkResult(WebSocketClient& client, ErrorFn error_fn) const
    {
        bool res = true;
        // create a random UUID generator
        boost::uuids::random_generator generator;
        // generate a UUID
        boost::uuids::uuid uuid        = generator();
        std::string        uuid_string = to_string(uuid);
        try {
            std::string requestID    = uuid_string;
            auto        now          = std::chrono::system_clock::now();
            auto        duration     = now.time_since_epoch();
            auto        milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            std::string timestamp    = std::to_string(milliseconds);
            std::string jsonString   = R"({
                                        "Id":"",
                                        "Data":{
                                            "Cmd":)" +
                                     std::to_string(ElegooLinkCommand::ELEGOO_GET_STATUS) + R"(,
                                            "Data":{},
                                            "RequestID":")" +
                                     uuid_string + R"(",
                                            "MainboardID":"",
                                            "TimeStamp":)" +
                                     timestamp + R"(,
                                            "From":1
                                        }
                                    })";
            std::cout << "send: " << jsonString << std::endl;
            BOOST_LOG_TRIVIAL(info) << "start print, param: " << jsonString;
            bool needWrite = true;
            // wait 60s
            auto start_time = std::chrono::steady_clock::now();
            do {
                if (needWrite) {
                    client.send(jsonString);
                    needWrite = false;
                }
                std::string response = client.receive();
                std::cout << "Received: " << response << std::endl;
                BOOST_LOG_TRIVIAL(info) << "Received: " << response;
                pt::ptree          root;
                std::istringstream is(response);
                pt::read_json(is, root);
                auto status = root.get_child_optional("Status");
                if (status) {
                    auto currentStatus = status->get_child_optional("CurrentStatus");
                    if (currentStatus) {
                        std::vector<int> status;
                        for (auto& item : *currentStatus) {
                            status.push_back(item.second.get_value<int>());
                        }
                        if (std::find(status.begin(), status.end(), 8) != status.end()) {
                            // 8 is check file status, need to wait
                            needWrite = true;
                            // sleep 1s
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }
            } while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(60));
            if (std::chrono::steady_clock::now() - start_time >= std::chrono::seconds(60)) {
                res = false;
                error_fn(_L("Start print timeout"));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            BOOST_LOG_TRIVIAL(error) << "start print error: " << e.what();
            error_fn(_L("Start print failed") + "\n" + GUI::from_u8(e.what()));
            res = false;
        }
        return res;
    }


    std::map<std::string, std::string> ElegooLink::getAttributes(WebSocketClient& client) const
    {
        std::map<std::string, std::string> attributes;
        // create a random UUID generator
        boost::uuids::random_generator generator;
        // generate a UUID
        boost::uuids::uuid uuid        = generator();
        std::string        uuid_string = to_string(uuid);
        try {
            std::string requestID    = uuid_string;
            auto        now          = std::chrono::system_clock::now();
            auto        duration     = now.time_since_epoch();
            auto        milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            std::string timestamp    = std::to_string(milliseconds);
            std::string jsonString   = R"({
                                        "Id":"",
                                        "Data":{
                                            "Cmd":)" +
                                     std::to_string(ElegooLinkCommand::ELEGOO_GET_PROPERTIES) + R"(,
                                            "Data":{},
                                            "RequestID":")" +
                                     uuid_string + R"(",
                                            "MainboardID":"",
                                            "TimeStamp":)" +
                                     timestamp + R"(,
                                            "From":1
                                        }
                                    })";
            std::cout << "send: " << jsonString << std::endl;
            BOOST_LOG_TRIVIAL(info) << "start print, param: " << jsonString;
            bool needWrite = true;
            // wait 60s
            auto start_time = std::chrono::steady_clock::now();
            do {
                if (needWrite) {
                    client.send(jsonString);
                    needWrite = false;
                }
                std::string response = client.receive();
                std::cout << "Received: " << response << std::endl;
                BOOST_LOG_TRIVIAL(info) << "Received: " << response;
                pt::ptree          root;
                std::istringstream is(response);
                pt::read_json(is, root);
                auto attr = root.get_child_optional("Attributes");
                if (attr) {
                    auto machineName = attr->get_child_optional("MachineName");
                    if (machineName) {
                        attributes["MachineName"] = machineName->get_value<std::string>();
                    }
 
                }
            } while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(60));
            if (std::chrono::steady_clock::now() - start_time >= std::chrono::seconds(60)) {
                BOOST_LOG_TRIVIAL(error) << "get attributes timeout";
                return attributes;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            BOOST_LOG_TRIVIAL(error) << "start print error: " << e.what();
            return attributes;
        }
        return attributes;
    }


    // std::map<std::string, const Preset*> ElegooLink::getAmsConfigMap(const std::map<std::string, const Preset*>& compatible_filaments) const
    // {

    //     std::map<std::string, const Preset*> ams_config_map;  
    //     for (const auto& [filament_name, filament_config] : compatible_filaments) {
    //         std::string name = filament_name;
    
    //         boost::replace_all(name, "Elegoo", "");
    //         size_t at_pos = name.find("@");
    //         if (at_pos != std::string::npos)
    //             name = name.substr(0, at_pos);
    //         boost::trim(name);
    //         boost::replace_all(name, " ", "-");
    //         boost::to_upper(name);
    //         ams_config_map.emplace(name, filament_config);
    //     }
    //     return ams_config_map;
    // }

    // void ElegooLink::getFilamentMapping(PrintAmsTray& tray, std::vector<Preset*>& filaments)
    // {
    //     // std::string name = tray.filament_name;
    //     // boost::replace_all(name, "Elegoo", "");
    //     // size_t at_pos = name.find("@");
    //     // if (at_pos != std::string::npos)
    //     //     name = name.substr(0, at_pos);
    //     // boost::trim(name);hh

    //     // for (const auto& filament : filaments) {
    //     //   std::string vendor = filament->config.option<ConfigOptionString>("filament_vendor")->value;
    //     //   std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::toupper);
    //     //    if (vendor  == "ELEGOO") {
    //     //         // Get printer model
    //     //         std::string printer_model = filament->config.option<ConfigOptionString>("printer_model")->value;
    //     //         // Get printer model
    //     //         std::string printer_model = filament->config.option<ConfigOptionString>("printer_model")->value;
    //     //         // Get printer model
    //     //         std::string printer_model = filament->config.option<ConfigOptionString>("printer_model")->value;
    //     //         tray.filament_type = filament->filament_type;
    //     //         tray.filament_color = filament->filament_color;
    //     //         tray.filament_name = filament->filament_name;
    //     //    }
    //     // }
        
    // }
    void ElegooLink::getTrayFilamentMapping(PrintAmsTray& tray, const std::map<std::string, std::string>& vendor_filament_type_filament_ids, const std::map<std::string, std::string>& generic_filament_type_filament_ids)
    {
        std::string  brand = boost::to_upper_copy(tray.brand);
        std::string filament_name = boost::to_upper_copy(tray.filament_name);
        std::string  filament_type = boost::to_upper_copy(tray.filament_type);
        std::string  filament_id = "";
        if(brand == "ELEGOO"){
            auto it = vendor_filament_type_filament_ids.find(filament_name);
            if(it != vendor_filament_type_filament_ids.end()){
                filament_id = it->second;
                tray.filament_id = filament_id;
                return;
            } else {
                it = vendor_filament_type_filament_ids.find(filament_type);
                if (it != vendor_filament_type_filament_ids.end()) {
                    filament_id      = it->second;
                    tray.filament_id = filament_id;
                    return;
                }           
            }
        }
        auto it = generic_filament_type_filament_ids.find(filament_name);
        if(it != generic_filament_type_filament_ids.end()){
            filament_id = it->second;
            tray.filament_id = filament_id;
            return;
        } else {
            it = generic_filament_type_filament_ids.find(filament_type);
            if (it != generic_filament_type_filament_ids.end()) {
                filament_id      = it->second;
                tray.filament_id = filament_id;
                return;
            }
        }
        BOOST_LOG_TRIVIAL(error) << "ElegooLink::getTrayFilamentMapping: not found filament_id " << brand << " " << filament_name;
        return;
    }

    PrintAmsInfo ElegooLink::get_ams(const std::map<std::string, std::string>& vendor_filament_type_filament_ids, const std::map<std::string, std::string>& generic_filament_type_filament_ids)
    {
        return amsVirtualTestData(vendor_filament_type_filament_ids, generic_filament_type_filament_ids);

        if (m_host.empty()) {
            return PrintAmsInfo();
        }

        PrintAmsInfo ams_info;
        std::string     machine = "";
        WebSocketClient client;
        try {
            std::string wsUrl = get_host_from_url_no_port(m_host);
            client.connect(wsUrl, "3030", "/websocket");
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error getting attributes: " << e.what();
            return ams_info;
        }

        try {
            // Subscribe to AMS status topic
            std::string subscribe_msg = R"({
                "Id":"",
                "Data":{
                    "Cmd":0,
                    "Data":{},
                    "RequestID":"",
                    "MainboardID":"",
                    "TimeStamp":0,
                    "From":1
                },
                "Topic":"sdcp/ams/+/status"
            })";

            BOOST_LOG_TRIVIAL(info) << "Subscribing to AMS status: " << subscribe_msg;
            client.send(subscribe_msg);

            // Wait for response with timeout
            auto start_time = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(10)) {
                std::string response = client.receive();
                BOOST_LOG_TRIVIAL(info) << "Received AMS status: " << response;

                pt::ptree          root;
                std::istringstream is(response);
                pt::read_json(is, root);

                // Check if this is an AMS status message
                auto topic = root.get_optional<std::string>("Topic");
                if (!topic || topic->find("sdcp/ams/") == std::string::npos || topic->find("/status") == std::string::npos) {
                    continue;
                }

                auto data = root.get_child_optional("Data");
                if (!data)
                    continue;

                auto ams_data = data->get_child_optional("Data");
                if (!ams_data)
                    continue;

                // Parse AMS connection status
                ams_info.ams_connect_status     = ams_data->get<bool>("ams_connect_status", false);
                ams_info.ams_type               = ams_data->get<std::string>("ams_type", "");
                ams_info.ams_connect_num        = ams_data->get<int>("ams_connect_num", 0);
                ams_info.nozzle_filament_status = ams_data->get<bool>("nozzle_filament_status", false);

                auto ams_list = ams_data->get_child_optional("ams_list");
                if (ams_list) {
                    for (const auto& ams_obj : *ams_list) {
                        PrintAms ams;
                        ams.ams_id      = ams_obj.second.get<std::string>("ams_id", "");
                        ams.temperature = ams_obj.second.get<double>("temperature", 0.0);
                        ams.humidity    = ams_obj.second.get<int>("humidity", 0);

                        // Parse trays
                        auto trays = ams_obj.second.get_child_optional("tray_list");
                        if (trays) {
                            for (const auto& tray_obj : *trays) {
                                PrintAmsTray tray;
                                tray.tray_id           = tray_obj.second.get<std::string>("tray_id", "");
                                tray.from              = tray_obj.second.get<std::string>("from", "");
                                tray.brand             = tray_obj.second.get<std::string>("brand", "");
                                tray.serial_number     = tray_obj.second.get<int>("serial_number", 0);
                                tray.filament_type     = tray_obj.second.get<std::string>("filament_type", "");
                                tray.filament_name     = tray_obj.second.get<std::string>("filament_name", "");
                                tray.filament_color    = tray_obj.second.get<std::string>("filament_color", "");
                                tray.filament_diameter = tray_obj.second.get<std::string>("filament_diameter", "");
                                tray.min_nozzle_temp   = tray_obj.second.get<int>("min_nozzle_temp", 0);
                                tray.max_nozzle_temp   = tray_obj.second.get<int>("max_nozzle_temp", 0);
                                tray.min_bed_temp      = tray_obj.second.get<int>("min_bed_temp", 0);
                                tray.max_bed_temp      = tray_obj.second.get<int>("max_bed_temp", 0);
                                tray.enabled           = tray_obj.second.get<bool>("enabled", false);
                                getTrayFilamentMapping(tray, vendor_filament_type_filament_ids, generic_filament_type_filament_ids);
                                ams.tray_list.push_back(tray);
                            }
                        }

                        ams_info.ams_list.push_back(ams);
                    }
                }

                // Parse virtual tray
                auto vt_tray_obj = ams_data->get_child_optional("vt_tray");
                if (vt_tray_obj) {
                    PrintAmsTray vt_tray;
                    vt_tray.tray_id           = vt_tray_obj->get<std::string>("tray_id", "");
                    vt_tray.from              = vt_tray_obj->get<std::string>("from", "");
                    vt_tray.brand             = vt_tray_obj->get<std::string>("brand", "");
                    vt_tray.serial_number     = vt_tray_obj->get<int>("serial_number", 0);
                    vt_tray.filament_type     = vt_tray_obj->get<std::string>("filament_type", "");
                    vt_tray.filament_name     = vt_tray_obj->get<std::string>("filament_name", "");
                    vt_tray.filament_color    = vt_tray_obj->get<std::string>("filament_color", "");
                    vt_tray.filament_diameter = vt_tray_obj->get<std::string>("filament_diameter", "");
                    vt_tray.min_nozzle_temp   = vt_tray_obj->get<int>("min_nozzle_temp", 0);
                    vt_tray.max_nozzle_temp   = vt_tray_obj->get<int>("max_nozzle_temp", 0);
                    vt_tray.min_bed_temp      = vt_tray_obj->get<int>("min_bed_temp", 0);
                    vt_tray.max_bed_temp      = vt_tray_obj->get<int>("max_bed_temp", 0);
                    vt_tray.enabled           = vt_tray_obj->get<bool>("enabled", false);
                    getTrayFilamentMapping(vt_tray, vendor_filament_type_filament_ids, generic_filament_type_filament_ids);
                    ams_info.vt_tray = vt_tray;
                }

                break;
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error getting AMS status: " << e.what();
        }

        return ams_info;
    }

    std::string ElegooLink::buildAmsDataString(const std::string& filamentAmsMapping) const
    {
        if (filamentAmsMapping.empty())
            return "";

        nlohmann::json amsMapping = nlohmann::json::parse(filamentAmsMapping);
        std::string    colorMapping;
        const auto&    filamentList = amsMapping["filamentList"];
        int            colorNum     = amsMapping.value("colorNum", 0);
        int            amsNum       = amsMapping.value("amsNum", 0);

        for (const auto& filament : filamentList) {
            std::string color        = filament.value("filamentColor", "");
            std::string amsId        = filament.value("amsId", "");
            std::string trayId       = filament.value("trayId", "");
            std::string filamentType = filament.value("filamentType", "");
            colorMapping += R"({"color":")" + color + R"(","ams_id":")" + amsId + R"(","tray_id":")" + trayId + R"(","filament_type":")" +
                            filamentType + R"("},)";
        }
        if (!colorMapping.empty() && colorMapping.back() == ',')
            colorMapping.pop_back();

        return R"("ColorNum":)" + std::to_string(colorNum) + R"(,"AmsNum":)" + std::to_string(amsNum) + R"(,"ColorMapping":[)" +
               colorMapping + R"(])";
    }

    PrintAmsInfo ElegooLink::amsVirtualTestData(const std::map<std::string, std::string>& vendor_filament_type_filament_ids,
                                                const std::map<std::string, std::string>& generic_filament_type_filament_ids)
    {
        PrintAmsInfo ams_info;
        ams_info.ams_connect_status     = true;
        ams_info.ams_type               = "0302";
        ams_info.ams_connect_num        = 1;
        ams_info.nozzle_filament_status = true;

        PrintAms ams;
        ams.ams_id      = "0";
        ams.temperature = 25.5;
        ams.humidity    = 45;

        PrintAmsTray tray;
        tray.tray_id           = "0";
        tray.brand             = "Elegoo";
        tray.serial_number     = 0;
        tray.filament_type     = "PLA";
        tray.filament_name     = "PLA";
        tray.filament_color    = "#FFFFFF";
        tray.filament_diameter = "1.75";
        tray.min_nozzle_temp   = 180;
        tray.max_nozzle_temp   = 220;
        tray.min_bed_temp      = 0;
        tray.max_bed_temp      = 60;
        tray.enabled           = true;

        getTrayFilamentMapping(tray, vendor_filament_type_filament_ids, generic_filament_type_filament_ids);
        ams.tray_list.push_back(tray);

        PrintAmsTray tray2;
        tray2.tray_id           = "1";
        tray2.from              = "ams";
        tray2.brand             = "Elegoo";
        tray2.serial_number     = 1;
        tray2.filament_type     = "PLA";
        tray2.filament_name     = "PLA+";
        tray2.filament_color    = "#000000";
        tray2.filament_diameter = "1.75";
        tray2.min_nozzle_temp   = 180;
        tray2.max_nozzle_temp   = 220;
        tray2.min_bed_temp      = 0;
        tray2.max_bed_temp      = 60;
        tray2.enabled           = true;

        getTrayFilamentMapping(tray2, vendor_filament_type_filament_ids, generic_filament_type_filament_ids);
        ams.tray_list.push_back(tray2);

        PrintAmsTray tray3;
        tray3.tray_id        = "2";
        tray3.from           = "ams";
        tray3.brand          = "Elegoo";
        tray3.serial_number  = 2;
        tray3.filament_type  = "PLA";
        tray3.filament_name  = "PLA";
        tray3.filament_color = "#FF0000";

        getTrayFilamentMapping(tray3, vendor_filament_type_filament_ids, generic_filament_type_filament_ids);
        ams.tray_list.push_back(tray3);

        PrintAmsTray tray4;
        tray4.tray_id        = "3";
        tray4.from           = "ams";
        tray4.brand          = "Elegoo";
        tray4.serial_number  = 3;
        tray4.filament_type  = "PLA";
        tray4.filament_name  = "PLA";
        tray4.filament_color = "#808080";

        getTrayFilamentMapping(tray4, vendor_filament_type_filament_ids, generic_filament_type_filament_ids);
        ams.tray_list.push_back(tray4);

        if (ams.tray_list.size() > 0) {
            ams_info.ams_list.push_back(ams);
        }
        return ams_info;
    }
}
