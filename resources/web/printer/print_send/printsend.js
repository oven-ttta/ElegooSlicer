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
                bedType: 'btPTE',
                currentProjectPrinterModel: '',
                filamentList: []
            },

            // MMS info (separated from printInfo for independent loading)
            mmsInfo: {
                mmsSystemName: '',
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
            },

            // Printer management
            printerList: [],
            curPrinter: null,
            printerDropdownOpen: false,

            // Bed type management
            bedTypes: [
                { value: 'btPTE', name: this.$t ? this.$t('printSend.texturedA') : 'Textured A', icon: 'img/bt_pte.png' },
                { value: 'btPC', name: this.$t ? this.$t('printSend.smoothB') : 'Smooth B', icon: 'img/bt_pc.png' }
            ],
            selectedBedTypeValue: 'btPTE',
            bedDropdownOpen: false,

            // Filament management
            currentPage: 1,
            pageSize: 5,
            hasMmsInfo: false,
            mMmsConnected: false,

            // UI state
            isEditingName: false,
            showFilamentSection: false,
            isIniting: false,
            // Currently selected heated bed type in the app
            currentBedType: ""
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
            return Math.ceil(((this.printInfo.filamentList && this.printInfo.filamentList.length) || 0) / this.pageSize);
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
            return (this.mmsInfo && this.mmsInfo.mmsList) || [];
        },

        // Check if selected printer model does not match the current project printer model
        printerModelNotMatch() {
            return this.curPrinter && this.printInfo.currentProjectPrinterModel && this.curPrinter.printerModel !== this.printInfo.currentProjectPrinterModel;
        },

        bedTypeNotMatch() {
            return this.printInfo.uploadAndPrint && this.currentBedType !== this.selectedBedTypeValue && this.curPrinter.printCapabilities.supportsHeatedBedSwitching;
        }
    },

    methods: {
        // Lifecycle methods
        async init() {
            this.isIniting = true;
            await this.requestPrinterList();
            await this.getCurrentBedType();
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
            } catch (error) {
                console.error('Failed to request print task:', error);
            } finally {
                loading.close();
            }
        },

        async requestMmsInfo() {         
            const loading = ElLoading.service({
                lock: true,
            });
            try {
                const params = {
                    printerId: this.curPrinter.printerId
                };
                const response = await this.ipcRequest('request_mms_info', params);
                this.mmsInfo = response.mmsInfo;
                this.printInfo.filamentList = response.mappedFilamentList;        
                return true;
            } catch (error) {
                console.error('Failed to request MMS info:', error);
                return false;  
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
                this.mmsInfo &&
                this.mmsInfo.mmsList &&
                this.mmsInfo.mmsList.length > 0
            ) {
                expand = true;
            }
            try {
                nativeIpc.sendEvent('expand_window', { expand });
            } catch (error) {
                console.error('Failed to request printer list:', error);
            }
        },

        async getCurrentBedType() {
            try {
                const response = await this.ipcRequest('get_current_bed_type', { printerId: this.curPrinter.printerId });
                if (response && response.bedType) {
                    console.log("Current bed type:", response.bedType);
                    this.currentBedType = response.bedType;
                    console.log("Selected bed type:", this.selectedBedTypeValue);
                }
            } catch (error) {
                console.error('Failed to get current bed type:', error);
            }
        },

        async refreshPrinterList() {
            await this.requestPrinterList();
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
            console.log("Selected bed type:", bedType);
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
            return str.replace(/[\\/:*?"<>|]/g, '');
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
            const filamentSection = document.getElementsByClassName('filament-section');
            if (filamentSection && filamentSection[0]) {
                filamentSection[0].click();
            }
            console.log('Selected MMS Tray:', tray);
            for (let i = 0; i < this.printInfo.filamentList.length; i++) {
                if (this.printInfo.filamentList[i].index === filamentIndex) {
                    const filament = this.printInfo.filamentList[i];
                    if (filament.filamentType !== tray.filamentType && tray.filamentType !== '') {
                        this.showStatusTip(this.$t('printSend.filamentTypeNotMatch'));
                        return;
                    }
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
                nativeIpc.sendEvent('cancel_print', {});
            } catch (error) {
                console.error('Failed to cancel print:', error);
            }
        },

        async upload() {
            if(this.curPrinter.connectStatus !== 1) {
                this.showStatusTip(this.$t('printSend.printerNotConnected'));
                return;
            }
            // Update task with current UI state
            this.printInfo.selectedPrinterId = this.curPrinter.printerId;
            this.printInfo.bedType = this.selectedBedTypeValue;

            //Validate filament mapping if MMS is present
            if(this.printInfo.uploadAndPrint && this.curPrinter.systemCapabilities.supportsMultiFilament && !this.mMmsConnected) {
                this.showStatusTip(this.$t('printSend.printerMmsNotConnected'));
                return;
            }
            if (this.hasMmsInfo && this.printInfo.uploadAndPrint && !this.checkFilamentMapping()) {
                this.showStatusTip(this.$t('printSend.someFilamentsNotMapped'));
                return;
            }

            const loading = ElLoading.service({
                lock: true,
            });

            try {
                const uploadData = {
                    ...this.printInfo,
                };
                await this.ipcRequest('start_upload', uploadData);
            } catch (error) {
                console.error('Failed to start upload:', error);
            } finally {
                loading.close();
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
                { value: 'btPTE', name: this.$t('printSend.texturedA'), icon: 'img/bt_pte.png' },
                { value: 'btPC', name: this.$t('printSend.smoothB'), icon: 'img/bt_pc.png' }
            ];
            // Handle printer change logic if needed
            await this.requestPrintTask();

            this.mMmsConnected  = await this.requestMmsInfo();
         
            // Set bed type if provided
            if (this.printInfo.bedType) {
                this.selectedBedTypeValue = this.printInfo.bedType;
            }

            // Check for MMS info
            if (this.mmsInfo && this.mmsInfo.mmsList && this.mmsInfo.mmsList.length > 0) {
                this.hasMmsInfo = true;
            } else {
                this.hasMmsInfo = false;
            }

            this.$nextTick(() => {
                this.adjustModelNameWidth();
            });
        },

        onBedTypeChanged(bedType) {
            // Handle bed type change logic if needed
        },

        refreshShowFilamentSection() {
            setTimeout(() => {
                this.showFilamentSection = this.printInfo.uploadAndPrint && this.hasMmsInfo;
            }, this.printInfo.uploadAndPrint && this.hasMmsInfo ? 100 : 0);
        }
    },

    mounted() {
        // Initialize bed types with translations
        this.bedTypes = [
            { value: 'btPTE', name: this.$t('printSend.texturedA'), icon: 'img/bt_pte.png' },
            { value: 'btPC', name: this.$t('printSend.smoothB'), icon: 'img/bt_pc.png' }
        ];

        // Initialize the application
        this.init();
        disableRightClickMenu();
    },

    watch: {
        'mmsInfo.mmsList': {
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

