function renderManualForm(containerId) {
    const formHtml = `
        <form id="manual-form" autocomplete="off">
            <div class="form-group-box">
                <div class="form-group">
                    <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 6px;">
                        <label>Printer Name</label>
                        <div class="form-group-advanced">
                            <span>Advanced</span>
                            <label class="toggle-switch">
                                <input type="checkbox" id="advanced-switch" onclick="toggleAdvanced()">
                                <span class="toggle-slider"></span>
                            </label>
                        </div>
                    </div>
                    <input type="text" name="printer_name" required>
                </div>
                <div class="form-group">
                    <label>Model</label>
                    <select name="printer_model">
                        <option value="ELEGOO Link">ELEGOO Link</option>
                    </select>
                </div>
                <div class="form-group row">
                    <label>Host Name, IP or URL</label>
                    <div class="input-group">
                        <input type="text" name="printer_ip" required>
                        <button type="button" onclick="testPrinterConnection()">Test</button>
                    </div>
                </div>
                <div class="form-group">
                    <label>Device Username</label>
                    <input type="text" name="username">
                </div>
                <div class="form-group">
                    <label>API/Password</label>
                    <input type="password" name="api_key">
                </div>
                <div class="form-group row">
                    <label>HTTPS CA File</label>
                    <div class="input-group">
                        <input type="text" name="ca_file" readonly>
                        <button type="button">Preview</button>
                    </div>
                </div>
            </div>
        </form>
    `;
    
    document.getElementById(containerId).innerHTML = formHtml;
}

function getManualFormData() {
    const form = document.getElementById('manual-form');
    if (!form) return null;
    
    const formData = {};
    const formElements = form.elements;
    
    for (let i = 0; i < formElements.length; i++) {
        const element = formElements[i];
        if (element.name) {
            formData[element.name] = element.value;
        }
    }
    
    return formData;
}

function testPrinterConnection() {
    const formData = getManualFormData();
    if (formData && formData.printer_ip) {
        console.log('Testing connection to:', formData.printer_ip);
        // Implement connection test logic here
    }
} 