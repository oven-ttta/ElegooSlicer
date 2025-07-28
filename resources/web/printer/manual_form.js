// Form logic functions for manual form

// Listen for messages from parent
window.addEventListener('message', function(event) {
    if (event.data.command === 'get_form_data') {
        const formData = getManualFormData();
        window.parent.postMessage({
            command: 'form_data',
            data: formData
        }, '*');
    }
});

function toggleAdvanced() {
    const advancedSwitch = document.getElementById('advanced-switch');
    const advancedFields = document.querySelectorAll('.advanced-field');
    
    if (advancedSwitch && advancedFields.length > 0) {
        const isChecked = advancedSwitch.checked;
        
        advancedFields.forEach(field => {
            if (isChecked) {
                field.style.display = 'block';
            } else {
                field.style.display = 'none';
            }
        });
    }
}

function testPrinterConnection() {
    const formData = getManualFormData();
    if (formData && formData.printer_ip) {
        // Send test connection request to parent
        window.parent.postMessage({
            command: 'test_printer_connection',
            data: formData
        }, '*');
    }
}

function getManualFormData() {
    const form = document.getElementById('manual-form');
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