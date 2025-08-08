#pragma once

#include <wx/dialog.h>
#include <wx/webview.h>
#include <wx/colour.h>
#include <nlohmann/json.hpp>
#include "Plater.hpp"
#include "PrintHostDialogs.hpp"
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
                    bool                           switchToDeviceTab,
                    const std::map<int, DynamicPrintConfig>& filamentAmsList);

    ~PrintSendDialogEx();

    virtual void EndModal(int ret) override;

    virtual void                               init() override;
    virtual std::map<std::string, std::string> extendedInfo() const
    {
        return {{"bedType", std::to_string(mBedType)},
                {"timeLapse", mTimeLapse ? "true" : "false"},
                {"heatedBedLeveling", mHeatedBedLeveling ? "true" : "false"},
                {"autoRefill", mAutoRefill ? "true" : "false"},
                {"selectedPrinterId", mSelectedPrinterId},
                {"filamentAmsMapping", mFilamentAmsMapping.dump()}};
    }   

private:
    void onScriptMessage(wxWebViewEvent &evt);
    void runScript(const wxString &javascript);
    nlohmann::json preparePrintTask();
    nlohmann::json getPrinterList();
    void onPrint(const nlohmann::json &printInfo);
    void onCancel();
    std::string getCurrentProjectName();
    void getFilamentAmsMapping(nlohmann::json &filamentList, nlohmann::json &trayFilamentList);
    void saveFilamentAmsMapping(nlohmann::json &filamentList);
    
    BedType appBedType() const;
    void    refresh();

    wxWebView* mBrowser;

    Plater*  mPlater{ nullptr };
    int mPrintPlateIdx;
    ThumbnailData                       mCurInputThumbnailData;
    ThumbnailData                       mCurNoLightThumbnailData;
    ThumbnailData                       mPreviewThumbnailData;//when ams map change
    std::vector<wxColour>               mPreviewColorsInThumbnail;
    std::vector<wxColour>               mCurColorsInThumbnail;
    std::vector<bool>                   mEdgePixels;
    bool    mTimeLapse{false};
    bool    mHeatedBedLeveling;
    BedType mBedType;
    bool    mAutoRefill;
    std::string mSelectedPrinterId;
    std::string mProjectName;
    std::map<int, DynamicPrintConfig> mFilamentAmsList;
    nlohmann::json mFilamentAmsMapping;
   
};
}} // namespace Slic3r::GUI 
