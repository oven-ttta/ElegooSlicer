// Vue.js filament sync application
const { createApp } = Vue;
const { ElButton, ElDialog, ElInput, ElSelect, ElOption, ElForm, ElFormItem } = ElementPlus;

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

        // Lifecycle methods
        async init() {
            await this.requestPrinterList();
        },

        // Communication with backend
        requestPrinterList() {
            const message = {
                id: Math.round(new Date() / 1000).toString(),
                method: "getPrinterList",
                type: "request",
                params: {
                }
            };
            SendWXMessage(JSON.stringify(message));
        },

        // Request printer filament info when printer is selected
        requestPrinterFilamentInfo(printerId) {
            const message = {
                id: Math.round(new Date() / 1000).toString(),
                method: "getPrinterFilamentInfo",
                type: "request",
                params: {
                    printerId: printerId
                }
            };
            SendWXMessage(JSON.stringify(message));
        },

        // Handle messages from backend
        handleStudio(response) {
            const method = response.method;

            if (method === 'getPrinterList') {
                this.printerList = response.data || [];
                this.updatePrinterSelection();
            } else if (method === 'getPrinterFilamentInfo') {
                this.mmsInfo = response.data.mmsInfo;
                this.printFilamentList = response.data.printFilamentList;
            }
        },

        updatePrinterSelection() {
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
                this.requestPrinterFilamentInfo(this.curPrinter.printerId);
            }
        },


        // Handle printer selection change
        onPrinterChanged(printer) {
            this.curPrinter = printer;
            if (printer && printer.printerId) {
                this.requestPrinterFilamentInfo(printer.printerId);
            }
        },

        refreshPrinterList(event, device) {
            event.stopPropagation();
            event.preventDefault();
            this.requestPrinterList();
        },

        cancel() {
            // Send close dialog method to backend
            const message = {
                id: Math.round(new Date() / 1000).toString(),
                method: "closeDialog",
                type: "request",
                params: {
                }
            };
            SendWXMessage(JSON.stringify(message));
        },

        sync() {
            // Send sync method to backend
            const message = {
                id: Math.round(new Date() / 1000).toString(),
                method: "syncMmsFilament",
                type: "request",
                params: {
                    mmsInfo: this.mmsInfo,
                    printFilamentList: this.printFilamentList,
                    printer: this.curPrinter
                }
            };
            SendWXMessage(JSON.stringify(message));
        }
    },

    // Initialize the application
    mounted() {
        // Initialize the application
        this.init();

        // Set up global message handler
        window.HandleStudio = (response) => {
            this.handleStudio(response);
        };

        // Set up global functions that might be called externally
        window.OnInit = () => {
            this.init();
        };
    },
};

// Create and mount the Vue app
const app = createApp(FilamentSyncApp)
    .use(ElementPlus)
    .use(i18n)
    .mount('#app');
