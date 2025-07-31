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
    } else if (event.data.command === 'form_data') {
        const formData = event.data.data;
        formData.command = "manual_add_printer";
        window.parent.postMessage(formData, '*');
    } else if (event.data.command === 'response_printer_model_list') {
        printerModelList = event.data.data;
        const iframe = document.querySelector('#add-printer-manual-form iframe');
        if (iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({
                command: 'render_printer_model_list',
                data: printerModelList
            }, '*');
        }
    } 
});



function getSelectedPrinterInfo() {
    const vendorSelect = document.getElementById('printer_vendor_select');
    const modelSelect = document.getElementById('printer_model_select');
    const hostTypeSelect = document.getElementById('host_type_select');
    
    return {
        vendor: vendorSelect.value,
        model: modelSelect.value,
        hostType: hostTypeSelect.value
    };
}

function resetPrinterSelection() {
    const vendorSelect = document.getElementById('printer_vendor_select');
    const modelSelect = document.getElementById('printer_model_select');
    const hostTypeSelect = document.getElementById('host_type_select');
    
    vendorSelect.value = '';
    modelSelect.value = '';
    hostTypeSelect.value = '';
    
    // Trigger change event to clear linkage
    vendorSelect.dispatchEvent(new Event('change'));
}

function validatePrinterSelection() {
    const selectedInfo = getSelectedPrinterInfo();
    return selectedInfo.vendor && selectedInfo.model && selectedInfo.hostType;
}

function getPrinterModelData(vendor, model) {
    if (!window.printerModelData) return null;
    
    const vendorData = window.printerModelData.find(v => v.vendor === vendor);
    if (!vendorData) return null;
    
    return vendorData.models.find(m => m.model_name === model);
}

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
        window.parent.postMessage({command: 'bind_printer', printer: printers[window.selectedPrinterIdx]}, '*');
    } else {
        // Get form data from iframe
        const iframe = document.querySelector('#add-printer-manual-form iframe');
        if (iframe && iframe.contentWindow) {
            iframe.contentWindow.postMessage({command: 'get_form_data'}, '*');
        }
    }
}

function requestDiscoverPrinters() {
    // Show loading state
    showDiscoverLoading();
    window.parent.postMessage({command: 'discover_printers'}, '*');  
}

function showDiscoverLoading() {
    // Hide the tbody and show loading
    $('#discover-list-table').hide();
    $('#discover-loading').show();
}

function hideDiscoverLoading() {
    // Hide loading and show tbody
    $('#discover-loading').hide();
    $('#discover-list-table').show();
}

function renderDiscoverList(printers) {
    var html = "";
    printers.forEach(function(p, idx) {
        html += `
        <tr onclick="selectPrinterRow(this, ${idx})">
            <td><img src="${p.printerImg}" class="add-printer-img"/></td>
            <td><span class="add-printer-model">${p.machineModel}</span><br><span class="add-printer-ip">${p.ip}</span></td>
            <td><span class="add-printer-status">Connectable</span></td>
        </tr>`;
    });
    $('#discover-list-table').html(html);
    // Hide loading after rendering
    hideDiscoverLoading();
}

function selectPrinterRow(row, idx) {
    $('#discover-list-table tr').removeClass('selected');
    $(row).addClass('selected');
    window.selectedPrinterIdx = idx;
}