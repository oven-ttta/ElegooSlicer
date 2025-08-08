let printer = null;

function validatePrinterName(name) {
    // Only restrict backslash and forward slash, allow other characters
    const invalidCharsRegex = /[\\\/]/;
    return !invalidCharsRegex.test(name) && name.length <= 50;
}

function validateIPAddress(ip) {
    // Basic IP address validation: numbers and dots only
    const ipRegex = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!ipRegex.test(ip)) {
        return false;
    }
    // Check each octet is between 0-255
    const octets = ip.split('.');
    return octets.every(octet => {
        const num = parseInt(octet, 10);
        return num >= 0 && num <= 255;
    });
}

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
    
    // Real-time input restriction for printer name
    $input.on('input', function() {
        var value = $(this).val();
        // Remove backslash and forward slash in real-time
        var cleanValue = value.replace(/[\\\/]/g, '');
        // Limit to 50 characters
        if (cleanValue.length > 50) {
            cleanValue = cleanValue.substring(0, 50);
        }
        if (value !== cleanValue) {
            $(this).val(cleanValue);
        }
    });
    
    function saveChanges() {
        var newName = $input.val().trim();
        if (newName && newName !== currentName) {
            if (!validatePrinterName(newName)) {
                alert('Printer name cannot contain backslash (\\) or forward slash (/) and must be 50 characters or less.');
                $input.focus().select();
                return;
            }
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
    
    // Real-time input restriction for IP address
    $input.on('input', function() {
        var value = $(this).val();
        // Only allow numbers and dots
        var cleanValue = value.replace(/[^\d\.]/g, '');
        // Prevent multiple consecutive dots
        cleanValue = cleanValue.replace(/\.+/g, '.');
        // Prevent dot at the beginning
        cleanValue = cleanValue.replace(/^\./, '');
        // Prevent dot at the end
        cleanValue = cleanValue.replace(/\.$/, '');
        // Limit to 4 octets (3 dots max)
        var dots = (cleanValue.match(/\./g) || []).length;
        if (dots > 3) {
            var parts = cleanValue.split('.');
            cleanValue = parts.slice(0, 4).join('.');
        }
        // Limit each octet to 3 digits max
        var parts = cleanValue.split('.');
        parts = parts.map(function(part) {
            return part.length > 3 ? part.substring(0, 3) : part;
        });
        cleanValue = parts.join('.');
        
        if (value !== cleanValue) {
            $(this).val(cleanValue);
        }
    });
    
    function saveChanges() {
        var newHost = $input.val().trim();
        if (newHost && newHost !== currentHost) {
            if (!validateIPAddress(newHost)) {
                alert('Please enter a valid IP address (e.g., 192.168.1.100)');
                $input.focus().select();
                return;
            }
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
