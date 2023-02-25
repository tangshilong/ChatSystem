#pragma once
#include <mutex>
#include <queue>
#include <winsock2.h>

#include "ClientManager.h"

class MessageServer;

// 消息类型
enum MsgType
{
	LOGIN,
	LOGOUT,
	SEND_MSG,
	SEND_ALL,

	CONNECT,
	QUIT,
	Invalid,
};

// 消息头
struct MsgHead
{
	MsgHead():
		_type(Invalid),
		_msg_len(0)
	{
		_time = time(0);
		memset(_name,0,sizeof(_name));
		memset(_to_name,0,sizeof(_to_name));
	}

	MsgType _type;
	char _name[32];// 用户名
	char _to_name[32];// 发送给对方 用户名
	int _msg_len;// 消息内容长度

	time_t _time; // 时间
};

// 登录消息体
struct LoginBody
{
	LoginBody():
		_success(false)
	{
		memset(_pwd, 0, sizeof(_pwd));
	}

	char _pwd[20];// 密码
	bool _success; // 登录是否成功
};


// 压入队列用消息包
class MsgPacket {
public:
	MsgPacket();

	void SetSocket(SOCKET socket);
	SOCKET GetSocket();

	void SetMsg(const char* msg, int len);
	char* GetMsg();

	MsgHead* GetMsgHead();
	char* GetMsgBody();

private:
	char _name[32];
	SOCKET _socket;
	char _msg[4096];
};

class MsgHandler
{
public:
	MsgHandler();
	~MsgHandler();

	void Start();

	// 取出、压入一个消息包到接受队列
	MsgPacket* PopRecvPacket( );
	void PushRecvPacket( MsgPacket* packet);

	// 取出、压入一个消息包到发送队列
	MsgPacket* PopSendPacket();
	void PushSendPacket(MsgPacket* packet);

	void SetClientManager(ClientMananger* client_manager);
	ClientMananger* GetClientManager();

	void SetMessageServer(MessageServer* message_server);
	MessageServer* GetMessageServer();

public:

	// 记录离线消息
	void SaveOfflineMsg(MsgPacket* packet);
	// 发送离线消息
	void SendOfflineMsg(char* name, SOCKET socket);


	// 处理登录
	void LoginFunc( MsgPacket* packet);

	// 处理登出
	void LogOutFunc(MsgPacket* packet);

	// 普通发送消息
	void SendMsgFunc( MsgPacket* packet);

	// 群发消息
	void SendAllFunc( MsgPacket* packet);
	

	// 处理接受消息
	void ReadPacket();

	// 接受线程
	static void MsgRecvThread(MsgHandler* handler);
	// 发送线程
	static void MsgSendThread(MsgHandler* handler);

private:

	std::thread	_recv_thread;
	std::mutex _recv_mutex;
	std::queue<MsgPacket*> _recv_queue; //

	std::thread	_send_thread;
	std::mutex _send_mutex;
	std::queue<MsgPacket*> _send_queue;


	ClientMananger* _client_manager;
	MessageServer* _message_server;
};

