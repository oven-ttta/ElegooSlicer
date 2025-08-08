let printer = null;

function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}

function deleteDevice() {
    if (!confirm('Are you sure you want to delete this device? This action cannot be undone.')) {
        return;
    }
    window.parent.postMessage({command: 'delete_printer', printerId: printer.printerId}, '*');
}

window.addEventListener('message', function(event) {
    if (event.data && event.data.command === 'printer_settings') {
        printer = event.data.printer;
        showPrinterInfo();
    }
});

function editPrinterName() {
    var $textElement = $('#printer-setting-name .printer-setting-info-text');
    var currentName = $textElement.text();
    
    var $input = $('<input type="text" class="printer-setting-info-input" value="' + currentName + '">');
    $textElement.hide();
    $textElement.after($input);
    $input.focus().select();
    
    function saveChanges() {
        var newName = $input.val().trim();
        if (newName && newName !== currentName) {
            window.parent.postMessage({command: 'update_printer_name', printerName: newName, printerId: printer.printerId}, '*');
            $textElement.text(newName);
        }
        $input.remove();
        $textElement.show().css('display', 'flex');
    }
    
    function cancelChanges() {
        $input.remove();
        $textElement.show().css('display', 'flex');
    }
    
    $input.on('keydown', function(e) {
        if (e.keyCode === 13) { 
            e.preventDefault();
            saveChanges();
        } else if (e.keyCode === 27) { 
            e.preventDefault();
            cancelChanges();
        }
    });
    
    $input.on('blur', function(e) {
        setTimeout(function() {
            if ($input.is(':visible')) {
                saveChanges();
            }
        }, 100);
    });
}

function editPrinterHost() {
    var $textElement = $('#printer-setting-host .printer-setting-info-text');
    var currentHost = $textElement.text();
    
    var $input = $('<input type="text" class="printer-setting-info-input" value="' + currentHost + '">');
    $textElement.hide();
    $textElement.after($input);
    $input.focus().select();
    
    function saveChanges() {
        var newHost = $input.val().trim();
        if (newHost && newHost !== currentHost) {
            window.parent.postMessage({ 
                command: "update_printer_host", 
                host: newHost,
                printerId: printer.printerId
            }, '*');
            $textElement.text(newHost);
        }
        $input.remove();
        $textElement.show().css('display', 'flex');
    }
    
    function cancelChanges() {
        $input.remove();
        $textElement.show().css('display', 'flex');
    }
    
    $input.on('keydown', function(e) {
        if (e.keyCode === 13) {
            e.preventDefault();
            saveChanges();
        } else if (e.keyCode === 27) {
            e.preventDefault();
            cancelChanges();
        }
    });
    
    $input.on('blur', function(e) {
        setTimeout(function() {
            if ($input.is(':visible')) {
                saveChanges();
            }
        }, 100);
    });
}

function checkFirmwareUpdate() {
    window.parent.postMessage({ command: "check_firmware_update" }, '*');
}

function showPrinterInfo() {
    if (printer.printerName) {
        $('#printer-setting-name .printer-setting-info-text').text(printer.printerName);
    }
    if (printer.host) {
        $('#printer-setting-host .printer-setting-info-text').text(printer.host);
    }
    if (printer.printerModel) {
        $('.printer-setting-info-item:eq(1) .printer-setting-info-value').text(printer.printerModel);
    }
    if (printer.firmwareVersion) {
        $('.printer-setting-firmware-version').text(printer.firmwareVersion);
    }
    if (printer.printerImg) {
        $('.printer-setting-image').attr('src', printer.printerImg);
    }
    if (printer.firmwareUpdate && printer.firmwareUpdate == 1) {
        $('.printer-setting-update-tag').text('Update Available').addClass('available');
        $('.printer-setting-refresh-btn').show();
    } else {
        $('.printer-setting-update-tag').text('Latest Version').removeClass('available');
        $('.printer-setting-refresh-btn').hide();
    }
}
