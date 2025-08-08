let printer = null;

function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}

function deletePrinter() {
    if (confirm('Are you sure you want to delete this printer?')) {
        window.parent.postMessage({command: 'delete_printer', printerId: printer.printerId}, '*');
    }
}

window.addEventListener('message', function(event) {
    if (event.data.command === 'update_physical_printer') {;
        const newPrinter = event.data.printer;
        if(newPrinter.printerName != printer.printerName) {
            window.parent.postMessage({command: 'update_printer_name', printerName: newPrinter.printerName, printerId: printer.printerId}, '*');
        }
        if(newPrinter.host != printer.host) {
            window.parent.postMessage({command: 'update_printer_host', host: newPrinter.host, printerId: printer.printerId}, '*');
        }
    }
});

function confirmPrinter() {
    const iframe = document.querySelector('#printer-setting-physical-manual-form iframe');
        if (iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({
                command: 'update_physical_printer',
            }, '*');
        }
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
            printer = event.data.printer;
            iframe.contentWindow.postMessage({
                command: 'render_printer_model_list',
                printerModelList: event.data.printerModelList,
                printer: printer
            }, '*');
        }
    }
}); 