document.addEventListener('DOMContentLoaded', async () => {
    const userInfo = userManager.getUserInfo();

    // 严格登录检查（移除 sessionId 校验）
    if (!userInfo || !await userManager.checkLogin()) {
        console.warn('未登录状态，无法获取用户信息');
        return;
    }

    // 显示用户基本信息
    document.getElementById('username').textContent = userInfo.username;
    document.getElementById('email').textContent = userInfo.email;

    // 绑定退出登录按钮事件
    document.getElementById('logoutBtn').addEventListener('click', () => {
        userManager.logout();
    });
});