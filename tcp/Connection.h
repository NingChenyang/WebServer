#pragma once

#include <memory>
#include "Callbacks.h"
#include "Buffer.h"
#include "Socket.h"
#include "TimeStamp.h"
#include <sys/socket.h>
#include <any>
#include <mutex>
class EventLoop;
class Channel;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	enum class StateE
	{
		kDisconnected,
		kConnecting,
		kConnected,
		kDisconnecting
	};

public:
	Connection(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);

	~Connection();

	EventLoop *GetLoop() const { return loop_; }

	void SetOnMessageCallback(const OnMessageCallback &cb);
	void SetOnClosedCallback(const OnClosedCallback &cb);
	void SetOnSentCallback(const OnSentCallback &cb);
	void SetOnConnectedCallback(const OnConnectedCallback &cb);

	void SetCloseCallback(const CloseCallback &cb)
	{
		closeCallback_ = cb;
	}

	bool Connected() const { return state_ == StateE::kConnected; }
	bool Disconnected() const { return state_ == StateE::kDisconnected; }
	void SetState(StateE state) { state_ = state; }
	void Send(Buffer *message);
	void Send(const void *message, size_t len);
	void Send(const std::string &messgage);
	void SendInLoop(const void *message, size_t len);
	// 建立连接
	void ConnectEstablished();
	// 销毁连接
	void ConnectDestroyed();

	bool Timeout(time_t now, int val);

	// Advanced interface
	Buffer *InputBuffer()
	{
		return &input_buffer_;
	}

	Buffer *OutputBuffer()
	{
		return &output_buffer_;
	}

	void ShutDown();

	void ForceClose();

	void ShutdownInLoop(); // Declare the ShutdownInLoop method

	const InetAddress &localAddress() const { return localAddr_; }
	const InetAddress &peerAddress() const { return peerAddr_; }

	int fd() const { return socket_->fd(); }
	// void EnableWriting();

	void SetContext(const std::any &context);
	const std::any &GetContext() const;
	std::any &GetMutableContext();

	std::mutex &GetMutex() { return mutex_; }

private:
	void HandleRead();
	void HandleWrite();
	void HandleClose();
	void HandleError();

private:
	EventLoop *loop_;

	StateE state_; // use atomic variable

	std::unique_ptr<Socket> socket_;
	std::unique_ptr<Channel> channel_;

	const InetAddress localAddr_;
	const InetAddress peerAddr_;

	CloseCallback closeCallback_;

	OnMessageCallback messageCallback_;
	OnClosedCallback closedCallback_;
	OnSentCallback sentCallback_;
	OnConnectedCallback connectedCallback_;

	Buffer input_buffer_;
	Buffer output_buffer_;
	TimeStamp last_time_;
	std::any context_;
	std::mutex mutex_; // 保护Connection对象的访问
};
