const { createApp } = Vue;
const { ElInput, ElButton, ElPopover, ElDialog, ElTab, ElTabPane, ElSelect, ElOption, ElForm, ElFormItem, ElLoading } = ElementPlus;

const PrinterManager = {
    setup() {
        const printerStore = usePrinterStore();
        return {
            printerStore
        };
    },
    data() {
        return {
            showAddPrinter: false,
            currentPrinter: null,
            statusUpdateInterval: null,
            // Show printer settings modal for discovered devices
            showPrinterAdvancedSettings: false,
            // Show printer settings modal for manual connection devices
            showPrinterSimpleSettings: false,
        };
    },

    mounted() {
        TranslatePage();
        this.init();
        this.printerStore.init();
    },

    watch: {
        //Actively request to discover printers when the add printer dialog is shown
        showAddPrinter: {
            handler(newVal) {
                if (newVal) {
                    if (this.printerStore.isDiscovering) {
                        return;
                    }
                    this.printerStore.requestDiscoverPrinters();
                }
            },
            immediate: true
        }
    },

    beforeUnmount() {
        this.printerStore.uninit();
        if (this.statusUpdateInterval) {
            clearInterval(this.statusUpdateInterval);
        }
    },

    methods: {
        init() {
            this.printerStore.requestPrinterModelList();
            this.printerStore.requestPrinterList();
            this.printerStore.startStatusUpdates();
        },


        getPrinterStatus(printerStatus, connectStatus) {
            // If not connected, always show Offline
            if (connectStatus === 0) {
                return "Offline";
            }

            const statusMap = {
                [-1]: "Offline",
                [0]: "Idle",
                [1]: "Printing",
                [2]: "Paused",
                [3]: "Pausing",
                [4]: "Canceled",
                [5]: "Self-checking",
                [6]: "Auto-leveling",
                [7]: "PID calibrating",
                [8]: "Resonance testing",
                [9]: "Updating",
                [10]: "File copying",
                [11]: "File transferring",
                [12]: "Homing",
                [13]: "Preheating",
                [14]: "Filament operating",
                [15]: "Extruder operating",
                [16]: "Print completed",
                [17]: "RFID recognizing",
                [999]: "Error",
                [1000]: "Unknown"
            };

            return statusMap[printerStatus] || "";
        },

        getPrinterStatusStyle(printerStatus, connectStatus) {
            if (connectStatus === 0) {
                return {
                    color: 'var(--printer-status-offline-color)',
                    backgroundColor: 'var(--printer-status-offline-bg)'
                };
            }
            if (printerStatus == 16 || printerStatus == 0) {
                return {
                    color: 'var(--printer-status-success-color)',
                    backgroundColor: 'var(--printer-status-success-bg)'
                };
            } else if (printerStatus == 999) {
                return {
                    color: 'var(--printer-status-error-color)',
                    backgroundColor: 'var(--printer-status-error-bg)'
                };
            } else if (printerStatus == 1000) {
                return {
                    color: 'var(--printer-status-warning-color)',
                    backgroundColor: 'var(--printer-status-warning-bg)'
                };
            } else {
                return {
                    color: 'var(--printer-status-primary-color)',
                    backgroundColor: 'var(--printer-status-primary-bg)'
                };
            }
        },

        getPrinterProgress(printer) {
            return (printer.printTask && typeof printer.printTask.progress === 'number') ? printer.printTask.progress : 0;
        },

        getPrinterRemainingTime(printer) {
            if (!printer.printTask) return '';

            const { currentTime, totalTime, estimatedTime } = printer.printTask;

            // If estimatedTime is provided, it already represents remaining seconds
            const remainingTime = (typeof estimatedTime === 'number' && estimatedTime > 0)
                ? estimatedTime
                : Math.max(0, (totalTime || 0) - (currentTime || 0));

            const hours = Math.floor(remainingTime / 3600);
            const minutes = Math.floor((remainingTime % 3600) / 60);
            const seconds = remainingTime % 60;
            const pad = (n) => String(n).padStart(2, '0');

            return `${pad(hours)}:${pad(minutes)}:${pad(seconds)}`;
        },

        handleRefresh() {
            this.printerStore.requestPrinterList();
        },

        showAddPrinterModal() {
            this.showAddPrinter = true;
        },

        showPrinterSettingsByIndex(index) {
            console.log("Showing", this.printerStore);
            if (index >= 0 && index < this.printerStore.printers.length) {
                this.currentPrinter = this.printerStore.printers[index];
                this.showPrinterAdvancedSettings = (this.currentPrinter && this.currentPrinter.isPhysicalPrinter) !== true;
                this.showPrinterSimpleSettings = !this.showPrinterAdvancedSettings;
            }
        },

        closeModals() {
            this.showAddPrinter = false;
            this.showPrinterAdvancedSettings = false;
            this.showPrinterSimpleSettings = false;
            this.currentPrinter = null;
        },
    }
};

// Create Vue app
const app = createApp(PrinterManager);

// Register all components
app.component('add-printer-component', AddPrinterComponent);
app.component('manual-form-component', ManualFormComponent);
app.component('printer-auth-component', PrinterAuthComponent);
app.component('printer-setting-physical-component', PrinterSettingPhysicalComponent);
app.component('printer-setting-component', PrinterSettingComponent);

app.use(createPinia());
app.use(ElementPlus);
// Mount the app
app.mount('#app');




