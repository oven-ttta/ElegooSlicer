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
    
    // 显示对话框内容
    $('.printer-auth-dialog').show();
    
    // 设置输入框事件
    setupCodeInputs();
}

function closeModal() {
    window.parent.postMessage({command: 'close_printer_auth'}, '*');
}

function connectPrinter() {
    // 获取所有输入框的值并组合
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
        
        // 只允许数字
        if (!/^\d*$/.test(value)) {
            $(this).val('');
            return;
        }
        
        // 如果输入了数字，自动跳转到下一个输入框
        if (value.length === 1 && index < 5) {
            $('.code-input[data-index="' + (index + 1) + '"]').focus();
        }
    });
    
    $('.code-input').on('keydown', function(e) {
        const index = parseInt($(this).data('index'));
        
        // 处理退格键
        if (e.key === 'Backspace' && $(this).val() === '' && index > 0) {
            $('.code-input[data-index="' + (index - 1) + '"]').focus();
        }
    });
    
    // 设置第一个输入框为焦点
    $('.code-input[data-index="0"]').focus();
}

function showHelp() {
    alert('Please check the printer screen or manual for the 6-digit access code.');
}

