function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}

function deletePrinter() {
    if (confirm('Are you sure you want to delete this printer?')) {
        window.parent.postMessage({command: 'delete_printer'}, '*');
    }
}

function connectPrinter() {
    // Get form data from iframe
    const iframe = document.querySelector('#printer-setting-physical-manual-form iframe');
    if (iframe && iframe.contentWindow) {
        iframe.contentWindow.postMessage({command: 'get_form_data'}, '*');
    }
}

function requestPrinterModelList() {
    window.parent.postMessage({command: 'request_printer_model_list'}, '*');
}

window.addEventListener('message', function(event) {
    if (event.data.command === 'form_data') {
        const formData = event.data.data;
        formData.command = "connect_physical_printer";
        window.parent.postMessage(formData, '*');
    } else if (event.data.command === 'response_printer_model_list') {
        const iframe = document.querySelector('#printer-setting-physical-manual-form iframe');
        if (iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({
                command: 'render_printer_model_list',
                data: event.data.data
            }, '*');
        }
    }
}); 