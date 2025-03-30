document.addEventListener('DOMContentLoaded', function () {
    // 获取退出按钮
    const logoutBtn = document.getElementById('logoutBtn');

    // 只在点击退出按钮时处理退出逻辑
    logoutBtn.addEventListener('click', async function (e) {
        e.preventDefault();

        try {
            // 发送退出请求到服务器
            const response = await fetch('/api/logout', {
                method: 'POST',
                credentials: 'same-origin'
            });

            if (response.ok) {
                // 删除认证cookie
                document.cookie = "auth_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT;";

                // 显示退出成功消息
                alert('退出成功');

                // 跳转到登录页
                window.location.replace('/index.html');
            } else {
                alert('退出失败，请重试');
            }
        } catch (error) {
            console.error('Logout error:', error);
            alert('网络错误，请稍后重试');
        }
    });

    // 检查登录状态函数
    function checkAuthStatus() {
        const cookies = document.cookie.split(';');
        // 遍历所有cookie查找auth_token
        const hasAuthToken = cookies.some(cookie => {
            const [key, value] = cookie.split('=').map(part => part.trim());
            return key === 'auth_token' && value === 'valid';
        });

        if (!hasAuthToken) {
            console.log('认证失败，当前cookies:', document.cookie); // 添加调试日志
            window.location.replace('/index.html');
            return false;
        }
        return true;
    }

    // 消息提示函数
    function showMessage(message, type = 'info') {
        // 创建消息容器
        const messageDiv = document.createElement('div');
        messageDiv.className = `message ${type}`;
        messageDiv.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 25px;
            border-radius: 4px;
            animation: slideIn 0.5s ease;
            z-index: 1000;
        `;

        // 根据消息类型设置样式
        switch (type) {
            case 'success':
                messageDiv.style.backgroundColor = '#16a34a';
                messageDiv.style.color = 'white';
                break;
            case 'error':
                messageDiv.style.backgroundColor = '#dc2626';
                messageDiv.style.color = 'white';
                break;
            default:
                messageDiv.style.backgroundColor = '#2563eb';
                messageDiv.style.color = 'white';
        }

        // 设置消息内容
        messageDiv.textContent = message;

        // 添加到页面
        document.body.appendChild(messageDiv);

        // 添加动画样式
        const style = document.createElement('style');
        style.textContent = `
            @keyframes slideIn {
                from { transform: translateX(100%); opacity: 0; }
                to { transform: translateX(0); opacity: 1; }
            }
            @keyframes slideOut {
                from { transform: translateX(0); opacity: 1; }
                to { transform: translateX(100%); opacity: 0; }
            }
        `;
        document.head.appendChild(style);

        // 3秒后移除消息
        setTimeout(() => {
            messageDiv.style.animation = 'slideOut 0.5s ease forwards';
            setTimeout(() => {
                document.body.removeChild(messageDiv);
            }, 500);
        }, 3000);
    }
});