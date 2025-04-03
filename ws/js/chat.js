// 优化格式：重新整理缩进、空格和模块分隔

document.addEventListener('DOMContentLoaded', async () => {
    // 检查登录并更新用户信息
    if (!await userManager.checkLogin()) return;
    const userInfo = userManager.getUserInfo();
    if (!userInfo) {
        window.location.replace('login.html');
        return;
    }
    document.querySelectorAll('.avatar').forEach(avatar => {
        avatar.textContent = userInfo.username.charAt(0).toUpperCase();
    });
    document.getElementById('currentUsername').textContent = userInfo.username;

    // 初始化WebSocket和聊天室列表
    connectWebSocket();
    fetchChatRooms();

    // 设置消息发送
    const sendBtn = document.getElementById('sendBtn');
    const messageInput = document.getElementById('messageInput');
    sendBtn.addEventListener('click', sendMessage);
    messageInput.addEventListener('keypress', e => { if (e.key === 'Enter') sendMessage(); });

    // 聊天室切换逻辑
    document.querySelectorAll('.room-item').forEach(item => {
        item.addEventListener('click', function () {
            document.querySelectorAll('.room-item').forEach(room => room.classList.remove('active'));
            this.classList.add('active');
            const roomName = this.textContent.trim();
            updateRoomTitle(roomName);
            loadRoomMessages(roomName);
        });
    });

    // 注销处理
    window.addEventListener('beforeunload', () => {
        leaveCurrentRoom();
        if (ws) ws.close();
    });
});

// ------------------ WebSocket与消息处理 ------------------

let ws;
let isConnected = false;
let messageQueue = [];
let isSending = false;
const CHUNK_SIZE = 16384; // 16KB
let localMessageIds = new Set(); // 记录本地发送的消息ID

function connectWebSocket() {
    const userInfo = userManager.getUserInfo();
    if (!userInfo) {
        console.error('未找到用户信息，无法建立WebSocket连接');
        return;
    }
    ws = new WebSocket('ws://172.21.211.240:9999');

    let heartbeatInterval;
    ws.onopen = () => {
        console.log('WebSocket连接已建立');
        isConnected = true;
        heartbeatInterval = setInterval(() => {
            if (isConnected) {
                ws.send(JSON.stringify({ type: 'heartbeat', timestamp: Date.now() }));
            }
        }, 30000);
        if (messageQueue.length > 0) processMessageQueue();
        if (userManager.getUserInfo()) {
            ws.send(JSON.stringify({ type: 'join', username: userManager.getUserInfo().username, room: currentRoom }));
            loadRoomMessages(currentRoom);
        }
    };
    ws.onmessage = event => {
        const data = JSON.parse(event.data);
        if (data.type === 'heartbeat_ack') {
            console.log('收到心跳响应');
            return;
        }
        if (data.type === 'message') displayMessage(data);
    };
    ws.onclose = () => {
        console.log('WebSocket连接已关闭');
        isConnected = false;
        if (heartbeatInterval) clearInterval(heartbeatInterval);
        setTimeout(connectWebSocket, 3000);
    };
    ws.onerror = error => {
        console.error('WebSocket错误:', error);
        isConnected = false;
        if (heartbeatInterval) clearInterval(heartbeatInterval);
    };
}

function sendMessage() {
    const input = document.getElementById('messageInput');
    const message = input.value;
    if (!message || !message.trim().length || !isConnected) {
        if (!isConnected) alert('与服务器的连接已断开，请稍后重试');
        return;
    }
    const userInfo = userManager.getUserInfo();
    const msgId = `${userInfo.username}-${Date.now()}`;
    const messageData = {
        type: 'message',
        content: message,
        username: userInfo.username,
        room: currentRoom,
        timestamp: new Date().toISOString(),
        local: true,
        id: msgId
    };
    input.value = '';
    messageQueue.push(messageData);
    displayMessage(messageData);
    localMessageIds.add(msgId);
    processMessageQueue();
}

const MAX_FRAME_SIZE = 65535; // WebSocket帧的最大大小
async function processMessageQueue() {
    if (isSending || messageQueue.length === 0 || !isConnected) return;
    isSending = true;
    const currentMessage = messageQueue[0];
    try {
        const messageStr = JSON.stringify(currentMessage);
        const encoder = new TextEncoder();
        const messageBytes = encoder.encode(messageStr);

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
                    chunkInfo: { index: i + 1, total: chunks, messageId: `${currentMessage.username}-${currentMessage.timestamp}` }
                };
                await sendWithRetry(JSON.stringify(chunkData));
                await new Promise(resolve => setTimeout(resolve, 50));
            }
        } else {
            await sendWithRetry(messageStr);
        }
        messageQueue.shift();
    } catch (error) {
        console.error('发送消息失败:', error);
        if (error.message.includes('EPOLLHUP')) reconnectWebSocket();
    } finally {
        isSending = false;
        if (messageQueue.length > 0) setTimeout(processMessageQueue, 100);
    }
}

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
    if (ws) ws.close();
    isConnected = false;
    console.log('正在重新连接...');
    setTimeout(connectWebSocket, 1000);
}

let messageChunks = new Map(); // 存储分片消息
function displayMessage(data) {
    const currentUser = userManager.getUserInfo();
    if (currentUser && data.username === currentUser.username && data.id && localMessageIds.has(data.id)) return;

    if (data.isChunk) {
        const key = data.chunkInfo.messageId;
        if (!messageChunks.has(key)) messageChunks.set(key, new Array(data.chunkInfo.total));
        const chunks = messageChunks.get(key);
        chunks[data.chunkInfo.index - 1] = data.content;
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
    const messageContent = data.content ? data.content.replace(/ /g, '&nbsp;') : '';
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
        </div>`;
    messageContainer.appendChild(messageElement);
    const shouldScroll = messageContainer.scrollTop + messageContainer.clientHeight >= messageContainer.scrollHeight - 100;
    if (shouldScroll) scrollToBottom(messageContainer);
}

function scrollToBottom(element) {
    const start = element.scrollTop, end = element.scrollHeight, duration = 300, startTime = performance.now();
    function scroll() {
        const progress = (performance.now() - startTime) / duration;
        if (progress < 1) {
            element.scrollTop = start + (end - start) * easeOutCubic(progress);
            requestAnimationFrame(scroll);
        } else {
            element.scrollTop = end;
        }
    }
    function easeOutCubic(t) { return 1 - Math.pow(1 - t, 3); }
    requestAnimationFrame(scroll);
}

function formatTime(timestamp) {
    try {
        return timestamp ? new Date(timestamp).toLocaleString() : new Date().toLocaleString();
    } catch (error) {
        return new Date().toLocaleString();
    }
}

// ------------------ 聊天室列表及切换 ------------------

let currentRoom = '公共大厅';
async function fetchChatRooms() {
    try {
        const userInfo = userManager.getUserInfo();
        if (!userInfo || !userInfo.username) {
            console.error('用户未登录');
            return;
        }
        const authToken = document.cookie.split('; ').find(row => row.startsWith('auth_token='))?.split('=')[1];
        const response = await fetch(`/api/chatrooms?username=${encodeURIComponent(userInfo.username)}`, {
            headers: { 'Cookie': `auth_token=${authToken || ''}` }
        });
        const result = await response.json();
        if (result.success) {
            renderRoomList(result.data);
        } else {
            console.error('获取聊天室列表失败:', result.message);
            if (response.status === 401) userManager.handleAuthError();
        }
    } catch (error) {
        console.error('获取聊天室列表失败:', error);
    }
}

function renderRoomList(rooms) {
    const roomList = document.getElementById('roomList');
    roomList.innerHTML = '';
    rooms.forEach(room => {
        const li = document.createElement('li');
        li.className = `room-item ${room === currentRoom ? 'active' : ''}`;
        li.innerHTML = `<i class="fas fa-hashtag"></i> ${room}`;
        li.addEventListener('click', () => switchRoom(room));
        roomList.appendChild(li);
    });
}

function switchRoom(roomName) {
    if (roomName === currentRoom) return;
    leaveCurrentRoom();
    currentRoom = roomName;
    updateRoomTitle(roomName);
    clearMessages();
    loadRoomMessages(roomName);
    document.querySelectorAll('.room-item').forEach(item => {
        item.classList.toggle('active', item.textContent.trim() === roomName);
    });
    if (isConnected) {
        const userInfo = userManager.getUserInfo();
        ws.send(JSON.stringify({ type: 'join', username: userInfo.username, room: roomName }));
    }
}

function clearMessages() {
    document.getElementById('messageContainer').innerHTML = '';
}

async function loadRoomMessages(roomName) {
    clearMessages();
    try {
        const response = await fetch(`/api/messages?room=${encodeURIComponent(roomName)}`);
        if (!response.ok) {
            console.error('获取历史消息失败，状态码：', response.status);
            return;
        }
        const result = await response.json();
        const messages = result.data || result;
        messages.forEach(message => {
            // 转换服务端返回的字段
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
        ws.send(JSON.stringify({ type: 'leave', username: userInfo.username, room: currentRoom }));
    }
}

window.addEventListener('beforeunload', () => {
    leaveCurrentRoom();
    if (ws) ws.close();
});