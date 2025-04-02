document.addEventListener('DOMContentLoaded', () => {
    // 检查登录状态
    userManager.checkLogin();

    // 获取和显示用户信息
    const userInfo = userManager.getUserInfo();
    if (userInfo) {
        document.getElementById('username').textContent = userInfo.username;
        document.getElementById('email').textContent = userInfo.email;
    }

    // 退出登录功能
    document.getElementById('logoutBtn').addEventListener('click', () => {
        localStorage.removeItem('userInfo');
        window.location.href = 'login.html';
    });
});