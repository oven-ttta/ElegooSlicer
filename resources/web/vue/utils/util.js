
function isEmptyString(str) {
    return !str || str.trim().length === 0;
}

function isValidURL(url) {
    const pattern = new RegExp('^(https?:\\/\\/)?' + // protocol
        '((([a-z0-9\\-]+\\.)+[a-z]{2,})|' + // domain name
        'localhost|' + // localhost
        '\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}|' + // IP address
        '\\[?[a-f0-9]*:[a-f0-9:%.~+\\-]*\\]?)' + // IPv6
        '(\\:\\d+)?(\\/[^\\s]*)?$', 'i'); // port and path
    return pattern.test(url);
}

function disableRightClickMenu() {
    // 只在非开发模式下禁用右键菜单
    const isDev = GetQueryString("dev") === "true";
    
    if (!isDev) {
        document.addEventListener('contextmenu', (e) => {
            e.preventDefault();
        });
    }
}