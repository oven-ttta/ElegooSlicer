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

    async mounted() {
        this.init();

        nativeIpc.on('onUserInfoUpdated', (data) => {
            console.log('Received user info update event from backend:', data);
            this.printerStore.userInfo = data;
            console.log('loginErrorMessage:', this.printerStore.userInfo.loginErrorMessage);
        });

        await nativeIpc.request('ready', {});

        disableRightClickMenu();
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

        // Returns HTML with the login keyword highlighted in primary blue
        getLoginToViewHtml() {
            const isMainClient = this.printerStore.isMainClient;
            if (!isMainClient) {
                return this.$t("printerManager.isNotMainClient");
            }
            const loginWord = `<span class="login-link" onclick="loginLinkClickHandler()">${this.$t("printerManager.login")}</span>`; 
            // loginToView is expected to contain a {0} placeholder for the login word
            const raw = this.$t("printerManager.loginToView", [loginWord]);
            return raw;
        },


        canShowProgressText(printerStatus, connectStatus) {
            return PrinterStatusUtils.canShowProgressText(printerStatus, connectStatus);
        },
        getPrinterStatus(printerStatus, connectStatus) {
            return PrinterStatusUtils.getPrinterStatus(printerStatus, connectStatus, this.$t);
        },

        getPrinterStatusStyle(printerStatus, connectStatus) {
            return PrinterStatusUtils.getPrinterStatusStyle(printerStatus, connectStatus);
        },

        shouldShowWarningIcon(printerStatus, connectStatus) {
            return PrinterStatusUtils.shouldShowWarningIcon(printerStatus, connectStatus);
        },

        getWarningTooltip(printerStatus) {
            return PrinterStatusUtils.getWarningTooltip(printerStatus, this.$t);
        },

        getPrinterProgress(printer) {
            return PrinterStatusUtils.getPrinterProgress(printer);
        },

        getPrinterRemainingTime(printer) {
            return PrinterStatusUtils.getPrinterRemainingTime(printer);
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
            if (index >= 0 && index < this.printerStore.printers.length) {
                this.currentPrinter = this.printerStore.printers[index];
                this.showPrinterAdvancedSettings = (this.currentPrinter && this.currentPrinter.isPhysicalPrinter) !== true;
                this.showPrinterSimpleSettings = !this.showPrinterAdvancedSettings;
            }
        },
        showPrinterSettings(printer) {
            this.currentPrinter = printer;
            this.showPrinterAdvancedSettings = (this.currentPrinter && this.currentPrinter.isPhysicalPrinter) !== true;
            this.showPrinterSimpleSettings = !this.showPrinterAdvancedSettings;
        },

        closeModals() {
            this.showAddPrinter = false;
            this.showPrinterAdvancedSettings = false;
            this.showPrinterSimpleSettings = false;
            this.currentPrinter = null;
        },
    }
};


window.loginLinkClickHandler = function () {
    // Implement the login logic here
    console.log("Login link clicked");
    try {
        nativeIpc.request("checkLoginStatus", {});
    } catch (error) {
        console.error('Check login status failed:', error);
    }
};


