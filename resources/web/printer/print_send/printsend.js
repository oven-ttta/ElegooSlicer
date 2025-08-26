// Vue.js refactored version of printsend.js
const { createApp, unref } = Vue;
const { ElInput, ElButton, ElPopover, ElLoading } = ElementPlus;

const PrintSendApp = {
    data() {
        return {
            // Print  data

            printInfo: {
                modelName: '',
                printTime: '00:00:00',
                totalWeight: 0,
                layerCount: 0,
                thumbnail: '',
                timeLapse: false,
                heatedBedLeveling: false,
                uploadAndPrint: false,
                switchToDeviceTab: false,
                autoRefill: false,
                bedType: 'btPEI',
                filamentList: [],
                mmsInfo: {
                    mmsList: [
                        {
                            mmsId: '',
                            mmsName: '',
                            trayList: [
                                {
                                    trayId: '',
                                    trayName: '',
                                    filamentType: '',
                                    filamentName: '',
                                    filamentColor: '',
                                }
                            ]
                        }
                    ]
                }
            },

            // Printer management
            printerList: [],
            curPrinter: null,
            printerDropdownOpen: false,

            // Bed type management
            bedTypes: [
                { value: 'btPEI', name: this.$t ? this.$t('printSend.texturedA') : 'Textured A', icon: 'img/bt_pei.png' },
                { value: 'btPC', name: this.$t ? this.$t('printSend.smoothB') : 'Smooth B', icon: 'img/bt_pc.png' }
            ],
            selectedBedTypeValue: 'btPEI',
            bedDropdownOpen: false,

            // Filament management
            currentPage: 1,
            pageSize: 5,
            hasMmsInfo: false,

            // UI state
            isEditingName: false,
            showFilamentSection: false,
            isIniting: false
        };
    },

    computed: {

        selectedBedType() {
            return this.bedTypes.find(b => b.value === this.selectedBedTypeValue) || this.bedTypes[0];
        },

        modelImageSrc() {
            return this.printInfo.thumbnail
                ? `data:image/png;base64,${this.printInfo.thumbnail}`
                : '';
        },

        showBedDropdown() {
            return this.printInfo.uploadAndPrint;
        },

        showPrintOptions() {
            return this.printInfo.uploadAndPrint;
        },

        totalPages() {
            return Math.ceil((this.printInfo.filamentList?.length || 0) / this.pageSize);
        },

        currentPageFilaments() {
            if (!this.printInfo.filamentList) return [];
            const start = (this.currentPage - 1) * this.pageSize;
            const end = Math.min(start + this.pageSize, this.printInfo.filamentList.length);
            return this.printInfo.filamentList.slice(start, end).map((filament) => ({
                ...filament
            }));
        },

        mmsFilamentList() {
            return this.printInfo.mmsInfo?.mmsList || [];
        }
    },

    methods: {
        // Lifecycle methods
        async init() {
            this.isIniting = true;
            await this.requestPrinterList();
            this.isIniting = false;
        },

        // IPC Communication methods
        async ipcRequest(method, params = {}, timeout = 10000) {
            try {
                const response = await nativeIpc.request(method, params, timeout);
                return response;
            } catch (error) {
                console.error(`IPC request failed for ${method}:`, error);
                this.showStatusTip(error.message || 'Request failed');
                throw error;
            }
        },


        // Communication with backend
        async requestPrintTask() {
            const loading = ElLoading.service({
                lock: true,
            });
            try {
                const params = {
                    printerId: this.curPrinter.printerId
                };
                const response = await this.ipcRequest('request_print_task', params);
                this.printInfo = { ...this.printInfo, ...response };
                this.updatePrintInfo();
            } catch (error) {
                console.error('Failed to request print task:', error);
            } finally {
                loading.close();
            }
        },

        async requestPrinterList() {
            // const loading = ElLoading.service({
            //     lock: true,
            // });
            try {
                const response = await this.ipcRequest('request_printer_list', {});
                this.printerList = response || [];
                await this.updatePrinterSelection();
            } catch (error) {
                console.error('Failed to request printer list:', error);
            } finally {
                // loading.close();
            }
        },

        async resizeWindow() {
            let expand = false;
            if (this.printInfo && this.printInfo.uploadAndPrint &&
                this.printInfo.mmsInfo &&
                this.printInfo.mmsInfo.mmsList &&
                this.printInfo.mmsInfo.mmsList.length > 0
            ) {
                expand = true;
            }
            try {
                await this.ipcRequest('expand_window', { expand });
            } catch (error) {
                console.error('Failed to request printer list:', error);
            } finally {
                // loading.close();
            }
        },

        async refreshPrinterList() {
            await this.requestPrinterList();
        },

        updatePrintInfo() {
            const printInfo = this.printInfo;
            // Set bed type if provided
            if (printInfo.bedType) {
                this.selectedBedTypeValue = printInfo.bedType;
            }

            // Check for MMS info
            if (printInfo.mmsInfo && printInfo.mmsInfo.mmsList && printInfo.mmsInfo.mmsList.length > 0) {
                this.hasMmsInfo = true;
            } else {
                this.hasMmsInfo = false;
            }

            this.$nextTick(() => {
                this.adjustModelNameWidth();
            });
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
            await this.onPrinterChanged();
        },

        // Printer dropdown methods
        togglePrinterDropdown() {
            this.printerDropdownOpen = !this.printerDropdownOpen;
            if (this.printerDropdownOpen) {
                this.bedDropdownOpen = false;
            }
        },

        closePrinterDropdown() {
            this.printerDropdownOpen = false;
        },

        selectPrinter(printer) {
            this.curPrinter = printer;
            this.printerDropdownOpen = false;
            this.onPrinterChanged();
        },

        // Bed type dropdown methods
        toggleBedDropdown() {
            this.bedDropdownOpen = !this.bedDropdownOpen;
            if (this.bedDropdownOpen) {
                this.printerDropdownOpen = false;
            }
        },

        closeBedDropdown() {
            this.bedDropdownOpen = false;
        },

        selectBedType(bedType) {
            this.selectedBedTypeValue = bedType.value;
            this.bedDropdownOpen = false;
            this.onBedTypeChanged(bedType.value);
        },

        // Model name editing
        startEditing() {
            this.isEditingName = true;
            this.$nextTick(() => {
                this.$refs.modelNameInput.focus();
            });
        },

        stopEditing() {
            this.isEditingName = false;
            this.printInfo.modelName = this.filterModelName(this.printInfo.modelName);
            this.adjustModelNameWidth();
        },

        filterModelName(str) {
            return str.replace(/[^a-zA-Z0-9\u4e00-\u9fa5_\-. ]/g, '');
        },

        adjustModelNameWidth() {
            const modelNameInput = this.$refs.modelNameInput;
            if (!modelNameInput) return;
            const input = modelNameInput.input;
            const span = document.createElement('span');
            span.style.visibility = 'hidden';
            span.style.position = 'fixed';
            span.style.font = window.getComputedStyle(input).font;
            span.innerText = modelNameInput.ref.value || modelNameInput.ref.placeholder;
            document.body.appendChild(span);
            input.style.width = (span.offsetWidth + 20) + 'px';
            document.body.removeChild(span);
        },

        // Filament management
        prevPage() {
            if (this.currentPage > 1) {
                this.currentPage--;
            }
        },

        nextPage() {
            if (this.currentPage < this.totalPages) {
                this.currentPage++;
            }
        },

        getFilamentStyle(filament) {
            const color = filament.filamentColor || '#888';
            const textColor = this.getContrastColor(color);
            return {
                background: color,
                color: textColor,
            };
        },

        getFilamentSvg(filament) {
            const color = filament.filamentColor || '#888';
            return getFilamentSvg(color);
        },

        getMmsCardStyle(mmsFilament) {
            const color = mmsFilament.filamentColor || '#888';
            const textColor = this.getContrastColor(color);
            return {
                background: color,
                color: textColor,
                border: `1px solid ${textColor}`
            };
        },

        getContrastColor(hexColor) {
            hexColor = hexColor.replace('#', '');
            if (hexColor.length === 3) {
                hexColor = hexColor.split('').map(x => x + x).join('');
            }
            const r = parseInt(hexColor.substr(0, 2), 16);
            const g = parseInt(hexColor.substr(2, 2), 16);
            const b = parseInt(hexColor.substr(4, 2), 16);
            const brightness = (r * 299 + g * 587 + b * 114) / 1000;
            return brightness > 180 ? '#222' : '#fff';
        },

        // MMS popover methods  
        updateFilamentMapping(filamentIndex, tray) {
            document.getElementsByClassName('filament-section')?.[0].click();
            console.log('Selected MMS Tray:', tray);
            for (let i = 0; i < this.printInfo.filamentList.length; i++) {
                if (this.printInfo.filamentList[i].index === filamentIndex) {
                    const filament = this.printInfo.filamentList[i];
                    filament.mappedMmsFilament.filamentColor = tray.filamentColor;
                    filament.mappedMmsFilament.filamentName = tray.filamentName;
                    filament.mappedMmsFilament.filamentType = tray.filamentType;
                    filament.mappedMmsFilament.trayName = tray.trayName;
                    filament.mappedMmsFilament.mmsId = tray.mmsId;
                    filament.mappedMmsFilament.trayId = tray.trayId;
                    filament.mappedMmsFilament.filamentWeight = tray.filamentWeight;
                    filament.mappedMmsFilament.filamentDensity = tray.filamentDensity;
                    filament.mappedMmsFilament.minNozzleTemp = tray.minNozzleTemp;
                    filament.mappedMmsFilament.maxNozzleTemp = tray.maxNozzleTemp;
                    filament.mappedMmsFilament.minBedTemp = tray.minBedTemp;
                    filament.mappedMmsFilament.maxBedTemp = tray.maxBedTemp;
                    filament.mappedMmsFilament.status = tray.status;
                    filament.mappedMmsFilament.filamentDiameter = tray.filamentDiameter;
                    filament.mappedMmsFilament.filamentId = tray.filamentId;
                    filament.mappedMmsFilament.vendor = tray.vendor;
                    filament.mappedMmsFilament.serialNumber = tray.serialNumber;
                    filament.mappedMmsFilament.settingId = tray.settingId;
                    filament.mappedMmsFilament.from = tray.from;
                    break;
                }
            }

        },

        // Action methods
        async cancel() {
            try {
                await this.ipcRequest('cancel_print', {});
            } catch (error) {
                console.error('Failed to cancel print:', error);
            }
        },

        async upload() {
            // Update task with current UI state
            this.printInfo.selectedPrinterId = this.curPrinter.printerId;
            this.printInfo.bedType = this.selectedBedTypeValue;

            // Validate filament mapping if MMS is present
            if (this.hasMmsInfo && this.printInfo.uploadAndPrint && !this.checkFilamentMapping()) {
                this.showStatusTip(this.$t('printSend.someFilamentsNotMapped'));
                return;
            }

            try {
                await this.ipcRequest('start_upload', this.printInfo);
            } catch (error) {
                console.error('Failed to start upload:', error);
            }
        },

        checkFilamentMapping() {
            return this.printInfo.filamentList.every(filament =>
                filament.mappedMmsFilament &&
                filament.mappedMmsFilament.trayName.trim() !== "" &&
                filament.mappedMmsFilament.filamentName.trim() !== "" &&
                filament.mappedMmsFilament.filamentType.trim() !== "" &&
                filament.mappedMmsFilament.mmsId.trim() !== "" &&
                filament.mappedMmsFilament.trayId.trim() !== ""
            );
        },

        // Utility methods
        formatWeight(weight) {
            return weight ? `${weight.toFixed(2)}g` : '0.0g';
        },

        showStatusTip(message) {
            if (window.ElementPlus && window.ElementPlus.ElMessage) {
                window.ElementPlus.ElMessage.error({
                    message: message,
                    duration: 5000,
                    showClose: true
                });
            }
        },

        // Event handlers called by external code
        async onPrinterChanged() {
            // Update bed types with current translations
            this.bedTypes = [
                { value: 'btPEI', name: this.$t('printSend.texturedA'), icon: 'img/bt_pei.png' },
                { value: 'btPC', name: this.$t('printSend.smoothB'), icon: 'img/bt_pc.png' }
            ];
            // Handle printer change logic if needed
            await this.requestPrintTask();
        },

        onBedTypeChanged(bedType) {
            // Handle bed type change logic if needed
        },

        refreshShowFilamentSection() {
            setTimeout(() => {
                this.showFilamentSection = this.printInfo.uploadAndPrint && this.hasMmsInfo;
            }, this.printInfo.uploadAndPrint&&this.hasMmsInfo ? 100 : 0);
        }
    },

    mounted() {
        // Initialize bed types with translations
        this.bedTypes = [
            { value: 'btPEI', name: this.$t('printSend.texturedA'), icon: 'img/bt_pei.png' },
            { value: 'btPC', name: this.$t('printSend.smoothB'), icon: 'img/bt_pc.png' }
        ];

        // Initialize the application
        this.init();
    },

    watch: {
        'printInfo.mmsInfo.mmsList': {
            async handler(newValue, oldValue) {
                this.resizeWindow();
                this.refreshShowFilamentSection();
            },
            immediate: false
        },
        'printInfo.uploadAndPrint': {
            async handler(newValue, oldValue) {
                // React to upload and print toggle changes
                if (!newValue) {
                    this.currentPage = 1; // Reset pagination when hiding filament section
                }
                this.resizeWindow();
                this.refreshShowFilamentSection();
            },
            immediate: false
        }

    }
};


const app = createApp(PrintSendApp)

// Register ElementPlus components globally
app.use(ElementPlus, {
    components: {
        ElInput,
        ElButton,
        ElPopover
    }
});

// Create and mount the Vue app
app.use(i18n);
app.mount('#app');

