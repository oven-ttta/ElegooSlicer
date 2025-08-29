#pragma once

#include <wx/dialog.h>
#include <wx/webview.h>
#include <wx/colour.h>
#include <nlohmann/json.hpp>
#include "Plater.hpp"
#include "PrintHostDialogs.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#include <slic3r/Utils/WebviewIPCManager.h>
#if wxUSE_WEBVIEW_IE
#include "wx/msw/webview_ie.h"
#endif
#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif

namespace webviewIpc {
    class WebviewIPCManager;
}

namespace Slic3r { namespace GUI {
class PrintSendDialogEx : public PrintHostSendDialog
{
public:
    PrintSendDialogEx(Plater*                        plater,
                    int                            printPlateIdx,
                    const boost::filesystem::path& path,
                    PrintHostPostUploadActions     postActions,
                    const wxArrayString&           groups,
                    const wxArrayString&           storagePaths,
                    const wxArrayString&           storageNames,
                    bool                           switchToDeviceTab);

    ~PrintSendDialogEx();

    virtual void EndModal(int ret) override;

    virtual void                               init() override;
    virtual boost::filesystem::path filename() const override{
        return into_path(m_cachedModelName);
    }
    virtual std::string group() const override {return "";}
    virtual std::string storage() const override {return "";}
    virtual std::map<std::string, std::string> extendedInfo() const override;

protected:
    void OnCloseWindow(wxCloseEvent& event);

    BedType getCurrentBedType() const;
private:
    void setupIPCHandlers();
    webviewIpc::IPCResult getPrinterList();
    webviewIpc::IPCResult preparePrintTask(const std::string &printerId);
    webviewIpc::IPCResult onPrint(const nlohmann::json &printInfo);
    void onCancel();
    std::string getCurrentProjectName(); 
    BedType appBedType() const;
    void    refresh();

    wxWebView* mBrowser;
    webviewIpc::WebviewIPCManager* mIpc = nullptr;
    Plater*  mPlater{ nullptr };
    int mPrintPlateIdx;

    bool    mTimeLapse{false};
    bool    mHeatedBedLeveling;
    BedType mBedType;
    bool    mAutoRefill;
    std::string mSelectedPrinterId;
    std::string mProjectName;
    std::vector<PrintFilamentMmsMapping> mPrintFilamentList;
    bool mHasMms;

    // async operation tracking
    bool m_isDestroying{false};
    std::shared_ptr<bool> m_lifeTracker;
    bool m_asyncOperationInProgress{false};
    wxString m_cachedModelName;
   
};
}} // namespace Slic3r::GUI 
