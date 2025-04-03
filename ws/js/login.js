document.addEventListener('DOMContentLoaded', function () {
    // 创建错误信息容器
    const errorContainer = document.createElement('div');
    errorContainer.className = 'error-message';
    const form = document.getElementById('loginForm');
    form.parentNode.insertBefore(errorContainer, form.nextSibling);

    // 显示错误信息的函数
    function showError(message) {
        errorContainer.textContent = message;
        errorContainer.style.display = 'block';
        setTimeout(() => {
            errorContainer.style.display = 'none';
        }, 3000);
    }

    document.getElementById('loginForm').addEventListener('submit', async (e) => {
        e.preventDefault();
        const submitBtn = form.querySelector('button[type="submit"]');
        submitBtn.disabled = true;
        submitBtn.textContent = '登录中...';

        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;

        try {
            const response = await fetch('/api/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ username, password }),
                credentials: 'include'
            });

            const data = await response.json();
            if (data.success) {
                const userInfo = {
                    ...data.data,
                    avatar: data.data.username.charAt(0).toUpperCase()
                };
                // 新增：存储 sessionId（如果服务端返回）
                if (data.data.sessionId) {
                    sessionStorage.setItem('sessionId', data.data.sessionId);
                }
                sessionStorage.setItem('userInfo', JSON.stringify(userInfo));
                window.location.href = '/home.html';
            } else {
                showError(data.message || '用户名或密码错误');
            }
        } catch (error) {
            console.error('登录请求出错:', error);
            showError('网络请求失败，请稍后重试');
        } finally {
            submitBtn.disabled = false;
            submitBtn.textContent = '登录';
        }
    });
});