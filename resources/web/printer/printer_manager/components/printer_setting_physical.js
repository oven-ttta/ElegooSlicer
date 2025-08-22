const PrinterSettingPhysicalTemplate = /*html*/
    `
        <div class="printer-setting-physical-dialog">
            <div style="flex: 1; overflow: auto;">
                <manual-form-component
                    ref="manualFormComponent"
                    :printer="printer"
                ></manual-form-component>
            </div>
            <div class="add-printer-footer">
                <el-button type="danger" @click="deletePrinter">{{ $t('printerSettingPhysical.delete') }}</el-button>
                <el-button type="primary" @click="confirmPrinter">{{ $t('printerSettingPhysical.confirm') }}</el-button>
            </div>
        </div>
    `;

// Printer Setting Physical Component
const PrinterSettingPhysicalComponent = {
    template: PrinterSettingPhysicalTemplate,
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
        };
    },

    mounted() {

    },

    beforeUnmount() {
    },

    watch: {

    },

    methods: {

        closeModal() {
            this.$emit('close-modal');
        },

        async deletePrinter() {
            if (confirm(this.$t('printerSettingPhysical.confirmDeletePrinter'))) {
               await this.printerStore.requestDeletePrinter(this.printer.printerId);
               this.closeModal();
            }
        },

        async confirmPrinter() {
            if (this.$refs.manualFormComponent) {
                const updatedPrinter = await this.$refs.manualFormComponent.getFormData();
                if (updatedPrinter) {
                    this.handleUpdatePhysicalPrinter(updatedPrinter);
                }
            }
        },

        handleUpdatePhysicalPrinter(newPrinter) {
            if (!this.printer) return;

            // check if name has changed
            if (newPrinter.printerName !== this.printer.printerName) {
                this.printerStore.requestUpdatePrinterName(this.printer.printerId, newPrinter.printerName);
            }

            // check if host has changed
            if (newPrinter.host !== this.printer.host) {
                this.printerStore.requestUpdatePrinterHost(this.printer.printerId, newPrinter.host);
            }
        }
    }
};