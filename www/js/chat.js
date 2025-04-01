var ws = null;
var messageArea = document.getElementById('messageArea');

function appendMessage(message, type = 'received') {
    const messageDiv = document.createElement('div');
    messageDiv.className = `chat-message ${type}`;
    messageDiv.textContent = message;
    messageArea.appendChild(messageDiv);
    messageArea.scrollTop = messageArea.scrollHeight;
}

function connectWebSocket() {
    if (ws != null) {
        appendMessage("已经连接了服务器", "received");
        return;
    }

    ws = new WebSocket('ws://172.21.211.240:9999');

    ws.onopen = function () {
        appendMessage("成功连接到服务器", "received");
        document.querySelector('.chat-btn.connect').disabled = true;
        document.querySelector('.chat-btn.disconnect').disabled = false;
    };

    ws.onmessage = function (evt) {
        appendMessage(evt.data, "received");
    };

    ws.onclose = function (event) {
        appendMessage(`与服务器断开连接 - 关闭码: ${event.code} 原因: ${event.reason}`, "received");
        ws = null;
        document.querySelector('.chat-btn.connect').disabled = false;
        document.querySelector('.chat-btn.disconnect').disabled = true;
    };

    ws.onerror = function (event) {
        appendMessage("连接错误，请确保WebSocket服务器已启动(端口9999)", "received");
        console.error("WebSocket错误:", event);
        ws = null;
    };
}

function sendMessage() {
    if (!ws) {
        appendMessage("请先连接服务器！", "received");
        return;
    }

    var messageInput = document.getElementById('messageInput');
    var message = messageInput.value;

    if (message) {
        ws.send(message);
        appendMessage(message, "sent");
        messageInput.value = '';
    }
}

function closeWebSocket() {
    if (ws) {
        ws.close();
        ws = null;
    }
}

// 添加回车发送功能
document.getElementById('messageInput').addEventListener('keypress', function (e) {
    if (e.key === 'Enter') {
        sendMessage();
    }
});
