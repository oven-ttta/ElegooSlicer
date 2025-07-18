function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}
function deletePrinter() {
    if (confirm('Are you sure you want to delete this printer?')) {
        var tSend = { command: "delete_printer", sequence_id: Date.now() };
        SendWXMessage(JSON.stringify(tSend));
    }
}

function connectPrinter() {
    var formData = getManualFormData();
    if (formData) {
        formData.command = "connect_printer";
        formData.sequence_id = Date.now();
        SendWXMessage(JSON.stringify(formData));
    }
}

window.addEventListener('message', function(event) {
    if (event.data && event.data.command === 'printer_settings') {
        alert(JSON.stringify(event.data.data));
    }
});


$(document).ready(function() {
    renderManualForm('manual-form-container');
}); 