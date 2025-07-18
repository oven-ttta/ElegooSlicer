var mPrintTask = null;
let currentPage = 1;
const pageSize = 4;
var hasAmsInfo = false;

function OnInit() {
	TranslatePage();
	RequestPrintTask();
	initEventHandlers();
}

function RequestPrintTask()
{
	var tSend={};
	tSend['sequence_id']=Math.round(new Date() / 1000);
	tSend['command']="request_print_task";
		
	SendWXMessage( JSON.stringify(tSend) );		
}

function HandleStudio(pVal)
{
	let strCmd=pVal['command'];
	
	if(strCmd=='response_print_task')
	{
		mPrintTask = pVal['response'];
		UpdatePrintInfo();
	}

}

function UpdatePrintInfo()
{
	if (!mPrintTask) return;
	const printInfo = mPrintTask.printInfo;
	const amsInfo = mPrintTask.amsInfo;
	$('#model-name').val(printInfo.modelName);
	adjustModelNameWidth();
	if (printInfo.thumbnail) {
		$('#model-image').attr('src', 'data:image/png;base64,' + printInfo.thumbnail);
	}

	$('#print-time').text(printInfo.printTime);
	$('#print-weight').text((printInfo.totalWeight).toFixed(2) + 'g');
	$('#print-layer-count').text(printInfo.layerCount);

	$('#option-timelapse').prop('checked', printInfo.timeLapse);
	$('#option-bedlevel').prop('checked', printInfo.heatedBedLeveling);
	$('#option-upload-and-print').prop('checked', printInfo.uploadAndPrint);
	$('#option-switch-to-device-tab').prop('checked', printInfo.switchToDeviceTab);
	$('#option-auto-refill').prop('checked', printInfo.autoRefill);

	if (printInfo.bedType == "btPC") {
		$('#bedB').prop('checked', true);
	} else {
		$('#bedA').prop('checked', true);
	}
	if (amsInfo.trayFilamentList && amsInfo.trayFilamentList.length > 0) {
		renderFilamentMapping(printInfo.filamentList, amsInfo.trayFilamentList);
		hasAmsInfo = true;
	} else {
		hasAmsInfo = false;
	}
	updateDisplay();
}

function getContrastColor(hexColor) {
	hexColor = hexColor.replace('#', '');

	if (hexColor.length === 3) {
		hexColor = hexColor.split('').map(x => x + x).join('');
	}
	const r = parseInt(hexColor.substr(0,2),16);
	const g = parseInt(hexColor.substr(2,2),16);
	const b = parseInt(hexColor.substr(4,2),16);
	const brightness = (r * 299 + g * 587 + b * 114) / 1000;
	return brightness > 180 ? '#222' : '#fff';
}

function renderFilamentMapping(filamentList, amsTrayFilamentList) {
	const list = document.getElementById('filament-list');
	list.innerHTML = '';
	if (!filamentList || !amsTrayFilamentList) return;
	const totalPages = Math.ceil(filamentList.length / pageSize);
	const start = (currentPage - 1) * pageSize;
	const end = Math.min(start + pageSize, filamentList.length);
	document.getElementById('filament-page').textContent = `${currentPage}/${totalPages}`;
	for (let i = start; i < end; i++) {
		const filament = filamentList[i];
		const filamentColor = filament.filamentColor;
		const amsColor = filament.amsFilamentColor || '#888';
		const circleTextColor = getContrastColor(filamentColor);
		const amsTextColor = getContrastColor(amsColor);
		const filamentDiv = document.createElement('div');
		filamentDiv.className = 'mapping-container';
		filamentDiv.innerHTML = `
			<div class="filament-rect" style="background:${filamentColor};color:${circleTextColor}; border:1px solid ${circleTextColor};" title="${filament.filamentName}">
				<div class="filament-type">${filament.filamentType}</div>
				<div class="filament-dash"></div>
				<div class="filament-weight">${filament.filamentWeight}</div>
			</div>
			<div class="tray-arrow"></div>
			<div class="tray-rect" style="background:${amsColor};color:${amsTextColor}; border:1px solid ${amsTextColor};">
				<div class="tray-index">${filament.trayIndex || '?'}</div>
				<div class="tray-dash"></div>
				<div class="tray-filament-type">${filament.amsFilamentType || '?'}</div>
				<button class="slot-arrow-btn" data-index="${i}" style="color:${amsTextColor};">▼</button>
			</div>
		`;
		const arrowBtn = filamentDiv.querySelector('.slot-arrow-btn');
		arrowBtn.addEventListener('click', function(e) {
			e.stopPropagation();
			showAmsPopup(i, amsTrayFilamentList, filament);
		});
		list.appendChild(filamentDiv);
	}
}

function showAmsPopup(filamentIdx, amsTrayFilamentList, filament) {
	document.querySelectorAll('.ams-popover').forEach(e => e.remove());
	const btn = document.querySelector(`.slot-arrow-btn[data-index="${filamentIdx}"]`);
	if (!btn) return;
	const popover = document.createElement('div');
	popover.className = 'ams-popover';

	const amsGrid = document.createElement('div');
	amsGrid.style.display = 'grid';
	amsGrid.style.gridTemplateColumns = 'repeat(4, 1fr)';
	amsGrid.style.justifyItems = 'center';
	amsGrid.style.alignItems = 'center';

	amsTrayFilamentList.forEach(amsFilament => {
		const amsFilamentColor = amsFilament.amsFilamentColor;
		const amsFilamentTextColor = getContrastColor(amsFilamentColor);
		const amsFilamentBtn = document.createElement('button');
		amsFilamentBtn.className = 'tray-popover-card';
		amsFilamentBtn.style.background = amsFilamentColor;
		amsFilamentBtn.style.color = amsFilamentTextColor;
		amsFilamentBtn.innerHTML = `
			<div class="tray-index">${amsFilament.trayIndex}</div>
			<div class="tray-dash"></div>
			<div class="tray-filament-type">${amsFilament.amsFilamentType}</div>
		`;
		amsFilamentBtn.onclick = function(e) {
			e.stopPropagation();
			updateFilamentMapping(filamentIdx, amsFilament);
			popover.remove();
		};
		amsGrid.appendChild(amsFilamentBtn);
	});

	popover.appendChild(amsGrid);

	document.body.appendChild(popover);
	const rect = btn.getBoundingClientRect();
	const scrollTop = window.scrollY || document.documentElement.scrollTop;
	const scrollLeft = window.scrollX || document.documentElement.scrollLeft;
	popover.style.top = (rect.bottom + scrollTop + 6) + 'px';
	popover.style.left = (rect.left + rect.width / 2 + scrollLeft) + 'px';
	popover.style.transform = 'translateX(-50%)';

	setTimeout(() => {
		document.addEventListener('mousedown', onClickOutside);
	}, 0);
	function onClickOutside(e) {
		if (!popover.contains(e.target)) {
			popover.remove();
			document.removeEventListener('mousedown', onClickOutside);
		}
	}
}

function updateFilamentMapping(filamentIdx, amsFilament) {
	//dalert(filamentIdx);
	const trayContainer = document.querySelector(`.slot-arrow-btn[data-index="${filamentIdx}"]`).closest('.tray-rect');
	trayContainer.querySelector('.tray-index').textContent = amsFilament.trayIndex;
	trayContainer.querySelector('.tray-filament-type').textContent = amsFilament.amsFilamentType;
	trayContainer.style.background = amsFilament.amsFilamentColor;
	const trayTextColor = getContrastColor(amsFilament.amsFilamentColor);	
	trayContainer.style.color = trayTextColor;
	trayContainer.querySelector('.slot-arrow-btn').style.color = trayTextColor;

	if (mPrintTask && mPrintTask.printInfo && mPrintTask.printInfo.filamentList) {
		mPrintTask.printInfo.filamentList[filamentIdx].trayIndex = amsFilament.trayIndex;
		mPrintTask.printInfo.filamentList[filamentIdx].trayId = amsFilament.trayId;
		mPrintTask.printInfo.filamentList[filamentIdx].amsId = amsFilament.amsId;
		mPrintTask.printInfo.filamentList[filamentIdx].amsFilamentColor = amsFilament.amsFilamentColor;
		mPrintTask.printInfo.filamentList[filamentIdx].amsFilamentType = amsFilament.amsFilamentType;
		mPrintTask.printInfo.filamentList[filamentIdx].amsFilamentName = amsFilament.amsFilamentName;
	}
}

function initEventHandlers() {
	document.getElementById('prev-page').addEventListener('click', function() {
		if (currentPage > 1) {
			currentPage--;
			if (mPrintTask && mPrintTask.printInfo && mPrintTask.amsInfo) {
				renderFilamentMapping(mPrintTask.printInfo.filamentList, mPrintTask.amsInfo.trayFilamentList);
			}
		}
	});

	document.getElementById('next-page').addEventListener('click', function() {
		if (mPrintTask && mPrintTask.printInfo && mPrintTask.amsInfo) {
			const totalPages = Math.ceil(mPrintTask.printInfo.filamentList.length / pageSize);
			if (currentPage < totalPages) {
				currentPage++;
				renderFilamentMapping(mPrintTask.printInfo.filamentList, mPrintTask.amsInfo.trayFilamentList);
			}
		}
	});

	$('#cancel-btn').on('click', function() {
		SendWXMessage(JSON.stringify({
			command: 'cancel_print',
			sequence_id: Math.round(new Date() / 1000)
		}));
	});

	$('#upload-btn').on('click', function() {
		mPrintTask.printInfo.timeLapse = $('#option-timelapse').prop('checked');
		mPrintTask.printInfo.heatedBedLeveling = $('#option-bedlevel').prop('checked');
		mPrintTask.printInfo.uploadAndPrint = $('#option-upload-and-print').prop('checked');
		mPrintTask.printInfo.switchToDeviceTab = $('#option-switch-to-device-tab').prop('checked');
		mPrintTask.printInfo.autoRefill = $('#option-auto-refill').prop('checked');

		if ($('#bedA').hasClass('selected')) {
			mPrintTask.printInfo.bedType = 'btPC';
		} else if ($('#bedB').hasClass('selected')) {
			mPrintTask.printInfo.bedType = 'btPEI';
		}
		if (hasAmsInfo) {
			if (!checkFilamentMapping(mPrintTask.printInfo.filamentList)) {
				showStatusTip(getTranslation('some_filaments_not_mapped'));
				return;
			}
		}
		SendWXMessage(JSON.stringify({
			command: 'start_upload',
			sequence_id: Math.round(new Date() / 1000),
			data: mPrintTask
		}));
	});

	
	document.getElementById('bedA').addEventListener('click', function() {
		document.getElementById('bedA').classList.add('selected');
		document.getElementById('bedB').classList.remove('selected');
	});
	document.getElementById('bedB').addEventListener('click', function() {
		document.getElementById('bedB').classList.add('selected');
		document.getElementById('bedA').classList.remove('selected');
	});

	$('#edit-name-btn').on('click', function() {
		const $input = $('#model-name');
		$input.prop('readonly', false).focus();
	});

	function filterModelName(str) {
		return str.replace(/[^a-zA-Z0-9\u4e00-\u9fa5_\- ]/g, '');
	}

	$('#model-name').on('blur', function() {
		$(this).prop('readonly', true);
		let filtered = $(this).val();
		// $(this).val(filtered);
		if (mPrintTask && mPrintTask.printInfo) {
			mPrintTask.printInfo.modelName = filtered;
		}
		adjustModelNameWidth();
	});
	$('#model-name').on('keydown', function(e) {
		if (e.key === 'Enter') {
			$(this).prop('readonly', true);
			$(this).blur();
			// let filtered = filterModelName($(this).val());
			// $(this).val(filtered);
			if (mPrintTask && mPrintTask.printInfo) {
				mPrintTask.printInfo.modelName = filtered;
			}
			adjustModelNameWidth();
		}
	});

	$('#option-upload-and-print').on('change', function() {
		updateDisplay();
	});

	$('#model-name').on('input', adjustModelNameWidth);
	$(function() { adjustModelNameWidth(); });
}

function adjustModelNameWidth() {
	var input = document.getElementById('model-name');
	var span = document.createElement('span');
	span.style.visibility = 'hidden';
	span.style.position = 'fixed';
	span.style.font = window.getComputedStyle(input).font;
	span.innerText = input.value || input.placeholder;
	document.body.appendChild(span);
	input.style.width = (span.offsetWidth + 20) + 'px';
	document.body.removeChild(span);
}

function updateDisplay() {
	if ($('#option-upload-and-print').prop('checked')) {
		document.querySelector('[data-option="option-timelapse"]').style.display = 'block';
		document.querySelector('[data-option="option-bedlevel"]').style.display = 'block';
		document.querySelector('#bedA').style.display = 'block';
		document.querySelector('#bedB').style.display = 'block';
		if (hasAmsInfo) {
			document.querySelector('.filament-info').style.display = 'block';
		} else {
			document.querySelector('.filament-info').style.display = 'none';
		}
	} else {
		document.querySelector('[data-option="option-timelapse"]').style.display = 'none';
		document.querySelector('[data-option="option-bedlevel"]').style.display = 'none';
		document.querySelector('.filament-info').style.display = 'none';
		document.querySelector('#bedA').style.display = 'none';
		document.querySelector('#bedB').style.display = 'none';
	}
}

function checkFilamentMapping(filamentList) {
	for (const filament of filamentList) {
		if (
			filament.trayIndex === undefined || filament.trayIndex === null || filament.trayIndex === '' ||
			filament.amsId === undefined || filament.amsId === null || filament.amsId === '' ||
			filament.trayId === undefined || filament.trayId === null || filament.trayId === ''
		) {
			return false;
		}
	}
	return true;
}

function showStatusTip(msg) {
	const tip = document.getElementById('status-tip');
	tip.textContent = msg;
	tip.classList.add('show');
	tip.classList.remove('hide');
	if (tip._timer) clearTimeout(tip._timer);
	tip._timer = setTimeout(() => {
		tip.classList.add('hide');
		setTimeout(() => { tip.classList.remove('show'); }, 200);
	}, 5000);
}

document.addEventListener('DOMContentLoaded', adjustModelNameWidth);
document.getElementById('model-name').addEventListener('input', adjustModelNameWidth);

document.getElementById('model-name').value = modelName;
adjustModelNameWidth();

function getTranslation(key) {
	// Ensure LangText is loaded
	if (typeof LangText === 'undefined') {
		console.warn('Translation system not initialized');
		return key;
	}
	
	// Get current language
	const currentLang = localStorage.getItem(LANG_COOKIE_NAME) || 'en';
	
	// Check if translation exists
	if (!LangText[currentLang] || !LangText[currentLang][key]) {
		console.warn(`Translation missing for key: ${key} in language: ${currentLang}`);
		return LangText['en'][key] || key;
	}
	
	return LangText[currentLang][key];
}