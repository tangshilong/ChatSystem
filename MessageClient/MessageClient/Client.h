#pragma once
#include <string>
#include <unordered_map>
#include <thread>
#include <winsock2.h>

#include "MsgHandler.h"

// �ͻ�����
class Client
{
public:
	Client();
	~Client();

	bool Start();
	void Stop();

	// ���� socket
	bool ConnectSocket();

public:

	// ��������ָ��
	MsgType GetMsgType(std::string& head);

	// �������ӷ�����ָ��
	void ConnectServer(std::string& msg, int pos);

	// ��������ָ��
	void DealInputMsg(std::string& msg);

public:
	// ϵͳ�Ƿ���������
	bool IsRuning();
	void SetRunning(bool status = true);

	bool IsLogin();
	void SetLogin(bool status = true);

	std::string& GetName();
	void SetName(std::string&& name);

	std::string& GetPassword();
	void SetPassword(std::string&& password);

	std::string& GetIP();
	void SetIP(std::string&& ip);

	int GetServerPort();
	void SetServerPort(int port);

	SOCKET GetSocket();

	MsgHandler& GetMsgHandler();

	// ��Ϣ���� �����߳�
	static void RecvThread(Client* client);
	static void SendThread(Client* client);
private:
	bool _is_running;	// ϵͳ���б�־ true �������� false �������� Ĭ��Ϊfalse

	bool _is_login; //�Ƿ��¼ true �ѵ�¼ false δ��¼

	std::string _server_ip;
	int _server_port;

	std::string _name;
	std::string _password;

	SOCKET _socket;

	MsgHandler _msg_handler;

	std::thread*	_recv_thread;
	std::thread*	_send_thread;

	std::unordered_map<std::string, MsgType> _msg_type;
};

