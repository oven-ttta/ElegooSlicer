
let globalPrinterList = [];
let globalPrinterModelList = null; // Cache for printer model list

function OnInit() {
	TranslatePage();
    requestPrinterModelList();
    requestPrinterList();
    requestPrinterListStatus();

}
function requestPrinterList() {
    var tSend={};
    tSend['command']="request_printer_list";
    tSend['sequence_id']=Math.round(new Date() / 1000);
    SendWXMessage( JSON.stringify(tSend) );
}
function requestPrinterListStatus() {
    setInterval(function () {
        var tSend = {};
        tSend['command'] = "request_printer_list_status";
        tSend['sequence_id'] = Math.round(new Date() / 1000);
        SendWXMessage(JSON.stringify(tSend));
    }, 1000);
}
function requestPrinterModelList() {
    var tSend={};
    tSend['command']="request_printer_model_list";
    tSend['sequence_id']=Math.round(new Date() / 1000);
    SendWXMessage( JSON.stringify(tSend) );
}

function HandleStudio(pVal)
{
	let strCmd=pVal['command'];
	if(strCmd=="response_printer_list") {
		let printers=pVal['response'];
		renderAllPrinters(printers);
	} else if(strCmd=="response_printer_list_status") {
		let printers=pVal['response'];
		renderPrinterListStatus(printers);
	} else if(strCmd=="response_discover_printers") {
		var add_printer_modal = document.querySelector('.add-printer-modal');
		if (add_printer_modal) {
			var add_printer_iframe = add_printer_modal.querySelector('iframe');
			if (add_printer_iframe && add_printer_iframe.contentWindow) {
				add_printer_iframe.contentWindow.postMessage({
					command: 'discover_printers',
					printers: pVal.response
				}, '*');
			}
		}
	} else if(strCmd=="response_add_printer" || strCmd=="response_update_printer_name" || strCmd=="response_add_physical_printer" || strCmd=="response_update_printer_host") {
        // refresh printer list after add or update
        requestPrinterList();
	} else if(strCmd=="response_delete_printer") {
        // refresh printer list after delete
        requestPrinterList();
        closeModals();
    } else if(strCmd=="response_printer_model_list") {
        let printerModelList=pVal['response'];
        globalPrinterModelList = printerModelList;
    }
}

function getPrinterStatus(printerStatus, connectStatus) {
    // If not connected, always show Offline
    if (connectStatus === 0) {
        return "Offline";
    }
    switch(printerStatus) {
        case -1: return "Offline";
        case 0: return "Idle";
        case 1: return "Printing";
        case 2: return "Paused";
        case 3: return "Pausing";
        case 4: return "Canceled";
        case 5: return "Self-checking";
        case 6: return "Auto-leveling";
        case 7: return "PID calibrating";
        case 8: return "Resonance testing";
        case 9: return "Updating";
        case 10: return "File copying";
        case 11: return "File transferring";
        case 12: return "Homing";
        case 13: return "Preheating";
        case 14: return "Filament operating";
        case 15: return "Extruder operating";
        case 16: return "Print completed";
        case 17: return "RFID recognizing";
        case 999: return "Error";
        case 1000: return "Unknown";
        default: return "";
    }
}

function getRemainingTime(currentTicks, totalTicks, estimatedTime) {
    // If estimatedTime is provided, it already represents remaining seconds
    let remainingTime = (typeof estimatedTime === 'number' && estimatedTime > 0)
        ? estimatedTime
        : Math.max(0, (totalTicks || 0) - (currentTicks || 0));
    let hours = Math.floor(remainingTime / 3600);
    let minutes = Math.floor((remainingTime % 3600) / 60);
    let seconds = remainingTime % 60;
    const pad = (n) => String(n).padStart(2, '0');
    return `${pad(hours)}:${pad(minutes)}:${pad(seconds)}`;    
}


function renderPrinterCards(printers) {
    let html = "";
    printers.forEach((p, index) => {
        html += `
            <div class="printer-card" id="printer-card-${p.printerId}">
            <div class="printer-card-header">
            <div class="printer-img"><img src="${p.printerImg || ''}" alt="Printer" /></div>
            <div class="printer-header-info">
                <div class="printer-title">${p.printerName}</div>
                <div class="printer-host">${p.host || ''}</div>
                <button class="printer-detail-btn" onclick="showPrinterDetail('${p.printerId}')">Details</button>
            </div>
            <div class="card-menu" title="more" onclick="showPrinterSettingsByIndex(${index})">&#8942;</div>
            </div>
            <div class="progress-bar-container">
            <span class="progress-label">${(p.printTask && typeof p.printTask.progress === 'number') ? p.printTask.progress : 0}%</span>
            <div class="progress-bar-bg">
                <div class="progress-bar" style="width:${(p.printTask && typeof p.printTask.progress === 'number') ? p.printTask.progress : 0}%"></div>
            </div>
            </div>
            <div class="printer-status-row">
            <button class="printer-status-btn">${getPrinterStatus(p.printerStatus, p.connectStatus)}</button>
            <span class="remaining-time">${p.printTask ? getRemainingTime(p.printTask.currentTime, p.printTask.totalTime, p.printTask.estimatedTime) : ''}</span>
            </div>
        </div>
        `;
    });
    return html;
}

function renderAllPrinters(printers) {
    globalPrinterList = printers;
    let html = renderPrinterCards(printers);
    $("#printer-list").html(html);
}

function renderPrinterListStatus(printers) {
    // Incremental update: update existing DOM nodes per printer
    (printers || []).forEach((p) => {
        const card = document.getElementById(`printer-card-${p.printerId}`);
        if (!card) return; // skip if card not rendered yet

        // Progress
        const progress = (p.printTask && typeof p.printTask.progress === 'number') ? p.printTask.progress : 0;
        const progressLabel = card.querySelector('.progress-label');
        if (progressLabel) progressLabel.textContent = `${progress}%`;
        const progressBar = card.querySelector('.progress-bar');
        if (progressBar) progressBar.style.width = `${progress}%`;

        // Status text
        const statusBtn = card.querySelector('.printer-status-btn');
        if (statusBtn) statusBtn.textContent = getPrinterStatus(p.printerStatus, p.connectStatus);

        // Remaining time
        const remainingSpan = card.querySelector('.remaining-time');
        if (remainingSpan) {
            if (p.printTask) {
                remainingSpan.textContent = getRemainingTime(
                    p.printTask.currentTime,
                    p.printTask.totalTime,
                    p.printTask.estimatedTime
                );
            } else {
                remainingSpan.textContent = '';
            }
        }

        // Optional: host or name might change
        const hostEl = card.querySelector('.printer-host');
        if (hostEl && typeof p.host === 'string') hostEl.textContent = p.host;
        const titleEl = card.querySelector('.printer-title');
        if (titleEl && typeof p.printerName === 'string') titleEl.textContent = p.printerName;
    });
}

function showPrinterDetail(printerId) {
	var tSend={};
	tSend['command']="request_printer_detail";
    tSend['sequence_id']=Math.round(new Date() / 1000);
	tSend['printerId']=printerId;	
	SendWXMessage( JSON.stringify(tSend) );	
}


document.addEventListener('DOMContentLoaded', function() {
    document.getElementById('add-btn').onclick = function() {
        document.getElementById('modal-overlay').style.display = 'block';     
        var dialog = document.createElement('div');
        dialog.innerHTML = `
            <div class="add-printer-modal">
                <iframe src="add_printer.html" class="add-printer-iframe"></iframe>
            </div>
        `;     
        document.body.appendChild(dialog);     
        var iframe = dialog.querySelector('iframe');
        iframe.onload = function() {
            iframe.contentWindow.postMessage({
                command: 'init',
                printerModelList: globalPrinterModelList,
                printer: null
            }, '*');
        };        
        return dialog;
    };

    window.addEventListener('message', function(event) {
        if (event.data.command == 'discover_printers') {
            var tSend={};
            tSend['command']="request_discover_printers";
            tSend['sequence_id']=Math.round(new Date() / 1000);
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command == 'delete_printer') {
            var tSend={};
            tSend['command']="request_delete_printer";
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['printerId']=event.data.printerId;
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command === 'closeModal') {
            closeModals();
        } else if (event.data.command === 'add_printer') {
            if(event.data.printer.authMode == 'token' && (event.data.printer.extraInfo == '' || event.data.printer.extraInfo.token == null || event.data.printer.extraInfo.token == '')) {
                // if auth mode is token and token is not set, show auth dialog
                showPrinterAuth(event.data.printer);
            } else {
                var tSend={};
                tSend['command']="request_add_printer";
                tSend['sequence_id']=Math.round(new Date() / 1000);
                tSend['printer']=event.data.printer;
                SendWXMessage( JSON.stringify(tSend) );
            }
        } else if (event.data.command === 'add_physical_printer') {
            var tSend={};
            tSend['command']="request_add_physical_printer";
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['printer']=event.data.printer;
            SendWXMessage( JSON.stringify(tSend) );
        }  else if (event.data.command === 'update_printer_name') {
            var tSend={};
            tSend['command']="request_update_printer_name";
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['printerId']=event.data.printerId;
            tSend['printerName']=event.data.printerName;
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command === 'update_printer_host') {
            var tSend={};
            tSend['command']="request_update_printer_host";
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['printerId']=event.data.printerId;
            tSend['host']=event.data.host;
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command === 'close_printer_auth') {
            closePrinterAuth();
        }
    });
});


function showPrinterSettingsByIndex(index) {
    if (index >= 0 && index < globalPrinterList.length) {
        showPrinterSettings(globalPrinterList[index]);
    }
}

function showPrinterSettings(printer) {
    document.getElementById('modal-overlay').style.display = 'block';
    var dialog = document.createElement('div');
    if (printer.isPhysicalPrinter) {
        dialog.innerHTML = `
            <div class="printer-setting-physical-modal">
                <iframe src="printer_setting_physical.html" class="printer-setting-physical-iframe"></iframe>
            </div>
        `;

    } else {
        dialog.innerHTML = `
            <div class="printer-setting-modal">
                <iframe src="printer_setting.html" class="printer-setting-iframe"></iframe>
            </div>
        `;
    }
    
    document.body.appendChild(dialog);
    var iframe = dialog.querySelector('iframe');
    iframe.onload = function() {
        if (printer.isPhysicalPrinter) {
            if(globalPrinterModelList) {
                iframe.contentWindow.postMessage({
                    command: 'render_printer_model_list',
                    printerModelList: globalPrinterModelList,
                    printer: printer
                }, '*');
            }
        }else {
            iframe.contentWindow.postMessage({
                command: 'printer_settings',
                printer: printer
            }, '*');
        }
    }
    return dialog;
}


function closeModals() {
    const modals = document.querySelectorAll('.add-printer-modal, .printer-setting-modal, .printer-setting-physical-modal, .printer-auth-modal');
    modals.forEach(modal => {
        if (modal.parentNode) {
            modal.parentNode.removeChild(modal);
        }
    });
    
    document.getElementById('modal-overlay').style.display = 'none';
}


function showPrinterAuth(printer) {
    // create and show auth dialog
    var dialog = document.createElement('div');
    dialog.innerHTML = `
        <div class="printer-auth-modal">
            <iframe src="printer_auth.html" class="printer-auth-iframe"></iframe>
        </div>
    `;
    
    document.body.appendChild(dialog);
    
    // show dialog
    dialog.querySelector('.printer-auth-modal').style.display = 'block';
    
    // wait for iframe load
    var iframe = dialog.querySelector('iframe');
    iframe.onload = function() {
        iframe.contentWindow.postMessage({
            command: 'init',
            printer: printer
        }, '*');
    };
    
    window.currentPrinterAuthDialog = dialog;
}

function closePrinterAuth() {
    if (window.currentPrinterAuthDialog) {
        window.currentPrinterAuthDialog.remove();
        window.currentPrinterAuthDialog = null;
    }
}

