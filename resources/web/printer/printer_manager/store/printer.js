const { createPinia, defineStore } = Pinia;

// 定义一个 store
const usePrinterStore = defineStore('printer', {
  state: () => ({
    discoveredPrinters: [],
    isDiscovering: false,
    printers: [],
    printerModelList: null,
  }),
  actions: {

    init() {
      window.HandleStudio = (data) => {
        this.handleStudio(data);
      };
    },
    uninit() {
      window.HandleStudio = null;
    },

    requestPrinterList() {
      console.log("Requesting printer list");
      const tSend = {
        command: "request_printer_list",
        sequence_id: Math.round(new Date() / 1000)
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    startStatusUpdates() {
      this.statusUpdateInterval = setInterval(() => {
        const tSend = {
          command: "request_printer_list_status",
          sequence_id: Math.round(new Date() / 1000)
        };
        SendWXMessage(JSON.stringify(tSend));
      }, 1000);
    },

    requestPrinterModelList() {
      const tSend = {
        command: "request_printer_model_list",
        sequence_id: Math.round(new Date() / 1000)
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    handleStudio(pVal) {
      const strCmd = pVal['command'];
      console.log("received command:", pVal);
      switch (strCmd) {
        case "response_printer_list":
          this.printers = pVal['response'] || [];
          break;

        case "response_printer_list_status":
          this.updatePrinterListStatus(pVal['response'] || []);
          break;

        case "response_discover_printers":
          this.discoveredPrinters = pVal.response || [];
          this.isDiscovering = false;
          break;

        case "response_add_printer":
        case "response_update_printer_name":
        case "response_add_physical_printer":
        case "response_update_printer_host":
          this.requestPrinterList();
          break;

        case "response_delete_printer":
          this.requestPrinterList();
          this.closeModals();
          break;

        case "response_printer_model_list":
          this.printerModelList = pVal['response'];
          break;
      }
    },

    requestDiscoverPrinters() {
      this.isDiscovering = true;
      const tSend = {
        command: "request_discover_printers",
        sequence_id: Math.round(new Date() / 1000)
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    requestDeletePrinter(printerId) {
      console.log(`Requesting deletion of printer with ID: ${printerId}`);
      const tSend = {
        command: "request_delete_printer",
        sequence_id: Math.round(new Date() / 1000),
        printerId: printerId
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    requestAddPrinter(printer) {
      const tSend = {
        command: "request_add_printer",
        sequence_id: Math.round(new Date() / 1000),
        printer: printer
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    requestAddPhysicalPrinter(printer) {
      console.log("Requesting addition of physical printer:", printer);
      const tSend = {
        command: "request_add_physical_printer",
        sequence_id: Math.round(new Date() / 1000),
        printer: printer
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    requestUpdatePrinterName(printerId, printerName) {
      const tSend = {
        command: "request_update_printer_name",
        sequence_id: Math.round(new Date() / 1000),
        printerId: printerId,
        printerName: printerName
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    requestUpdatePrinterHost(printerId, host) {
      const tSend = {
        command: "request_update_printer_host",
        sequence_id: Math.round(new Date() / 1000),
        printerId: printerId,
        host: host
      };
      SendWXMessage(JSON.stringify(tSend));
    },
    // 处理打印机信息更新
    updatePrinterInfo(data) {
      if (data.command === 'update_printer_info') {
        this.loadPrinterList();
      }
    },
    showPrinterDetail(printerId) {
      const tSend = {
        command: "request_printer_detail",
        sequence_id: Math.round(new Date() / 1000),
        printerId: printerId
      };
      SendWXMessage(JSON.stringify(tSend));
    },

    updatePrinterListStatus(printers) {
      printers.forEach((statusPrinter) => {
        const printerIndex = this.printers.findIndex(p => p.printerId === statusPrinter.printerId);
        if (printerIndex !== -1) {
          // Update specific properties without replacing the entire printer object
          this.printers[printerIndex] = {
            ...this.printers[printerIndex],
            ...statusPrinter
          };
        }
      });
    },
  }
});