let mPrintTask = null;
let currentPage = 1;
const pageSize = 4;
let hasAmsInfo = false;
let mPrinterList = [];
let selectedPrinterId = null;
let selectedBedType = null;

function OnInit() {
	TranslatePage();
	RequestPrintTask();
	RequestPrinterList();
	initEventHandlers();
	
	document.querySelector('.print-options-section').classList.add('hidden');
}

function RequestPrintTask() {
	var tSend = {};
	tSend['sequence_id'] = Math.round(new Date() / 1000);
	tSend['command'] = "request_print_task";
	SendWXMessage(JSON.stringify(tSend));
}

function RequestPrinterList() {
	var tSend = {};
	tSend['sequence_id'] = Math.round(new Date() / 1000);
	tSend['command'] = "request_printer_list";
	SendWXMessage(JSON.stringify(tSend));
}

function HandleStudio(pVal) {
	let strCmd = pVal['command'];
	
	if (strCmd == 'response_print_task') {
		mPrintTask = pVal['response'];
		UpdatePrintInfo();
	} else if (strCmd == 'response_printer_list') {
		mPrinterList = pVal['response'] || [];
		UpdatePrinterList();
	}
}

function UpdatePrintInfo() {
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

	if (printInfo.bedType) {
		selectedBedType = printInfo.bedType;
		const bedOptions = document.getElementById('bed-options');
		if (bedOptions) {
			bedOptions.querySelectorAll('.dropdown-item').forEach(opt => opt.classList.remove('selected'));
			const selectedOption = bedOptions.querySelector(`[data-value="${printInfo.bedType}"]`);
			if (selectedOption) {
				selectedOption.classList.add('selected');
				const bedHeader = document.getElementById('bed-header');
				const title = selectedOption.querySelector('.dropdown-item-title').textContent;
				const imageSrc = selectedOption.querySelector('.dropdown-item-icon').src;
				bedHeader.innerHTML = `
					<img src="${imageSrc}" alt="" class="dropdown-icon">
					<div class="dropdown-content">
						<div class="dropdown-title">${title}</div>
					</div>
					<img src="img/arrow_down.svg" alt="" class="dropdown-arrow">
				`;
			}
		}
	}
	
	if (amsInfo && amsInfo.trayFilamentList && amsInfo.trayFilamentList.length > 0) {
		renderFilamentMapping(printInfo.filamentList, amsInfo.trayFilamentList);
		hasAmsInfo = true;
	} else {
		hasAmsInfo = false;
	}
	updateDisplay();
}

function UpdatePrinterList() {
	if(mPrinterList.length == 0) {
		return;
	}
	const optionsContainer = document.getElementById('printer-options');
	optionsContainer.innerHTML = '';
	
	let selectedPrinter = null
	
	mPrinterList.forEach(printer => {
		if(printer.selected) {
			selectedPrinter = printer;
		}
	});

	if(selectedPrinter == null) {
		selectedPrinter = mPrinterList[0];
	}
	
	// Set the default selected printer ID
	selectedPrinterId = selectedPrinter.id;
	
	mPrinterList.forEach(printer => {
		const option = document.createElement('li');	

		const isSelected = selectedPrinter && printer.id === selectedPrinter.id;
		option.className = isSelected ? 'dropdown-item selected' : 'dropdown-item';
		
		option.setAttribute('data-value', printer.id);
		option.setAttribute('data-printer', JSON.stringify(printer));
		
		const icon = document.createElement('img');
		icon.src = printer.printerImg;
		icon.alt = '';
		icon.className = 'dropdown-item-icon';
		
		const content = document.createElement('div');
		content.className = 'dropdown-item-content';
		
		const title = document.createElement('div');
		title.className = 'dropdown-item-title';
		title.textContent = printer.name;
		
		content.appendChild(title);
		
		if (printer.model) {
			const subtitle = document.createElement('div');
			subtitle.className = 'dropdown-item-subtitle';
			subtitle.textContent = printer.model;
			content.appendChild(subtitle);
		}
		
		option.appendChild(icon);
		option.appendChild(content);
		
		optionsContainer.appendChild(option);
	});

	document.querySelector('#printer-header .dropdown-icon').src = selectedPrinter.printerImg;	
	document.querySelector('#printer-header .dropdown-title').textContent = selectedPrinter.name;
	document.querySelector('#printer-header .dropdown-subtitle').textContent = selectedPrinter.machineModel || '';	
	
	// Add click event listeners
	optionsContainer.querySelectorAll('.dropdown-item').forEach(option => {
		option.addEventListener('click', function() {
			const value = this.getAttribute('data-value');
			const printer = JSON.parse(this.getAttribute('data-printer'));
			
			// Update header content (not the entire header)
			const headerIcon = document.querySelector('#printer-header .dropdown-icon');
			const headerTitle = document.querySelector('#printer-header .dropdown-title');
			const headerSubtitle = document.querySelector('#printer-header .dropdown-subtitle');
			
			if (headerIcon) {
				headerIcon.src = printer.printerImg;
			}
			if (headerTitle) headerTitle.textContent = printer.name;
			if (headerSubtitle) headerSubtitle.textContent = printer.machineModel || '';
			
			optionsContainer.querySelectorAll('.dropdown-item').forEach(opt => opt.classList.remove('selected'));
			this.classList.add('selected');
			
			document.getElementById('printer-options').hidden = true;
			document.getElementById('printer-header').classList.remove('active');
			
			selectedPrinterId = value;
			OnPrinterChanged(value);
		});
	});
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
				<div class="filament-weight">${filament.filamentWeight}g</div>
			</div>
			<div class="tray-arrow"></div>
			<div class="tray-rect" style="background:${amsColor};color:${amsTextColor}; border:1px solid ${amsTextColor};" title="${filament.amsFilamentName || 'No mapping'}">
				<div class="tray-index">${filament.amsTrayIndex || '?'}</div>
				<div class="tray-dash"></div>
				<div class="tray-filament-type">${filament.amsFilamentType || 'N/A'}</div>
			</div>
		`;
		list.appendChild(filamentDiv);
	}
}

function showAmsPopup(filamentIdx, amsTrayFilamentList, filament) {
	const popover = document.createElement('div');
	popover.className = 'ams-popover';
	popover.style.left = '50%';
	popover.style.top = '50%';
	popover.style.transform = 'translate(-50%, -50%)';
	
	let popoverContent = '<div style="margin-bottom: 8px; font-weight: 500;">Select AMS Tray:</div>';
	amsTrayFilamentList.forEach((amsFilament, index) => {
		const amsColor = amsFilament.filamentColor || '#888';
		const amsTextColor = getContrastColor(amsColor);
		popoverContent += `
			<div class="tray-popover-card" data-index="${index}" style="background:${amsColor};color:${amsTextColor}; border:1px solid ${amsTextColor};">
				<div class="tray-index">${index + 1}</div>
				<div class="tray-dash"></div>
				<div class="tray-filament-type">${amsFilament.filamentType}</div>
			</div>
		`;
	});
	
	popover.innerHTML = popoverContent;
	document.body.appendChild(popover);
	
	function onClickOutside(e) {
		if (!popover.contains(e.target)) {
			document.body.removeChild(popover);
			document.removeEventListener('click', onClickOutside);
		}
	}
	
	setTimeout(() => document.addEventListener('click', onClickOutside), 0);
	
	popover.querySelectorAll('.tray-popover-card').forEach(card => {
		card.addEventListener('click', function() {
			const amsIndex = parseInt(this.getAttribute('data-index'));
			updateFilamentMapping(filamentIdx, amsTrayFilamentList[amsIndex]);
			document.body.removeChild(popover);
			document.removeEventListener('click', onClickOutside);
		});
	});
}

function updateFilamentMapping(filamentIdx, amsFilament) {
	if (mPrintTask && mPrintTask.printInfo && mPrintTask.printInfo.filamentList) {
		mPrintTask.printInfo.filamentList[filamentIdx].amsFilamentColor = amsFilament.filamentColor;
		mPrintTask.printInfo.filamentList[filamentIdx].amsFilamentName = amsFilament.filamentName;
		mPrintTask.printInfo.filamentList[filamentIdx].amsFilamentType = amsFilament.filamentType;
		mPrintTask.printInfo.filamentList[filamentIdx].amsTrayIndex = amsFilament.trayIndex;
		renderFilamentMapping(mPrintTask.printInfo.filamentList, mPrintTask.amsInfo.trayFilamentList);
	}
}

function initEventHandlers() {
	// Debounce function to reduce frequent calls
	function debounce(func, wait) {
		let timeout;
		return function executedFunction(...args) {
			const later = () => {
				clearTimeout(timeout);
				func(...args);
			};
			clearTimeout(timeout);
			timeout = setTimeout(later, wait);
		};
	}

	// Optimized dropdown handlers
	document.getElementById('printer-header').addEventListener('click', function(e) {
		// Handle refresh button click
		if (e.target.closest('.printer-refresh-btn')) {
			e.preventDefault();
			e.stopPropagation();
			OnRefreshPrinterList();
			return;
		}
		
		e.preventDefault();
		const options = document.getElementById('printer-options');
		const isVisible = !options.hidden;
		
		document.getElementById('bed-options').hidden = true;
		document.getElementById('bed-header').classList.remove('active');
		
		if (isVisible) {
			options.hidden = true;
			this.classList.remove('active');
		} else {
			options.hidden = false;
			this.classList.add('active');
		}
	});
	
	document.getElementById('bed-header').addEventListener('click', function(e) {
		e.preventDefault();
		const options = document.getElementById('bed-options');
		const isVisible = !options.hidden;
		
		document.getElementById('printer-options').hidden = true;
		document.getElementById('printer-header').classList.remove('active');
		
		if (isVisible) {
			options.hidden = true;
			this.classList.remove('active');
		} else {
			options.hidden = false;
			this.classList.add('active');
		}
	});
	
	document.getElementById('bed-options').addEventListener('click', function(e) {
		if (e.target.closest('.dropdown-item')) {
			const option = e.target.closest('.dropdown-item');
			const value = option.getAttribute('data-value');
			const title = option.querySelector('.dropdown-item-title').textContent;
			const imageSrc = option.querySelector('.dropdown-item-icon').src;
			
			const header = document.getElementById('bed-header');
			header.innerHTML = `
				<img src="${imageSrc}" alt="" class="dropdown-icon">
				<div class="dropdown-content">
					<div class="dropdown-title">${title}</div>
				</div>
				<img src="img/arrow_down.svg" alt="" class="dropdown-arrow">
			`;
			
			this.querySelectorAll('.dropdown-item').forEach(opt => opt.classList.remove('selected'));
			option.classList.add('selected');
			
			this.hidden = true;
			document.getElementById('bed-header').classList.remove('active');
			
			OnBedTypeChanged(value);
		}
	});
	
	// Optimized click outside handler
	document.addEventListener('click', debounce(function(e) {
		if (!e.target.closest('.dropdown')) {
			document.getElementById('printer-options').hidden = true;
			document.getElementById('bed-options').hidden = true;
			document.getElementById('printer-header').classList.remove('active');
			document.getElementById('bed-header').classList.remove('active');
		}
	}, 10));

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
		mPrintTask.printInfo.selectedPrinterId = selectedPrinterId;
		const selectedBedOption = document.querySelector('#bed-options .dropdown-item.selected');
		if (selectedBedOption) {
			mPrintTask.printInfo.bedType = selectedBedOption.getAttribute('data-value');
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
		if (mPrintTask && mPrintTask.printInfo) {
			mPrintTask.printInfo.modelName = filtered;
		}
		adjustModelNameWidth();
	});

	$('#model-name').on('keydown', function(e) {
		if (e.key === 'Enter') {
			$(this).prop('readonly', true);
			$(this).blur();
			let filtered = filterModelName($(this).val());
			if (mPrintTask && mPrintTask.printInfo) {
				mPrintTask.printInfo.modelName = filtered;
			}
			adjustModelNameWidth();
		}
	});

	$('#option-upload-and-print').on('change', function() {
		updateDisplay();
	});

	// Debounced input handler
	$('#model-name').on('input', debounce(adjustModelNameWidth, 100));
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
		document.querySelector('.print-options-section').classList.remove('hidden');
		document.querySelector('.bed-dropdown').classList.remove('hidden');
		if (hasAmsInfo) {
			document.querySelector('.filament-section').classList.remove('hidden');
		} else {
			document.querySelector('.filament-section').classList.add('hidden');
		}
	} else {
		document.querySelector('.print-options-section').classList.add('hidden');
		document.querySelector('.bed-dropdown').classList.add('hidden');
		document.querySelector('.filament-section').classList.add('hidden');
	}
}

function showStatusTip(msg) {
	const tip = document.getElementById('status-tip');
	tip.textContent = msg;
	tip.style.display = 'block';
	setTimeout(() => {
		tip.style.display = 'none';
	}, 3000);
}

function OnBedTypeChanged(bedType) {
	selectedBedType = bedType;
}

function OnPrinterChanged(printerId) {
	selectedPrinterId = printerId;
}

function getTranslation(key) {
	return window.g_translation && window.g_translation[key] ? window.g_translation[key] : key;
}