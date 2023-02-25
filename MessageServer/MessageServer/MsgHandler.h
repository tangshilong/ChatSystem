#pragma once
#include <mutex>
#include <queue>
#include <winsock2.h>

#include "ClientManager.h"

class MessageServer;

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
	MsgHead():
		_type(Invalid),
		_msg_len(0)
	{
		_time = time(0);
		memset(_name,0,sizeof(_name));
		memset(_to_name,0,sizeof(_to_name));
	}

	MsgType _type;
	char _name[32];// �û���
	char _to_name[32];// ���͸��Է� �û���
	int _msg_len;// ��Ϣ���ݳ���

	time_t _time; // ʱ��
};

// ��¼��Ϣ��
struct LoginBody
{
	LoginBody():
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

	// ȡ����ѹ��һ����Ϣ�������ܶ���
	MsgPacket* PopRecvPacket( );
	void PushRecvPacket( MsgPacket* packet);

	// ȡ����ѹ��һ����Ϣ�������Ͷ���
	MsgPacket* PopSendPacket();
	void PushSendPacket(MsgPacket* packet);

	void SetClientManager(ClientMananger* client_manager);
	ClientMananger* GetClientManager();

	void SetMessageServer(MessageServer* message_server);
	MessageServer* GetMessageServer();

public:

	// ��¼������Ϣ
	void SaveOfflineMsg(MsgPacket* packet);
	// ����������Ϣ
	void SendOfflineMsg(char* name, SOCKET socket);


	// �����¼
	void LoginFunc( MsgPacket* packet);

	// ����ǳ�
	void LogOutFunc(MsgPacket* packet);

	// ��ͨ������Ϣ
	void SendMsgFunc( MsgPacket* packet);

	// Ⱥ����Ϣ
	void SendAllFunc( MsgPacket* packet);
	

	// ���������Ϣ
	void ReadPacket();

	// �����߳�
	static void MsgRecvThread(MsgHandler* handler);
	// �����߳�
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

