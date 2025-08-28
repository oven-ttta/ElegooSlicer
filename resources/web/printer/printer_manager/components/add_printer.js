
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

            <el-dialog v-model="showPrinterAuth" :title="$t('addPrinterDialog.connectToPrinter')" width="600" center>
                <printer-auth-component ref="printerAuthComponent" :printer="pendingAuthPrinter" @close-modal="clearAuthData"
                    @add-printer="connectPrinter1"></printer-auth-component>
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
            showPrinterAuth: false,
            pendingAuthPrinter: null,
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

        clearAuthData() {
            // this.pendingAuthPrinter = null;
            this.showPrinterAuth = false;
            this.$nextTick(() => {
                if (this.$refs.printerAuthComponent) {
                    this.$refs.printerAuthComponent.accessCodes = ["", "", "", "", "", ""];
                }
            });
        },

        closeModal() {
            this.$emit('close-modal');
        },

        async connectPrinter() {
            try {
                if (this.activeTab === 'discover') {
                    if (this.selectedPrinterIdx === null) {
                        alert(this.$t('addPrinterDialog.pleaseSelectPrinter'));
                        return;
                    }
                    const printer = this.printers[this.selectedPrinterIdx];
                    if (printer.authMode === 2 && (printer.accessCode === '' || printer.accessCode === null)) {
                        this.pendingAuthPrinter = printer;
                        this.showPrinterAuth = true;
                        this.$nextTick(() => {
                            if (this.$refs.printerAuthComponent) {
                                this.$refs.printerAuthComponent.accessCodes = ["", "", "", "", "", ""];
                                this.$refs.printerAuthComponent.focusFirstInput();
                            }
                        });

                    } else {
                        await this.printerStore.requestAddPrinter(printer);
                    }
                    return;
                } else {
                    // call method directly through component reference
                    if (this.$refs.manualFormComponent) {
                        const printer = await this.$refs.manualFormComponent.getFormData();
                        if (printer) {
                            await this.printerStore.requestAddPhysicalPrinter(printer);
                            this.closeModal();
                        }
                    }
                }

            } catch (error) {

            }
        },

        async connectPrinter1(printer) {
            try {
                await this.printerStore.requestAddPrinter(printer);
                this.clearAuthData();
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
