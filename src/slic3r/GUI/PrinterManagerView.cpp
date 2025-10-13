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
#include <wx/dc.h>
#include <wx/bitmap.h>
#include <wx/aui/auibook.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
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
#include <chrono>
#include <boost/filesystem.hpp>
#include "slic3r/Utils/WebviewIPCManager.h"
#include <fstream>
#include <mutex>
#include "slic3r/Utils/PrinterNetworkEvent.hpp"

#define FIRST_TAB_NAME _L("Connected Printer")
#define TAB_MAX_WIDTH 200
#define TAB_MIN_WIDTH 20
#define TAB_PADDING 8
#define TAB_ICON_TEXT_SPACING 8
#define TAB_CLOSE_BUTTON_SIZE 20
#define TAB_CLOSE_BUTTON_MARGIN 6
#define TAB_SEPARATOR_WIDTH 1
#define TAB_HEIGHT 28
#define TAB_BORDER_WIDTH 1

namespace Slic3r {
namespace GUI {

// Static mutex for tab state file operations
static std::mutex s_tabStateMutex;

class TabArt : public wxAuiDefaultTabArt
{
private:
    bool isDarkMode() const {
        return GUI_App::dark_mode();
    }

    wxAuiNotebook* getNotebookFrom(wxWindow* wnd) const {
        wxWindow* current = wnd;
        while (current) {
            if (auto nb = dynamic_cast<wxAuiNotebook*>(current)) {
                return nb;
            }
            current = current->GetParent();
        }
        return nullptr;
    }

    int calculateTabWidth(wxWindow* wnd, bool isFirstTab) const {
        if (isFirstTab) {
            return TAB_MAX_WIDTH;
        }
        
        wxAuiNotebook* notebook = getNotebookFrom(wnd);
        if (!notebook) {
            return TAB_MAX_WIDTH;
        }
        
        const int totalPages = notebook->GetPageCount();
        const int otherPages = totalPages - 1; // Exclude first tab
        
        if (otherPages <= 0) {
            return TAB_MAX_WIDTH;
        }
        
        const int totalWidth = notebook->GetClientSize().x;
        const int availableWidth = totalWidth - TAB_MAX_WIDTH;
        
        if (availableWidth <= 0) {
            return TAB_MIN_WIDTH;
        }
        
        const int avgWidth = availableWidth / otherPages;
        return std::max(TAB_MIN_WIDTH, std::min(TAB_MAX_WIDTH, avgWidth));
    }

    void drawTabContent(wxDC& dc, const wxRect& tab_rect, const wxBitmap& icon, 
                       const wxString& text, const wxColour& text_colour, bool isFirstTab) const {
        dc.SetTextForeground(text_colour);
        wxSize icon_size = icon.IsOk() ? icon.GetSize() : wxSize(0, 0);
        if (isFirstTab) {
            icon_size = wxSize(16, 16);
        } else {
            icon_size = wxSize(22, 12);
        }
       
        const wxSize text_size = dc.GetTextExtent(text);
        
        // Calculate positions
        const int icon_x = tab_rect.x + TAB_PADDING;
        const int text_x = icon_x + icon_size.x + (icon.IsOk() ? TAB_ICON_TEXT_SPACING : 0);
        const int icon_y = tab_rect.y + (tab_rect.height - icon_size.y) / 2;
        const int text_y = tab_rect.y + (tab_rect.height - text_size.y) / 2;
        
        // Draw icon
        if (icon.IsOk()) {
            dc.DrawBitmap(icon, icon_x, icon_y);
        }
        
        // Draw text with clipping (no ellipsis)
        const int close_button_space = isFirstTab ? 0 : (TAB_CLOSE_BUTTON_SIZE + TAB_CLOSE_BUTTON_MARGIN);
        const int text_max_width = tab_rect.x + tab_rect.width - text_x - close_button_space;
        
        if (text_max_width > 0) {
            wxRect text_clip(text_x, tab_rect.y, text_max_width, tab_rect.height);
            dc.SetClippingRegion(text_clip);
            dc.DrawText(text, text_x, text_y);
            dc.DestroyClippingRegion();
        }
    }

    void drawCloseButton(wxDC& dc, const wxRect& tab_rect, int tabWidth, 
                        int close_button_state, wxRect* out_button_rect) const {
        wxRect close_rect(
            tab_rect.x + tabWidth - TAB_CLOSE_BUTTON_SIZE - TAB_CLOSE_BUTTON_MARGIN,
            tab_rect.y + (tab_rect.height - TAB_CLOSE_BUTTON_SIZE) / 2,
            TAB_CLOSE_BUTTON_SIZE,
            TAB_CLOSE_BUTTON_SIZE
        );
        
        if (out_button_rect) *out_button_rect = close_rect;
        
        // Simple SVG with color modification
        wxBitmap close_icon = create_scaled_bitmap("topbar_close", nullptr, TAB_CLOSE_BUTTON_SIZE);
        if (close_icon.IsOk()) {
            auto close_icon_ = wxBitmap(close_icon.ConvertToImage().Rescale(TAB_CLOSE_BUTTON_SIZE, TAB_CLOSE_BUTTON_SIZE));
            // Change color based on state
            if (close_button_state == wxAUI_BUTTON_STATE_HOVER || close_button_state == wxAUI_BUTTON_STATE_PRESSED) {
                wxImage img = close_icon_.ConvertToImage();
                wxColour new_color = isDarkMode() ? wxColour(255, 120, 120) : wxColour(200, 0, 0);
                
                // Simple color replacement: replace dark pixels with new color
                for (int y = 0; y < img.GetHeight(); y++) {
                    for (int x = 0; x < img.GetWidth(); x++) {
                        if (img.GetAlpha(x, y) > 0) { // Only process non-transparent pixels
                            img.SetRGB(x, y, new_color.Red(), new_color.Green(), new_color.Blue());
                        }
                    }
                }
                close_icon_ = wxBitmap(img);
            }
            
            const int icon_x = close_rect.x + (close_rect.width - close_icon_.GetWidth()) / 2;
            const int icon_y = close_rect.y + (close_rect.height - close_icon_.GetHeight()) / 2;
            dc.DrawBitmap(close_icon_, icon_x, icon_y);
        }
    }

    void getColorScheme(wxColour& activeTab, wxColour& inactiveTab, wxColour& hoverTab, wxColour& activeText, 
                       wxColour& inactiveText, wxColour& background, wxColour& border, wxColour &tabHeaderBackground,
                       wxColour& separator) const {
        if (isDarkMode()) {
            // dark mode color scheme
            activeTab = wxColour(107, 107, 107);     
            inactiveTab = wxColour(45, 45, 48);   // inactive tab: light gray
            hoverTab = wxColour(70, 70, 70);      // hover tab: medium gray
            activeText = wxColour(255, 255, 255);    // active tab text: white
            inactiveText = wxColour(200, 200, 200);  // inactive tab text: light gray
            background = wxColour(45, 45, 45);       // tab bar background: dark gray
            border =  wxColour(0, 0, 0);           // border: black
            tabHeaderBackground = wxColour(45, 45, 45);   // tab header background: dark gray
            separator = wxColour(80, 80, 80);        
        } else {
            // light mode color scheme
            activeTab = wxColour(107, 107, 107);     
            inactiveTab = wxColour(56, 68, 70);      // inactive tab: dark gray
            hoverTab = wxColour(80, 90, 95);        // hover tab: lighter gray
            activeText = wxColour(255, 255, 255);    // active tab text: white
            inactiveText = wxColour(200, 200, 200);     // inactive tab text: dark gray
            background = wxColour(250, 250, 250);    // tab bar background: almost white
            border = wxColour(0, 0, 0);  // border: black
            tabHeaderBackground = wxColour(56, 68, 70);
            separator = wxColour(120, 120, 120); 
        }
    }

    // Get icon for tab based on caption
    wxBitmap getTabIcon(const wxString& caption) const {
        if (caption == FIRST_TAB_NAME) {
            return create_scaled_bitmap("printer_manager", nullptr, 16);  
        } else {
            return create_scaled_bitmap("elegoo_tab", nullptr, 12);  
        }
        return wxBitmap();
    }

public:
    TabArt() {
        wxColour activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator;
        getColorScheme(activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator);

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
        const bool isFirstTab = (page.caption == FIRST_TAB_NAME);
        const bool isActive = page.active;

        // Get color scheme
        wxColour activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator;
        getColorScheme(activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator);
        
        // Calculate tab width
        int tabWidth = calculateTabWidth(wnd, isFirstTab);
  
        wxRect tab_rect = in_rect;
        tab_rect.width = tabWidth - TAB_SEPARATOR_WIDTH;   
        tab_rect.height = tab_rect.height - TAB_BORDER_WIDTH;
        // Get icon and text
        wxBitmap icon = getTabIcon(page.caption);
        wxString text = page.caption;
        wxSize text_size = dc.GetTextExtent(text);
        wxSize icon_size = icon.IsOk() ? icon.GetSize() : wxSize(0, 0);
        

        // Check if mouse is over this tab
        wxPoint mousePos = wnd->ScreenToClient(wxGetMousePosition());
        bool mouseOverTab = tab_rect.Contains(mousePos);
        
        // Select colors based on active state and hover state
        wxColour tab_colour;
        if (isActive) {
            tab_colour = activeTab;
        } else if (mouseOverTab) {
            tab_colour = hoverTab;
        } else {
            tab_colour = inactiveTab;
        }
        
        const wxColour text_colour = isActive ? activeText : inactiveText;
        
        // Draw tab background for active tab or hovered tab except first tab is active
        dc.SetBrush(wxBrush(tab_colour));
        dc.SetPen(wxPen(tab_colour, 0)); 
        if ((!isFirstTab && isActive) || (mouseOverTab && !isActive)) {
            dc.DrawRectangle(tab_rect);
        }
        
        // Draw icon and text
        drawTabContent(dc, tab_rect, icon, text, text_colour, isFirstTab);

        // Draw close button for non-first tabs when mouse is over the tab or tab is active
        if (!isFirstTab && (isActive || mouseOverTab)) {
            drawCloseButton(dc, tab_rect, tabWidth, close_button_state, out_button_rect);
        }
        
        // Draw separator
        DrawTabSeparator(dc, tab_rect.x + tab_rect.width, tab_rect.y, tab_rect.height);
             
        // Set output parameters
        if (out_tab_rect) *out_tab_rect = tab_rect;
        if (x_extent) *x_extent = tabWidth;
    }
    
    void DrawBackground(wxDC& dc, wxWindow* wnd, const wxRect& rect) override
    {
        wxColour activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator;
        getColorScheme(activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator);

        // Draw header background
        auto headerRect = rect;
        dc.SetPen(wxPen(tabHeaderBackground, 0));
        dc.SetBrush(wxBrush(tabHeaderBackground));
        dc.DrawRectangle(rect);

        // Draw header bottom border
        dc.SetPen(wxPen(border, TAB_BORDER_WIDTH));
        dc.DrawLine(rect.x, rect.y + rect.height - TAB_BORDER_WIDTH, rect.x + rect.width, rect.y + rect.height - TAB_BORDER_WIDTH);
    }

    int GetBorderWidth(wxWindow* wnd) override {
        return 0; // Reserve space for top and bottom borders
    }

    wxSize GetTabSize(wxDC& dc, wxWindow* wnd, const wxString& caption, const wxBitmap& bitmap, bool active, int close_button_state, int* x_extent) override {
        // Get the default tab size
        wxSize default_size = wxAuiDefaultTabArt::GetTabSize(dc, wnd, caption, bitmap, active, close_button_state, x_extent);  
        // Return custom size with modified height
        return wxSize(default_size.x, TAB_HEIGHT);
    }
    void DrawBorder(wxDC& dc, wxWindow* wnd, const wxRect& rect) override
    {
        wxColour activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator;
        getColorScheme(activeTab, inactiveTab, hoverTab, activeText, inactiveText, background, border, tabHeaderBackground, separator);

        // Draw the main background
        dc.SetBrush(wxBrush(background));
        dc.SetPen(wxPen(background, 0));
        dc.DrawRectangle(rect);

        auto headerRect = rect;
        headerRect.height = TAB_HEIGHT + TAB_BORDER_WIDTH + 2;
        if(headerRect.height <= rect.height) {
            dc.SetPen(wxPen(tabHeaderBackground, 0));
            dc.SetBrush(wxBrush(tabHeaderBackground));
            dc.DrawRectangle(headerRect);
        }
        // Draw header top border
        dc.SetPen(wxPen(border, TAB_BORDER_WIDTH));
        dc.DrawLine(headerRect.x, headerRect.y, headerRect.x + headerRect.width, headerRect.y);
    }

    // Draw separator between tabs
    void DrawTabSeparator(wxDC& dc, int x, int y, int height) const
    {
        wxColour separator_color = isDarkMode() ? wxColour(80, 80, 80) : wxColour(120, 120, 120);
        dc.SetPen(wxPen(separator_color, TAB_SEPARATOR_WIDTH));
        dc.DrawLine(x, y, x, y + height);
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
    wxString strlang = wxGetApp().current_language_code_safe();
    if (strlang != "")
        TargetUrl = wxString::Format("%s?lang=%s", TargetUrl, strlang);
    if(wxGetApp().app_config->get_bool("developer_mode")){
            TargetUrl = TargetUrl + "&dev=true";
    }  
    //TargetUrl = "https://np-sit.elegoo.com.cn//elegooSlicer";
    mBrowser->LoadURL(TargetUrl);
    
    // 设置 ElegooSlicer UserAgent
    wxString theme = wxGetApp().dark_mode() ? "dark" : "light";
#ifdef __WIN32__
    mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (Windows NT 10.0; Win64; x64)", 
        SLIC3R_VERSION, theme));
#elif defined(__WXMAC__)
    mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)", 
        SLIC3R_VERSION, theme));
#elif defined(__linux__)
    mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (X11; Linux x86_64)", 
        SLIC3R_VERSION, theme));
#else
    mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (compatible; ElegooSlicer)", 
        SLIC3R_VERSION, theme));
#endif

    std::thread([this]() {
        try {

            // std::this_thread::sleep_for(std::chrono::seconds(10));
            // if (m_lifeTracker && *m_lifeTracker) {
                
            //     // 发送语言设置消息
            //     nlohmann::json languageMessage;
            //     languageMessage["code"] = "zh-CN";
            //     languageMessage["name"] = "中文";
                
            //     m_ipc->request("client.setLanguage", languageMessage, [](const webviewIpc::IPCResult& response) {
            //         BOOST_LOG_TRIVIAL(info) << "Language setting response: " << response.message;
            //     });


            //     // 发送区域设置消息
            //     nlohmann::json regionMessage;
            //     regionMessage["code"] = "CN";
            //     regionMessage["name"] = "中国大陆";
                
            //     m_ipc->request("client.setRegion", regionMessage, [](const webviewIpc::IPCResult& response) {
            //         BOOST_LOG_TRIVIAL(info) << "Region setting response: " << response.message;
            //     });
            //     BOOST_LOG_TRIVIAL(info) << "Sent region setting message via IPC after 3s delay";
          /*  } else {
                BOOST_LOG_TRIVIAL(info) << "PrinterManagerView destroyed, skipping language/region setting";
            }*/
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error in delayed language/region setting: " << e.what();
        }
    }).detach();

    // Load saved tab state
    loadTabState();

    Bind(wxEVT_CLOSE_WINDOW, &PrinterManagerView::onClose, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &PrinterManagerView::onClosePrinterTab, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_BEGIN_DRAG, &PrinterManagerView::onTabBeginDrag, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_DRAG_MOTION, &PrinterManagerView::onTabDragMotion, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_END_DRAG, &PrinterManagerView::onTabEndDrag, this);
    mTabBar->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &PrinterManagerView::onTabChanged, this);


}

PrinterManagerView::~PrinterManagerView() {
    m_isDestroying = true;
    
    // Save tab state before destruction
    saveTabState();
    
    // Reset the life tracker to signal all async operations that this object is being destroyed
    m_lifeTracker.reset();
    
    if (m_ipc) {
        delete m_ipc;
        m_ipc = nullptr;
    }
}

void PrinterManagerView::openPrinterTab(const std::string& printerId, bool saveState)
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
    wxString url = from_u8(printerInfo.webUrl);
    
    if(PrintHost::get_print_host_type(printerInfo.hostType) == htElegooLink && (printerInfo.printerModel == "Elegoo Centauri Carbon 2")) 
    {
        std::string accessCode = printerInfo.accessCode;
        url = url + wxString("?id=") + from_u8(printerInfo.printerId) + "&ip=" + printerInfo.host +"&sn=" + from_u8(printerInfo.serialNumber) + "&access_code=" + accessCode;
    }

    view->load_url(url);

    mTabBar->AddPage(view, from_u8(printerInfo.host));
    mTabBar->SetSelection(mTabBar->GetPageCount() - 1);
    mPrinterViews[printerId] = view;
    Layout();
    
    // Update tab state after adding new tab
    if(saveState)
        saveTabState();
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
    
    // Update tab state after closing tab
    saveTabState();
    
    event.Skip();
}

void PrinterManagerView::onTabBeginDrag(wxAuiNotebookEvent& event)
{
    // Prevent dragging the first tab (index 0)
    if ( event.GetSelection() == 0  ) {
        event.Veto();
        mFirstTabClicked = true;
        return;
    }
    mFirstTabClicked = false;
    event.Skip();
}

void PrinterManagerView::onTabDragMotion(wxAuiNotebookEvent& event)
{
    if (mFirstTabClicked) {
        event.Veto();
        return;
    }
    // Prevent dropping at position 0
    wxPoint screenPt = wxGetMousePosition();
    wxPoint nbPt = mTabBar->ScreenToClient(screenPt);
    long flags = 0;
    int targetIndex = mTabBar->HitTest(nbPt, &flags);   
    if (targetIndex == 0) {
        event.Veto();
        return;
    }
    event.Skip();
}

void PrinterManagerView::onTabEndDrag(wxAuiNotebookEvent& event)
{
    if (mFirstTabClicked) {
        event.Veto();
        return;
    }   
    saveTabState();
    event.Skip();
}

void PrinterManagerView::onTabChanged(wxAuiNotebookEvent& event)
{
    // Save tab state when active tab changes
    saveTabState();
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

    
    PrinterNetworkEvent::getInstance()->connectStatusChanged.connect([this](const PrinterConnectStatusEvent& event) {
        // RTC token change handled by network layer
        for(auto it = mPrinterViews.begin(); it != mPrinterViews.end(); ++it) {
            if(it->first == event.printerId) {
                nlohmann::json data;
                data["status"] = event.status;
                it->second->onConnectionStatus(data);
                break;
            }
        }
    });
    PrinterNetworkEvent::getInstance()->eventRawChanged.connect([this](const PrinterEventRawEvent& event) {
        // Event raw change handled by network layer
        for(auto it = mPrinterViews.begin(); it != mPrinterViews.end(); ++it) {
            if(it->first == event.printerId) {
                nlohmann::json data;
                data["event"] = event.event;
                it->second->onPrinterEventRaw(data);
                break;
            }
        }
    });

    PrinterNetworkEvent::getInstance()->rtcTokenChanged.connect([this](const PrinterRtcTokenEvent& event) {
        // RTC token change handled by network layer
        for(auto it = mPrinterViews.begin(); it != mPrinterViews.end(); ++it) {
            nlohmann::json data;
            data["rtcToken"] = event.userInfo.rtcToken;
            data["userId"] = event.userInfo.userId;
            data["rtcTokenExpireTime"] = event.userInfo.rtcTokenExpireTime;
            it->second->onRtcTokenChanged(data);          
        }
    });
    PrinterNetworkEvent::getInstance()->rtmMessageChanged.connect([this](const PrinterRtmMessageEvent& event) {
        // RTM message change handled by network layer
        for(auto it = mPrinterViews.begin(); it != mPrinterViews.end(); ++it) {
            if(it->first == event.printerId) {
                nlohmann::json data;
                data["message"] = event.message;
                it->second->onRtmMessage(data);
                break;
            }
        }
    });
    
    m_ipc->onRequest("report.userInfo", [this](const webviewIpc::IPCRequest& request){
       auto data = request.params.value("data", nlohmann::json::object());
       std::string uuid = data.value("uuid", "");
       if(uuid.empty()) {
            return webviewIpc::IPCResult::success();
       }
       std::string user_id = data.value("user_id", "");
       std::string access_token = data.value("access_token", "");
       std::string refresh_token = data.value("refresh_token", "");
       int64_t expires_time = data.value("expires_time", 0);
       std::string openid = data.value("openid", "");
       std::string avatar = data.value("avatar", "");
       std::string email = data.value("email", "");
       std::string nickname = data.value("nickname", "");

       mBrowser->LoadURL("https://np-sit.elegoo.com.cn//elegooSlicer");

       //528270068@qq.com
       mBrowser->SetUserAgent(wxString::Format("ElegooSlicer/%s (%s) Mozilla/5.0 (Windows NT 10.0; Win64; x64)", 
        SLIC3R_VERSION, wxGetApp().dark_mode() ? "dark" : "light"));
        return webviewIpc::IPCResult::success();
    });


    m_ipc->onRequest("report.websiteOpen", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string url = params.value("url", "");
        wxLaunchDefaultBrowser(url);
        return webviewIpc::IPCResult::success();
    });


    m_ipc->onRequest("report.slicerOpen", [this](const webviewIpc::IPCRequest& request){
        auto params = request.params;
        std::string url = params.value("url", "");
        wxLaunchDefaultBrowser(url);
        return webviewIpc::IPCResult::success();
    });

    m_ipc->onRequest("report.notLogged", [this](const webviewIpc::IPCRequest& request){
        if (mIsLogin)
            return webviewIpc::IPCResult::success();
        mIsLogin     = true;
        wxString url = "https://np-sit.elegoo.com.cn//account//slicer-login?language=zh-CN&region=US";
        //wxLaunchDefaultBrowser(url);
        mBrowser->LoadURL(url);
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
        if(PrintHost::get_print_host_type(printerInfo.hostType) == htElegooLink && (printerInfo.printerModel == "Elegoo Centauri Carbon 2")) 
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
        errorCode = PrinterNetworkErrorCode::INVALID_PARAMETER;
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
        errorCode = PrinterNetworkErrorCode::PRINTER_UNKNOWN_ERROR;
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

void PrinterManagerView::saveTabState()
{
    std::lock_guard<std::mutex> lock(s_tabStateMutex);
    try {
        nlohmann::json tabState = nlohmann::json::object();
        nlohmann::json tabs = nlohmann::json::array();
        
        // Save all printer tabs (skip the first tab which is always "Connected Printer")
        for (size_t i = 1; i < mTabBar->GetPageCount(); ++i) {
            wxWindow* page = mTabBar->GetPage(i);
            if (page) {
                // Find the printer ID for this page
                for (const auto& pair : mPrinterViews) {
                    if (pair.second == page) {
                        nlohmann::json tabInfo;
                        tabInfo["printerId"] = pair.first;
                        tabInfo["tabName"] = mTabBar->GetPageText(i).ToStdString();
                        tabs.push_back(tabInfo);
                        break;
                    }
                }
            }
        }
        
        // Save current active tab index
        int activeTabIndex = mTabBar->GetSelection();
        tabState["tabs"] = tabs;
        tabState["activeTabIndex"] = activeTabIndex;
        
        // Save to file
        std::string filePath = (boost::filesystem::path(Slic3r::data_dir()) / "user" / "printer_tab_state.json").string();
        
        // Ensure directory exists
        boost::filesystem::path dir = boost::filesystem::path(filePath).parent_path();
        if (!boost::filesystem::exists(dir)) {
            boost::filesystem::create_directories(dir);
        }
        
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << tabState.dump(4);
            file.close();
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to save tab state: %s", e.what());
    }
}

void PrinterManagerView::loadTabState()
{
    std::lock_guard<std::mutex> lock(s_tabStateMutex);
    try {
        std::string filePath = (boost::filesystem::path(Slic3r::data_dir()) / "user" / "printer_tab_state.json").string();
        
        if (!boost::filesystem::exists(filePath)) {
            return; // No saved state
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return;
        }
        
        nlohmann::json tabState;
        file >> tabState;
        file.close();
        
        if (!tabState.is_object() || !tabState.contains("tabs")) {
            return;
        }
        
        nlohmann::json tabs = tabState["tabs"];
        if (!tabs.is_array()) {
            return;
        }
        
        // Load tabs in order
        for (const auto& tabInfo : tabs) {
            if (tabInfo.contains("printerId")) {
                std::string printerId = tabInfo["printerId"];
                openPrinterTab(printerId, false);
            }
        }
        int activeTabIndex = 0;
        // Restore active tab after all tabs are loaded
        if (tabState.contains("activeTabIndex")) {
            activeTabIndex = tabState["activeTabIndex"];
      
        }
        if (activeTabIndex >= 0 && activeTabIndex < mTabBar->GetPageCount()) {
            mTabBar->SetSelection(activeTabIndex);
        }
    } catch (const std::exception& e) {
        wxLogError("Failed to load tab state: %s", e.what());
    }
}

} // GUI
} // Slic3r


