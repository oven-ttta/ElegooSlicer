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
    const iframe = document.querySelector('iframe');
    if (iframe && iframe.contentWindow) {
        iframe.contentWindow.postMessage({command: 'get_form_data'}, '*');
    }
}

window.addEventListener('message', function(event) {
    if (event.data.command === 'form_data') {
        // Forward form data from iframe to parent with command
        const formData = event.data.data;
        formData.command = "connect_physical_printer";
        window.parent.postMessage(formData, '*');
    }
}); 