var printers = null;

function OnInit() {
    switchTab('discover');
}

function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}
window.addEventListener('message', function(event) {
    if (event.data.command === 'init') {
        OnInit();
    } else if (event.data.command === 'discover_printers') {
        printers = event.data.data;
        renderDiscoverList(printers);
    }
});

function switchTab(tab) {
    if (tab === 'discover') {
        $('#discover-panel').show();
        $('#manual-panel').hide();
        $('#tab-discover').addClass('active');
        $('#tab-manual').removeClass('active');
        $('#discover-tip').show();
        $('#manual-tip').hide();
        requestDiscoverPrinters(); 
    } else {
        $('#discover-panel').hide();
        $('#manual-panel').show();
        $('#tab-discover').removeClass('active');
        $('#tab-manual').addClass('active');
        $('#discover-tip').hide();
        $('#manual-tip').show();
        renderManualForm('manual-form-container');
    }
}

function connectPrinter() {
    if(printers == null) {
        return;
    }
    if ($('#tab-discover').hasClass('active')) {
        window.parent.postMessage({command: 'bind_printers', printers: printers}, '*');
    } else {
        var printer;
        printer.ip = $('#manual-form-container input[name="ip"]').val();
        printer.port = $('#manual-form-container input[name="port"]').val();
        printer.vendor = $('#manual-form-container input[name="vendor"]').val();
        printer.machineModel = $('#manual-form-container input[name="machineModel"]').val();
        printer.protocolVersion = $('#manual-form-container input[name="protocolVersion"]').val();
        printer.firmwareVersion = $('#manual-form-container input[name="firmwareVersion"]').val();
        manualAddPrinter();
    }
}

function requestDiscoverPrinters() {
    window.parent.postMessage({command: 'discover_printers'}, '*');  
}

function renderDiscoverList(printers) {
    var html = "";
    printers.forEach(function(p, idx) {
        html += `
        <tr onclick="selectPrinterRow(this, ${idx})">
            <td><img src="${p.printerImg}" class="add-printer-img"/></td>
            <td><span class="add-printer-model">${p.machineModel}</span><br><span class="add-printer-ip">${p.ip}</span></td>
            <td><span class="add-printer-status">${'Connectable'}</span></td>
        </tr>`;
    });
    $('#discover-list-table').html(html);
}

function selectPrinterRow(row, idx) {
    $('#discover-list-table tr').removeClass('selected');
    $(row).addClass('selected');
    window.selectedPrinterIdx = idx;
}

function bindSelectedPrinter() {
    if (window.selectedPrinterIdx == null) {
        return;
    }
    var tSend = { command: "bind_printer", idx: window.selectedPrinterIdx };
    SendWXMessage(JSON.stringify(tSend));
}

function manualAddPrinter() {
    var data = {};
    $('#manual-form').serializeArray().forEach(function(item) {
        data[item.name] = item.value;
    });
    data.command = "manual_add_printer";
    SendWXMessage(JSON.stringify(data));
}

function testPrinterConnection() {
    // Implement connection test logic if needed
}