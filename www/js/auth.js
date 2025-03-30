document.addEventListener('DOMContentLoaded', function () {
    document.getElementById('loginForm').addEventListener('submit', function (e) {
        e.preventDefault();
        const username = document.getElementById('username').value.trim();
        const password = document.getElementById('password').value;

        if (!username) {
            alert('用户名不能为空');
            return;
        }
        if (!password) {
            alert('密码不能为空');
            return;
        }

        // 发送登录请求
        fetch('/api/login', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                username: username,
                password: password
            }),
            credentials: 'same-origin', // 添加凭证设置
            redirect: 'follow'
        })
            .then(response => {
                if (response.redirected) {
                    // 使用 Promise 来控制跳转流程
                    return new Promise(resolve => {
                        // 移除事件监听器
                        document.getElementById('loginForm').removeEventListener('submit', arguments.callee);

                        // 创建一个预加载对象
                        const preloadLink = document.createElement('link');
                        preloadLink.rel = 'preload';
                        preloadLink.as = 'document';
                        preloadLink.href = response.url;
                        document.head.appendChild(preloadLink);

                        // 延迟执行实际跳转
                        setTimeout(() => {
                            window.location.replace(response.url);
                            resolve();
                        }, 100);
                    });
                }
                return response.json();
            })
            .then(data => {
                if (data && !data.success) {
                    alert(data.message || '登录失败');
                }
            })
            .catch(error => {
                console.error('Error:', error);
                alert('网络请求失败');
            });
    });
});