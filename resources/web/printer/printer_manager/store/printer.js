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


    async init() {
      const loading = ElLoading.service({
        lock: true,
      });
      await new Promise(resolve => setTimeout(resolve, 500));
      // Load printer models and printer list
      await this.requestPrinterModelList();
      await this.requestPrinterList();
      await this.startStatusUpdates();
      loading.close();
    },

    uninit() {
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
        let message = ''
        // if (method === "request_add_printer" || method === "request_add_physical_printer") {

        //   if (params.printer.authMode === 'token' && (error.code === 200 || error.code === 202 || error.code === 203)) {
        //     message = i18n.global.t("printerAuth.failedToAddPrinterInvalidAccessCode");
        //   } else {
        //     message = i18n.global.t("printerAuth.failedToAddPrinter");
        //   }

        // } else {
        message = `${error.message || 'Unknown error occurred'}`
        // }
        // Show error notification using Element Plus message component
        if (window.ElementPlus && window.ElementPlus.ElMessage) {
          window.ElementPlus.ElMessage.error({
            message: message,
            duration: 5000,
            showClose: true
          });
        }

        throw error;
      }
    },

    async requestPrinterList() {
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
      const loading = ElLoading.service({
        lock: true,
      });
      try {
        await this.ipcRequest('request_delete_printer', { printerId });
        this.requestPrinterList();
        await new Promise(resolve => setTimeout(resolve, 500));
        // Response handling is done in event listeners
      } catch (error) {
        console.error('Failed to delete printer:', error);
        throw error;
      }
      finally {
        loading.close();
      }
    },

    async requestAddPrinter(printer) {
      const loading = ElLoading.service({
        lock: true,
      });
      try {

        await new Promise(resolve => setTimeout(resolve, 500));
        await this.ipcRequest('request_add_printer', { printer });
        this.requestPrinterList();
        ElementPlus.ElMessage.success({
          message: i18n.global.t("printerManager.addPrinterSuccess"),
          duration: 3000,
        });
      } catch (error) {
        console.error('Failed to add printer:', error);
        throw error;
      } finally {
        loading.close();
      }
    },

    async requestAddPhysicalPrinter(printer) {
      const loading = ElLoading.service({
        lock: true,
      });
      try {
        await new Promise(resolve => setTimeout(resolve, 500));
        await this.ipcRequest('request_add_physical_printer', { printer });
        this.requestPrinterList();
      } catch (error) {
        console.error('Failed to add physical printer:', error);
        throw error;
      } finally {
        loading.close();
      }
    },


    async requestUpdatePrinterNameAndHost(printerId, printerName, host) {
      // const loading = ElLoading.service({
      //   lock: true,
      // });
      try {

        if (isEmptyString(printerName) && isEmptyString(host)) {
          // Nothing to update
          return;
        }

        if (printerName) {
          await this.ipcRequest('request_update_printer_name', {
            printerId,
            printerName
          });
        }

        if (host) {
          const loading = ElLoading.service({
            lock: true,
          });
          try {
            await this.ipcRequest('request_update_printer_host', {
              printerId,
              host
            });
          } finally {
            loading.close();
          }
        }

        ElementPlus.ElMessage.success({
          message: i18n.global.t("printerManager.modifySuccess"),
          duration: 3000,
        });

      } catch (error) {
        console.error('Failed to update printer name:', error);
        throw error;
      } finally {
        // loading.close();
      }

    },

    async requestUpdatePrinterName(printerId, printerName) {
      await this.requestUpdatePrinterNameAndHost(printerId, printerName, null);

    },

    async requestUpdatePrinterHost(printerId, host) {
      await this.requestUpdatePrinterNameAndHost(printerId, null, host);
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