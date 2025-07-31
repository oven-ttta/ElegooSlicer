var printer = null;

function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}

function deleteDevice() {
    if (!confirm('Are you sure you want to delete this device? This action cannot be undone.')) {
        return;
    }
    window.parent.postMessage({command: 'delete_printer', id: printer.id}, '*');
}

window.addEventListener('message', function(event) {
    if (event.data && event.data.command === 'printer_settings') {
        printer = event.data.data;
        showPrinterInfo(event.data.data);
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
            window.parent.postMessage({command: 'update_printer_name', name: newName, id: printer.id}, '*');
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

function editPrinterIP() {
    var $textElement = $('#printer-setting-ip .printer-setting-info-text');
    var currentIP = $textElement.text();
    
    var $input = $('<input type="text" class="printer-setting-info-input" value="' + currentIP + '">');
    $textElement.hide();
    $textElement.after($input);
    $input.focus().select();
    
    function saveChanges() {
        var newIP = $input.val().trim();
        if (newIP && newIP !== currentIP) {
            window.parent.postMessage({ 
                command: "update_printer_ip", 
                ip: newIP
            }, '*');
            $textElement.text(newIP);
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

function showPrinterInfo(printer) {
    if (printer.name) {
        $('#printer-setting-name .printer-setting-info-text').text(printer.name);
    }
    if (printer.ip) {
        $('#printer-setting-ip .printer-setting-info-text').text(printer.ip);
    }
    if (printer.machineModel) {
        $('.printer-setting-info-item:eq(1) .printer-setting-info-value').text(printer.machineModel);
    }
    if (printer.firmwareVersion) {
        $('.printer-setting-firmware-version').text(printer.firmwareVersion);
    }
    if (printer.printerImg) {
        $('.printer-setting-image').attr('src', printer.printerImg);
    }
    if (printer.firmwareUpdate && printer.firmwareUpdate == 1) {
        $('.printer-setting-update-tag').text('Update Available').addClass('available');
    } else {
        $('.printer-setting-update-tag').text('Latest Version').removeClass('available');
    }
}
