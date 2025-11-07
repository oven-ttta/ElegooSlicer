const ManualFormTemplate = /*html*/
    `
        <el-form
            ref="manualForm"
            class="manual-form"
            :model="formData"
            :rules="formRules"
            label-position="top"
            autocomplete="off"
        >
            <div class="form-group-box">
                <div class="form-group">
                    <div class="form-group-header">
                        <label class="required">{{ $t('manualForm.printerModel') }}</label>
                        <div class="form-group-advanced">
                            <span>{{ $t('manualForm.advanced') }}</span>
                            <label class="toggle-switch">
                                <input type="checkbox" v-model="showAdvanced" @change="toggleAdvanced"/>
                                <span class="toggle-slider"></span>
                            </label>
                        </div>
                    </div>
                    <div class="form-group-row">
                        <div class="form-group-col">
                            <el-form-item prop="vendor">
                                <el-select
                                    :show-arrow="false"
                                    v-model="formData.vendor"
                                    @change="onVendorChange"
                                    :disabled="isVendorDisabled"
                                    :placeholder="$t('manualForm.selectVendor')"
                                >
                                    <el-option
                                        v-for="vendor in vendors"
                                        :key="vendor"
                                        :value="vendor"
                                        :label="vendor"
                                    />
                                </el-select>
                            </el-form-item>
                        </div>
                        <div class="form-group-col">
                            <el-form-item prop="printerModel">
                                <el-select
                                    :show-arrow="false"
                                    v-model="formData.printerModel"
                                    @change="onModelChange"
                                    :disabled="isModelDisabled"
                                    :placeholder="$t('manualForm.selectModel')"
                                >
                                    <el-option
                                        v-for="model in availableModels"
                                        :key="model.modelName"
                                        :value="model.modelName"
                                        :label="model.modelName"
                                        :data-host-type="model.hostType"
                                    />
                                </el-select>
                            </el-form-item>
                        </div>
                    </div>
                </div>

                <div class="form-group">
                    <label class="required">{{ $t('manualForm.printerName') }}</label>
                    <el-form-item prop="printerName">
                        <el-input
                            type="text"
                            v-model="formData.printerName"
                            :placeholder="formData.printerModel"
                            maxlength="50"
                            required
                            @keydown="onPrinterNameKeydown"
                        />
                    </el-form-item>
                </div>

                <div class="form-group">
                    <label class="required">{{ $t('manualForm.hostType') }}</label>
                    <el-form-item prop="hostType">
                        <el-select v-model="formData.hostType" :placeholder="$t('manualForm.selectHostType')" :show-arrow="false">
                            <el-option
                                v-for="hostType in hostTypes"
                                :key="hostType"
                                :value="hostType"
                                :label="hostType"
                            />
                        </el-select>
                    </el-form-item>
                </div>

                <div class="form-group">
                    <label class="required">{{ $t('manualForm.hostNameIpUrl') }}</label>
                    <el-form-item prop="host">
                        <el-input
                            type="text"
                            v-model="formData.host"
                            required
                            @keydown="onHostKeydown"
                        />
                    </el-form-item>
                </div>
                <div class="form-group">
                    <label>{{ $t('manualForm.serialNumber') }}</label>
                    <el-form-item prop="serialNumber">
                        <el-input
                            type="text"
                            v-model="formData.serialNumber"
                            :placeholder="$t('manualForm.serialNumberPlaceholder')"
                            maxlength="50"
                        />
                    </el-form-item>
                </div>

                <div class="form-group">
                    <label>{{ $t('manualForm.accessCode') }}</label>
                    <el-form-item prop="accessCode">
                        <el-input
                            type="password"
                            v-model="formData.accessCode"
                            :placeholder="$t('manualForm.accessCodePlaceholder')"
                            maxlength="20"
                            show-password
                        />
                    </el-form-item>
                </div>

                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label>{{ $t('manualForm.webUrl') }}</label>
                    <el-form-item prop="webUrl">
                        <el-input 
                        type="text" 
                        v-model="formData.webUrl"
                        @keydown="onWebUrlKeydown"
                        />
                    </el-form-item>
                </div>
                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label>{{ $t('manualForm.password') }}</label>
                    <el-input 
                    type="password" 
                    v-model="formData.password"                 
                    @keydown="onPasswordKeydown"
                    />
                </div>
                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label>{{ $t('manualForm.httpsCAFile') }}</label>
                    <div class="input-group">
                        <el-input type="text" v-model="formData.caFile" readonly/>
                        <button type="button" class="btn-secondary">{{ $t('manualForm.preview') }}</button>
                    </div>
                </div>
                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label class="checkbox-label">
                        <span>{{ $t('manualForm.ignoreCertRevocation') }}</span>
                        <input type="checkbox" v-model="formData.ignoreCertRevocation"/>
                    </label>
                </div>
                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label class="form-note">{{ $t('manualForm.httpsCAFileNote') }}</label>
                </div>
            </div>
        </el-form>
    `;


// Manual Form Component (can be imported by other applications)
const ManualFormComponent = {
    template: ManualFormTemplate,
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

    emits: [],

    data() {
        return {
            formData: {
                vendor: '',
                printerModel: '',
                printerName: '',
                hostType: '',
                host: '',
                serialNumber: '',
                accessCode: '',
                webUrl: '',
                password: '',
                caFile: '',
                ignoreCertRevocation: false
            },
            formRules: {
                vendor: [
                    { required: true, message: this.$t('manualForm.pleaseSelectVendor'), trigger: 'change' }
                ],
                printerModel: [
                    { required: true, message: this.$t('manualForm.pleaseSelectPrinterModel'), trigger: 'change' }
                ],
                // printerName: [
                //     { required: true, message: this.$t('manualForm.pleaseEnterPrinterName'), trigger: 'change' },
                //     // { min: 1, max: 50, message: this.$t('manualForm.lengthShouldBe1To50'), trigger: 'change' }
                // ],
                hostType: [
                    { required: true, message: this.$t('manualForm.pleaseSelectHostType'), trigger: 'change' }
                ],
                host: [
                    { required: true, message: this.$t('manualForm.pleaseEnterHostNameIpUrl'), trigger: 'blur' },
                    {
                        validator: (rule, value, callback) => {
                            this.printerStore.validateHost(rule, value, callback);
                        },
                        trigger: 'blur'
                    }
                ]
            },
            showAdvanced: false,
            vendors: [],
            availableModels: [],
            hostTypes: [],
            allModels: [],
            isVendorDisabled: false,
            isModelDisabled: false
        };
    },

    mounted() {
    },

    beforeUnmount() {
    },

    computed: {
        printerModelList() {
            return this.printerStore.printerModelList;
        }
    },

    watch: {
        printerModelList: {
            handler(newVal) {
                if (newVal) {
                    this.renderPrinterModelList(newVal, this.printer);
                }
            },
            immediate: true
        },

        printer: {
            handler(newVal) {
                if (newVal && this.printerModelList) {
                    this.renderPrinter(newVal);
                }
            },
            immediate: true
        }
    },

    methods: {
        renderPrinterModelList(printerModelList, printer = null) {

            if (!printerModelList) return;

            // extract vendor list
            this.vendors = printerModelList.map(vendorModels => vendorModels.vendor);

            // extract all models
            this.allModels = [];
            printerModelList.forEach(vendorModels => {
                this.allModels.push(...vendorModels.models);
            });

            // extract unique host types
            const uniqueHostTypes = new Set();
            this.allModels.forEach(model => {
                uniqueHostTypes.add(model.hostType);
            });
            this.hostTypes = Array.from(uniqueHostTypes);

            // if there is vendor, default select the first one
            if (this.vendors.length > 0 && !this.formData.vendor) {
                this.formData.vendor = this.vendors[0];
                this.onVendorChange();
            }

            // if there is printer data to fill, execute after model list rendering
            if (printer) {
                this.renderPrinter(printer);
            }
        },

        onVendorChange() {
            const selectedVendor = this.formData.vendor;
            this.availableModels = [];

            if (selectedVendor && this.printerModelList) {
                const vendorData = this.printerModelList.find(vendor => vendor.vendor === selectedVendor);
                if (vendorData) {
                    this.availableModels = vendorData.models;

                    if (this.availableModels.length > 0 && !this.formData.printerModel) {
                        this.formData.printerModel = this.availableModels[0].modelName;
                        this.onModelChange();
                    }
                }
            }
        },

        onModelChange() {
            const selectedModel = this.formData.printerModel;
            if (selectedModel) {
                const model = this.availableModels.find(m => m.modelName === selectedModel);
                if (model) {
                    this.formData.hostType = model.hostType;
                }
            }
        },

        toggleAdvanced() {
            // showAdvanced is automatically updated through v-model
        },

        onPrinterNameKeydown(e) {
            if (e.keyCode === 13) { // Enter key
                e.preventDefault();
                this.$refs.manualForm.validateField('printerName');
            }
        },

        onHostKeydown(e) {
            if (e.keyCode === 13) { // Enter key
                e.preventDefault();
                this.$refs.manualForm.validateField('host');
            }
        },

        onWebUrlKeydown(e) {
            if (e.keyCode === 13) {
                e.preventDefault();
                this.$refs.manualForm.validateField('webUrl');
            }
        },

        onPasswordKeydown(e) {
            if (e.keyCode === 13) {
                e.preventDefault();
                this.$refs.manualForm.validateField('password');
            }
        },

        renderPrinter(printer) {
            if (!printer) return;

            // handle vendor and model selection
            if (printer.vendor && printer.vendor !== this.formData.vendor) {
                this.formData.vendor = printer.vendor;
                this.onVendorChange();
            }

            if (printer.printerModel && printer.printerModel !== this.formData.printerModel) {
                this.formData.printerModel = printer.printerModel;
                this.onModelChange();
            }

            if (printer.hostType && printer.hostType !== this.formData.hostType) {
                this.formData.hostType = printer.hostType;
            }

            // if it is an existing printer, disable vendor and model selection
            if (printer.vendor || printer.printerModel) {
                this.isVendorDisabled = true;
                this.isModelDisabled = true;
            }

            // fill other fields
            this.formData.printerName = printer.printerName ? printer.printerName : '';
            this.formData.host = printer.host ? printer.host : '';
            this.formData.serialNumber = printer.serialNumber ? printer.serialNumber : '';
            this.formData.accessCode = printer.accessCode ? printer.accessCode : '';
            this.formData.webUrl = printer.webUrl ? printer.webUrl : '';
            this.formData.password = printer.password ? printer.password : '';

            let extraInfo = {};
            if (printer.extraInfo) {
                if (typeof printer.extraInfo === 'string') {
                    try {
                        extraInfo = JSON.parse(printer.extraInfo);
                    } catch (error) {
                        extraInfo = {};
                    }
                } else {
                    extraInfo = printer.extraInfo;
                }
            }
            this.formData.caFile = extraInfo.caFile ? extraInfo.caFile : '';
            this.formData.ignoreCertRevocation = extraInfo.ignoreCertRevocation ? extraInfo.ignoreCertRevocation : false;
        },

        getFormData() {
            return new Promise((resolve) => {
                this.$refs.manualForm.validate((valid) => {
                    if (valid) {
                        const printerName = this.formData.printerName.trim() ? this.formData.printerName.trim() : this.formData.printerModel;
                        const host = this.formData.host ? this.formData.host.trim() : '';
                        const webUrl = this.formData.webUrl ? this.formData.webUrl.trim() : '';
                        const serialNumber = this.formData.serialNumber ? this.formData.serialNumber.trim() : '';
                        const accessCode = this.formData.accessCode ? this.formData.accessCode.trim() : '';
                        const password = this.formData.password ? this.formData.password.trim() : '';
                        const extraInfo = {
                            caFile: this.formData.caFile,
                            ignoreCertRevocation: this.formData.ignoreCertRevocation
                        };
                        const printer = {
                            printerName,
                            host,
                            vendor: this.formData.vendor,
                            printerModel: this.formData.printerModel,
                            hostType: this.formData.hostType,
                            webUrl,
                            serialNumber,
                            accessCode,
                            password,
                            extraInfo: JSON.stringify(extraInfo)
                        };
                        resolve(printer);
                    } else {
                        resolve(null);
                    }
                });
            });
        },

        // add form reset method
        resetForm() {
            this.$refs.manualForm.resetFields();
            this.showAdvanced = false;
            this.isVendorDisabled = false;
            this.isModelDisabled = false;
            this.formData.serialNumber = '';
            this.formData.accessCode = '';
            this.formData.webUrl = '';
            this.formData.password = '';
            this.formData.caFile = '';
            this.formData.ignoreCertRevocation = false;
        },

        // add form validation method
        validateForm() {
            return new Promise((resolve) => {
                this.$refs.manualForm.validate((valid) => {
                    resolve(valid);
                });
            });
        }
    }
};
