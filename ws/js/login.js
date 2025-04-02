document.addEventListener('DOMContentLoaded', function () {
    document.getElementById('loginForm').addEventListener('submit', async (e) => {
        e.preventDefault();

        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;

        try {
            const response = await fetch('/api/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ username, password })
            });

            const data = await response.json();

            if (data.success) {
                // 获取用户名的首字符作为头像
                const avatarText = username.charAt(0).toUpperCase();
                const userInfo = {
                    ...data.data,
                    avatar: avatarText
                };
                // 保存用户信息到localStorage
                localStorage.setItem('userInfo', JSON.stringify(userInfo));
                // 跳转到主页
                window.location.href = '/home.html';
            } else {
                alert(data.message || '登录失败');
            }
        } catch (error) {
            console.error('Login error:', error);
            alert('登录请求失败');
        }
    });
});