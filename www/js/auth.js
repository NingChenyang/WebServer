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
            })
        })
            .then(response => {
                if (response.redirected) {
                    window.location.href = response.url;
                    return;
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