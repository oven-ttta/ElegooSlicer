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
                    <el-form
                        ref="printerForm"
                        :model="formData"
                        :rules="formRules"
                        label-position="top"
                        autocomplete="off"
                    >
                        <div class="printer-setting-info-item">
                            <span class="printer-setting-info-label">Name</span>
                            <div class="printer-setting-info-value">
                                <span v-show="!isEditingName" class="printer-setting-info-text">{{ (printer && printer.printerName) || '' }}</span>
                                <el-form-item v-show="isEditingName" prop="printerName" class="printer-setting-form-item">
                                    <el-input
                                        v-model="formData.printerName"
                                        ref="nameInput"
                                        @blur="saveNameChanges"
                                        @keydown="onNameKeydown"
                                        size="small"
                                        placeholder="Enter printer name"
                                    />
                                </el-form-item>
                                <button class="printer-setting-edit-btn" @click="editPrinterName">
                                    <img src="img/edit.svg" alt="Edit" />
                                </button>
                            </div>
                        </div>

                        <div class="printer-setting-info-item">
                            <span class="printer-setting-info-label">Model</span>
                            <span class="printer-setting-info-value">{{ (printer && printer.printerModel) || '' }}</span>
                        </div>

                        <div class="printer-setting-info-item">
                            <span class="printer-setting-info-label">Host</span>
                            <div class="printer-setting-info-value">
                                <span v-show="!isEditingHost" class="printer-setting-info-text">{{ (printer && printer.host) || '' }}</span>
                                <el-form-item v-show="isEditingHost" prop="host" class="printer-setting-form-item">
                                    <el-input
                                        v-model="formData.host"
                                        ref="hostInput"
                                        @blur="saveHostChanges"
                                        @keydown="onHostKeydown"
                                        size="small"
                                        placeholder="Enter host, IP or URL"
                                    />
                                </el-form-item>
                                <button class="printer-setting-edit-btn" @click="editPrinterHost">
                                    <img src="img/edit.svg" alt="Edit" />
                                </button>
                            </div>
                        </div>

                        <div class="printer-setting-info-item">
                            <span class="printer-setting-info-label">Firmware</span>
                            <span class="printer-setting-info-value">
                                <span class="printer-setting-info-text">
                                    <span class="printer-setting-firmware-version">{{ (printer && printer.firmwareVersion) || '' }}</span>
                                    <span class="printer-setting-update-tag" :class="{ available: printer && printer.firmwareUpdate === 1 }">
                                        {{ getFirmwareUpdateText() }}
                                    </span>
                                </span>
                                <button v-if="showRefreshButton" class="printer-setting-refresh-btn" @click="checkFirmwareUpdate">
                                    <img src="img/refresh.svg" alt="Refresh" />
                                </button>
                            </span>
                        </div>
                    </el-form>
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
            formData: {
                printerName: '',
                host: ''
            },
            formRules: {
                printerName: [
                    { required: true, message: 'Please enter printer name', trigger: 'change' },
                    { min: 1, max: 50, message: 'Length should be 1 to 50 characters', trigger: 'change' }
                ],
                host: [
                    { required: true, message: 'Please enter host name, IP or URL', trigger: 'blur' },
                    { 
                        validator: (rule, value, callback) => {
                            this.printerStore.validateHost(rule, value, callback);
                        },
                        trigger: 'blur'
                    }
                ]
            }
        };
    },

    computed: {
        showRefreshButton() {
            return this.printer && this.printer.firmwareUpdate === 1;
        }
    },

    mounted() {
        // Initialize form data with current printer data
        this.syncFormData();
    },

    beforeUnmount() {
        // Reset editing state when component is unmounted
        this.resetEditingState();
    },

    watch: {
        // Watch for printer prop changes to sync form data
        printer: {
            handler(newPrinter) {
                if (newPrinter) {
                    this.syncFormData();
                    this.resetEditingState();
                }
            },
            immediate: true
        }
    },

    methods: {
        closeModal() {
            // Reset editing state before closing modal
            this.resetEditingState();
            this.$emit('close-modal');
        },

        deleteDevice() {
            if (!confirm('Are you sure you want to delete this device? This action cannot be undone.')) {
                return;
            }
            this.printerStore.requestDeletePrinter(this.printer.printerId);
        },

        editPrinterName() {
            this.formData.printerName = (this.printer && this.printer.printerName) || '';
            this.isEditingName = true;
            this.$nextTick(() => {
                if (this.$refs.nameInput) {
                    this.$refs.nameInput.focus();
                    this.$refs.nameInput.select();
                }
            });
        },

        editPrinterHost() {
            this.formData.host = (this.printer && this.printer.host) || '';
            this.isEditingHost = true;
            this.$nextTick(() => {
                if (this.$refs.hostInput) {
                    this.$refs.hostInput.focus();
                    this.$refs.hostInput.select();
                }
            });
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
            const newName = this.formData.printerName.trim();
            const currentName = (this.printer && this.printer.printerName) || '';
            
            if (newName && newName !== currentName) {
                // Use Element Plus form validation
                this.$refs.printerForm.validateField('printerName', (valid) => {
                    if (valid) {
                        this.printer.printerName = newName;
                        this.printerStore.requestUpdatePrinterName(this.printer.printerId, newName);
                        this.isEditingName = false;
                    } else {
                        // Validation failed, keep editing mode and show error
                        console.log('Name validation failed');
                    }
                });
            } else {
                this.isEditingName = false;
            }
        },

        saveHostChanges() {
            const newHost = this.formData.host.trim();
            const currentHost = (this.printer && this.printer.host) || '';

            if (newHost && newHost !== currentHost) {
                // Use Element Plus form validation
                this.$refs.printerForm.validateField('host', (valid) => {
                    if (valid) {
                        this.printer.host = newHost; 
                        this.printerStore.requestUpdatePrinterHost(this.printer.printerId, newHost);
                        this.isEditingHost = false;
                    } else {
                        // Validation failed, keep editing mode and show error
                        console.log('Host validation failed');
                    }
                });
            } else {
                this.isEditingHost = false;
            }
        },

        cancelNameChanges() {
            this.isEditingName = false;
            // Restore original value
            this.formData.printerName = (this.printer && this.printer.printerName) || '';
            // Clear validation errors
            if (this.$refs.printerForm) {
                this.$refs.printerForm.clearValidate('printerName');
            }
        },

        cancelHostChanges() {
            this.isEditingHost = false;
            // Restore original value
            this.formData.host = (this.printer && this.printer.host) || '';
            // Clear validation errors
            if (this.$refs.printerForm) {
                this.$refs.printerForm.clearValidate('host');
            }
        },

        checkFirmwareUpdate() {
            // this.$emit('check-firmware-update');
        },

        getFirmwareUpdateText() {
            if (this.printer && this.printer.firmwareUpdate === 1) {
                return 'Update Available';
            }
            return 'Latest Version';
        },

        // Sync form data with current printer data
        syncFormData() {
            if (this.printer) {
                this.formData.printerName = this.printer.printerName || '';
                this.formData.host = this.printer.host || '';
            }
        },

        // Reset editing state to display mode
        resetEditingState() {
            this.isEditingName = false;
            this.isEditingHost = false;
            // Clear any validation errors
            if (this.$refs.printerForm) {
                this.$refs.printerForm.clearValidate();
            }
        }
    }
};
