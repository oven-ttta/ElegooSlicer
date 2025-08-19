#pragma once

#include <wx/dialog.h>
#include <wx/webview.h>
#include <wx/colour.h>
#include <nlohmann/json.hpp>
#include "Plater.hpp"
#include "PrintHostDialogs.hpp"
#include "libslic3r/PrinterNetworkInfo.hpp"
#if wxUSE_WEBVIEW_IE
#include "wx/msw/webview_ie.h"
#endif
#if wxUSE_WEBVIEW_EDGE
#include "wx/msw/webview_edge.h"
#endif
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
    virtual std::map<std::string, std::string> extendedInfo() const override;

private:
    void onScriptMessage(wxWebViewEvent &evt);
    void runScript(const wxString &javascript);
    nlohmann::json getPrinterList();
    nlohmann::json preparePrintTask(const std::string &printerId);
    void onPrint(const nlohmann::json &printInfo);
    void onCancel();
    std::string getCurrentProjectName(); 
    BedType appBedType() const;
    void    refresh();

    wxWebView* mBrowser;
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
   
};
}} // namespace Slic3r::GUI 
