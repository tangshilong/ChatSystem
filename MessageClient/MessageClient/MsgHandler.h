#pragma once
#include <mutex>
#include <queue>
#include <winsock2.h>

class Client;

#define BUFF_SIZE 1024

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
	MsgHead() :
		_type(Invalid),
		_msg_len(0)
	{
		_time = time(0);

		memset(_name, 0, sizeof(_name));
		memset(_to_name, 0, sizeof(_to_name));
	}

	MsgType _type;
	char _name[32];// 用户名
	char _to_name[32];// 发送给对方 用户名
	int _msg_len;// 消息内容长度

	time_t _time;//时间
};

// 登录消息体
struct LoginBody
{
	LoginBody() :
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

	void SetMsg(const char* msg, int len);
	char* GetMsg();

	MsgHead* GetMsgHead();
	char* GetMsgBody();

private:
	char _name[32];
	char _msg[4096];
};

// 消息处理类
class MsgHandler
{

public:
	MsgHandler();
	~MsgHandler();

	void Start();
	void Stop();

	// 取出、压入一个消息包到接受队列
	MsgPacket* PopRecvPacket();
	void PushRecvPacket(MsgPacket* packet);

	// 取出、压入一个消息包到发送队列
	MsgPacket* PopSendPacket();
	void PushSendPacket(MsgPacket* packet);

	void SetClient(Client* client);
	Client* GetClient();
public:

	// 处理请求
	void LoginRequest(std::string& msg, int pos);
	void LogoutRequest(std::string& msg, int pos);
	void SendMsgRequest(std::string& msg, int pos);
	void SendAllRequest(std::string& msg, int pos);
	void QuitRequest();

	// 处理应答
	void RecvMsg(MsgPacket* packet);
	void LoginResponse(MsgPacket* packet);
	void LogoutResponse(MsgPacket* packet);

	// 接受消息处理线程
	static void MsgDealThread(MsgHandler* handler);

private:
	std::thread*	_Msg_Deal_thread; // 消息处理线程


	std::mutex _recv_mutex;
	std::queue<MsgPacket*> _recv_queue;//消息接受队列

	std::mutex _send_mutex;
	std::queue<MsgPacket*> _send_queue;// 消息发送队列

	Client* _client;
};

