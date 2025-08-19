
const AddPrinterTemplate = /*html*/`
        <div class="add-printer-dialog">
          
            <el-tabs v-model="activeTab" class="add-printer-tabs" @tab-change="switchTab">
                <el-tab-pane label="Discover" class="add-printer-tabpane" name="discover">
                    <div class="add-printer-table">
                        <div class="add-printer-table-header">
                                <span>Device</span>
                                <span>Info</span>
                                <span>Status</span>
                        </div>
                        <div v-show="isLoading" >
                            <div class="linear-loading"></div>
                        </div>
                        <div v-if="printers && printers.length === 0" class="discover-empty">
                            <img src="./img/discover-empty.svg" width="180" />
                            <div>No Devices Found</div>
                        </div>
                        <div v-else class="add-printer-table-body">
                            <div v-for="(printer, idx) in printers" 
                                    :key="idx"
                                    :class="printerLineClass(idx)"
                                    @click="selectPrinter(idx)">
                            
                                <span>
                                    <img :src="printer.printerImg" class="add-printer-img"/>
                                </span>
                                <span>
                                    <span class="add-printer-name">{{ printer.printerName }}</span><br>
                                    <span class="add-printer-host">{{ printer.host }}</span>
                                </span>
                                <span>
                                    <span class="add-printer-status">Connectable</span>
                                </span>
                            </div>
                        </div>

                    </div>
                </el-tab-pane>
                <el-tab-pane label="Manual Add" class="add-printer-tabpane"  name="manual">
                        <div class="add-printer-content">
                                <manual-form-component 
                                    ref="manualFormComponent"
                                    :printer="null" ></manual-form-component>
                        </div>
                </el-tab-pane>
            </el-tabs>

            <el-dialog v-model="showPrinterAuth" title="Connect to Printer" width="600" center>
                <printer-auth-component ref="printerAuthComponent" :printer="pendingAuthPrinter" @close-modal="clearAuthData"
                    @add-printer="printerStore.requestAddPrinter"></printer-auth-component>
            </el-dialog>

            <button type="button" :disabled="isLoading" v-show="activeTab==='discover'" class="btn-primary" style=" position: absolute; right: 0px; top: 0px;" @click="requestDiscoverPrinters">Refresh</button>
            <div class="add-printer-footer">
                <button type="button" class="btn-secondary" @click="closeModal">Close</button>   
                <span class="help-link" v-show="activeTab==='discover'" @click.prevent="showHelp" style="align-self: flex-end;">Cannot find printer?</span>
                <button type="button" class="btn-primary" @click="connectPrinter">Connect</button>
            </div>

                        <!-- Help Dialog -->
            <el-dialog 
                v-model="showHelpDialog"
                title="Help"
                width="400px"
                center
            >
                <p>请确保打印机已经联网，并且与当前软件处于同一个网络，如果刷新后依旧无法发现，可尝试进行手动添加。</p>
                <template #footer>
                    <span class="dialog-footer">
                        <button class="btn-primary" @click="showHelpDialog = false">OK</button>
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
        },

        closeModal() {
            this.$emit('close-modal');
        },

        async connectPrinter() {
            if (this.activeTab === 'discover') {
                if (this.selectedPrinterIdx === null) {
                    alert('Please select a printer');
                    return;
                }
                const printer = this.printers[this.selectedPrinterIdx];
                if (printer.authMode === 'token' &&
                    (printer.extraInfo === '' ||
                        !(printer.extraInfo && printer.extraInfo.token) ||
                        printer.extraInfo.token === '')) {
                    this.pendingAuthPrinter = printer;
                    this.showPrinterAuth = true;
                } else {
                    this.printerStore.requestAddPrinter(printer);
                }

            } else {
                // call method directly through component reference
                if (this.$refs.manualFormComponent) {
                    const printer = await this.$refs.manualFormComponent.getFormData();
                    if (printer) {
                        this.printerStore.requestAddPhysicalPrinter(printer);
                    }
                }
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
        showHelp(){
            this.showHelpDialog = true;
        }
    }
};
