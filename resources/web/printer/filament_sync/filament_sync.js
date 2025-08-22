// Vue.js filament sync application
const { createApp } = Vue;
const { ElButton, ElDialog, ElInput, ElSelect, ElOption, ElForm, ElFormItem, ElLoading } = ElementPlus;

const FilamentSyncApp = {
    data() {
        return {
            curPrinter: null,
            printerList: [],
            mmsInfo: null,
            printFilamentList: [],
        };
    },

    methods: {
        getFilamentSvg(filament) {
            const color = filament?.filamentColor || '#888';
            return getFilamentSvg(color);
        },

        // IPC Communication methods
        async ipcRequest(method, params = {}, timeout = 10000) {
            try {
                const response = await nativeIpc.request(method, params, timeout);
                return response;
            } catch (error) {
                console.error(`IPC request failed for ${method}:`, error);
                throw error;
            }
        },

        // Lifecycle methods
        async init() {
            await this.requestPrinterList();
        },

        // Communication with backend
        async requestPrinterList() {
            try {
                const response = await this.ipcRequest('getPrinterList', {});
                this.printerList = response || [];
                this.updatePrinterSelection();
            } catch (error) {
                console.error('Failed to request printer list:', error);
            }
        },

        // Request printer filament info when printer is selected
        async requestPrinterFilamentInfo(printerId) {
           const loading = ElLoading.service({
                lock: true,
            });
            try {
                const params = { printerId: printerId };
                const response = await this.ipcRequest('getPrinterFilamentInfo', params);
                this.mmsInfo = response.mmsInfo;
                this.printFilamentList = response.printFilamentList;
            } catch (error) {
                console.error('Failed to request printer filament info:', error);
            }finally {
                loading.close();
            }
        },

        async updatePrinterSelection() {
            if (this.printerList.length === 0) return;
            let selectedPrinter;       
            if (this.curPrinter && this.curPrinter.printerId) {
                // find the printer by printerId
                let cur = this.printerList.find(p => p.printerId === this.curPrinter.printerId);
                if (cur) {
                    selectedPrinter = cur;
                }
            }        
            // if not found, find the selected printer
            if (!selectedPrinter) {
                selectedPrinter = this.printerList.find(p => p.selected);
                if (!selectedPrinter) {
                    selectedPrinter = this.printerList[0];
                }
            }          
            this.curPrinter = selectedPrinter;       
            // Request filament info for the selected printer
            if (this.curPrinter && this.curPrinter.printerId) {
                await this.requestPrinterFilamentInfo(this.curPrinter.printerId);
            }
        },


        // Handle printer selection change
        async onPrinterChanged(printer) {
            this.curPrinter = printer;
            if (printer && printer.printerId) {
                await this.requestPrinterFilamentInfo(printer.printerId);
            }
        },

        async refreshPrinterList(event, device) {
            event.stopPropagation();
            event.preventDefault();
            await this.requestPrinterList();
        },

        async cancel() {
            try {
                await this.ipcRequest('closeDialog', {});
            } catch (error) {
                console.error('Failed to close dialog:', error);
            }
        },

        async sync() {
            try {
                const params = {
                    mmsInfo: this.mmsInfo,
                    printFilamentList: this.printFilamentList,
                    printer: this.curPrinter
                };
                await this.ipcRequest('syncMmsFilament', params);
            } catch (error) {
                console.error('Failed to sync filament:', error);
            }
        }
    },

    // Initialize the application
    mounted() {
        // Initialize the application
        this.init();
    },
    computed: {
        canSync() {
            return this.curPrinter && 
                   this.printerList && 
                   this.printerList.length > 0 &&
                   this.mmsInfo && 
                   this.mmsInfo.mmsList && 
                   this.mmsInfo.mmsList.length > 0;
        }
    }
};

// Create and mount the Vue app
const app = createApp(FilamentSyncApp)
    .use(ElementPlus)
    .use(i18n)
    .mount('#app');
