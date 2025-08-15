// Vue.js refactored version of printsend.js
const { createApp, unref } = Vue;
const { ElInput, ElButton, ElPopover } = ElementPlus;

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
                    mmsList: []
                }
            },
            
            // Printer management
            printerList: [],
            curPrinter: null,
            printerDropdownOpen: false,

            // Bed type management
            bedTypes: [
                { value: 'btPEI', name: 'Textured A', icon: 'img/bt_pei.png' },
                { value: 'btPC', name: 'Smooth B', icon: 'img/bt_pc.png' }
            ],
            selectedBedTypeValue: 'btPEI',
            bedDropdownOpen: false,

            // Filament management
            currentPage: 1,
            pageSize: 8,
            hasMmsInfo: false,

            // UI state
            isEditingName: false,
            statusTip: {
                show: false,
                message: ''
            },
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

        showFilamentSection() {
            return this.printInfo.uploadAndPrint && this.hasMmsInfo;
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
            this.translatePage();
            await this.requestPrinterList();
        },


        // Communication with backend
        requestPrintTask() {
            const message = {
                sequence_id: Math.round(new Date() / 1000),
                command: "request_print_task",
                printerId: this.curPrinter.printerId
            };
            SendWXMessage(JSON.stringify(message));
        },

        requestPrinterList() {
            const message = {
                sequence_id: Math.round(new Date() / 1000),
                command: "request_printer_list"
            };
            console.log('Requesting printer list:', message);
            SendWXMessage(JSON.stringify(message));
        },

        refreshPrinterList() {
            this.requestPrinterList();
        },

        // Handle messages from backend
        handleStudio(data) {
            const command = data.command;

            if (command === 'response_print_task') {
                console.log('response_print_task', data.response);
                this.printInfo = { ...this.printInfo, ...data.response };
                this.updatePrintInfo();
            } else if (command === 'response_printer_list') {
                this.printerList = data.response || [];
                this.updatePrinterSelection();
            }
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

        updatePrinterSelection() {
            if (this.printerList.length === 0) return;

            // Find selected printer or use first one
            let selectedPrinter = this.printerList.find(p => p.selected);
            if (!selectedPrinter) {
                selectedPrinter = this.printerList[0];
            }

            this.curPrinter = selectedPrinter;
            this.onPrinterChanged();
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
            for(let i = 0; i < this.printInfo.filamentList.length; i++) {
                if(this.printInfo.filamentList[i].index === filamentIndex) {
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
        cancel() {
            const message = {
                command: 'cancel_print',
                sequence_id: Math.round(new Date() / 1000)
            };
            SendWXMessage(JSON.stringify(message));
        },

        upload() {
            // Update task with current UI state
            this.printInfo.selectedPrinterId = this.curPrinter.printerId;
            this.printInfo.bedType = this.selectedBedTypeValue;

            // Validate filament mapping if MMS is present
            if (this.hasMmsInfo && !this.checkFilamentMapping()) {
                this.showStatusTip(this.getTranslation('some_filaments_not_mapped'));
                return;
            }

            const message = {
                command: 'start_upload',
                sequence_id: Math.round(new Date() / 1000),
                data: this.printInfo
            };
            SendWXMessage(JSON.stringify(message));
        },

        checkFilamentMapping() {
            return this.printInfo.filamentList.every(filament =>
                filament.mappedMmsFilament && 
                filament.mappedMmsFilament.trayName && 
                filament.mappedMmsFilament.trayName.trim() !== ""
            );
        },

        // Utility methods
        formatWeight(weight) {
            return weight ? `${weight.toFixed(2)}g` : '0.0g';
        },

        showStatusTip(message) {
            this.statusTip.message = message;
            this.statusTip.show = true;
            setTimeout(() => {
                this.statusTip.show = false;
            }, 3000);
        },

        translatePage() {
            if (typeof TranslatePage === 'function') {
                TranslatePage();
            }
        },

        getTranslation(key) {
            return window.g_translation && window.g_translation[key] ? window.g_translation[key] : key;
        },

        // Event handlers called by external code
        onPrinterChanged() {
            // Handle printer change logic if needed
            this.requestPrintTask();
        },

        onBedTypeChanged(bedType) {
            // Handle bed type change logic if needed
        }
    },

    mounted() {
        // Initialize the application
        this.init();

        // Set up global message handler
        window.HandleStudio = (data) => {
            this.handleStudio(data);
        };

        // Set up global functions that might be called externally
        window.OnInit = () => {
            this.init();
        };
    },

    watch: {
        'printInfo.uploadAndPrint'(newValue) {
            // React to upload and print toggle changes
            if (!newValue) {
                this.currentPage = 1; // Reset pagination when hiding filament section
            }
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
app.mount('#app');

