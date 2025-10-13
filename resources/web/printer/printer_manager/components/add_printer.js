
const AddPrinterTemplate = /*html*/`
        <div class="add-printer-dialog">
          
            <el-tabs v-model="activeTab" class="add-printer-tabs" @tab-change="switchTab">
                <el-tab-pane :label="$t('addPrinterDialog.discover')" class="add-printer-tabpane" name="discover">
                    <div class="add-printer-table">
                        <div class="add-printer-table-header">
                                <span>{{ $t('addPrinterDialog.device') }}</span>
                                <span>{{ $t('addPrinterDialog.info') }}</span>
                                <span>{{ $t('addPrinterDialog.status') }}</span>
                        </div>
                        <div v-show="isLoading" >
                            <div class="linear-loading"></div>
                        </div>
                        <div v-if="printers && printers.length === 0" class="discover-empty">
                            <img src="./img/discover-empty.svg" width="180" />
                            <div>{{ $t('addPrinterDialog.noDevicesFound') }}</div>
                        </div>
                        <div v-else class="add-printer-table-body">
                            <div v-for="(printer, idx) in printers" 
                                    :key="idx"
                                    :class="printerLineClass(idx)"
                                    @click="selectPrinter(idx)"
                                    @dblclick="connectPrinter">
                            
                                <span>
                                    <img :src="printer.printerImg" class="add-printer-img"/>
                                </span>
                                <span>
                                    <span class="add-printer-name">{{ printer.printerName }}</span><br>
                                    <span class="add-printer-host">{{ printer.host }}</span>
                                </span>
                                <span>
                                    <span class="add-printer-status">{{ $t('addPrinterDialog.connectable') }}</span>
                                </span>
                            </div>
                        </div>

                    </div>
                </el-tab-pane>
                <el-tab-pane :label="$t('addPrinterDialog.manualAdd')" class="add-printer-tabpane"  name="manual">
                        <div class="add-printer-content">
                                <manual-form-component 
                                    ref="manualFormComponent"
                                    :printer="null" ></manual-form-component>
                        </div>
                </el-tab-pane>
            </el-tabs>

            <el-dialog v-model="showPrinterAccessAuth" :title="$t('addPrinterDialog.connectToPrinter')" width="600" center>
                <printer-access-auth-component ref="printerAccessAuthComponent" :printer="pendingAccessAuthPrinter" @close-modal="clearAccessAuthData"
                    @add-printer="connectPrinterAuth"></printer-access-auth-component>
            </el-dialog>

            <el-dialog v-model="showPrinterPinAuth" :title="$t('addPrinterDialog.bindPrinter')" width="600" center>
                <printer-pin-auth-component ref="printerPinAuthComponent" :printer="pendingPinAuthPrinter" @close-modal="clearPinAuthData"
                    @add-printer="connectPrinterAuth"></printer-pin-auth-component>
            </el-dialog>

            <button type="button" :disabled="isLoading" v-show="activeTab==='discover'" class="btn-primary" style=" position: absolute; right: 0px; top: 0px;" @click="requestDiscoverPrinters">{{ $t('addPrinterDialog.refresh') }}</button>
            <div class="add-printer-footer">
                <button type="button" class="btn-secondary" @click="closeModal">{{ $t('addPrinterDialog.close') }}</button>   
                <span class="help-link" v-show="activeTab==='discover'" @click.prevent="showHelp" style="align-self: flex-end;">{{ $t('addPrinterDialog.cannotFindPrinter') }}</span>
                <button type="button" class="btn-primary" @click="connectPrinter">{{ $t('addPrinterDialog.connect') }}</button>
            </div>

                        <!-- Help Dialog -->
            <el-dialog 
                v-model="showHelpDialog"
                :title="$t('addPrinterDialog.help')"
                width="400px"
                center
            >
                <p>{{ $t('addPrinterDialog.helpMessage') }}</p>
                <template #footer>
                    <span class="dialog-footer">
                        <button class="btn-primary" @click="showHelpDialog = false">{{ $t('addPrinterDialog.ok') }}</button>
                    </span>
                </template>
            </el-dialog>
        </div>
    `;


// Add Printer Component
const AddPrinterComponent = {
    template: AddPrinterTemplate,

    setup() {
        const printerStore = usePrinterStore();
        return {
            printerStore
        };
    },
    props: {

    },
    emits: ['close-modal'],
    data() {
        return {
            activeTab: 'discover',
            selectedPrinterIdx: null,
            showPrinterAccessAuth: false,
            showPrinterPinAuth: false,
            pendingAccessAuthPrinter: null,
            pendingPinAuthPrinter: null,
            showHelpDialog: false
        };
    },

    mounted() {
    },

    watch: {
    },

    beforeUnmount() {
    },

    computed: {
        printers() {
            return this.printerStore.discoveredPrinters;
        },
        isLoading() {
            return this.printerStore.isDiscovering;
        }
    },

    methods: {
        switchTab(tab) {
            this.activeTab = tab;
        },

        clearAccessAuthData() {
            this.pendingAccessAuthPrinter = null;
            this.showPrinterAccessAuth = false;
            this.$nextTick(() => {
                if (this.$refs.printerAccessAuthComponent) {
                    this.$refs.printerAccessAuthComponent.accessCodes = ["", "", "", "", "", ""];
                }
            });
        },

        clearPinAuthData() {
            this.pendingPinAuthPrinter = null;
            this.showPrinterPinAuth = false;
            this.$nextTick(() => {
                if (this.$refs.printerPinAuthComponent) {
                    this.$refs.printerPinAuthComponent.pinCodes = ["", "", "", "", "", ""];
                }
            });
        },

        closeModal() {
            if (this.$refs.manualFormComponent) {
                this.$refs.manualFormComponent.resetForm();
            }
            this.$emit('close-modal');
        },

        async connectPrinter() {
            try {
                if (this.activeTab === 'discover') {
                    if (this.selectedPrinterIdx === null) {
                        ElementPlus.ElMessage({
                            message: this.$t('addPrinterDialog.pleaseSelectPrinter'),
                            type: 'error',
                        });
                        return;
                    }
                    const printer = this.printers[this.selectedPrinterIdx];
                    
                    // authMode === 2: requires access code
                    if (printer.authMode === 2 && (printer.accessCode === '' || printer.accessCode === null)) {
                        this.pendingAccessAuthPrinter = printer;
                        this.showPrinterAccessAuth = true;
                        this.$nextTick(() => {
                            if (this.$refs.printerAccessAuthComponent) {
                                this.$refs.printerAccessAuthComponent.accessCodes = ["", "", "", "", "", ""];
                                this.$refs.printerAccessAuthComponent.focusFirstInput();
                            }
                        });
                        return;
                    }
                    
                    // authMode === 3: requires pin code
                    if (printer.authMode === 3 && (printer.pinCode === '' || printer.pinCode === null)) {
                        this.pendingPinAuthPrinter = printer;
                        this.showPrinterPinAuth = true;
                        this.$nextTick(() => {
                            if (this.$refs.printerPinAuthComponent) {
                                this.$refs.printerPinAuthComponent.pinCodes = ["", "", "", "", "", ""];
                                this.$refs.printerPinAuthComponent.focusFirstInput();
                            }
                        });
                        return;
                    }
                    
                    // no auth required or auth already provided
                    await this.printerStore.requestAddPrinter(printer);
                    return;
                } else {
                    // manual add tab
                    if (this.$refs.manualFormComponent) {
                        const printer = await this.$refs.manualFormComponent.getFormData();
                        if (printer) {
                            await this.printerStore.requestAddPhysicalPrinter(printer);
                            this.$refs.manualFormComponent.resetForm();
                            this.closeModal();
                        }
                    }
                }

            } catch (error) {
                console.error('Failed to connect printer:', error);
            }
        },

        async connectPrinterAuth(printer) {
            try {
                await this.printerStore.requestAddPrinter(printer);
                this.clearAccessAuthData();
                this.clearPinAuthData();
            } catch (error) {
                console.error('Failed to connect printer:', error);
            }
        },

        requestDiscoverPrinters() {
            this.printerStore.requestDiscoverPrinters();
        },

        selectPrinter(idx) {
            this.selectedPrinterIdx = idx;
        },

        printerLineClass(idx) {
            return {
                'add-printer-table-tr': true,
                'selected': this.selectedPrinterIdx === idx
            };
        },
        showHelp() {
            this.showHelpDialog = true;
        }
    }
};
