#pragma once
#include <string>
#include <unordered_map>
#include <thread>
#include <winsock2.h>

#include "MsgHandler.h"

// 客户端类
class Client
{
public:
	Client();
	~Client();

	bool Start();
	void Stop();

	// 连接 socket
	bool ConnectSocket();

public:

	// 解析输入指令
	MsgType GetMsgType(std::string& head);

	// 处理连接服务器指令
	void ConnectServer(std::string& msg, int pos);

	// 处理输入指令
	void DealInputMsg(std::string& msg);

public:
	// 系统是否正在运行
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

	// 消息接受 发送线程
	static void RecvThread(Client* client);
	static void SendThread(Client* client);
private:
	bool _is_running;	// 系统运行标志 true 正在运行 false 结束运行 默认为false

	bool _is_login; //是否登录 true 已登录 false 未登录

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

