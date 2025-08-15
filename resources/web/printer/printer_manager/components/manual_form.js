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
                        <label class="required">Printer Model</label>
                        <div class="form-group-advanced">
                            <span>Advanced</span>
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
                                    :title="isVendorDisabled ? 'Vendor cannot be modified for existing printers' : ''"
                                    placeholder="Select vendor"
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
                                    :title="isModelDisabled ? 'Model cannot be modified for existing printers' : ''"
                                    placeholder="Select model"
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
                    <label class="required">Printer Name</label>
                    <el-form-item prop="printerName">
                        <el-input type="text" v-model="formData.printerName"  required/>
                    </el-form-item>
                </div>
                
                <div class="form-group">
                    <label class="required">Host Type</label>
                    <el-form-item prop="hostType">
                        <el-select v-model="formData.hostType" placeholder="Select host type" :show-arrow="false">
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
                    <label class="required">Host Name, IP or URL</label>
                    <el-form-item prop="host">
                        <el-input type="text" v-model="formData.host" required/>
                    </el-form-item>
                </div>
                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label>Device UI</label>
                    <el-input type="text" v-model="formData.deviceUi"/>
                </div>
                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label>API/Password</label>
                    <el-input type="password" v-model="formData.apiKey"/>
                </div>
                
                <div class="form-group advanced-field" v-show="showAdvanced">
                    <label>HTTPS CA File</label>
                    <div class="input-group">
                        <el-input type="text" v-model="formData.caFile" readonly/>
                        <button type="button" class="btn-secondary">Preview</button>
                    </div>
                    <label>HTTPS CA file is optional. It is only needed if you use HTTPS with a self-signed certificate.</label>
                </div>
            </div>
        </el-form>
    `;


// Manual Form Component (可以被其他应用导入)
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
                deviceUi: '',
                apiKey: '',
                caFile: ''
            },
            formRules: {
                vendor: [
                    { required: true, message: 'Please select a vendor', trigger: 'change' }
                ],
                printerModel: [
                    { required: true, message: 'Please select a printer model', trigger: 'change' }
                ],
                printerName: [
                    { required: true, message: 'Please enter printer name', trigger: 'change' },
                    { min: 1, max: 50, message: 'Length should be 1 to 50 characters', trigger: 'change' }
                ],
                hostType: [
                    { required: true, message: 'Please select host type', trigger: 'change' }
                ],
                host: [
                    { required: true, message: 'Please enter host name, IP or URL', trigger: 'blur' },
                    { 
                        pattern: /^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$|^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$|^https?:\/\/.+$/,
                        message: 'Please enter a valid hostname, IP address or URL',
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
            
            // 提取厂商列表
            this.vendors = printerModelList.map(vendorModels => vendorModels.vendor);
            
            // 提取所有模型
            this.allModels = [];
            printerModelList.forEach(vendorModels => {
                this.allModels.push(...vendorModels.models);
            });
            
            // 提取唯一的主机类型
            const uniqueHostTypes = new Set();
            this.allModels.forEach(model => {
                uniqueHostTypes.add(model.hostType);
            });
            this.hostTypes = Array.from(uniqueHostTypes);
            
            // 如果有厂商，默认选择第一个
            if (this.vendors.length > 0 && !this.formData.vendor) {
                this.formData.vendor = this.vendors[0];
                this.onVendorChange();
            }
            
            // 如果有打印机数据要填充，在模型列表渲染后执行
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
                    
                    if (!this.formData.printerName) {
                        this.formData.printerName = model.modelName;
                    }
                }
            }
        },
        
        toggleAdvanced() {
            // showAdvanced 已经通过 v-model 自动更新
        },
        
        renderPrinter(printer) {
            if (!printer) return;
            
            // 处理厂商和模型选择
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
            
            // 如果是现有打印机，禁用厂商和模型选择
            if (printer.vendor || printer.printerModel) {
                this.isVendorDisabled = true;
                this.isModelDisabled = true;
            }
            
            // 填充其他字段
            if (printer.printerName) {
                this.formData.printerName = printer.printerName;
            }
            
            if (printer.host) {
                this.formData.host = printer.host;
            }
            
            if (printer.deviceUi) {
                this.formData.deviceUi = printer.deviceUi;
            }
            
            if (printer.apiKey) {
                this.formData.apiKey = printer.apiKey;
            }
            
            if (printer.caFile) {
                this.formData.caFile = printer.caFile;
            }
        },
        
        getFormData() {
            return new Promise((resolve) => {
                this.$refs.manualForm.validate((valid) => {
                    if (valid) {
                        const printer = {
                            printerName: this.formData.printerName,
                            host: this.formData.host,
                            apiKey: this.formData.apiKey,
                            caFile: this.formData.caFile,
                            deviceUi: this.formData.deviceUi,
                            vendor: this.formData.vendor,
                            printerModel: this.formData.printerModel,
                            hostType: this.formData.hostType
                        };
                        resolve(printer);
                    } else {
                        resolve(null);
                    }
                });
            });
        },

        // 添加表单重置方法
        resetForm() {
            this.$refs.manualForm.resetFields();
        },

        // 添加表单验证方法
        validateForm() {
            return new Promise((resolve) => {
                this.$refs.manualForm.validate((valid) => {
                    resolve(valid);
                });
            });
        }
    }
};
