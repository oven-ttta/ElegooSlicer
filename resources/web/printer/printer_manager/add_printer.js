let printers = null;

function closeModal() {
    window.parent.postMessage({command: 'closeModal'}, '*');
}

window.addEventListener('message', function(event) {
    if (event.data.command === 'init') {
        switchTab('discover'); 
        const iframe = document.querySelector('#add-printer-manual-form iframe');
        if (iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({
                command: 'render_printer_model_list',
                printerModelList: event.data.printerModelList,
                printer: event.data.printer
            }, '*');
        }
    } else if (event.data.command === 'discover_printers') {
        printers = event.data.printers;
        renderDiscoverList(printers);
    } else if (event.data.command === 'add_physical_printer') {
        window.parent.postMessage({command: 'add_physical_printer', printer: event.data.printer}, '*');
    }
});

function switchTab(tab) {
    if (tab === 'discover') {
        $('#discover-panel').show();
        $('#add-printer-manual-form').hide();
        $('#tab-discover').addClass('active');
        $('#tab-manual').removeClass('active');
        requestDiscoverPrinters();
    } else {
        $('#discover-panel').hide();
        $('#add-printer-manual-form').show();
        $('#tab-discover').removeClass('active');
        $('#tab-manual').addClass('active');
    }
}

function connectPrinter() {
    if ($('#tab-discover').hasClass('active')) {
        if (window.selectedPrinterIdx == null) {
            alert('Please select a printer');
            return;
        }
        window.parent.postMessage({ command: 'add_printer', printer: printers[window.selectedPrinterIdx] }, '*');
    } else {
        const iframe = document.querySelector('#add-printer-manual-form iframe');
        if (iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({
                command: 'add_physical_printer'
            }, '*');
        }
    }
}

function requestDiscoverPrinters() {
    $('#discover-list-table').hide();
    $('#discover-loading').show();
    window.parent.postMessage({command: 'discover_printers'}, '*');  
}

function hideDiscoverLoading() {
    $('#discover-loading').hide();
    $('#discover-list-table').show();
}

function renderDiscoverList(printers) {
    let html = "";
    printers.forEach(function(p, idx) {
        html += `
        <tr onclick="selectPrinterRow(this, ${idx})">
            <td><img src="${p.printerImg}" class="add-printer-img"/></td>
            <td><span class="add-printer-name">${p.printerName}</span><br><span class="add-printer-host">${p.host}</span></td>
            <td><span class="add-printer-status">Connectable</span></td>
        </tr>`;
    });
    $('#discover-list-table').html(html);
    hideDiscoverLoading();
}

function selectPrinterRow(row, idx) {
    $('#discover-list-table tr').removeClass('selected');
    $(row).addClass('selected');
    window.selectedPrinterIdx = idx;
}