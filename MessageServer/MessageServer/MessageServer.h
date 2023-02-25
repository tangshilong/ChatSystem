#pragma once
#include <map>
#include <string>
#include <thread>
#include <winsock2.h>

#include "ClientManager.h"
#include "MsgHandler.h"


/**
 * 聊天服务器管理类
 */
class MessageServer
{
public:

	MessageServer();
	~MessageServer();

	// 启动服务
	bool StartServer();

	// 绑定socket
	bool InitSocket();

public:
	// 系统是否正在运行
	bool IsRuning();
	void SetRunning(bool status = true);

	SOCKET GetServerSocekt();

	ClientMananger& GetClientManger();
	MsgHandler& GetMessageHandler();

	// select IO模型接收线程
	static void SelectThread(MessageServer* message_server);
private:
	
	bool _is_running;	// 系统运行标志 true 正在运行 false 结束运行 默认为false

	std::string _ip;	// 服务器地址
	int _port;			// 服务器监听端口
	SOCKET _server_socket; // 服务器监听socket


	// select 线程
	std::thread	_select_thread;

	ClientMananger _client_manager;

	MsgHandler _msg_handler;
};

