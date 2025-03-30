document.addEventListener('DOMContentLoaded', function () {
    const form = document.getElementById('registerForm');
    const errorContainer = document.createElement('div');
    errorContainer.className = 'error-message';
    form.parentNode.insertBefore(errorContainer, form.nextSibling);

    form.addEventListener('submit', async function (e) {
        e.preventDefault();
        const submitBtn = form.querySelector('button[type="submit"]');
        submitBtn.disabled = true;
        submitBtn.textContent = '注册中...';

        // 获取表单数据
        const formData = {
            username: document.getElementById('regUsername').value.trim(),
            email: document.getElementById('email').value.trim(),
            password: document.getElementById('regPassword').value,
            confirmPassword: document.getElementById('confirmPassword').value
        };

        // 客户端验证
        errorContainer.textContent = '';
        if (!formData.username || !formData.email || !formData.password) {
            showError('所有字段均为必填项');
            return;
        }
        if (formData.password !== formData.confirmPassword) {
            showError('两次输入的密码不一致');
            return;
        }
        if (!validateEmail(formData.email)) {
            showError('邮箱格式不正确');
            return;
        }

        try {
             fetch('/api/register', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    username: formData.username,
                    email: formData.email,
                    password: formData.password
                })
            })

           .then(response => {
               if (response.redirected) {
                   showSuccess("正在跳转...");
                   window.location.href = response.url;
                   return;
               }
               return response.json();
           })
                 .then(data => {
                     if (data && !data.success) {
                         showError(data.message || '注册失败');
                     }
                 })
            
        } catch (error) {
            console.error('注册请求失败:', error);
            showError('网络请求失败，请稍后重试');
        } finally {
            submitBtn.disabled = false;
            submitBtn.textContent = '注册';
        }
    });

    function showError(message) {
        errorContainer.textContent = message;
        errorContainer.style.display = 'block';
        setTimeout(() => {
            errorContainer.style.display = 'none';
        }, 3000);
    }

    function validateEmail(email) {
        return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);
    }
    // 添加成功提示函数
    function showSuccess(message) {
        errorContainer.textContent = message;
        errorContainer.style.color = '#4CAF50'; // 成功消息使用绿色
        errorContainer.style.display = 'block';
    }
});
