const { createPinia, defineStore } = Pinia;

// Define a store
const usePrinterStore = defineStore('printer', {
  state: () => ({
    discoveredPrinters: [],
    isDiscovering: false,
    printers: [],
    printerModelList: null,
    statusUpdateInterval: null,
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
      // Register event handlers for printer-related events
      nativeIpc.on('response_printer_list', (data) => {
        this.printers = data || [];
      });

      nativeIpc.on('response_printer_list_status', (data) => {
        this.updatePrinterListStatus(data || []);
      });

      nativeIpc.on('response_discover_printers', (data) => {
        this.discoveredPrinters = data || [];
        this.isDiscovering = false;
      });

      nativeIpc.on('response_add_printer', () => {
        this.requestPrinterList();
      });

      nativeIpc.on('response_update_printer_name', () => {
        this.requestPrinterList();
      });

      nativeIpc.on('response_add_physical_printer', () => {
        this.requestPrinterList();
      });

      nativeIpc.on('response_update_printer_host', () => {
        this.requestPrinterList();
      });

      nativeIpc.on('response_delete_printer', () => {
        this.requestPrinterList();
        this.closeModals();
      });

      nativeIpc.on('response_printer_model_list', (data) => {
        this.printerModelList = data;
      });
    },
    uninit() {
      // Clean up event listeners
      nativeIpc.off('response_printer_list');
      nativeIpc.off('response_printer_list_status');
      nativeIpc.off('response_discover_printers');
      nativeIpc.off('response_add_printer');
      nativeIpc.off('response_update_printer_name');
      nativeIpc.off('response_add_physical_printer');
      nativeIpc.off('response_update_printer_host');
      nativeIpc.off('response_delete_printer');
      nativeIpc.off('response_printer_model_list');

      // Clear status update interval
      if (this.statusUpdateInterval) {
        clearInterval(this.statusUpdateInterval);
        this.statusUpdateInterval = null;
      }
    },

    async ipcRequest(method, params, timeout = 10000) {
      try {
        const response = await nativeIpc.request(method, params, timeout);
        return response;
      } catch (error) {
        console.error(`IPC: Request ${method} failed`, error);
        // Show error notification using Element Plus message component
        if (window.ElementPlus && window.ElementPlus.ElMessage) {
          window.ElementPlus.ElMessage.error({
            message: `Request failed: ${method} - ${error.message || 'Unknown error occurred'}`,
            duration: 5000,
            showClose: true
          });
        }
        
        throw error;
      }
    },

    async requestPrinterList() {
      console.log("Requesting printer list");
      try {
        const response = await this.ipcRequest('request_printer_list', {});
        this.printers = response || [];
      } catch (error) {
        console.error('Failed to request printer list:', error);
      }
    },

    startStatusUpdates() {
      // Clear any existing interval
      if (this.statusUpdateInterval) {
        clearInterval(this.statusUpdateInterval);
      }

      this.statusUpdateInterval = setInterval(async () => {
        try {
          const response = await this.ipcRequest('request_printer_list_status', {});
          this.updatePrinterListStatus(response || []);
        } catch (error) {
          console.error('Failed to update printer status:', error);
        }
      }, 3000);
    },

    stopStatusUpdates() {
      if (this.statusUpdateInterval) {
        clearInterval(this.statusUpdateInterval);
        this.statusUpdateInterval = null;
      }
    },

    async requestPrinterModelList() {
      try {
        const response = await this.ipcRequest('request_printer_model_list', {});
        this.printerModelList = response;
      } catch (error) {
        console.error('Failed to request printer model list:', error);
      }
    },

    async requestDiscoverPrinters() {
      this.isDiscovering = true;
      try {
        const response = await this.ipcRequest('request_discover_printers', {}, 30 * 1000);
        this.discoveredPrinters = response || [];
        this.isDiscovering = false;
      } catch (error) {
        console.error('Failed to discover printers:', error);
        this.isDiscovering = false;
      }
    },

    async requestDeletePrinter(printerId) {
      console.log(`Requesting deletion of printer with ID: ${printerId}`);
      try {
        await this.ipcRequest('request_delete_printer', { printerId });
        this.requestPrinterList();
        // Response handling is done in event listeners
      } catch (error) {
        console.error('Failed to delete printer:', error);
      }
    },

    async requestAddPrinter(printer) {
      try {
        await this.ipcRequest('request_add_printer', { printer });
        this.requestPrinterList();
      } catch (error) {
        console.error('Failed to add printer:', error);
      }
    },

    async requestAddPhysicalPrinter(printer) {
      console.log("Requesting addition of physical printer:", printer);
      try {
        await this.ipcRequest('request_add_physical_printer', { printer });
        this.requestPrinterList();
      } catch (error) {
        console.error('Failed to add physical printer:', error);
      }
    },

    async requestUpdatePrinterName(printerId, printerName) {
      try {
        await this.ipcRequest('request_update_printer_name', {
          printerId,
          printerName
        });
        // Response handling is done in event listeners
      } catch (error) {
        console.error('Failed to update printer name:', error);
      }
    },

    async requestUpdatePrinterHost(printerId, host) {
      try {
        await this.ipcRequest('request_update_printer_host', {
          printerId,
          host
        });
        // Response handling is done in event listeners
      } catch (error) {
        console.error('Failed to update printer host:', error);
      }
    },
    // Handle printer information updates
    updatePrinterInfo(data) {
      if (data.command === 'update_printer_info') {
        this.loadPrinterList();
      }
    },
    async showPrinterDetail(printerId) {
      try {
        const response = await this.ipcRequest('request_printer_detail', { printerId });
        return response;
      } catch (error) {
        console.error('Failed to get printer detail:', error);
        return null;
      }
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

    // Helper method for closing modals (to be implemented by UI components)
    closeModals() {
      // This method should be overridden or called from UI components
      // to handle modal closing logic
      console.log('closeModals called - should be handled by UI components');
    },
  }
});