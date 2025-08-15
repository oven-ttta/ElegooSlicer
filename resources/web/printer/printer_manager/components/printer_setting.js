const PrinterSettingTemplate = /*html*/
    `
        <div class="printer-setting-dialog">
            <div class="printer-setting-content">
                <div class="printer-setting-left">
                    <div class="printer-setting-image-container">
                        <img class="printer-setting-image" :src="(printer && printer.printerImg) || ''" />
                    </div>
                    <button class="printer-setting-delete-btn" @click="deleteDevice">Delete Device</button>
                </div>
                
                <div class="printer-setting-right">
                    <div class="printer-setting-info-item">
                        <span class="printer-setting-info-label">Name</span>
                        <span class="printer-setting-info-value">
                            <span v-if="!isEditingName" class="printer-setting-info-text">{{ (printer && printer.printerName) || '' }}</span>
                            <el-input
                                v-if="isEditingName"
                                v-model="editingName"
                                ref="nameInput"
                                type="text" 
                                @keydown="onNameKeydown"
                                @blur="saveNameChanges"
                                @input="onNameInput"
                            />
                            <button v-if="!isEditingName" class="printer-setting-edit-btn" @click="editPrinterName">
                                <img src="img/edit.svg" alt="Edit" />
                            </button>
                        </span>
                    </div>
                    
                    <div class="printer-setting-info-item">
                        <span class="printer-setting-info-label">Model</span>
                        <span class="printer-setting-info-value">{{ (printer && printer.printerModel) || '' }}</span>
                    </div>
                    
                    <div class="printer-setting-info-item">
                        <span class="printer-setting-info-label">Host</span>
                        <span class="printer-setting-info-value">
                            <span v-if="!isEditingHost" class="printer-setting-info-text">{{ (printer && printer.host) || '' }}</span>
                            <el-input 
                                v-if="isEditingHost"
                                v-model="editingHost"
                                ref="hostInput"
                                type="text" 
                                @keydown="onHostKeydown"
                                @blur="saveHostChanges"
                                @input="onHostInput"
                            />
                            <button v-if="!isEditingHost" class="printer-setting-edit-btn" @click="editPrinterHost">
                                <img src="img/edit.svg" alt="Edit" />
                            </button>
                        </span>
                    </div>
                    
                    <div class="printer-setting-info-item">
                        <span class="printer-setting-info-label">Firmware</span>
                        <span class="printer-setting-info-value">
                            <span class="printer-setting-info-text">
                                <span class="printer-setting-firmware-version">{{ (printer && printer.firmwareVersion) || '' }}</span>
                                <span 
                                    class="printer-setting-update-tag"
                                    :class="{ available: printer && printer.firmwareUpdate === 1 }"
                                >
                                    {{ getFirmwareUpdateText() }}
                                </span>
                            </span>
                            <button 
                                v-if="showRefreshButton"
                                class="printer-setting-refresh-btn" 
                                @click="checkFirmwareUpdate"
                            >
                                <img src="img/refresh.svg" alt="Refresh" />
                            </button>
                        </span>
                    </div>
                </div>
            </div>
        </div>
    `;


// Printer Setting Component
const PrinterSettingComponent = {
    template: PrinterSettingTemplate,
    setup() {
        const printerStore = usePrinterStore();
        return {
            printerStore
        };
    },
    props: {
        printer: {
            type: Object,
            default: null
        }
    },

    emits: ['close-modal'],

    data() {
        return {
            isEditingName: false,
            isEditingHost: false,
            editingName: '',
            editingHost: ''
        };
    },

    computed: {
        showRefreshButton() {
            return this.printer && this.printer.firmwareUpdate === 1;
        }
    },

    mounted() {
    },

    beforeUnmount() {
    },

    watch: {
    },

    methods: {
        validatePrinterName(name) {
            // Only restrict backslash and forward slash, allow other characters
            const invalidCharsRegex = /[\\\/]/;
            return !invalidCharsRegex.test(name) && name.length <= 50;
        },

        validateIPAddress(ip) {
            // Basic IP address validation: numbers and dots only
            const ipRegex = /^(\d{1,3}\.){3}\d{1,3}$/;
            if (!ipRegex.test(ip)) {
                return false;
            }
            // Check each octet is between 0-255
            const octets = ip.split('.');
            return octets.every(octet => {
                const num = parseInt(octet, 10);
                return num >= 0 && num <= 255;
            });
        },

        closeModal() {
            this.$emit('close-modal');
        },

        deleteDevice() {
            if (!confirm('Are you sure you want to delete this device? This action cannot be undone.')) {
                return;
            }
            this.printerStore.requestDeletePrinter(this.printer.printerId);
        },

        editPrinterName() {
            this.editingName = (this.printer && this.printer.printerName) || '';
            this.isEditingName = true;
            this.$nextTick(() => {
                if (this.$refs.nameInput) {
                    this.$refs.nameInput.focus();
                    this.$refs.nameInput.select();
                }
            });
        },

        editPrinterHost() {
            this.editingHost = (this.printer && this.printer.host) || '';
            this.isEditingHost = true;
            this.$nextTick(() => {
                if (this.$refs.hostInput) {
                    this.$refs.hostInput.focus();
                    this.$refs.hostInput.select();
                }
            });
        },

        onNameInput() {
            // Remove backslash and forward slash in real-time
            let cleanValue = this.editingName.replace(/[\\\/]/g, '');
            // Limit to 50 characters
            if (cleanValue.length > 50) {
                cleanValue = cleanValue.substring(0, 50);
            }
            if (this.editingName !== cleanValue) {
                this.editingName = cleanValue;
            }
        },

        onHostInput() {
            // Only allow numbers and dots
            let cleanValue = this.editingHost.replace(/[^\d\.]/g, '');
            // Prevent multiple consecutive dots
            cleanValue = cleanValue.replace(/\.+/g, '.');
            // Prevent dot at the beginning
            cleanValue = cleanValue.replace(/^\./, '');
            // Prevent dot at the end
            cleanValue = cleanValue.replace(/\.$/, '');
            // Limit to 4 octets (3 dots max)
            const dots = (cleanValue.match(/\./g) || []).length;
            if (dots > 3) {
                const parts = cleanValue.split('.');
                cleanValue = parts.slice(0, 4).join('.');
            }
            // Limit each octet to 3 digits max
            const parts = cleanValue.split('.');
            const limitedParts = parts.map(part => part.length > 3 ? part.substring(0, 3) : part);
            cleanValue = limitedParts.join('.');

            if (this.editingHost !== cleanValue) {
                this.editingHost = cleanValue;
            }
        },

        onNameKeydown(e) {
            if (e.keyCode === 13) { // Enter
                e.preventDefault();
                this.saveNameChanges();
            } else if (e.keyCode === 27) { // Escape
                e.preventDefault();
                this.cancelNameChanges();
            }
        },

        onHostKeydown(e) {
            if (e.keyCode === 13) { // Enter
                e.preventDefault();
                this.saveHostChanges();
            } else if (e.keyCode === 27) { // Escape
                e.preventDefault();
                this.cancelHostChanges();
            }
        },

        saveNameChanges() {
            console.log('Saving printer name changes...');
            const newName = this.editingName.trim();
            const currentName = (this.printer && this.printer.printerName) || '';
            console.log(`New Name: ${newName}, Current Name: ${currentName}`);
            if (newName && newName !== currentName) {
                if (!this.validatePrinterName(newName)) {
                    alert('Printer name cannot contain backslash (\\) or forward slash (/) and must be 50 characters or less.');
                    this.$nextTick(() => {
                        if (this.$refs.nameInput) {
                            this.$refs.nameInput.focus();
                            this.$refs.nameInput.select();
                        }
                    });
                    return;
                }
                this.printer.printerName = newName;
                this.printerStore.requestUpdatePrinterName(this.printer.printerId, newName);
            }
            this.isEditingName = false;
        },

        saveHostChanges() {
            const newHost = this.editingHost.trim();
            const currentHost = (this.printer && this.printer.host) || '';

            if (newHost && newHost !== currentHost) {
                if (!this.validateIPAddress(newHost)) {
                    alert('Please enter a valid IP address (e.g., 192.168.1.100)');
                    this.$nextTick(() => {
                        if (this.$refs.hostInput) {
                            this.$refs.hostInput.focus();
                            this.$refs.hostInput.select();
                        }
                    });
                    return;
                }
                this.printerStore.updatePrinterHost(this.printer.printerId, newHost);
            }
            this.isEditingHost = false;
        },

        cancelNameChanges() {
            this.isEditingName = false;
            this.editingName = '';
        },

        cancelHostChanges() {
            this.isEditingHost = false;
            this.editingHost = '';
        },

        checkFirmwareUpdate() {
            // this.$emit('check-firmware-update');
        },

        getFirmwareUpdateText() {
            if (this.printer && this.printer.firmwareUpdate === 1) {
                return 'Update Available';
            }
            return 'Latest Version';
        }
    }
};
