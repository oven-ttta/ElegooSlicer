const { createApp } = Vue;
const { ElInput, ElButton, ElPopover, ElDialog, ElTab, ElTabPane, ElSelect, ElOption, ElForm, ElFormItem, ElLoading, ElMessageBox, ElPopconfirm } = ElementPlus;

const Recent = {
    data() {
        return {
            recentFiles: []
        };
    },
    methods: {
        async init() {
            await this.ipcRequest('getRecentFiles', {});
        },
        
        async onClickNewProject() {
            console.log('New project clicked');
            await this.ipcRequest('createNewProject', {});
        },
        
        async onClickOpenProject() {
            console.log('Open project clicked');
            await this.ipcRequest('openProject', {});
        },
        
        async onDeleteAllRecentFiles() {
            console.log('Delete all recent files clicked');
            await this.ipcRequest('clearRecentFiles', {});
        },
        
        async onDeleteRecentFile() {
            console.log('Delete recent file clicked');
            await this.ipcRequest('removeFromRecent', {});
        },
        
        async onExploreRecentFile() {
            console.log('Explore recent file clicked');
            await this.ipcRequest('openFileInExplorer', {});
        },
        
        // IPC Communication methods
        async ipcRequest(method, params = {}, timeout = 10000) {
            try {
                const response = await nativeIpc.request(method, params, timeout);
                return response;
            } catch (error) {
                console.error(`IPC request failed for ${method}:`, error);
                this.showStatusTip(error.message || 'Request failed');
                throw error;
            }
        },

        showStatusTip(message) {
            if (window.ElementPlus && window.ElementPlus.ElMessage) {
                window.ElementPlus.ElMessage.error({
                    message: message,
                    duration: 5000,
                    showClose: true
                });
            }
        },

        async sendMessage(command, data) {
            try {
                return await this.ipcRequest(command, data);
            } catch (error) {
                console.error('Send message failed:', error);
            }
        }
    },
    
    async mounted() {
        // console.log('Recent component mounted');
        // await this.init();
    }
};

const app = createApp(Recent)
    .use(ElementPlus)
    .use(i18n)
    .mount('#app');
