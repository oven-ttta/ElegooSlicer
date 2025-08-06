function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}

function deletePrinter() {
    if (confirm('Are you sure you want to delete this printer?')) {
        window.parent.postMessage({command: 'delete_printer'}, '*');
    }
}

function confirmPrinter() {

}

window.addEventListener('message', function(event) {
    if (event.data.command === 'add_printer') {
        window.parent.postMessage({command: 'add_printer', printer: event.data.data}, '*');
    }
});

window.addEventListener('message', function(event) {
    if (event.data.command === 'render_printer_model_list') {
        const iframe = document.querySelector('#printer-setting-physical-manual-form iframe');
        if (iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({
                command: 'render_printer_model_list',
                data: event.data.data
            }, '*');
        }
    }
}); 