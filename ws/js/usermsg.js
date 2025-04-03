document.addEventListener('DOMContentLoaded', async () => {
    // 获取用户信息
    const userInfo = userManager.getUserInfo();

    // 严格的登录检查
    if (!userInfo || !await userManager.checkLogin()) {
        console.warn('未登录状态，无法获取用户信息');
        window.location.replace('login.html');
        return;
    }

    // 显示用户基本信息
    const usernameElement = document.getElementById('username');
    const emailElement = document.getElementById('email');

    if (usernameElement && userInfo.username) {
        usernameElement.textContent = userInfo.username;
    }

    if (emailElement && userInfo.email) {
        emailElement.textContent = userInfo.email;
    }

    // 绑定退出登录按钮事件
    const logoutBtn = document.getElementById('logoutBtn');
    if (logoutBtn) {
        logoutBtn.addEventListener('click', () => {
            userManager.logout();
            window.location.replace('login.html');
        });
    }
});