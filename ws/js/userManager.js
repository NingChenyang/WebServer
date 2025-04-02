// 用户信息管理工具
const userManager = {
    // 获取用户信息
    getUserInfo() {
        const userInfo = localStorage.getItem('userInfo');
        return userInfo ? JSON.parse(userInfo) : null;
    },

    // 检查用户是否已登录
    isLoggedIn() {
        return this.getUserInfo() !== null;
    },

    // 用户登出
    logout() {
        localStorage.removeItem('userInfo');
        fetch('/api/logout', { method: 'POST' })
            .finally(() => {
                window.location.href = '/login.html';
            });
    },

    // 更新用户信息
    updateUserInfo(userInfo) {
        localStorage.setItem('userInfo', JSON.stringify(userInfo));
    },

    // 更新用户界面
    updateUI() {
        const userInfo = this.getUserInfo();
        if (!userInfo) {
            window.location.replace('login.html');
            return;
        }

        // 更新用户名显示
        const usernameElements = document.querySelectorAll('#currentUsername');
        usernameElements.forEach(elem => {
            if (elem) elem.textContent = userInfo.username;
        });

        // 更新邮箱显示
        const emailElement = document.getElementById('email');
        if (emailElement) emailElement.textContent = userInfo.email;

        // 更新头像显示
        const avatarElements = document.querySelectorAll('.avatar, .avatar-large');
        avatarElements.forEach(elem => {
            if (elem) elem.textContent = userInfo.avatar;
        });
    },

    // 改进检查登录状态的方法
    checkLogin() {
        const userInfo = this.getUserInfo();
        if (!userInfo) {
            // 检查cookie
            const cookies = document.cookie.split(';');
            const hasAuthToken = cookies.some(cookie => {
                const [key, value] = cookie.split('=').map(part => part.trim());
                return key === 'auth_token' && value === 'valid';
            });

            if (!hasAuthToken) {
                window.location.replace('login.html');
                return false;
            }
            return true;
        }
        return true;
    }
};

// 在页面加载时自动更新用户信息
document.addEventListener('DOMContentLoaded', () => {
    userManager.updateUI();
});
