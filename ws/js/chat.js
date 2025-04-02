document.addEventListener('DOMContentLoaded', () => {
    // 检查登录状态
    userManager.checkLogin();
    // 建立WebSocket连接
    connectWebSocket();

    // 消息发送功能
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
});

// WebSocket连接
let ws;
let isConnected = false;

// 添加消息队列和发送状态管理
let messageQueue = [];
let isSending = false;

// 修改分片大小为更大的值
const CHUNK_SIZE = 16384; // 16KB

function connectWebSocket() {
    ws = new WebSocket('ws://172.21.211.240:9999');

    ws.onopen = () => {
        console.log('WebSocket连接已建立');
        isConnected = true;

        // 如果有未发送的消息，重新开始处理队列
        if (messageQueue.length > 0) {
            processMessageQueue();
        }

        // 发送加入房间信息
        const userInfo = userManager.getUserInfo();
        if (userInfo) {
            ws.send(JSON.stringify({
                type: 'join',
                username: userInfo.username,
                room: '公共大厅'
            }));
        }
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        // 只显示类型为 'message' 的消息
        if (data.type === 'message') {
            displayMessage(data);
        }
    };

    ws.onclose = () => {
        console.log('WebSocket连接已关闭');
        isConnected = false;
        // 尝试重新连接
        setTimeout(connectWebSocket, 3000);
    };

    ws.onerror = (error) => {
        console.error('WebSocket错误:', error);
        isConnected = false;
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
    const messageData = {
        type: 'message',
        content: message,
        username: userInfo.username,
        room: '公共大厅',
        timestamp: new Date().toISOString()
    };

    input.value = '';
    messageQueue.push(messageData);
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

function loadRoomMessages(roomName) {
    // 这里添加实际的房间消息加载逻辑
    console.log(`切换到房间: ${roomName}`);
}

function updateRoomTitle(roomName) {
    const header = document.querySelector('.chat-header h2');
    header.innerHTML = `<i class="fas fa-hashtag"></i> ${roomName}`;
}