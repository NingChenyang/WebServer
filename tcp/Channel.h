#pragma once
#include"Epoll.h"
#include"EventLoop.h"
#include <functional>
#include<memory>

using EventCallback = std::function<void()>;
class Epoll;
class EventLoop;
class Channel
{
public:
	Channel(int fd, EventLoop* loop);
	~Channel();
	void SetEvents(uint32_t event);
	uint32_t Events();
	void SetRevents(uint32_t revent);
	uint32_t Revents();

	bool IsInEpoll();
	void SetInEpoll(bool in);
	int fd()const;


	void SetReadCallback(EventCallback cb);
	void SetWriteCallback(EventCallback cb); 
	void SetCloseCallback(EventCallback cb); 
	void SetErrorCallback(EventCallback cb); 

	void SetET();

	void EnableReading();	//ע��ɶ��¼�
	void DisableReading();//ע���ɶ��¼�
	void EnableWriting();//ע���д�¼�
	void DisableWriting();//ע����д�¼�
	void DisableAll();//ע�������¼�

	//����fd��ǰ���¼�״̬
	bool IsNoneEvent()const;
	bool IsWrite()const;
	bool IsRead()const;

	//���ڹ���connection����������
	void Tie(const std::shared_ptr<void>&);

	void HandleEvent();
	void HandleEventWithGuard();
	void Remove();

private:
	void update();
private:
	int fd_;
	//Epoll *ep_;
	EventLoop* loop_;
	uint32_t events_;
	uint32_t revents_;
	bool is_in_poll_;

	//���ڹ���connection����������
	std::weak_ptr<void> tie_;
	bool tied_;


	EventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;

};

