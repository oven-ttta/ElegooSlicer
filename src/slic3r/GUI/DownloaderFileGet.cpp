#include "DownloaderFileGet.hpp"

#include <thread>
#include <curl/curl.h>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

#include "format.hpp"
#include "GUI.hpp"
#include "I18N.hpp"
#include "libslic3r/Utils.hpp"


#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
using namespace std;


namespace Slic3r {
namespace GUI {

const size_t DOWNLOAD_MAX_CHUNK_SIZE	= 10 * 1024 * 1024;
const size_t DOWNLOAD_SIZE_LIMIT		= 1024 * 1024 * 1024;


std::string FileGet::escape_url(const std::string& unescaped)
{
	std::string ret_val;
	CURL* curl = curl_easy_init();
	if (curl) {
		int decodelen;
		char* decoded = curl_easy_unescape(curl, unescaped.c_str(), unescaped.size(), &decodelen);
		if (decoded) {
			ret_val = std::string(decoded);
			curl_free(decoded);
		}
		curl_easy_cleanup(curl);
	}
	return ret_val;
}
bool FileGet::is_subdomain(const std::string& url, const std::string& domain)
{
	// domain should be f.e. printables.com (.com including)
	char* host;
	std::string host_string;
	CURLUcode rc;
	CURLU* curl = curl_url();
	if (!curl) {
		BOOST_LOG_TRIVIAL(error) << "Failed to init Curl library in function is_domain.";
		return false;
	}
	rc = curl_url_set(curl, CURLUPART_URL, url.c_str(), 0);
	if (rc != CURLUE_OK) {
		curl_url_cleanup(curl);
		return false;
	}
	rc = curl_url_get(curl, CURLUPART_HOST, &host, 0);
	if (rc != CURLUE_OK || !host) {
		curl_url_cleanup(curl);
		return false;
	}
	host_string = std::string(host);
	curl_free(host);
	// now host should be subdomain.domain or just domain
	if (domain == host_string) {
		curl_url_cleanup(curl);
		return true;
	}
	if(boost::ends_with(host_string, "." + domain)) {
		curl_url_cleanup(curl);
		return true;
	}
	curl_url_cleanup(curl);
	return false;
}

namespace {
unsigned get_current_pid()
{
#ifdef WIN32
	return GetCurrentProcessId();
#else
	return ::getpid();
#endif
}
}

// int = DOWNLOAD ID; string = file path
wxDEFINE_EVENT(EVT_DWNLDR_FILE_COMPLETE, wxCommandEvent);
// int = DOWNLOAD ID; string = error msg
wxDEFINE_EVENT(EVT_DWNLDR_FILE_ERROR, wxCommandEvent);
// int = DOWNLOAD ID; string = progress percent
wxDEFINE_EVENT(EVT_DWNLDR_FILE_PROGRESS, wxCommandEvent);
// int = DOWNLOAD ID; string = name
wxDEFINE_EVENT(EVT_DWNLDR_FILE_NAME_CHANGE, wxCommandEvent);
// int = DOWNLOAD ID; 
wxDEFINE_EVENT(EVT_DWNLDR_FILE_PAUSED, wxCommandEvent);
// int = DOWNLOAD ID; 
wxDEFINE_EVENT(EVT_DWNLDR_FILE_CANCELED, wxCommandEvent);

struct FileGet::priv
{
	const int m_id;
	std::string m_url;
	std::string m_filename;
	std::thread m_io_thread;
	wxEvtHandler* m_evt_handler;
	boost::filesystem::path m_dest_folder;
	boost::filesystem::path m_tmp_path; // path when ongoing download
	std::atomic_bool m_cancel { false };
	std::atomic_bool m_pause  { false };
	std::atomic_bool m_stopped { false }; // either canceled or paused - download is not running
	size_t m_written { 0 };
	size_t m_absolute_size { 0 };
	priv(int ID, std::string&& url, const std::string& filename, wxEvtHandler* evt_handler, const boost::filesystem::path& dest_folder);

	void get_perform();
};

FileGet::priv::priv(int ID, std::string&& url, const std::string& filename, wxEvtHandler* evt_handler, const boost::filesystem::path& dest_folder)
	: m_id(ID)
	, m_url(std::move(url))
	, m_filename(filename)
	, m_evt_handler(evt_handler)
	, m_dest_folder(dest_folder)
{
}
std::string extract_remote_filename(const std::string& str) {
    std::regex r("filename=\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(str.begin(), str.end(), match, r)) {
        return match.str(1);
    } else {
        return "";
    }
}
void FileGet::priv::get_perform()
{
	assert(m_evt_handler);
	assert(!m_url.empty());
	assert(!m_filename.empty());
	assert(boost::filesystem::is_directory(m_dest_folder));

	m_stopped = false;

	// open dest file
	std::string extension;
	if (m_written == 0)
	{
		boost::filesystem::path dest_path = m_dest_folder / m_filename;
		extension = dest_path.extension().string();
		std::string just_filename = m_filename.substr(0, m_filename.size() - extension.size());
		std::string final_filename = just_filename;
        // Find unsed filename 
		try {
			size_t version = 0;
			while (boost::filesystem::exists(m_dest_folder / (final_filename + extension)) || boost::filesystem::exists(m_dest_folder / (final_filename + extension + "." + std::to_string(get_current_pid()) + ".download")))
			{
				++version;
				if (version > 999) {
					wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_ERROR);
					evt->SetString(GUI::format_wxstr(L"Failed to find suitable filename. Last name: %1%." , (m_dest_folder / (final_filename + extension)).string()));
					evt->SetInt(m_id);
					m_evt_handler->QueueEvent(evt);
					return;
				}
				final_filename = GUI::format("%1%(%2%)", just_filename, std::to_string(version));
			}
		} catch (const boost::filesystem::filesystem_error& e)
		{
			wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_ERROR);
			evt->SetString(e.what());
			evt->SetInt(m_id);
			m_evt_handler->QueueEvent(evt);
			return;
		}

        m_filename = sanitize_filename(final_filename + extension);

        m_tmp_path = m_dest_folder / (m_filename + "." + std::to_string(get_current_pid()) + ".download");

		wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_NAME_CHANGE);
		evt->SetString(boost::nowide::widen(m_filename));
		evt->SetInt(m_id);
		m_evt_handler->QueueEvent(evt);
	}
	
	boost::filesystem::path dest_path;
	if(!extension.empty())
		dest_path = m_dest_folder / m_filename;

	wxString temp_path_wstring(m_tmp_path.wstring());
	
	//std::cout << "dest_path: " << dest_path.string() << std::endl;
	//std::cout << "m_tmp_path: " << m_tmp_path.string() << std::endl;
	
	BOOST_LOG_TRIVIAL(info) << GUI::format("Starting download from %1% to %2%. Temp path is %3%",m_url, dest_path, m_tmp_path);

	FILE* file;
	// open file for writting
	if (m_written == 0)
		file = fopen(temp_path_wstring.c_str(), "wb");
	else 
		file = fopen(temp_path_wstring.c_str(), "ab");

	//assert(file != NULL);
	if (file == NULL) {
		wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_ERROR);
		// TRN %1% = file path
		evt->SetString(GUI::format_wxstr(_L("Can't create file at %1%"), temp_path_wstring));
		evt->SetInt(m_id);
		m_evt_handler->QueueEvent(evt);
		return;
	}

		std:: string range_string = std::to_string(m_written) + "-";

		size_t written_previously = m_written;
		size_t written_this_session = 0;
		Http::get(m_url)
			.size_limit(DOWNLOAD_SIZE_LIMIT) //more?
		.set_range(range_string)
		.on_header_callback([&](std::string header) {
			if(dest_path.empty()) {
				std::string filename = extract_remote_filename(header);
				if (!filename.empty()) {
					m_filename = filename;
					dest_path = m_dest_folder / m_filename;
					wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_NAME_CHANGE);
					evt->SetString(boost::nowide::widen(m_filename));
					evt->SetInt(m_id);
					m_evt_handler->QueueEvent(evt);
				}
			}
		})
		.on_progress([&](Http::Progress progress, bool& cancel) {
			// to prevent multiple calls into following ifs (m_cancel / m_pause)
			if (m_stopped){
				cancel = true;
				return;
			}
			if (m_cancel) {
				m_stopped = true;
				fclose(file);
				// remove canceled file
				std::remove(m_tmp_path.string().c_str());
				m_written = 0;
				cancel = true;
				wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_CANCELED);
				evt->SetInt(m_id);
				m_evt_handler->QueueEvent(evt);
				return;
				// TODO: send canceled event?
			}		
			if (m_pause) {
				m_stopped = true;
				fclose(file);
				cancel = true;
				if (m_written == 0)
					std::remove(m_tmp_path.string().c_str());
				wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_PAUSED);
				evt->SetInt(m_id);
				m_evt_handler->QueueEvent(evt);
				return;
			}
			
			if (m_absolute_size < progress.dltotal) {
				m_absolute_size = progress.dltotal;
			}

			if (progress.dlnow != 0) {
			if (progress.dlnow - written_this_session > DOWNLOAD_MAX_CHUNK_SIZE || progress.dlnow == progress.dltotal) {
					try
					{
						std::string part_for_write = progress.buffer.substr(written_this_session, progress.dlnow);
						fwrite(part_for_write.c_str(), 1, part_for_write.size(), file);
					}
					catch (const std::exception& e)
					{
						// fclose(file); do it?
						wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_ERROR);
						evt->SetString(e.what());
						evt->SetInt(m_id);
						m_evt_handler->QueueEvent(evt);
						cancel = true;
						return;
					}
					written_this_session = progress.dlnow;
					m_written = written_previously + written_this_session;
				}
				wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_PROGRESS);
				int             percent_total = m_absolute_size == 0 ? 0 : (written_previously + progress.dlnow) * 100 / m_absolute_size;
				evt->SetString(std::to_string(percent_total));
				evt->SetInt(m_id);
				m_evt_handler->QueueEvent(evt);
			}
			
		})
		.on_error([&](std::string body, std::string error, unsigned http_status) {
			if (file != NULL)
				fclose(file);
			wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_ERROR);
			if (!error.empty())
				evt->SetString(GUI::from_u8(error));
			else
				evt->SetString(GUI::from_u8(body));
			evt->SetInt(m_id);
			m_evt_handler->QueueEvent(evt);
		})
		.on_complete([&](std::string body, unsigned /* http_status */) {

			// TODO: perform a body size check
			// 
			//size_t body_size = body.size();
			//if (body_size != expected_size) {
			//	return;
			//}
			try
			{
				// Orca: thingiverse need this
				if (m_written < body.size())
				{
					// this code should never be entered. As there should be on_progress call after last bit downloaded.
					std::string part_for_write = body.substr(m_written);
					fwrite(part_for_write.c_str(), 1, part_for_write.size(), file);

                    wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_PROGRESS);
                    evt->SetString(std::to_string(100));
                    evt->SetInt(m_id);
                    m_evt_handler->QueueEvent(evt);
                }
				fclose(file);
				boost::filesystem::rename(m_tmp_path, dest_path);
			}
			catch (const std::exception& /*e*/)
			{
				//TODO: report?
				//error_message = GUI::format("Failed to write and move %1% to %2%", tmp_path, dest_path);
				wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_ERROR);
				evt->SetString("Failed to write and move.");
				evt->SetInt(m_id);
				m_evt_handler->QueueEvent(evt);
				return;
			}

			wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_COMPLETE);
			evt->SetString(dest_path.wstring());
			evt->SetInt(m_id);
			m_evt_handler->QueueEvent(evt);
		})
		.perform_sync();

}

FileGet::FileGet(int ID, std::string url, const std::string& filename, wxEvtHandler* evt_handler, const boost::filesystem::path& dest_folder)
	: p(new priv(ID, std::move(url), filename, evt_handler, dest_folder))
{}

FileGet::FileGet(FileGet&& other) : p(std::move(other.p)) {}

FileGet::~FileGet()
{
	if (p && p->m_io_thread.joinable()) {
		p->m_cancel = true;
		p->m_io_thread.join();
	}
}

void FileGet::get()
{
	assert(p);
	if (p->m_io_thread.joinable()) {
			// This will stop transfers being done by the thread, if any.
			// Cancelling takes some time, but should complete soon enough.
			p->m_cancel = true;
			p->m_io_thread.join();
	}
	p->m_cancel = false;
	p->m_pause = false;
	p->m_io_thread = std::thread([this]() {
		p->get_perform();
		});
}

void FileGet::cancel()
{
	if(p && p->m_stopped) {
		if (p->m_io_thread.joinable()) {
			p->m_cancel = true;
			p->m_io_thread.join();
			wxCommandEvent* evt = new wxCommandEvent(EVT_DWNLDR_FILE_CANCELED);
			evt->SetInt(p->m_id);
			p->m_evt_handler->QueueEvent(evt);
		}
	}

	if (p)
		p->m_cancel = true;
	
}

void FileGet::pause()
{
	if (p) {
		p->m_pause = true;
	}
}
void FileGet::resume()
{
	assert(p);
	if (p->m_io_thread.joinable()) {
		// This will stop transfers being done by the thread, if any.
		// Cancelling takes some time, but should complete soon enough.
		p->m_cancel = true;
		p->m_io_thread.join();
	}
	p->m_cancel = false;
	p->m_pause = false;
	p->m_io_thread = std::thread([this]() {
		p->get_perform();
		});
}



int hex2i(char c)
{
    if (isdigit(c))
        return c - '0';

    if (isxdigit(c)) {
        if (isupper(c))
            return 10 + c - 'A';

        else
            return 10 + c - 'a';
    }

    return -1;
}

string url_decode(const string& str, bool replace_plus)
{
    string result;

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            char c1 = str[i + 1];
            char c2 = str[i + 2];
            int v1 = hex2i(c1);
            int v2 = hex2i(c2);
            if (v1 != -1 && v2 != -1) {
                char decoded = (v1 << 4) | v2;
                result += decoded;
                i += 2;
            } else {
                result += str[i];
            }
        } else if (replace_plus && str[i] == '+') {
            result += ' ';

        } else {
            result += str[i];
        }
    }

    return result;
}

void split(const string& s, char delimiter, vector<string>& segments)
{
    stringstream ss(s);
    string segment;
    while (getline(ss, segment, delimiter)) {
        segments.push_back(segment);
    }
}

void split_query(const string& query, vector<pair<string, string>>& params)
{
    stringstream ss(query);
    string pair;
    while (getline(ss, pair, '&')) {
        size_t pos = pair.find('=');

        if (pos != string::npos) {
            string key = pair.substr(0, pos);

            string value = pair.substr(pos + 1);

            params.emplace_back(key, value);

        } else {
            params.emplace_back(pair, "");
        }
    }
}

string trim(const string& s)
{
    size_t first = s.find_first_not_of(" \t");
    if (first == string::npos)
        return "";
    size_t last = s.find_last_not_of(" \t");
    return s.substr(first, (last - first + 1));
}

string parse_content_disposition(const string& content_disp)
{
    vector<string> parts;
    split(content_disp, ';', parts);
    for (const string& part : parts) {
        string trimmed = trim(part);

        if (trimmed.find("filename=") == 0) {
            string value_part = trimmed.substr(9);

            value_part = trim(value_part);
            char quote = '\0';

            if (!value_part.empty() && (value_part[0] == '"' || value_part[0] == '\'')) {
                quote = value_part[0];

                value_part = value_part.substr(1);
            }

            if (quote != '\0') {
                size_t end_quote = value_part.find(quote);

                if (end_quote != string::npos) {
                    value_part = value_part.substr(0, end_quote);
                }

            } else {
                size_t space_pos = value_part.find(' ');

                if (space_pos != string::npos) {
                    value_part = value_part.substr(0, space_pos);
                }
            }

            return value_part;
        }
    }

    return "";
}

string FileGet::filename_from_url(const string& url)
{
    size_t query_start = url.find('?');
    string path_part = url.substr(0, query_start);
    string query_part = (query_start != string::npos) ? url.substr(query_start + 1) : "";
    vector<string> path_segments;
    split(path_part, '/', path_segments);
    string filename_from_path;
    for (auto it = path_segments.rbegin(); it != path_segments.rend(); ++it) {
        if (!it->empty()) {
            filename_from_path = *it;

            break;
        }
    }

    filename_from_path = url_decode(filename_from_path, false);
    vector<pair<string, string>> query_params;
    split_query(query_part, query_params);
    string filename_from_query;
    string content_disp_value;

    for (const auto& param : query_params) {
        if (param.first == "filename" || param.first == "file_name") {
            string decoded_value = url_decode(param.second, true);
            // Replace directory separators with underscores to match browser behavior
            for (char& c : decoded_value) {
                if (c == '/' || c == '\\') {
                    c = '_';
                }
            }
            filename_from_query = decoded_value;

        } else if (param.first == "response-content-disposition") {
            content_disp_value = url_decode(param.second, true);
        }
    }

    string filename_from_cd;

    if (!content_disp_value.empty()) {
        string cd_filename = parse_content_disposition(content_disp_value);

        filename_from_cd = url_decode(cd_filename, false);
    }

    if (!filename_from_query.empty()) {
        return filename_from_query;

    } else if (!filename_from_cd.empty()) {
        return filename_from_cd;

    } 
    return filename_from_path;

}
}
}
