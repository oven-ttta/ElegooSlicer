const { createApp } = Vue;
const { ElInput, ElButton, ElPopover, ElDialog, ElTab, ElTabPane, ElSelect, ElOption, ElForm, ElFormItem, ElLoading, ElMessageBox, ElPopconfirm } = ElementPlus;

const Navigation = {
    data() {
        return {
            userInfo: {
                userId: '',
                username: '',
                token: '',
                refreshToken: '',
                hostType: '',
                accessTokenExpireTime: 0,
                refreshTokenExpireTime: 0,
                rtcToken: '',
                rtcTokenExpireTime: 0,
                nickname: '',
                email: '',
                avatar: '',
                openid: '',
                phone: '',
                country: '',
                language: '',
                timezone: '',
                createTime: 0,
                lastLoginTime: 0,
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
                if (response && response.success) {
                    this.userInfo = {
                        ...this.userInfo,
                        ...response.data
                    };
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
            // Since we removed the user menu, this method can be empty or removed
            console.log('User menu toggle clicked - menu is currently disabled');
        },

        refreshUserInfo(data) {
            //this.init();
            this.userInfo = {
                ...this.userInfo,
                ...data
            };
        }

    },
    
    async mounted() {
        await this.init();
        nativeIpc.on('onUserInfoUpdated', (data) => {
            console.log('Received user info update event from backend:', data);
            // Refresh user info
            this.refreshUserInfo(data);
        })
    }
};

const app = createApp(Navigation)
    .use(ElementPlus)
    .use(i18n)
    .mount('#app');

