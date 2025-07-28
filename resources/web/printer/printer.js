
let globalPrinterList = [];

function OnInit() {
	TranslatePage();
	RequestPrintTask();
}

function RequestPrintTask()
{
	var tSend={};
	tSend['sequence_id']=Math.round(new Date() / 1000);
	tSend['command']="request_printer_list";
		
	SendWXMessage( JSON.stringify(tSend) );		
}

function HandleStudio(pVal)
{
	let strCmd=pVal['command'];
	if(strCmd=="response_printer_list") {
		let printers=pVal['response'];
		renderAllPrinters(printers);
	} else if(strCmd=="response_discover_printers") {
		var add_printer_modal = document.querySelector('.add-printer-modal');
		if (add_printer_modal) {
			var add_printer_iframe = add_printer_modal.querySelector('iframe');
			if (add_printer_iframe && add_printer_iframe.contentWindow) {
				add_printer_iframe.contentWindow.postMessage({
					command: 'discover_printers',
					data: pVal.response
				}, '*');
			}
		}
	} else if(strCmd=="response_bind_printer" || strCmd=="response_update_printer_name") {
        RequestPrintTask();
	} else if(strCmd=="response_delete_printer") {
        RequestPrintTask();
        closeModals();
    }
}

function getPrinterStatus(printerStatus) {
	switch(printerStatus) {
		case 0:
			return "Printing";
		case 1:
			return "Idle";
		default:
			return "";
	}

}

function getRemainingTime(currentTicks, totalTicks) {
    let remainingTime = totalTicks - currentTicks;
    let hours = Math.floor(remainingTime / 3600);
    let minutes = Math.floor((remainingTime % 3600) / 60);
    let seconds = remainingTime % 60;
    return `${hours}:${minutes}:${seconds}`;    
}


function renderPrinterCards(printers) {
    let html = "";
    printers.forEach((p, index) => {
        html += `
            <div class="printer-card">
            <div class="printer-card-header">
            <div class="printer-img"><img src="${p.printerImg || ''}" alt="Printer" /></div>
            <div class="printer-header-info">
                <div class="printer-title">${p.name}</div>
                <div class="printer-ip">${p.ip || ''}</div>
                <button class="printer-detail-btn" onclick="showPrinterDetail('${p.id}')">Details</button>
            </div>
            <div class="card-menu" title="more" onclick="showPrinterSettingsByIndex(${index})">&#8942;</div>
            </div>
            <div class="progress-bar-container">
            <span class="progress-label">${p.printProgress || 0}%</span>
            <div class="progress-bar-bg">
                <div class="progress-bar" style="width:${p.printProgress || 0}%"></div>
            </div>
            </div>
            <div class="printer-status-row">
            <button class="printer-status-btn">${getPrinterStatus(p.printerStatus)}</button>
            <span class="remaining-time">${getRemainingTime(p.currentTicks, p.totalTicks)}</span>
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

function showPrinterDetail(id) {
	var tSend={};
	tSend['sequence_id']=Math.round(new Date() / 1000);
	tSend['command']="request_printer_detail";
	tSend['id']=id;	
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
            }, '*');
        };        
        return dialog;
    };

    window.addEventListener('message', function(event) {
        if (event.data.command == 'discover_printers') {
            var tSend={};
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['command']="request_discover_printers";
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command == 'connect_printer') {
            console.log('Connect printer message received');
        } else if (event.data.command == 'delete_printer') {
            var tSend={};
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['command']="request_delete_printer";
            tSend['id']=event.data.id;
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command === 'closeModal') {
            closeModals();
        } else if (event.data.command === 'bind_printer') {
            var tSend={};
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['command']="request_bind_printer";
            tSend['printer']=event.data.printer;
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command === 'update_printer_name') {
            var tSend={};
            tSend['sequence_id']=Math.round(new Date() / 1000);
            tSend['command']="request_update_printer_name";
            tSend['id']=event.data.id;
            tSend['name']=event.data.name;
            SendWXMessage( JSON.stringify(tSend) );
        } else if (event.data.command === 'delete_printer') {
        
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
    if (!printer.isPhysicalPrinter) {
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
        iframe.contentWindow.postMessage({
            command: 'printer_settings',
            data: printer
        }, '*'); 
    }
    return dialog;
}


function closeModals() {
    const modals = document.querySelectorAll('.add-printer-modal, .printer-setting-modal, .printer-setting-physical-modal');
    modals.forEach(modal => {
        if (modal.parentNode) {
            modal.parentNode.removeChild(modal);
        }
    });
    
    document.getElementById('modal-overlay').style.display = 'none';
}




