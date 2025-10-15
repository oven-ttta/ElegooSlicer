const { createApp } = Vue;
const { ElInput, ElButton, ElPopover, ElDialog, ElTab, ElTabPane, ElSelect, ElOption, ElForm, ElFormItem, ElLoading, ElMessageBox, ElPopconfirm } = ElementPlus;

const Navigation = {
    data() {
        return {
            
            userInfo: {
                userId: '',
                nickname: '',
                avatar: '',
                loginStatus: 0
            },
            currentPage: 'recent',
            showUserMenu: false
        }
    },
    methods: {
        async init() {
            try {
                const response = await this.ipcRequest('getUserInfo', {});
                console.log('getUserInfo response:', response);
                if (response) {
                    this.userInfo = response;
                }
            } catch (error) {
                console.error('Failed to get user info:', error);
                // Don't show error message for getUserInfo - user might just be not logged in
            }
        },
        async navigateToPage(pageName) {
            console.log('Navigate to page method called:', pageName);
            this.currentPage = pageName;
            
            await this.ipcRequest('navigateToPage', { page: pageName });
        },
        
        async beginDownloadNetworkPlugin() {
            console.log('Download network plugin clicked');
            await this.ipcRequest('downloadNetworkPlugin', {});
        },
        
        // IPC Communication methods
        async ipcRequest(method, params = {}, timeout = 10000) {
            try {
                const response = await nativeIpc.request(method, params, timeout);
                return response;
            } catch (error) {
                console.error(`IPC request failed for ${method}:`, error);
                throw error;
            }
        },
        async onLoginOrRegister() {
            console.log('Login or Register clicked');
            try {
                await this.ipcRequest('showLoginDialog', {});
                await this.init(); // Refresh user info after login dialog closes
            } catch (error) {
                console.error('Login dialog failed:', error);
            }
        },

        toggleUserMenu() {
            this.showUserMenu = !this.showUserMenu;
            console.log('User menu toggled:', this.showUserMenu);
        },
        
        async onLogout() {
            console.log('Logout clicked');
            try {
                await this.ipcRequest('logout', {});
                // Reset user info after logout
                this.userInfo.loginStatus = 0;
                this.userInfo.nickname = '';
                this.userInfo.avatar = '';
                this.userInfo.userId = '';
                this.showUserMenu = false;
            } catch (error) {
                console.error('Logout failed:', error);
            }
        },

        closeUserMenu() {
            this.showUserMenu = false;
        },

        handleImageError(event) {
            // 当头像图片加载失败时，设置为默认头像
            event.target.src = 'img/default-avatar.jpg';
            event.target.onerror = null; // 防止无限循环
        },
    },
    
    async mounted() {
        await this.init();
        nativeIpc.on('onUserInfoUpdated', (data) => {
            console.log('Received user info update event from backend:', data);
            this.userInfo = data;
        });
        
        await this.ipcRequest('ready', {});
    }
};

const app = createApp(Navigation)
    .use(ElementPlus)
    .use(i18n)
    .mount('#app');

