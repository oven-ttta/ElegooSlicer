const { createPinia, defineStore } = Pinia;

// Define a store
const usePrinterStore = defineStore('printer', {
  state: () => ({
    discoveredPrinters: [],
    isDiscovering: false,
    printers: [],
    printerModelList: null,
  }),
  actions: {
    validateHost(rule, value, callback) {
      if (!value || value.trim().length === 0) {
        callback(new Error('Please enter host name, IP or URL'));
        return;
      }

      const trimmedValue = value.trim();

      // Helper function to validate IPv4 address
      const isValidIPv4 = (ip) => {
        // Must match pattern: number.number.number.number
        const ipPattern = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;
        const match = ip.match(ipPattern);
        
        if (!match) {
          return { valid: false, error: 'Please enter a valid hostname, IP address or URL' };
        }

        // Check each segment is between 0-255
        for (let i = 1; i <= 4; i++) {
          const segment = parseInt(match[i], 10);
          if (segment < 0 || segment > 255) {
            return { valid: false, error: `Please enter a valid hostname, IP address or URL` };
          }
        }

        return { valid: true };
      };

      // Helper function to validate URL
      const isValidURL = (url) => {
        if (!url.startsWith('http://') && !url.startsWith('https://')) {
          return false;
        }
        return url.length >= 10; // Basic length check
      };

      const isValidHostname = (hostname) => {
        const hostnamePattern = /^[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9]$/;
        return hostnamePattern.test(hostname);
      };

      // Validation logic
      if (/^\d+\./.test(trimmedValue)) {
        // Starts with number + dot - check if it's a valid IPv4
        const ipValidation = isValidIPv4(trimmedValue);
        if (!ipValidation.valid) {
          callback(new Error(ipValidation.error));
          return;
        }
        callback(); // IPv4 is valid
        return;
      }

      if (isValidURL(trimmedValue)) {
        callback(); // URL is valid
        return;
      }

      if (isValidHostname(trimmedValue)) {
        callback(); // Hostname is valid
        return;
      }

      callback(new Error('Please enter a valid hostname, IP address or URL'));
    },


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
    // Handle printer information updates
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