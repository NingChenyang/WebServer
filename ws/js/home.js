document.addEventListener('DOMContentLoaded', () => {
    // 检查登录状态
    userManager.checkLogin();

    // 获取用户信息
    const userInfo = userManager.getUserInfo();
    if (userInfo) {
        // 更新大头像
        const avatarLarge = document.querySelector('.avatar-large');
        if (avatarLarge) {
            avatarLarge.textContent = userInfo.avatar;
        }

        // 更新用户名和状态
        const username = document.getElementById('currentUsername');
        if (username) {
            username.textContent = userInfo.username;
        }
    }

    // 退出登录功能
    document.getElementById('logoutBtn').addEventListener('click', () => {
        localStorage.removeItem('userInfo');
        window.location.href = 'login.html';
    });

    // 添加聊天室相关功能
    const modal = document.getElementById('addRoomModal');
    const addRoomBtn = document.getElementById('addRoomBtn');
    const closeModal = document.getElementById('closeModal');
    const cancelAdd = document.getElementById('cancelAdd');
    const addRoomForm = document.getElementById('addRoomForm');

    // 打开模态框
    addRoomBtn.addEventListener('click', () => {
        modal.classList.add('show');
    });

    // 关闭模态框
    function closeModalHandler() {
        modal.classList.remove('show');
        addRoomForm.reset();
    }

    closeModal.addEventListener('click', closeModalHandler);
    cancelAdd.addEventListener('click', closeModalHandler);

    // 点击模态框外部关闭
    modal.addEventListener('click', (e) => {
        if (e.target === modal) {
            closeModalHandler();
        }
    });

    // 处理表单提交
    addRoomForm.addEventListener('submit', (e) => {
        e.preventDefault();
        const roomName = document.getElementById('roomName').value;
        const roomDesc = document.getElementById('roomDesc').value;

        // 这里可以添加创建房间的逻辑
        console.log('创建新房间:', { roomName, roomDesc });

        closeModalHandler();
        // 可以添加成功提示
        alert('聊天室创建成功！');
    });

    // 加入聊天室相关功能
    const joinModal = document.getElementById('joinRoomModal');
    const joinRoomBtn = document.getElementById('joinRoomBtn');
    const closeJoinModal = document.getElementById('closeJoinModal');
    const cancelJoin = document.getElementById('cancelJoin');
    const joinRoomForm = document.getElementById('joinRoomForm');

    // 打开加入聊天室模态框
    joinRoomBtn.addEventListener('click', () => {
        joinModal.classList.add('show');
    });

    // 关闭加入聊天室模态框
    function closeJoinModalHandler() {
        joinModal.classList.remove('show');
        joinRoomForm.reset();
    }

    closeJoinModal.addEventListener('click', closeJoinModalHandler);
    cancelJoin.addEventListener('click', closeJoinModalHandler);

    // 点击模态框外部关闭
    joinModal.addEventListener('click', (e) => {
        if (e.target === joinModal) {
            closeJoinModalHandler();
        }
    });

    // 处理加入聊天室表单提交
    joinRoomForm.addEventListener('submit', (e) => {
        e.preventDefault();
        const roomId = document.getElementById('roomId').value.trim();

        if (roomId) {
            // 这里可以添加验证和加入房间的逻辑
            console.log('加入聊天室:', roomId);
            closeJoinModalHandler();
            // 可以添加成功提示并跳转
            alert('成功加入聊天室！');
            window.location.href = `chat.html?roomId=${roomId}`;
        }
    });
});
