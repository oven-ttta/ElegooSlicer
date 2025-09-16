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
        this.init();
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
            this.printerStore.init();
        },


        canShowProgressText(printerStatus, connectStatus) {
            return printerStatus === 1 || printerStatus === 2 || printerStatus === 3;
        },
        getPrinterStatus(printerStatus, connectStatus) {
            // If not connected, always show Offline
            if (connectStatus === 0) {
                return this.$t("printerManager.offline");
            }

            const statusMap = {
                [-1]: this.$t("printerManager.offline"),
                [0]: this.$t("printerManager.idle"),
                [1]: this.$t("printerManager.printing"),
                [2]: this.$t("printerManager.paused"),
                [3]: this.$t("printerManager.pausing"),
                [4]: this.$t("printerManager.canceled"),
                [5]: this.$t("printerManager.selfChecking"),
                [6]: this.$t("printerManager.autoLeveling"),
                [7]: this.$t("printerManager.pidCalibrating"),
                [8]: this.$t("printerManager.resonanceTesting"),
                [9]: this.$t("printerManager.updating"),
                [10]: this.$t("printerManager.fileCopying"),
                [11]: this.$t("printerManager.fileTransferring"),
                [12]: this.$t("printerManager.homing"),
                [13]: this.$t("printerManager.preheating"),
                [14]: this.$t("printerManager.filamentOperating"),
                [15]: this.$t("printerManager.extruderOperating"),
                [16]: this.$t("printerManager.printCompleted"),
                [17]: this.$t("printerManager.rfidRecognizing"),
                [998]: this.$t("printerManager.busy"),
                [999]: this.$t("printerManager.error"),
                [1000]: this.$t("printerManager.idNotMatch"),
                [1001]: this.$t("printerManager.authError"),
                [10000]: this.$t("printerManager.unknown")
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
            if (printerStatus === 16 || printerStatus === 0) {
                return {
                    color: 'var(--printer-status-success-color)',
                    backgroundColor: 'var(--printer-status-success-bg)'
                };
            } else if (printerStatus === 999 || printerStatus === 998) {
                return {
                    color: 'var(--printer-status-error-color)',
                    backgroundColor: 'var(--printer-status-error-bg)'
                };
            } else if (printerStatus === 1000 || printerStatus === 2 || printerStatus === 3) {
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

        shouldShowWarningIcon(printerStatus, connectStatus) {
            return connectStatus === 0 && (printerStatus === 1000 || printerStatus === 1001);
        },

        getWarningTooltip(printerStatus) {
            if (printerStatus === 1000) {
                return this.$t("printerManager.idNotMatch");
            } else if (printerStatus === 1001) {
                return this.$t("printerManager.authError");
            }
            return "";
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

        async handleRefresh() {
            const loading = ElLoading.service({
                lock: true,
            });
            try {
                await new Promise(resolve => setTimeout(resolve, 500));
                this.printerStore.requestPrinterList();
            } finally {
                loading.close();
            }
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





