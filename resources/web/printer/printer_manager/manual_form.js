window.addEventListener('message', function(event) {
    if (event.data.command === 'render_printer_model_list') {
        const printerModelList = event.data.printerModelList;
        const printer = event.data.printer;
        renderPrinterModelList(printerModelList, printer);
    } else if (event.data.command === 'add_physical_printer') {
        const printer = getFormData();
        window.parent.postMessage({command: 'add_physical_printer', printer: printer}, '*');
    }
});

function renderPrinterModelList(printerModelList, printer) {

    const vendorSelect = document.getElementById('printer_vendor_select');
    const modelSelect = document.getElementById('printer_model_select');
    const hostTypeSelect = document.getElementById('host_type_select');

    vendorSelect.innerHTML = '';
    modelSelect.innerHTML = '';
    hostTypeSelect.innerHTML = '';

    printerModelList.forEach(vendorModels => {
        const option = document.createElement('option');
        option.value = vendorModels.vendor;
        option.textContent = vendorModels.vendor;
        vendorSelect.appendChild(option);
    });


    const uniqueHostTypes = new Set();
    printerModelList.forEach(vendorModels => {
        vendorModels.models.forEach(model => {
            uniqueHostTypes.add(model.hostType);
        });
    });
    
    uniqueHostTypes.forEach(hostType => {
        const option = document.createElement('option');
        option.value = hostType;
        option.textContent = hostType;
        hostTypeSelect.appendChild(option);
    });

    vendorSelect.addEventListener('change', function() {
        const selectedVendor = this.value;
        modelSelect.innerHTML = '';
        
        if (selectedVendor) {
            const vendorData = printerModelList.find(vendor => vendor.vendor === selectedVendor);
            if (vendorData) {
                vendorData.models.forEach(model => {
                    const option = document.createElement('option');
                    option.value = model.modelName;
                    option.textContent = model.modelName;
                    option.dataset.hostType = model.hostType;
                    modelSelect.appendChild(option);
                });

                if (modelSelect.options.length > 0) {
                    modelSelect.selectedIndex = 0;
                    modelSelect.dispatchEvent(new Event('change'));
                }
            }
        }
    });
    
    modelSelect.addEventListener('change', function() {
        const selectedModel = this.value;
        if (selectedModel) {
            const selectedOption = this.options[this.selectedIndex];
            const hostType = selectedOption.dataset.hostType;
            if (hostType) {
                hostTypeSelect.value = hostType;
            }

            const printerName = selectedOption.textContent;
            const printerNameInput = document.querySelector('input[name="printer_name"]');
            if (printerNameInput) {
                printerNameInput.value = printerName;
            }
        }
    });
    
    if (vendorSelect.options.length > 0) {
        vendorSelect.selectedIndex = 0;
        vendorSelect.dispatchEvent(new Event('change'));
    }
    
    // If there's printer data to fill, do it after model list is rendered
    if (printer) {
        renderPrinter(printer);
    }
}

function toggleAdvanced() {
    const advancedSwitch = document.getElementById('advanced-switch');
    const advancedFields = document.querySelectorAll('.advanced-field');
    
    if (advancedSwitch && advancedFields.length > 0) {
        const isChecked = advancedSwitch.checked;
        advancedFields.forEach(field => {
            field.style.display = isChecked ? 'block' : 'none';
        });
    }
}

function testPrinterConnection() {

}
function renderPrinter(printer) {
    const form = document.querySelector('.manual-form');
    if (!form || !printer) return;

    // Fill printer name
    const printerNameInput = form.querySelector('input[name="printer_name"]');
    if (printerNameInput && printer.printerName) {
        printerNameInput.value = printer.printerName;
    }
    
    // Fill printer host
    const printerHostInput = form.querySelector('input[name="printer_host"]');
    if (printerHostInput) {
        if (printer.host) {
            printerHostInput.value = printer.host;
        }
    }
    
    const deviceUiInput = form.querySelector('input[name="device_ui"]');
    if (deviceUiInput && printer.deviceUi) {
        deviceUiInput.value = printer.deviceUi;
    }
    
    const apiKeyInput = form.querySelector('input[name="api_key"]');
    if (apiKeyInput && printer.apiKey) {
        apiKeyInput.value = printer.apiKey;
    }
    
    const caFileInput = form.querySelector('input[name="ca_file"]');
    if (caFileInput && printer.caFile) {
        caFileInput.value = printer.caFile;
    }
    
    // Handle vendor and model selection
    const vendorSelect = document.getElementById('printer_vendor_select');
    const modelSelect = document.getElementById('printer_model_select');
    const hostTypeSelect = document.getElementById('host_type_select');
    
    if (vendorSelect && modelSelect && hostTypeSelect) {
        if (printer.vendor && vendorSelect.value !== printer.vendor) {
            vendorSelect.value = printer.vendor;
            const changeEvent = new Event('change');
            vendorSelect.dispatchEvent(changeEvent);
        }
        
        if (printer.printerModel && modelSelect.value !== printer.printerModel) {
            modelSelect.value = printer.printerModel;
            const changeEvent = new Event('change');
            modelSelect.dispatchEvent(changeEvent);
        }
        
        if (printer.hostType && hostTypeSelect.value !== printer.hostType) {
            hostTypeSelect.value = printer.hostType;
        }
        
        if (printer.vendor || printer.printerModel) {
            if (vendorSelect) {
                vendorSelect.disabled = true;
                vendorSelect.title = "Vendor cannot be modified for existing printers";
            }
            
            if (modelSelect) {
                modelSelect.disabled = true;
                modelSelect.title = "Model cannot be modified for existing printers";
            }
            
            const vendorLabel = vendorSelect.parentElement.querySelector('label');
            const modelLabel = modelSelect.parentElement.querySelector('label');
            
            if (vendorLabel) vendorLabel.classList.add('disabled');
            if (modelLabel) modelLabel.classList.add('disabled');
        }
    }
}
function getFormData() {
    const form = document.querySelector('.manual-form');
    if (!form) {
        console.error('Manual form not found');
        return null;
    }

    const printer = {};
    
    try {
        printer.printerName = form.querySelector('input[name="printer_name"]').value;
        printer.host = form.querySelector('input[name="printer_host"]').value;
        printer.apiKey = form.querySelector('input[name="api_key"]').value;
        printer.caFile = form.querySelector('input[name="ca_file"]').value;
        printer.deviceUi = form.querySelector('input[name="device_ui"]').value;
        
        const vendorSelect = document.getElementById('printer_vendor_select');
        const modelSelect = document.getElementById('printer_model_select');
        const hostTypeSelect = document.getElementById('host_type_select');
        
        if (vendorSelect && vendorSelect.value) {
            printer.vendor = vendorSelect.value;
        }
        if (modelSelect && modelSelect.value) {
            printer.printerModel = modelSelect.value;
        }
        if (hostTypeSelect && hostTypeSelect.value) {
            printer.hostType = hostTypeSelect.value;
        }
    } catch (error) {
        alert('getFormData error: ' + error);
        return null;
    }
    
    if (!printer.vendor || !printer.printerModel || !printer.host || !printer.printerName) {
        alert('Missing required fields: vendor, model, host, and name');
        return null;
    }       
    return printer;
}

 