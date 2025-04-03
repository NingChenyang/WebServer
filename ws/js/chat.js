document.addEventListener('DOMContentLoaded', async () => {
    // 使用 await 等待验证完成
    if (!await userManager.checkLogin()) {
        return;
    }

    // 获取最新的用户信息
    const userInfo = userManager.getUserInfo();
    if (!userInfo) {
        window.location.replace('login.html');
        return;
    }

    // 更新页面上的用户信息显示
    document.querySelectorAll('.avatar').forEach(avatar => {
        avatar.textContent = userInfo.username.charAt(0).toUpperCase();
    });
    document.getElementById('currentUsername').textContent = userInfo.username;

    // 继续初始化WebSocket和其他功能
    connectWebSocket();
    // 初始化获取聊天室列表
    fetchChatRooms();

    const sendBtn = document.getElementById('sendBtn');
    const messageInput = document.getElementById('messageInput');

    sendBtn.addEventListener('click', sendMessage);
    messageInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') sendMessage();
    });

    // 修复聊天室切换
    const roomItems = document.querySelectorAll('.room-item');
    roomItems.forEach(item => {
        item.addEventListener('click', function () {
            // 移除之前的active类
            roomItems.forEach(room => room.classList.remove('active'));
            // 添加新的active类
            this.classList.add('active');

            // 更新聊天室标题
            const roomName = this.textContent.trim();
            updateRoomTitle(roomName);
            loadRoomMessages(roomName);
        });
    });

    // 修改退出处理
    window.addEventListener('beforeunload', () => {
        leaveCurrentRoom();
        if (ws) {
            ws.close();
        }
        // 不在这里处理cookie，让登出按钮处理
    });
});

// WebSocket连接
let ws;
let isConnected = false;

// 添加消息队列和发送状态管理
let messageQueue = [];
let isSending = false;

// 修改分片大小为更大的值
const CHUNK_SIZE = 16384; // 16KB

// 在文件顶部添加全局集合，用于记录本地发送的消息 ID
let localMessageIds = new Set();

function connectWebSocket() {
    const userInfo = userManager.getUserInfo();
    if (!userInfo) {
        console.error('未找到用户信息，无法建立WebSocket连接');
        return;
    }

    ws = new WebSocket('ws://172.21.211.240:9999');

    // 添加心跳定时器
    let heartbeatInterval;

    ws.onopen = () => {
        console.log('WebSocket连接已建立');
        isConnected = true;

        // 启动心跳
        heartbeatInterval = setInterval(() => {
            if (isConnected) {
                ws.send(JSON.stringify({
                    type: 'heartbeat',
                    timestamp: new Date().getTime()
                }));
            }
        }, 30000); // 每30秒发送一次心跳

        // 如果有未发送的消息，重新开始处理队列
        if (messageQueue.length > 0) {
            processMessageQueue();
        }

        // 发送加入房间信息并加载历史消息
        const userInfo = userManager.getUserInfo();
        if (userInfo) {
            ws.send(JSON.stringify({
                type: 'join',
                username: userInfo.username,
                room: currentRoom
            }));
            loadRoomMessages(currentRoom); // 新增：加载当前房间历史记录
        }
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        // 处理心跳响应
        if (data.type === 'heartbeat_ack') {
            console.log('收到心跳响应');
            return;
        }
        // 只显示类型为 'message' 的消息
        if (data.type === 'message') {
            displayMessage(data);
        }
    };

    ws.onclose = () => {
        console.log('WebSocket连接已关闭');
        isConnected = false;
        // 清除心跳定时器
        if (heartbeatInterval) {
            clearInterval(heartbeatInterval);
        }
        // 尝试重新连接
        setTimeout(connectWebSocket, 3000);
    };

    ws.onerror = (error) => {
        console.error('WebSocket错误:', error);
        isConnected = false;
        // 清除心跳定时器
        if (heartbeatInterval) {
            clearInterval(heartbeatInterval);
        }
    };
}

function sendMessage() {
    const input = document.getElementById('messageInput');
    const message = input.value;

    if (!message || !message.trim().length || !isConnected) {
        if (!isConnected) {
            alert('与服务器的连接已断开，请稍后重试');
        }
        return;
    }

    const userInfo = userManager.getUserInfo();
    // 生成唯一消息 ID
    const msgId = `${userInfo.username}-${Date.now()}`;
    const messageData = {
        type: 'message',
        content: message,
        username: userInfo.username,
        room: currentRoom,  // 使用当前房间变量而不是硬编码
        timestamp: new Date().toISOString(),
        local: true,  // 新增 local 标记，表示这是本地发送的消息
        id: msgId     // 新增消息 ID 属性
    };

    input.value = '';
    messageQueue.push(messageData);
    // 发送后立即显示
    displayMessage(messageData);
    // 记录本地消息的 ID，防止后续重复显示
    localMessageIds.add(msgId);
    processMessageQueue();
}

// 优化消息队列处理函数
const MAX_FRAME_SIZE = 65535; // WebSocket帧的最大大小

async function processMessageQueue() {
    if (isSending || messageQueue.length === 0 || !isConnected) {
        return;
    }

    isSending = true;
    const currentMessage = messageQueue[0];

    try {
        const messageStr = JSON.stringify(currentMessage);
        const encoder = new TextEncoder();
        const messageBytes = encoder.encode(messageStr);

        // 如果消息超过最大帧大小，进行分片发送
        if (messageBytes.length > MAX_FRAME_SIZE) {
            const chunks = Math.ceil(messageBytes.length / MAX_FRAME_SIZE);

            for (let i = 0; i < chunks; i++) {
                const start = i * MAX_FRAME_SIZE;
                const end = Math.min((i + 1) * MAX_FRAME_SIZE, messageBytes.length);
                const chunkBytes = messageBytes.slice(start, end);
                const chunkText = new TextDecoder().decode(chunkBytes);

                const chunkData = {
                    type: 'message',
                    content: chunkText,
                    username: currentMessage.username,
                    room: currentMessage.room,
                    timestamp: currentMessage.timestamp,
                    isChunk: true,
                    chunkInfo: {
                        index: i + 1,
                        total: chunks,
                        messageId: `${currentMessage.username}-${currentMessage.timestamp}`
                    }
                };

                await sendWithRetry(JSON.stringify(chunkData));
                // 添加短暂延迟，避免发送过快
                await new Promise(resolve => setTimeout(resolve, 50));
            }
        } else {
            // 消息长度在允许范围内，直接发送
            await sendWithRetry(messageStr);
        }

        messageQueue.shift();
    } catch (error) {
        console.error('发送消息失败:', error);
        if (error.message.includes('EPOLLHUP')) {
            reconnectWebSocket();
        }
    } finally {
        isSending = false;
        if (messageQueue.length > 0) {
            setTimeout(processMessageQueue, 100);
        }
    }
}

// 改进重试机制
async function sendWithRetry(data, maxRetries = 3) {
    let lastError;
    for (let i = 0; i < maxRetries; i++) {
        try {
            if (!isConnected) {
                await new Promise(resolve => setTimeout(resolve, 1000));
                continue;
            }

            ws.send(data);
            return;

        } catch (error) {
            lastError = error;
            await new Promise(resolve => setTimeout(resolve, 1000 * (i + 1)));
        }
    }
    throw lastError || new Error('发送失败');
}

function reconnectWebSocket() {
    if (ws) {
        ws.close();
    }
    isConnected = false;
    console.log('正在重新连接...');
    setTimeout(connectWebSocket, 1000);
}

// 修改消息接收逻辑，处理分片消息
let messageChunks = new Map(); // 存储分片消息

function displayMessage(data) {
    const currentUser = userManager.getUserInfo();
    // 如果消息来自当前用户且消息 ID 已记录，则跳过显示（避免重复）
    if (currentUser && data.username === currentUser.username && data.id && localMessageIds.has(data.id)) {
        return;
    }
    if (data.isChunk) {
        const key = data.chunkInfo.messageId;
        if (!messageChunks.has(key)) {
            messageChunks.set(key, new Array(data.chunkInfo.total));
        }

        const chunks = messageChunks.get(key);
        chunks[data.chunkInfo.index - 1] = data.content;

        // 检查是否所有分片都收到
        if (!chunks.includes(undefined)) {
            data.content = chunks.join('');
            data.isChunk = false;
            messageChunks.delete(key);
            displayCompleteMessage(data);
        }
    } else {
        displayCompleteMessage(data);
    }
}

function displayCompleteMessage(data) {
    const messageContainer = document.getElementById('messageContainer');
    const userInfo = userManager.getUserInfo();
    const isSentByMe = data.username === userInfo.username;

    const messageElement = document.createElement('div');
    messageElement.className = `message-item${isSentByMe ? ' sent' : ''}`;

    const messageContent = data.content ?
        data.content.replace(/ /g, '&nbsp;') : '';

    messageElement.innerHTML = `
        <div class="avatar">${data.username ? data.username.charAt(0).toUpperCase() : 'U'}</div>
        <div class="message-content">
            <div class="message-header">
                <span class="username">${data.username || 'Unknown'}</span>
                <span class="timestamp">${formatTime(data.timestamp)}</span>
            </div>
            <div class="message-bubble">
                <p style="white-space: pre-wrap; word-wrap: break-word;">${messageContent}</p>
            </div>
        </div>
    `;

    messageContainer.appendChild(messageElement);

    // 检查是否需要自动滚动
    const shouldScroll = messageContainer.scrollTop + messageContainer.clientHeight >= messageContainer.scrollHeight - 100;

    // 添加消息后滚动到底部
    if (shouldScroll) {
        scrollToBottom(messageContainer);
    }
}

// 添加平滑滚动函数
function scrollToBottom(element) {
    const start = element.scrollTop;
    const end = element.scrollHeight;
    const duration = 300; // 滚动动画持续时间（毫秒）
    const startTime = performance.now();

    function scroll() {
        const currentTime = performance.now();
        const progress = (currentTime - startTime) / duration;

        if (progress < 1) {
            element.scrollTop = start + (end - start) * easeOutCubic(progress);
            requestAnimationFrame(scroll);
        } else {
            element.scrollTop = end;
        }
    }

    // 缓动函数
    function easeOutCubic(t) {
        return 1 - Math.pow(1 - t, 3);
    }

    requestAnimationFrame(scroll);
}

function formatTime(timestamp) {
    try {
        return timestamp ? new Date(timestamp).toLocaleString() : new Date().toLocaleString();
    } catch (error) {
        return new Date().toLocaleString();
    }
}

// 在文件顶部添加
let currentRoom = '公共大厅'; // 当前聊天室

// 添加获取聊天室列表函数
// 修改后的获取聊天室列表请求
async function fetchChatRooms() {
    try {
        const userInfo = userManager.getUserInfo();
        if (!userInfo || !userInfo.username) {
            console.error('用户未登录');
            return; // 不要立即跳转，让userManager处理登录
        }

        // 获取authToken，但不立即验证
        const authToken = document.cookie.split('; ')
            .find(row => row.startsWith('auth_token='))
            ?.split('=')[1];

        const response = await fetch(`/api/chatrooms?username=${encodeURIComponent(userInfo.username)}`, {
            headers: {
                'Cookie': `auth_token=${authToken || ''}` // 即使没有token也发送请求
            }
        });

        const result = await response.json();
        if (result.success) {
            renderRoomList(result.data);
        } else {
            console.error('获取聊天室列表失败:', result.message);
            // 如果是认证问题，让userManager处理
            if (response.status === 401) {
                userManager.handleAuthError();
            }
        }
    } catch (error) {
        console.error('获取聊天室列表失败:', error);
    }
}

// 渲染聊天室列表
// 修改渲染房间列表函数
function renderRoomList(rooms) {
    const roomList = document.getElementById('roomList');
    roomList.innerHTML = '';

    rooms.forEach(room => {
        const li = document.createElement('li');
        // 修改类名判断逻辑
        li.className = `room-item ${room === currentRoom ? 'active' : ''}`; // 直接比较字符串
        li.innerHTML = `<i class="fas fa-hashtag"></i> ${room}`; // 直接使用字符串值
        li.addEventListener('click', () => switchRoom(room));
        roomList.appendChild(li);
    });
}

// 修改房间切换判断逻辑
function switchRoom(roomName) {
    if (roomName === currentRoom) return;

    // 离开当前房间
    leaveCurrentRoom();

    currentRoom = roomName;
    updateRoomTitle(roomName);
    clearMessages();
    loadRoomMessages(roomName);

    // 更新活动状态
    document.querySelectorAll('.room-item').forEach(item => {
        item.classList.toggle('active', item.textContent.trim() === roomName);
    });

    // 通知服务器加入新房间
    if (isConnected) {
        const userInfo = userManager.getUserInfo();
        ws.send(JSON.stringify({
            type: 'join',
            username: userInfo.username,
            room: roomName
        }));
    }
}

// 清空消息容器
function clearMessages() {
    document.getElementById('messageContainer').innerHTML = '';
}

// 修改 loadRoomMessages 函数以接收和加载历史聊天数据
async function loadRoomMessages(roomName) {
    clearMessages(); // 清空当前消息容器
    try {
        const response = await fetch(`/api/messages?room=${encodeURIComponent(roomName)}`);
        if (!response.ok) {
            console.error('获取历史消息失败，状态码：', response.status);
            return;
        }
        const result = await response.json();
        const messages = result.data || result; // 兼容返回结果有 data 字段或直接为数组
        messages.forEach(message => {
            // 新增：转换历史消息字段（服务端返回的字段为 message 和 sender）
            if (message.message && message.sender && !message.content) {
                message.content = message.message;
                message.username = message.sender;
            }
            displayCompleteMessage(message);
        });
    } catch (error) {
        console.error('加载历史聊天数据出错:', error);
    }
}

function leaveCurrentRoom() {
    if (isConnected && currentRoom) {
        const userInfo = userManager.getUserInfo();
        ws.send(JSON.stringify({
            type: 'leave',
            username: userInfo.username,
            room: currentRoom
        }));
    }
}

// 在DOMContentLoaded事件监听器中添加
window.addEventListener('beforeunload', () => {
    leaveCurrentRoom();
    // 关闭WebSocket连接
    if (ws) {
        ws.close();
    }
});