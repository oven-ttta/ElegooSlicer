// Vue.js refactored version of printsend.js
const { createApp, unref } = Vue;
const { ElInput, ElButton, ElPopover } = ElementPlus;
// Click outside directive
const clickOutside = {
    beforeMount(el, binding) {
        el.clickOutsideEvent = function (event) {
            if (!(el === event.target || el.contains(event.target))) {
                binding.value(event);
            }
        };
        document.body.addEventListener('click', el.clickOutsideEvent);
    },
    unmounted(el) {
        document.body.removeEventListener('click', el.clickOutsideEvent);
    }
};

const PrintSendApp = {
    directives: {
        clickOutside
    },
    data() {
        return {
            // Print task data
            printTask: {
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
                    filamentList: []
                },
                amsInfo: {
                    trayFilamentList: []
                }
            },

            // Printer management
            printerList: [],
            selectedPrinterId: null,
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
            pageSize: 4,
            hasAmsInfo: false,

            // UI state
            isEditingName: false,
            statusTip: {
                show: true,
                message: '111'
            },
        };
    },

    computed: {

        selectedPrinter() {
            return this.printerList.find(p => p.printerId === this.selectedPrinterId);
        },

        selectedBedType() {
            return this.bedTypes.find(b => b.value === this.selectedBedTypeValue) || this.bedTypes[0];
        },

        modelImageSrc() {
            return this.printTask.printInfo.thumbnail
                ? `data:image/png;base64,${this.printTask.printInfo.thumbnail}`
                : '';
        },

        showBedDropdown() {
            return this.printTask.printInfo.uploadAndPrint;
        },

        showFilamentSection() {
            return this.printTask.printInfo.uploadAndPrint && this.hasAmsInfo;
        },

        showPrintOptions() {
            return this.printTask.printInfo.uploadAndPrint;
        },

        totalPages() {
            return Math.ceil((this.printTask.printInfo.filamentList?.length || 0) / this.pageSize);
        },

        currentPageFilaments() {
            if (!this.printTask.printInfo.filamentList) return [];
            const start = (this.currentPage - 1) * this.pageSize;
            const end = Math.min(start + this.pageSize, this.printTask.printInfo.filamentList.length);
            return this.printTask.printInfo.filamentList.slice(start, end).map((filament, index) => ({
                ...filament,
                index: start + index
            }));
        },

        amsFilamentList() {
            // return this.printTask.amsInfo?.trayFilamentList || [];
            return [
                {
                    trayIndex: 0,
                    filamentColor: '#FF0000',
                    filamentName: 'Filament 1',
                    filamentType: 'PLA'
                },
                {
                    trayIndex: 1,
                    filamentColor: '#00FF00',
                    filamentName: 'Filament 2',
                    filamentType: 'PLA'
                },
                {
                    trayIndex: 2,
                    filamentColor: '#0000FF',
                    filamentName: 'Filament 3',
                    filamentType: 'PLA'
                },
                {
                    trayIndex: 2,
                    filamentColor: '#FFFFFF',
                    filamentName: 'Filament 3',
                    filamentType: 'PLA'
                }
            ]
        }
    },

    methods: {
        // Lifecycle methods
        async init() {
            this.translatePage();
            await this.requestPrintTask();
            await this.requestPrinterList();
        },


        // Communication with backend
        requestPrintTask() {
            const message = {
                sequence_id: Math.round(new Date() / 1000),
                command: "request_print_task"
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
                this.printTask = { ...this.printTask, ...data.response };
                this.updatePrintInfo();
            } else if (command === 'response_printer_list') {
                this.printerList = data.response || [];
                this.updatePrinterSelection();
            }
        },

        updatePrintInfo() {
            const printInfo = this.printTask.printInfo;
            const amsInfo = this.printTask.amsInfo;

            // Set bed type if provided
            if (printInfo.bedType) {
                this.selectedBedTypeValue = printInfo.bedType;
            }

            // Check for AMS info
            if (amsInfo && amsInfo.trayFilamentList && amsInfo.trayFilamentList.length > 0) {
                this.hasAmsInfo = true;
            } else {
                this.hasAmsInfo = false;
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

            this.selectedPrinterId = selectedPrinter.printerId;
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
            this.selectedPrinterId = printer.printerId;
            this.printerDropdownOpen = false;
            this.onPrinterChanged(printer.printerId);
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
            this.printTask.printInfo.modelName = this.filterModelName(this.printTask.printInfo.modelName);
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
            const color = filament.amsFilamentColor || '#888';
            return getFilamentSvg(color);
        },

        getAmsCardStyle(amsFilament) {
            const color = amsFilament.filamentColor || '#888';
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

        // AMS popover methods  
        selectAmsFilament(amsIndex, amsFilament) {

            document.getElementsByClassName('filament-section')?.[0].click();
            console.log('Selected AMS Filament:', amsFilament);
            this.updateFilamentMapping(amsIndex, amsFilament);
        },

        updateFilamentMapping(filamentIndex, amsFilament) {
            if (this.printTask.printInfo.filamentList && this.printTask.printInfo.filamentList[filamentIndex]) {
                const filament = this.printTask.printInfo.filamentList[filamentIndex];
                filament.amsFilamentColor = amsFilament.filamentColor;
                filament.amsFilamentName = amsFilament.filamentName;
                filament.amsFilamentType = amsFilament.filamentType;
                filament.amsTrayIndex = amsFilament.trayIndex;
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
            this.printTask.printInfo.selectedPrinterId = this.selectedPrinterId;
            this.printTask.printInfo.bedType = this.selectedBedTypeValue;

            // Validate filament mapping if AMS is present
            if (this.hasAmsInfo && !this.checkFilamentMapping()) {
                this.showStatusTip(this.getTranslation('some_filaments_not_mapped'));
                return;
            }

            const message = {
                command: 'start_upload',
                sequence_id: Math.round(new Date() / 1000),
                data: this.printTask
            };
            SendWXMessage(JSON.stringify(message));
        },

        checkFilamentMapping() {
            return this.printTask.printInfo.filamentList.every(filament =>
                filament.amsTrayIndex !== null && filament.amsTrayIndex !== undefined
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
        onPrinterChanged(printerId) {
            // Handle printer change logic if needed
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
        'printTask.printInfo.uploadAndPrint'(newValue) {
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

