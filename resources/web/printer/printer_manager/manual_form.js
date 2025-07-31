// Form logic functions for manual form

// Listen for messages from parent
window.addEventListener('message', function(event) {
    if (event.data.command === 'get_form_data') {
        const formData = getManualFormData();
        window.parent.postMessage({
            command: 'form_data',
            data: formData
        }, '*');
    } else if (event.data.command === 'render_printer_model_list') {
        renderPrinterModelList(event.data.data);
    }
});

function renderPrinterModelList(printerModelList) {
    alert(printerModelList.length);
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
            uniqueHostTypes.add(model.host_type);
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
            const vendorData = printerModelList.find(v => v.vendor === selectedVendor);
            if (vendorData) {
                vendorData.models.forEach(model => {
                    const option = document.createElement('option');
                    option.value = model.model_name;
                    option.textContent = model.model_name;
                    option.dataset.hostType = model.host_type;
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
        }
    });
    
    if (vendorSelect.options.length > 0) {
        vendorSelect.selectedIndex = 0;
        vendorSelect.dispatchEvent(new Event('change'));
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
    const formData = getManualFormData();
    if (formData && formData.printer_ip) {
        window.parent.postMessage({
            command: 'test_printer_connection',
            data: formData
        }, '*');
    }
}

function getManualFormData() {
    const form = document.querySelector('.manual-form');
    if (!form) return null;
    
    const formData = {};
    const formElements = form.elements;
    
    for (let i = 0; i < formElements.length; i++) {
        const element = formElements[i];
        if (element.name) {
            formData[element.name] = element.value;
        }
    }
    
    return formData;
} 