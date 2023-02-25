#pragma once
#include <mutex>
#include <queue>
#include <winsock2.h>

class Client;

#define BUFF_SIZE 1024

// ��Ϣ����
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

// ��Ϣͷ
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
	char _name[32];// �û���
	char _to_name[32];// ���͸��Է� �û���
	int _msg_len;// ��Ϣ���ݳ���

	time_t _time;//ʱ��
};

// ��¼��Ϣ��
struct LoginBody
{
	LoginBody() :
		_success(false)
	{
		memset(_pwd, 0, sizeof(_pwd));
	}

	char _pwd[20];// ����
	bool _success; // ��¼�Ƿ�ɹ�
};

// ѹ���������Ϣ��
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

// ��Ϣ������
class MsgHandler
{

public:
	MsgHandler();
	~MsgHandler();

	void Start();
	void Stop();

	// ȡ����ѹ��һ����Ϣ�������ܶ���
	MsgPacket* PopRecvPacket();
	void PushRecvPacket(MsgPacket* packet);

	// ȡ����ѹ��һ����Ϣ�������Ͷ���
	MsgPacket* PopSendPacket();
	void PushSendPacket(MsgPacket* packet);

	void SetClient(Client* client);
	Client* GetClient();
public:

	// ��������
	void LoginRequest(std::string& msg, int pos);
	void LogoutRequest(std::string& msg, int pos);
	void SendMsgRequest(std::string& msg, int pos);
	void SendAllRequest(std::string& msg, int pos);
	void QuitRequest();

	// ����Ӧ��
	void RecvMsg(MsgPacket* packet);
	void LoginResponse(MsgPacket* packet);
	void LogoutResponse(MsgPacket* packet);

	// ������Ϣ�����߳�
	static void MsgDealThread(MsgHandler* handler);

private:
	std::thread*	_Msg_Deal_thread; // ��Ϣ�����߳�


	std::mutex _recv_mutex;
	std::queue<MsgPacket*> _recv_queue;//��Ϣ���ܶ���

	std::mutex _send_mutex;
	std::queue<MsgPacket*> _send_queue;// ��Ϣ���Ͷ���

	Client* _client;
};

