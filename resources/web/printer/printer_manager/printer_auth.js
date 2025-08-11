let printer = null;

window.addEventListener('message', function(event) {
    if (event.data.command === 'init') {
        printer = event.data.printer;
        renderPrinterAuth(printer);
    }
});

function renderPrinterAuth(printer) {
    const authModel = printer.authMode;
    if (authModel == 'token') {
        // access code
        $('#printer-auth-header-title').text('Connect to Printer');
    } else if (authModel == 'basic') {
        // username and password
    } else if (authModel == 'pin') {
        // pin
        $('#printer-auth-header-title').text('Bind Printer');
    } else {
        // no authorization
        $('#printer-auth-header-title').text('Connect to Printer');
    }

    $('#printer-auth-printer-image').attr('src', printer.printerImg);
    $('#printer-auth-printer-name').text(printer.printerName);
    
    // show dialog content
    $('.printer-auth-dialog').show();
    
    // set input event
    setupCodeInputs();
}

function closeModal() {
    window.parent.postMessage({command: 'close_printer_auth'}, '*');
}

function connectPrinter() {
    // get all input values and combine
    let accessCode = '';
    $('.code-input').each(function() {
        accessCode += $(this).val() || '';
    });
    
    if (accessCode.length !== 6) {
        alert('Please enter the complete 6-digit access code');
        return;
    }
    
    printer.extraInfo = {
        token: accessCode
    };

    window.parent.postMessage({command: 'add_printer', printer: printer}, '*');
}

function setupCodeInputs() {
    $('.code-input').on('input', function() {
        const value = $(this).val();
        const index = parseInt($(this).data('index'));
        
        // only allow number
        if (!/^\d*$/.test(value)) {
            $(this).val('');
            return;
        }
        
        // if input a number, auto focus next input
        if (value.length === 1 && index < 5) {
            $('.code-input[data-index="' + (index + 1) + '"]').focus();
        }
    });
    
    $('.code-input').on('keydown', function(e) {
        const index = parseInt($(this).data('index'));
        
        // handle backspace
        if (e.key === 'Backspace' && $(this).val() === '' && index > 0) {
            $('.code-input[data-index="' + (index - 1) + '"]').focus();
        }
    });
    
    // set first input to focus
    $('.code-input[data-index="0"]').focus();
}

function showHelp() {
    alert('Please check the printer screen or manual for the 6-digit access code.');
}

