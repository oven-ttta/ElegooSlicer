const { createI18n } = VueI18n;
const langMessages = {
  en: {
    "connectedPrinters": "Connected Printers",
    "addPrinter": "Add Printer",
    "delete": "Delete",
    "confirm": "Confirm",
    "cancel": "Cancel",
    "noPrinterConnected": "No printer connected. Please go to \"Devices\" to discover or add a new printer.",
    "multiColor": {
      "CanvasPro": "CANVAS PRO",
    },
    "printerManager": {
      "connectedPrinters": "Connected Printers",
      "addPrinter": "Add Printer",
      "localPrinters": "Local Printers",
      "noPrintersAvailable": "No Printers Available",
      "details": "Details",
      "offline": "Offline",
      "idle": "Idle",
      "printing": "Printing",
      "paused": "Paused",
      "pausing": "Pausing",
      "canceled": "Canceled",
      "selfChecking": "Self-checking",
      "autoLeveling": "Auto-leveling",
      "pidCalibrating": "PID calibrating",
      "resonanceTesting": "Resonance testing",
      "updating": "Updating",
      "fileCopying": "File copying",
      "fileTransferring": "File transferring",
      "homing": "Homing",
      "preheating": "Preheating",
      "filamentOperating": "Filament operating",
      "extruderOperating": "Extruder operating",
      "printCompleted": "Print completed",
      "rfidRecognizing": "RFID recognizing",
      "error": "Error",
      "unknown": "Unknown",
      "printerSettings": "Printer Settings",
      "physicalPrinter": "Physical Printer",
      "addPrinterSuccess": "Printer added successfully.",
      "modifySuccess": "Printer modified successfully."
    },
    "addPrinterDialog": {
      "addPrinter": "Add Printer",
      "discover": "Discover",
      "manualAdd": "Manual Add",
      "device": "Device",
      "info": "Info",
      "status": "Status",
      "noDevicesFound": "No Devices Found",
      "connectable": "Connectable",
      "connectToPrinter": "Connect to Printer",
      "refresh": "Refresh",
      "close": "Close",
      "cannotFindPrinter": "Cannot find printer?",
      "connect": "Connect",
      "help": "Help",
      "helpMessage": "请确保打印机已经联网，并且与当前软件处于同一个网络，如果刷新后依旧无法发现，可尝试进行手动添加。",
      "ok": "OK",
      "pleaseSelectPrinter": "Please select a printer"
    },
    "manualForm": {
      "printerModel": "Printer Model",
      "advanced": "Advanced",
      "selectVendor": "Select vendor",
      "selectModel": "Select model",
      "printerName": "Printer Name",
      "hostType": "Host Type",
      "selectHostType": "Select host type",
      "hostNameIpUrl": "Host Name, IP or URL",
      "deviceUI": "Device UI",
      "apiPassword": "API/Password",
      "httpsCAFile": "HTTPS CA File",
      "preview": "Preview",
      "httpsCAFileNote": "HTTPS CA file is optional. It is only needed if you use HTTPS with a self-signed certificate.",
      "pleaseSelectVendor": "Please select a vendor",
      "pleaseSelectPrinterModel": "Please select a printer model",
      "pleaseEnterPrinterName": "Please enter printer name",
      "lengthShouldBe1To50": "Length should be 1 to 50 characters",
      "pleaseSelectHostType": "Please select host type",
      "pleaseEnterHostNameIpUrl": "Please enter host name, IP or URL",
      "vendorCannotBeModified": "Vendor cannot be modified for existing printers",
      "modelCannotBeModified": "Model cannot be modified for existing printers"
    },
    "printerAuth": {
      "enterAccessCode": "Please enter the 6-character access code on the printer",
      "cannotAccessCode": "Cannot access code?",
      "close": "Close",
      "connect": "Connect",
      "help": "Help",
      "helpMessage": "Please check the printer screen or manual for the 6-character access code.",
      "ok": "OK",
      "pleaseEnterCompleteAccessCode": "Please enter the complete 6-character access code",
      "connectToPrinter": "Connect to Printer",
      "bindPrinter": "Bind Printer",
      "failedToAddPrinterInvalidAccessCode": "Failed to add printer. Invalid access code.",
      "failedToAddPrinter": "Failed to add printer."
    },
    "printerSetting": {
      "deleteDevice": "Delete Device",
      "name": "Name",
      "model": "Model",
      "host": "Host",
      "firmware": "Firmware",
      "updateAvailable": "Update Available",
      "latestVersion": "Latest Version",
      "enterPrinterName": "Enter printer name",
      "enterHostIpUrl": "Enter host, IP or URL",
      "pleaseEnterPrinterName": "Please enter printer name",
      "lengthShouldBe1To50": "Length should be 1 to 50 characters",
      "pleaseEnterHostNameIpUrl": "Please enter host name, IP or URL",
      "confirmDeletePrinter": "Are you sure you want to delete this printer?"
    },
    "printerSettingPhysical": {
    },
    "filamentSync": {
      "pleaseSelectPrinter": "Please select a printer",
      "filamentInformation": "Filament Information",
      "printerOffline": "The printer is offline. Please check and refresh or retry.",
      "noFilamentInfo": "The printer has no filament information to sync. Please check and refresh or retry.",
      "cancel": "Cancel",
      "sync": "Sync",
      "failedToSyncFilament": "Failed to sync filament.",
      "warningTips":"The current workspace already has filament information. Confirming the sync will directly overwrite the filament information"
    },
    "printSend": {
      "printer": "Printer",
      "noPrinter": "No printer",
      "pleaseConnectPrinter": "Please connect a printer",
      "texturedA": "Textured A",
      "smoothB": "Smooth B",
      "autoRefill": "Auto Refill",
      "switchToDeviceTab": "Upload and switch to device tab",
      "uploadAndPrint": "Upload and Print",
      "printOptions": "Print Options",
      "autoBedLeveling": "Auto Bed Leveling",
      "timelapse": "Timelapse",
      "canvas": "CANVAS",
      "noMapping": "No mapping",
      "cancel": "Cancel",
      "upload": "Upload",
      "someFilamentsNotMapped": "Some filaments are not mapped to MMS trays",
      "filamentMapping": "Filament Mapping",
      "filamentTypeNotMatch": "Filament type not match",
      "printerModelNotMatch": "The selected printer does not match the gcode file. It may cause printing failure or even printer damage! It is recommended to re-slice or select the correct printer to start printing.",
      "bedTypeNotMatch":"The selected bed type does not match the file. Please confirm before starting the print."
    
    }
  },
  zh_CN: {
    "connectedPrinters": "已连接的打印机",
    "addPrinter": "添加打印机",
    "delete": "删除",
    "confirm": "确认",
    "cancel": "取消",
    "noPrinterConnected": "无打印机连接。请前往\"设备\"发现或添加新打印机。",
    "multiColor": {
      "CanvasPro": "画布专业版",
    },
    "printerManager": {
      "localPrinters": "本地打印机",
      "connectedPrinters": "已连接的打印机",
      "addPrinter": "添加打印机",
      "noPrintersAvailable": "无可用打印机",
      "details": "详细信息",
      "offline": "离线",
      "idle": "空闲",
      "printing": "打印中",
      "paused": "已暂停",
      "pausing": "暂停中",
      "canceled": "已取消",
      "selfChecking": "自检中",
      "autoLeveling": "自动调平",
      "pidCalibrating": "PID校准中", 
      "resonanceTesting": "振纹优化中",
      "updating": "更新中",
      "fileCopying": "文件复制中",
      "fileTransferring": "文件传输中",
      "homing": "回零中",
      "preheating": "预热中",
      "filamentOperating": "耗材操作中",
      "extruderOperating": "挤出机操作中",
      "printCompleted": "打印完成",
      "rfidRecognizing": "RFID识别中",
      "error": "错误",
      "unknown": "未知",
      "printerSettings": "打印机设置",
      "physicalPrinter": "物理打印机",
      "addPrinterSuccess": "添加打印机成功",
      "modifySuccess": "修改成功"
    },
    "addPrinterDialog": {
      "addPrinter": "添加打印机",
      "discover": "发现",
      "manualAdd": "手动添加",
      "device": "设备",
      "info": "信息",
      "status": "状态",
      "noDevicesFound": "未找到设备",
      "connectable": "可连接",
      "connectToPrinter": "连接到打印机",
      "refresh": "刷新",
      "close": "关闭",
      "cannotFindPrinter": "找不到打印机?",
      "connect": "连接",
      "help": "帮助",
      "helpMessage": "请确保打印机已经联网，并且与当前软件处于同一个网络，如果刷新后依旧无法发现，可尝试进行手动添加。",
      "ok": "确定",
      "pleaseSelectPrinter": "请选择一台打印机"
    },
    "manualForm": {
      "printerModel": "打印机型号",
      "advanced": "高级",
      "selectVendor": "选择厂商",
      "selectModel": "选择型号",
      "printerName": "打印机名称",
      "hostType": "主机类型",
      "selectHostType": "选择主机类型",
      "hostNameIpUrl": "主机名、IP 或 URL",
      "deviceUI": "设备界面",
      "apiPassword": "API/密码",
      "httpsCAFile": "HTTPS CA 文件",
      "preview": "预览",
      "httpsCAFileNote": "HTTPS CA文件是可选的。只有在使用HTTPS和自签名证书时才需要。",
      "pleaseSelectVendor": "请选择厂商",
      "pleaseSelectPrinterModel": "请选择打印机型号",
      "pleaseEnterPrinterName": "请输入打印机名称",
      "lengthShouldBe1To50": "长度应为1到50个字符",
      "pleaseSelectHostType": "请选择主机类型",
      "pleaseEnterHostNameIpUrl": "请输入主机名、IP或URL",
      "vendorCannotBeModified": "现有打印机无法修改厂商",
      "modelCannotBeModified": "现有打印机无法修改型号"
    },
    "printerAuth": {
      "enterAccessCode": "请在打印机上输入6位访问代码",
      "cannotAccessCode": "无法获取访问代码？",
      "close": "关闭",
      "connect": "连接",
      "help": "帮助",
      "helpMessage": "请检查打印机屏幕或手册获取6位访问代码。",
      "ok": "确定",
      "pleaseEnterCompleteAccessCode": "请输入完整的6位访问代码",
      "connectToPrinter": "连接到打印机",
      "bindPrinter": "绑定打印机",
      "failedToAddPrinterInvalidAccessCode": "添加打印机失败。访问代码无效。",
      "failedToAddPrinter": "添加打印机失败。"
    },
    "printerSetting": {
      "deleteDevice": "删除设备",
      "name": "名称",
      "model": "型号",
      "host": "主机",
      "firmware": "固件",
      "updateAvailable": "有可用更新",
      "latestVersion": "最新版本",
      "enterPrinterName": "输入打印机名称",
      "enterHostIpUrl": "输入主机、IP或URL",
      "pleaseEnterPrinterName": "请输入打印机名称",
      "lengthShouldBe1To50": "长度应为1到50个字符",
      "pleaseEnterHostNameIpUrl": "请输入主机名、IP或URL",
      "confirmDeletePrinter": "您确定要删除此打印机吗？"
    },
    "printerSettingPhysical": {


    },
    "filamentSync": {
      "pleaseSelectPrinter": "请选择一台打印机",
      "filamentInformation": "耗材信息",
      "printerOffline": "打印机离线。请检查并刷新或重试。",
      "noFilamentInfo": "打印机没有要同步的耗材信息。请检查并刷新或重试。",
      "cancel": "取消",
      "sync": "同步",
      "failedToSyncFilament": "同步耗材失败。",
      "warningTips":"当前渲染区已经存在耗材信息，确认同步后耗材信息将会被直接覆盖"
    },
    "printSend": {
      "printer": "打印机",
      "noPrinter": "无打印机",
      "pleaseConnectPrinter": "请连接打印机",
      "texturedA": "纹理 A",
      "smoothB": "光滑 B",
      "autoRefill": "自动补料",
      "switchToDeviceTab": "上传后并切换到设备标签",
      "uploadAndPrint": "上传并打印",
      "printOptions": "打印选项",
      "autoBedLeveling": "自动热床调平",
      "timelapse": "延时摄影",
      "canvas": "画布",
      "noMapping": "无映射",
      "cancel": "取消",
      "upload": "上传",
      "someFilamentsNotMapped": "某些耗材未映射到MMS托盘",
      "filamentMapping": "耗材映射",
      "filamentTypeNotMatch": "耗材类型不匹配",
      "printerModelNotMatch": "当前选中的打印机与gcode文件不匹配，可能会导致打印失败甚至打印机损坏！建议重新切片或者选择正确的打印机启动打印。",
      "bedTypeNotMatch":"当前选中的热床类型与文件不匹配，请确认后在启动打印。"
    }
  }
};

//merge LangText into langMessages
// if (typeof LangText === 'object' && LangText !== null) {
// if(LangText){
//   for (const lang in LangText) {
//     if (LangText.hasOwnProperty(lang)) {
//       langMessages[lang] = { ...langMessages[lang], ...LangText[lang] };
//     }
//   }
// }

function defaultLanguage() {
  let strLang = GetQueryString("lang");
  if (strLang != null) {
    //setCookie(LANG_COOKIE_NAME,strLang,LANG_COOKIE_EXPIRESECOND,'/');
    localStorage.setItem(LANG_COOKIE_NAME, strLang);
  } else {
    //strLang=getCookie(LANG_COOKIE_NAME);
    strLang = localStorage.getItem(LANG_COOKIE_NAME);
  }

  if (!langMessages.hasOwnProperty(strLang)) strLang = "en";
  return strLang;
}


const i18n = createI18n({
  locale: defaultLanguage(),
  fallbackLocale: 'en',
  messages: langMessages,

});
