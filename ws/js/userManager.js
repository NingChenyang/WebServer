const userManager = {
    // 获取用户信息
    getUserInfo() {
        const userInfo = sessionStorage.getItem('userInfo');
        return userInfo ? JSON.parse(userInfo) : null;
    },

    // 检查用户是否已登录
    isLoggedIn() {
        return this.getUserInfo() !== null;
    },

    // 更新用户信息
    updateUserInfo(userInfo) {
        if (!userInfo || !userInfo.username) {
            console.error('无效的用户信息');
            return;
        }
        sessionStorage.setItem('userInfo', JSON.stringify(userInfo));
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

    // 检查登录状态的方法
    async checkLogin() {
        try {
            const userInfo = this.getUserInfo();
            if (!userInfo || !userInfo.username) {
                throw new Error('No user info found');
            }

            const response = await fetch('/api/user/verify', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                credentials: 'include'
            });

            if (!response.ok) {
                throw new Error('Token verification failed');
            }

            return true;
        } catch (error) {
            console.error('验证失败:', error);
            this.clearUserData();
            window.location.replace('login.html');
            return false;
        }
    },

    // 清除用户数据
    clearUserData() {
        sessionStorage.removeItem('userInfo');
        sessionStorage.removeItem('sessionId'); // 新增：清除 sessionId
        document.cookie = "auth_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT;";
    },

    // 新增：退出登录方法
    logout() {
        this.clearUserData();
        // 确保清除所有会话数据
        sessionStorage.clear();
        localStorage.clear();
        document.cookie = "auth_token=; expires=Thu, 01 Jan 1970 00:00:00 GMT; path=/;";
        window.location.replace('login.html');
    }
};

// 在页面加载时自动更新用户信息
document.addEventListener('DOMContentLoaded', () => {
    userManager.updateUI();
});
