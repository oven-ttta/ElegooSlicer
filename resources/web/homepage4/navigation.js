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
            region: '',
        }
    },
    methods: {
        async init() {
            try {
                const response = await this.ipcRequest('getUserInfo', {});
                console.log('getUserInfo response:', response);
                if (response) {
                    this.userInfo = response;
                    this.region = response.region || '';
                }
            } catch (error) {
                console.error('Failed to get user info:', error);
            }
            
            try {
                const regionResponse = await this.ipcRequest('getRegion', {});
                this.region = regionResponse || '';
                console.log('Region:', this.region);
                
                if (this.region && this.region !== 'CN') {
                    this.currentPage = 'online-models';
                    await this.ipcRequest('navigateToPage', { page: 'online-models' });
                    console.log('Navigated to online-models for region:', this.region);
                }
            } catch (error) {
                console.error('Failed to get region:', error);
                this.region = '';
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


        async onClickUserAvatar() {
            console.log('User avatar clicked');
            try {
                await this.ipcRequest('checkLoginStatus', {});
            } catch (error) {
                console.error('Check login status failed:', error);
                throw error;
            }
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
            } catch (error) {
                console.error('Logout failed:', error);
            }
        },

        async onRelogin() {
            console.log('Re-login clicked');
            try {
                await this.ipcRequest('logout', {});
                this.userInfo.loginStatus = 0;
                this.userInfo.nickname = '';
                this.userInfo.avatar = '';
                this.userInfo.userId = '';
                await this.ipcRequest('showLoginDialog', {});
                await this.init();
            } catch (error) {
                console.error('Re-login dialog failed:', error);
            }
        },

        handleImageError(event) {
            // 当头像图片加载失败时，设置为默认头像
            event.target.src = 'img/default-avatar.svg';
            event.target.onerror = null; // 防止无限循环
        },
    },

    computed: {
        userName(){

            const loginStatus = this.userInfo ? this.userInfo.loginStatus : -1;
            if (loginStatus === -1) {
                return "";//this.$t('homepage.login')+' / '+this.$t('homepage.register');
            } else if (loginStatus === 1) {
                return this.userInfo.nickname || this.userInfo.email.split('@')[0] || this.userInfo.phone;
            }
            else {
                const text = this.userInfo.nickname || this.userInfo.email.split('@')[0] || this.userInfo.phone;
                return text;
            }
        },
        loginStatusStyle() {
            const status = this.userInfo ? this.userInfo.loginStatus : 0;
             if (status === 1) {
                return { color: '#4FD22A' };
            }  else {
                return { color: '#BBBBBB' };
            }
        }
    },

    async mounted() {

        await this.init();
      
        nativeIpc.on('onUserInfoUpdated', (data) => {
            console.log('Received user info update event from backend:', data);
            this.userInfo = data;
            this.region = data.region || '';
        });
        
        nativeIpc.on('onRegionChanged', async () => {
            console.log('Region changed event received');
            try {
                const regionResponse = await this.ipcRequest('getRegion', {});
                this.region = regionResponse || '';
                console.log('Region changed to:', this.region);
                
                // If switched to CN and currently on online-models, navigate to recent
                if (this.region === 'CN' && this.currentPage === 'online-models') {
                    console.log('Switched to CN region, navigating from online-models to recent');
                    await this.navigateToPage('recent');
                }
            } catch (error) {
                console.error('Failed to get region:', error);
                this.region = '';
            }
        });
        
        window.addEventListener('blur', () => {
            console.log('Window lost focus, closing all popovers');
            document.body.click();
        });

        await this.ipcRequest('ready', {});
    }
};

const app = createApp(Navigation)
    .use(ElementPlus)
    .use(i18n)
    .mount('#app');

