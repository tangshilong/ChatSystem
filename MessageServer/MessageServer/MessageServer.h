#pragma once
#include <map>
#include <string>
#include <thread>
#include <winsock2.h>

#include "ClientManager.h"
#include "MsgHandler.h"


/**
 * ���������������
 */
class MessageServer
{
public:

	MessageServer();
	~MessageServer();

	// ��������
	bool StartServer();

	// ��socket
	bool InitSocket();

public:
	// ϵͳ�Ƿ���������
	bool IsRuning();
	void SetRunning(bool status = true);

	SOCKET GetServerSocekt();

	ClientMananger& GetClientManger();
	MsgHandler& GetMessageHandler();

	// select IOģ�ͽ����߳�
	static void SelectThread(MessageServer* message_server);
private:
	
	bool _is_running;	// ϵͳ���б�־ true �������� false �������� Ĭ��Ϊfalse

	std::string _ip;	// ��������ַ
	int _port;			// �����������˿�
	SOCKET _server_socket; // ����������socket


	// select �߳�
	std::thread	_select_thread;

	ClientMananger _client_manager;

	MsgHandler _msg_handler;
};

