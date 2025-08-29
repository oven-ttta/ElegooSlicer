#include "PrinterManagerView.hpp"
#include "I18N.hpp"
#include "slic3r/GUI/wxExtensions.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "libslic3r_version.h"
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/toolbar.h>
#include <wx/textdlg.h>
#include <slic3r/GUI/Widgets/WebView.hpp>
#include <wx/webview.h>
#include "slic3r/Utils/PrinterManager.hpp"
#include "slic3r/Utils/PrintHost.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include <map>
#include <algorithm>
#include <thread>
#include <boost/filesystem.hpp>
#include "slic3r/Utils/WebviewIPCManager.h"

#define FIRST_TAB_NAME _L("Connected Printer")

namespace Slic3r {
namespace GUI {

class TabArt : public wxAuiDefaultTabArt
{
private:
    bool isDarkMode() const {
        return GUI_App::dark_mode();
    }

    void getColorScheme(wxColour& activeTab, wxColour& inactiveTab, wxColour& activeText, 
                       wxColour& inactiveText, wxColour& background, wxColour& border, wxColour &tabHeaderBackground) const {
        if (isDarkMode()) {
            // dark mode color scheme
            activeTab = wxColour(0, 120, 189);      // active tab: dark blue
            inactiveTab = wxColour(45, 45, 48);   // inactive tab: light gray
            activeText = wxColour(255, 255, 255);    // active tab text: white
            inactiveText = wxColour(200, 200, 200);  // inactive tab text: light gray
            background = wxColour(45, 45, 45);       // tab bar background: dark gray
            border =  wxColour(45, 45, 45);           // border: dark gray
            tabHeaderBackground = wxColour(45, 45, 45);   // tab header background: dark gray
        } else {
            // light mode color scheme
            activeTab = wxColour(0, 120, 189);      // active tab: steel blue
            inactiveTab = wxColour(56, 68, 70);      // inactive tab: dark gray
            activeText = wxColour(255, 255, 255);    // active tab text: white
            inactiveText = wxColour(200, 200, 200);     // inactive tab text: dark gray
            background = wxColour(250, 250, 250);    // tab bar background: almost white
            border = wxColour(250, 250, 250);  // border: light gray
            tabHeaderBackground = wxColour(56, 68, 70);
        }
    }

public:
    TabArt() {
        wxColour activeTab, inactiveTab, activeText, inactiveText, background, border, tabHeaderBackground;
        getColorScheme(activeTab, inactiveTab, activeText, inactiveText, background, border, tabHeaderBackground);

        SetColour(inactiveTab);        // inactive tab background color
        SetActiveColour(activeTab);    // active tab background color
    }

    wxAuiTabArt* Clone() override { return new TabArt(*this); }
    
    void DrawTab(wxDC& dc,
                 wxWindow* wnd,
                 const wxAuiNotebookPage& page,
                 const wxRect& in_rect,
                 int close_button_state,
                 wxRect* out_tab_rect,
                 wxRect* out_button_rect,
                 int* x_extent) override
    {
        bool isFirstTab = (page.caption == FIRST_TAB_NAME);

        // If it is the first tab, hide the close button
        if (isFirstTab) {
            close_button_state = wxAUI_BUTTON_STATE_HIDDEN;
        }

        // Completely custom drawing, do not call the parent class method to avoid white edge issues
        bool isActive = page.active;
        wxRect tab_rect = in_rect;

        wxColour tab_colour, text_colour, border_colour, background_colour, inactive_tab_colour, inactive_text_colour, tab_header_background_colour;
        getColorScheme(tab_colour, inactive_tab_colour, text_colour, inactive_text_colour, background_colour, border_colour, tab_header_background_colour);

        //  Select color based on tab state
        if (isActive) {
        } else {
            tab_colour = inactive_tab_colour;
            text_colour = inactive_text_colour;
        }

        // Calculate the actual size of the tab, text area
        wxString text = page.caption;
        wxSize text_size = dc.GetTextExtent(text);

        // Calculate tab width: text width + left/right margin + close button space
        int tab_width = text_size.x + 16; // Basic left/right margin
        if (!isFirstTab && close_button_state != wxAUI_BUTTON_STATE_HIDDEN) {
            tab_width += 20; // Close button space
        }

        // Adjust tab_rect width
        tab_rect.width = tab_width;

        // Completely fill the background to avoid white edges
        dc.SetBrush(wxBrush(tab_colour));
        dc.SetPen(wxPen(tab_colour, 0));
        dc.DrawRectangle(tab_rect);

        // Draw text
        dc.SetTextForeground(text_colour);

        // Calculate text position
        wxPoint text_pos;
        text_pos.x = tab_rect.x + 8; // Left margin
        text_pos.y = tab_rect.y + (tab_rect.height - text_size.y) / 2;
        dc.DrawText(text, text_pos);

        // Set close button position
        wxRect close_rect;
        if (!isFirstTab && close_button_state != wxAUI_BUTTON_STATE_HIDDEN) {
            close_rect.x = tab_rect.x + tab_rect.width - 18;
            close_rect.y = tab_rect.y + (tab_rect.height - 14) / 2;
            close_rect.width = 14;
            close_rect.height = 14;
            
            if (out_button_rect) *out_button_rect = close_rect;

            // Dark mode and light mode close button colors
            wxColour close_hover_bg, close_normal_colour, close_hover_colour;
            if (isDarkMode()) {
                close_hover_bg = wxColour(80, 40, 40);     // Dark red background
                close_normal_colour = wxColour(200, 200, 200); // Gray X
                close_hover_colour = wxColour(255, 120, 120);  // Light red X
            } else {
                close_hover_bg = wxColour(255, 200, 200);  // Light red background
                close_normal_colour = wxColour(200, 200, 200); // Gray X
                close_hover_colour = wxColour(200, 0, 0);      // Red X
            }

            // Draw close button background
            if (close_button_state == wxAUI_BUTTON_STATE_HOVER) {
                dc.SetBrush(wxBrush(close_hover_bg));
                dc.SetPen(wxPen(tab_colour, 0));
                dc.DrawRoundedRectangle(close_rect, 3);
            }

            // Draw close button X - centered in 14x14 area
            wxColour close_colour = (close_button_state == wxAUI_BUTTON_STATE_HOVER) ? 
                close_hover_colour : close_normal_colour;
            dc.SetPen(wxPen(close_colour, 2));
            
            // Calculate center point of the 14x14 button
            int center_x = close_rect.x + close_rect.width / 2;   // 7 pixels from left
            int center_y = close_rect.y + close_rect.height / 2;  // 7 pixels from top
            int half_size = 3; // X extends 2 pixels in each direction from center

            // Draw X lines centered in the button
            dc.DrawLine(center_x - half_size, center_y - half_size, 
                       center_x + half_size, center_y + half_size);
            dc.DrawLine(center_x + half_size, center_y - half_size, 
                       center_x - half_size, center_y + half_size);
        }

        // Set output parameters
        if (out_tab_rect) *out_tab_rect = tab_rect;
        if (x_extent) *x_extent = tab_rect.width;
    }
    
    void DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect) override
    {
        wxColour activeTab, inactiveTab, activeText, inactiveText, background, border, tabHeaderBackground;
        getColorScheme(activeTab, inactiveTab, activeText, inactiveText, background, border, tabHeaderBackground);


        dc.SetPen(wxPen(tabHeaderBackground, 1));
        dc.SetBrush(wxBrush(tabHeaderBackground));
        dc.DrawRectangle(rect);
    }

    int GetBorderWidth(
        wxWindow* wnd) override{
            return 0; // No border width needed
    }

    void DrawBorder(wxDC& dc, wxWindow* wnd, const wxRect& rect) override
    {
        wxColour activeTab, inactiveTab, activeText, inactiveText, background, border, tabHeaderBackground;
        getColorScheme(activeTab, inactiveTab, activeText, inactiveText, background, border, tabHeaderBackground);

        // Draw the main background area
        dc.SetBrush(wxBrush(background));
        dc.SetPen(wxPen(background, 1));
        dc.DrawRectangle(rect);

        // Draw only the top 26px area with tabHeaderBackground
        const int topAreaHeight = 25;
        if (rect.height >= topAreaHeight) {
            wxRect topRect = rect;
            topRect.height = topAreaHeight;
            dc.SetBrush(wxBrush(tabHeaderBackground));
            dc.SetPen(wxPen(tabHeaderBackground, 1));
            dc.DrawRectangle(topRect);
        }
        if(isDarkMode()){
            dc.SetPen(wxPen(wxColour(0x22, 0x22, 0x22), 1));
            dc.DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y);
        }else{
            dc.SetPen(wxPen(wxColour(0x60, 0x60, 0x60), 1));
            dc.DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y);
        }
    }
};


PrinterManagerView::PrinterManagerView(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), 
      m_isDestroying(false),
      m_lifeTracker(std::make_shared<bool>(true))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    mTabBar = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxAUI_NB_TOP | wxAUI_NB_TAB_MOVE | wxAUI_NB_CLOSE_ON_ALL_TABS | wxBORDER_NONE);
    mTabBar->SetArtProvider(new TabArt());
    mTabBar->SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));
    mainSizer->Add(mTabBar, 1, wxEXPAND);
    SetSizer(mainSizer);

    mBrowser = WebView::CreateWebView(mTabBar, "");
    if (mBrowser == nullptr) {
        wxLogError("Could not init mBrowser");
        return;
    }
    
    m_ipc = new webviewIpc::WebviewIPCManager(mBrowser);
    setupIPCHandlers();
    
    mTabBar->AddPage(mBrowser, FIRST_TAB_NAME);
    mBrowser->EnableAccessToDevTools(wxGetApp().app_config->get_bool("developer_mode"));
    auto _dir = resources_dir();
    std::replace(_dir.begin(), _dir.end(), '\\', '/');
    wxString TargetUrl = from_u8((boost::filesystem::path(resources_dir()) / "web/printer/printer_manager/printer.html").make_preferred().string());
    TargetUrl = "file://" + TargetUrl;
    mBrowser->LoadURL(TargetUrl);
    mTabBar->SetSelection(0);
    Bind(wxEVT_CLOSE_WINDOW, &PrinterManagerView::onClose, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &PrinterManagerView::onClosePrinterTab, this);
}

PrinterManagerView::~PrinterManagerView() {
    m_isDestroying = true;
    
    // Reset the life tracker to signal all async operations that this object is being destroyed
    m_lifeTracker.reset();
    
    if (m_ipc) {
        delete m_ipc;
        m_ipc = nullptr;
    }
}

void PrinterManagerView::openPrinterTab(const std::string& printerId)
{
    auto it = mPrinterViews.find(printerId);
    if (it != mPrinterViews.end()) {
        int idx = mTabBar->GetPageIndex(it->second);
        if (idx != wxNOT_FOUND) {
            mTabBar->SetSelection(idx);
            Layout();
            return;
        }
    }
    std::vector<PrinterNetworkInfo> printerList = PrinterManager::getInstance()->getPrinterList();
    PrinterNetworkInfo printerInfo;
    for (auto& printer : printerList) {
        if (printer.printerId == printerId) {
            printerInfo = printer;
            break;  
        }
    }
    if (printerInfo.printerId.empty()) {
        wxLogError("Printer %s not found", printerId.c_str());
        return;
    }
    PrinterWebView* view = new PrinterWebView(mTabBar);
    wxString url = printerInfo.webUrl;
    
    if(PrintHost::get_print_host_type(printerInfo.hostType) == htElegooLink && (printerInfo.printerModel == "Elegoo Centauri Carbon 2" || printerInfo.printerModel == "Elegoo Centauri 2")) 
    {
        std::string accessCode = printerInfo.accessCode;
        url = url + wxString("?id=") + from_u8(printerInfo.printerId) + "&ip=" + printerInfo.host +"&sn=" + from_u8(printerInfo.serialNumber) + "&access_code=" + accessCode;
    }

    view->load_url(url);
    mTabBar->AddPage(view, from_u8(printerInfo.printerName));
    mTabBar->SetSelection(mTabBar->GetPageCount() - 1);
    mPrinterViews[printerId] = view;
    Layout();
}

void PrinterManagerView::onClose(wxCloseEvent& evt)
{
    this->Hide();
}

void PrinterManagerView::onClosePrinterTab(wxAuiNotebookEvent& event)
{
    int page = event.GetSelection();
    if (page == wxNOT_FOUND) return;

    wxWindow* win = mTabBar->GetPage(page);
    for (auto it = mPrinterViews.begin(); it != mPrinterViews.end(); ++it) {
        if (it->second == win) {
            it->second->OnClose(wxCloseEvent());
            mPrinterViews.erase(it);
            mTabBar->SetSelection(0);
            break;
        }
    }
    event.Skip();
}
void PrinterManagerView::setupIPCHandlers()
{
    if (!m_ipc) return;

    // Handle request_printer_list
    m_ipc->onRequest("request_printer_list", [this](const webviewIpc::IPCRequest& request){
        return getPrinterList();
    });

    // Handle request_printer_model_list
    m_ipc->onRequest("request_printer_model_list", [this](const webviewIpc::IPCRequest& request){
        return getPrinterModelList();
    });

    // Handle request_printer_list_status
    m_ipc->onRequest("request_printer_list_status", [this](const webviewIpc::IPCRequest& request){
        return getPrinterListStatus();
    });

    // Handle request_printer_detail
    m_ipc->onRequest("request_printer_detail", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        if (!printerId.empty()) {
            openPrinterTab(printerId);
        }
        return webviewIpc::IPCResult::success();
    });

    // Handle request_discover_printers (async due to time-consuming operation)
    m_ipc->onRequestAsync("request_discover_printers", [this](const webviewIpc::IPCRequest& request, 
                                                           std::function<void(const webviewIpc::IPCResult&)> sendResponse) {  
        // Create a weak reference to track object lifetime
        std::weak_ptr<bool> life_tracker = m_lifeTracker;   
        // Run the discovery operation in a separate thread to avoid blocking the UI
        std::thread([life_tracker, sendResponse,this]() {
            try {
                // Check if the object still exists
                if (auto tracker = life_tracker.lock()) {
                    // Call the static method to avoid accessing this pointer
                    auto result = this->discoverPrinter();
                    // Final check before sending response
                    if (life_tracker.lock()) {
                        sendResponse(result);
                    }
                } else {
                    // Object was destroyed, don't send response
                    return;
                }
            } catch (const std::exception& e) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error(std::string("Discovery failed: ") + e.what()));
                }
            }
        }).detach();
    });

    // Handle request_add_printer (async)
    m_ipc->onRequestAsync("request_add_printer", [this](const webviewIpc::IPCRequest& request,
                                                        std::function<void(const webviewIpc::IPCResult&)> sendResponse) {  
        auto params = request.params;
        if (!params.contains("printer")) {
            sendResponse(webviewIpc::IPCResult::error("Missing printer parameter"));
            return;
        }
        
        // Create a weak reference to track object lifetime
        std::weak_ptr<bool> life_tracker = m_lifeTracker;
        nlohmann::json printer = params["printer"];
        
        // Run the add printer operation in a separate thread
        std::thread([life_tracker, printer, sendResponse, this]() {
            try {
                // Check if the object still exists
                if (auto tracker = life_tracker.lock()) {
                    auto result = addPrinter(printer);
                    
                    // Final check before sending response
                    if (life_tracker.lock()) {
                       sendResponse(result);
                    }
                }
            } catch (const std::exception& e) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error(std::string("Add printer failed: ") + e.what()));
                }
            } catch (...) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error("Add printer failed: Unknown error"));
                }
            }
        }).detach();
    });

    // Handle request_add_physical_printer (async)
    m_ipc->onRequestAsync("request_add_physical_printer", [this](const webviewIpc::IPCRequest& request,
                                                                 std::function<void(const webviewIpc::IPCResult&)> sendResponse) {
      
        auto params = request.params;
        if (!params.contains("printer")) {
            sendResponse(webviewIpc::IPCResult::error("Missing printer parameter"));
            return;
        }
        
        // Create a weak reference to track object lifetime
        std::weak_ptr<bool> life_tracker = m_lifeTracker;
        nlohmann::json printer = params["printer"];
        
        // Run the add physical printer operation in a separate thread
        std::thread([life_tracker, printer, sendResponse, this]() {
            try {
                // Check if the object still exists
                if (auto tracker = life_tracker.lock()) {
                    auto result = addPhysicalPrinter(printer);   
                    // Final check before sending response
                    if (life_tracker.lock()) {
                        sendResponse(result);
                    }
                } else {
                    // Object was destroyed, don't send response
                    return;
                }
            } catch (const std::exception& e) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error(std::string("Add physical printer failed: ") + e.what()));
                }
            } catch (...) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error("Add physical printer failed: Unknown error"));
                }
            }
        }).detach();
    });

    // Handle request_update_printer_name
    m_ipc->onRequest("request_update_printer_name", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        std::string printerName = params.value("printerName", "");
        return updatePrinterName(printerId, printerName);;
    });

    // Handle request_update_printer_host
    m_ipc->onRequestAsync("request_update_printer_host", [this](const webviewIpc::IPCRequest& request,
                                                                 std::function<void(const webviewIpc::IPCResult&)> sendResponse) {
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        std::string host = params.value("host", "");
 
        // Create a weak reference to track object lifetime
        std::weak_ptr<bool> life_tracker = m_lifeTracker;
        // Run the add printer operation in a separate thread
        std::thread([life_tracker, printerId, host, sendResponse, this]() {
            try {
                // Check if the object still exists
                if (auto tracker = life_tracker.lock()) {
                    auto result = updatePrinterHost(printerId, host);
                    // Final check before sending response
                    if (life_tracker.lock()) {
                        sendResponse(result);
                    }
                } else {
                    // Object was destroyed, don't send response
                    return;
                }
            } catch (const std::exception& e) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error(std::string("Update printer host failed: ") + e.what()));
                }
            } catch (...) {
                if (life_tracker.lock()) {
                    sendResponse(webviewIpc::IPCResult::error("Update printer host failed: Unknown error"));
                }
            }
        }).detach();
    });

    // Handle request_delete_printer
    m_ipc->onRequest("request_delete_printer", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string printerId = params.value("printerId", "");
        return deletePrinter(printerId);
    });

    // Handle request_browse_ca_file
    m_ipc->onRequest("request_browse_ca_file", [this](const webviewIpc::IPCRequest& request){
        return browseCAFile();
    });

    // Handle request_refresh_printers
    m_ipc->onRequest("request_refresh_printers", [this](const webviewIpc::IPCRequest& request){
        // Implementation for refresh printers
        return webviewIpc::IPCResult::success();
    });

    // Handle request_logout_print_host
    m_ipc->onRequest("request_logout_print_host", [this](const webviewIpc::IPCRequest& request){
        // Implementation for logout print host
        return webviewIpc::IPCResult::success();
    });

    // Handle request_connect_physical_printer
    m_ipc->onRequest("request_connect_physical_printer", [this](const webviewIpc::IPCRequest& request){
        // Implementation for connect physical printer
        return webviewIpc::IPCResult::success();
    });
}

webviewIpc::IPCResult PrinterManagerView::deletePrinter(const std::string& printerId)
{ 
    webviewIpc::IPCResult result;
    auto it = mPrinterViews.find(printerId);
    if (it != mPrinterViews.end()) {
        int page = mTabBar->GetPageIndex(it->second);
        if (page != wxNOT_FOUND) {
            mTabBar->DeletePage(page);
        }
        it->second->OnClose(wxCloseEvent());
        mPrinterViews.erase(it);
        mTabBar->SetSelection(0);
    }
    auto networkResult = PrinterManager::getInstance()->deletePrinter(printerId);
    result.message = networkResult.message;
    result.code = networkResult.isSuccess() ? 0 : static_cast<int>(networkResult.code);
    return result;
}
webviewIpc::IPCResult PrinterManagerView::updatePrinterName(const std::string& printerId, const std::string& printerName)
{
    webviewIpc::IPCResult result;
    auto it = mPrinterViews.find(printerId);
    if (it != mPrinterViews.end()) {
        int page = mTabBar->GetPageIndex(it->second);
        if (page != wxNOT_FOUND) {
            mTabBar->SetPageText(page, from_u8(printerName));
        }
    }
    auto networkResult = PrinterManager::getInstance()->updatePrinterName(printerId, printerName);
    result.message = networkResult.message;
    result.code = networkResult.isSuccess() ? 0 : static_cast<int>(networkResult.code);
    return result;
}
webviewIpc::IPCResult PrinterManagerView::updatePrinterHost(const std::string& printerId, const std::string& host)
{
    webviewIpc::IPCResult result;
    auto networkResult = PrinterManager::getInstance()->updatePrinterHost(printerId, host);
    result.message = networkResult.message;
    result.code = networkResult.isSuccess() ? 0 : static_cast<int>(networkResult.code);
    PrinterNetworkInfo printerInfo = PrinterManager::getInstance()->getPrinterNetworkInfo(printerId);
    
    if(result.code == 0 && !printerInfo.host.empty() && printerInfo.host != host) {     
        wxString url = printerInfo.webUrl;
        if(PrintHost::get_print_host_type(printerInfo.hostType) == htElegooLink && (printerInfo.printerModel == "Elegoo Centauri Carbon 2" || printerInfo.printerModel == "Elegoo Centauri 2")) 
        {
            std::string accessCode = printerInfo.accessCode;
            url = url + wxString("?id=") + from_u8(printerInfo.printerId) + "&ip=" + printerInfo.host +"&sn=" + from_u8(printerInfo.serialNumber) + "&access_code=" + accessCode;
        }
        auto it = mPrinterViews.find(printerId);
        if (it != mPrinterViews.end()) {
            int page = mTabBar->GetPageIndex(it->second);
            if (page != wxNOT_FOUND) {
                it->second->load_url(url);
            }
        }
    }
    return result;
}
webviewIpc::IPCResult PrinterManagerView::addPrinter(const nlohmann::json& printer)
{
    webviewIpc::IPCResult result;
    PrinterNetworkInfo printerInfo = convertJsonToPrinterNetworkInfo(printer);
    printerInfo.isPhysicalPrinter = false;
    auto networkResult = PrinterManager::getInstance()->addPrinter(printerInfo);
    result.message = networkResult.message;
    result.code = networkResult.isSuccess() ? 0 : static_cast<int>(networkResult.code);
    return result;
}
webviewIpc::IPCResult PrinterManagerView::addPhysicalPrinter(const nlohmann::json& printer)
{
    webviewIpc::IPCResult result;
    PrinterNetworkErrorCode errorCode = PrinterNetworkErrorCode::SUCCESS;
    PrinterNetworkInfo printerInfo;
    try {
        printerInfo.isPhysicalPrinter = true;
        printerInfo.hostType = printer["hostType"];
        printerInfo.host = printer["host"];
        printerInfo.printerName = printer["printerName"];
        printerInfo.vendor = printer["vendor"];
        printerInfo.printerModel = printer["printerModel"];
        auto networkResult = PrinterManager::getInstance()->addPrinter(printerInfo);
        result.message = networkResult.message;
        errorCode = networkResult.code;
    } catch (const std::exception& e) {
        wxLogMessage("Add physical printer error: %s", e.what());
        errorCode = PrinterNetworkErrorCode::INVALID_FORMAT;
        result.message = getErrorMessage(errorCode);
    }
    result.code = errorCode == PrinterNetworkErrorCode::SUCCESS ? 0 : static_cast<int>(errorCode);
    return result;
}
webviewIpc::IPCResult PrinterManagerView::discoverPrinter()
{
    webviewIpc::IPCResult result;
    auto printerListResult = PrinterManager::getInstance()->discoverPrinter();
    std::vector<PrinterNetworkInfo> printerList;
    if(printerListResult.hasData()) {
        printerList = printerListResult.data.value();
    }
    nlohmann::json response = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printer_obj = nlohmann::json::object();
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printer_obj["printerImg"] = PrinterManager::imageFileToBase64DataURI(img_path);
        response.push_back(printer_obj);
    }
    result.data = response;
    result.code = printerListResult.isSuccess() ? 0 : static_cast<int>(printerListResult.code);
    result.message = printerListResult.message;
    return result;
}
webviewIpc::IPCResult PrinterManagerView::getPrinterList()
{  
    webviewIpc::IPCResult result;
    auto printerList = PrinterManager::getInstance()->getPrinterList();
    nlohmann::json response = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printer_obj = nlohmann::json::object();
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        boost::filesystem::path resources_path(Slic3r::resources_dir());
        std::string img_path = resources_path.string() + "/profiles/" + printer.vendor + "/" + printer.printerModel + "_cover.png";
        printer_obj["printerImg"] = PrinterManager::imageFileToBase64DataURI(img_path);
        response.push_back(printer_obj);
    }
    result.data = response;
    result.code = 0;
    result.message = getErrorMessage(PrinterNetworkErrorCode::SUCCESS);
    return result;
}
webviewIpc::IPCResult PrinterManagerView::getPrinterListStatus()
{
    webviewIpc::IPCResult result;
    auto printerList = PrinterManager::getInstance()->getPrinterList();
    nlohmann::json response = json::array();
    for (auto& printer : printerList) {
        nlohmann::json printer_obj = nlohmann::json::object();
        printer_obj = convertPrinterNetworkInfoToJson(printer);
        response.push_back(printer_obj);
    }
    result.data = response;
    result.code = 0;
    result.message = getErrorMessage(PrinterNetworkErrorCode::SUCCESS);
    return result;
}
webviewIpc::IPCResult PrinterManagerView::browseCAFile()
{
    webviewIpc::IPCResult result;
    PrinterNetworkErrorCode errorCode = PrinterNetworkErrorCode::SUCCESS;
    std::string path = "";
    try {
        static const auto filemasks = _L("Certificate files (*.crt, *.pem)|*.crt;*.pem|All files|*.*");
        wxFileDialog openFileDialog(this, _L("Open CA certificate file"), "", "", filemasks, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() != wxID_CANCEL) {
            path = openFileDialog.GetPath().ToStdString();
        }
    } catch (const std::exception& e) {
        wxLogMessage("Browse CA file error: %s", e.what());
        errorCode = PrinterNetworkErrorCode::INTERNAL_ERROR;
    }
    result.data = path;
    result.code = errorCode == PrinterNetworkErrorCode::SUCCESS ? 0 : static_cast<int>(errorCode);
    result.message = getErrorMessage(errorCode);
    return result;
}
webviewIpc::IPCResult PrinterManagerView::getPrinterModelList()
{
    webviewIpc::IPCResult result;
    auto vendorPrinterModelConfigMap = PrinterManager::getVendorPrinterModelConfig();   
    nlohmann::json response = nlohmann::json::array();
    for (auto& vendor : vendorPrinterModelConfigMap) {
        nlohmann::json vendorObj = nlohmann::json::object();
        vendorObj["vendor"]      = vendor.first;
        vendorObj["models"]      = nlohmann::json::array();
        for (auto& model : vendor.second) {
            nlohmann::json modelObj = nlohmann::json::object();
            modelObj["modelName"]  = model.first;
            auto config = model.second;
            const auto opt = config.option<ConfigOptionEnum<PrintHostType>>("host_type");
            const auto hostType = opt != nullptr ? opt->value : htOctoPrint;
            std::string hostTypeStr = PrintHost::get_print_host_type_str(hostType);
            if (!hostTypeStr.empty()) {
                modelObj["hostType"]   = hostTypeStr;
            }
            vendorObj["models"].push_back(modelObj);
        }
        response.push_back(vendorObj);
    }
    result.data = response;
    result.code = 0;
    result.message = getErrorMessage(PrinterNetworkErrorCode::SUCCESS);
    return result;
}
} // GUI
} // Slic3r


