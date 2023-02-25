#include "Client.h"

#include <iostream>
#include <ostream>
#include <winsock2.h>

#include "MsgHandler.h"

Client::Client():
	_server_port(0),
	_socket(INVALID_SOCKET),
	_is_running(false),
	_is_login(false),
	_recv_thread(),
	_send_thread()
{
	_msg_type.insert(std::make_pair("HI", MsgType::CONNECT));
	_msg_type.insert(std::make_pair("LOGIN", MsgType::LOGIN));
	_msg_type.insert(std::make_pair("BYE", MsgType::LOGOUT));
	_msg_type.insert(std::make_pair("@ALL", MsgType::SEND_ALL));
	_msg_type.insert(std::make_pair("QUIT", MsgType::QUIT));
}

Client::~Client()
{
}

bool Client::Start()
{
	SetRunning();

	// 连接服务器
	if (!ConnectSocket())
	{
		_server_ip = "";
		_server_port = 0;

		std::cout << "Please input server address. Like: HI 127.0.0.1:8000" << std::endl;
		return false;
	}

	// 开启接受发送线程
	_recv_thread = new std::thread(RecvThread, this);
	_send_thread = new std::thread(SendThread, this);

	// 处理消息
	_msg_handler.SetClient(this);
	_msg_handler.Start();

	return true;
}

void Client::Stop()
{
	SetLogin(false);
	SetRunning(false);

	if (_socket != INVALID_SOCKET)
	{
		closesocket(_socket);
		_socket = INVALID_SOCKET;

		::WSACleanup();
	}

	if (_recv_thread)
	{
		if (_recv_thread->joinable())
		{
			_recv_thread->detach();
			delete _recv_thread;
		}

		_recv_thread = nullptr;
	}

	if (_send_thread)
	{
		if (_send_thread->joinable())
		{
			_send_thread->detach();
			delete _send_thread;
		}

		_send_thread = nullptr;
	}

	_msg_handler.Stop();
}

bool Client::IsRuning()
{
	return _is_running;
}

void Client::SetRunning(bool status)
{
	_is_running = status;
}

bool Client::IsLogin()
{
	return _is_login;
}

void Client::SetLogin(bool status)
{
	_is_login = status;
}

std::string& Client::GetName()
{
	return _name;
}

void Client::SetName(std::string&& name)
{
	_name = name;
}

std::string& Client::GetPassword()
{
	return _password;
}

void Client::SetPassword(std::string&& password)
{
	_password = password;
}


std::string& Client::GetIP()
{
	return _server_ip;
}

void Client::SetIP(std::string&& ip)
{
	_server_ip = ip;
}

int Client::GetServerPort()
{
	return _server_port;
}

void Client::SetServerPort(int port)
{
	_server_port = port;
}

SOCKET Client::GetSocket()
{
	return _socket;
}

MsgHandler& Client::GetMsgHandler()
{
	return _msg_handler;
}


MsgType Client::GetMsgType(std::string& head)
{
	auto iter = _msg_type.find(head);
	if (iter != _msg_type.end())
		return iter->second;

	// 判断是否是 @name
	if (head.length() > 1 && head[0] == '@')
		return MsgType::SEND_MSG;

	return MsgType::Invalid;
}

bool Client::ConnectSocket()
{
	WSADATA data_;
	if (WSAStartup(MAKEWORD(2, 2), &data_) != 0)
	{
		std::cout << "WSAStartup() init error " << GetLastError() << std::endl;
		return false;
	}

	//1.socket  通信用套接字，创建一个socket
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
	{
		std::cout << "Socket Error !" << std::endl;
		return false;
	}

	//设置成非阻塞模式。
	unsigned long ul = 1;
	if (ioctlsocket(_socket, FIONBIO, (unsigned long*)&ul) == SOCKET_ERROR)
	{
		std::cout << "ioctlsocket Error !" << std::endl;
		return false;
	}

	SOCKADDR_IN sfdaddr; //指定要连接服务器的结构体 ip 端口
	//初始化地址
	memset(&sfdaddr, 0, sizeof(sfdaddr));
	sfdaddr.sin_family = AF_INET;
	sfdaddr.sin_port = htons(_server_port);
	sfdaddr.sin_addr.s_addr = inet_addr(_server_ip.c_str());

	int try_count = 0;
	while (try_count < 10)
	{
		int ret = connect(_socket, (struct sockaddr*)&sfdaddr, sizeof(sfdaddr));
		if (ret == SOCKET_ERROR)
		{
			int r = WSAGetLastError();
			if (r == WSAEWOULDBLOCK || r == WSAEINVAL)
			{
				++try_count;
				Sleep(20);
				continue;
			}
			else if (r == WSAEISCONN) //套接字原来已经连接！！
			{
				break;
			}
			else
			{
				std::cout << "connect Error !" << std::endl;
				return false;
			}
		}
		if (ret == 0)
		{
			break;
		}
	}

	return true;
}

void Client::ConnectServer(std::string& msg, int pos)
{
	// 解析 ip 和 端口号
	int index = msg.find_first_of(':', pos + 1);
	_server_ip = msg.substr(pos + 1, index - pos - 1);
	_server_port = atol(msg.substr(index + 1).c_str());

	// 如果连接成功，需要登录
	if (Start())
	{
		std::cout << "Connect server success!\n" << std::endl;
		std::cout << "Now you can register or login!"
		"\nIf you have registered, please enter the correct account and password.\n" << std::endl;
		std::cout << "Please input username and password. Like: LOGIN Alice:123456" << std::endl;
	}
}

/**
 * 
 * 连接服务器		HI
 * 登录			LOGIN
 * 退出			BYE
 * 群发			@ALL
 * 单发			@name
 * 退出客户端		QUIT
 */
void Client::DealInputMsg(std::string& msg)
{
	// 解析头部消息，已空格分隔
	int pos = msg.find_first_of(' ');
	std::string head = msg.substr(0, pos);

	// 获取消息类型
	MsgType type = GetMsgType(head);

	switch (type)
	{
	case MsgType::CONNECT: // 连接服务器
		{
			ConnectServer(msg, pos);
			break;
		}
	case MsgType::LOGIN: // 登录
		{
			_msg_handler.LoginRequest(msg, pos);
			break;
		}
	case MsgType::LOGOUT: // 登出
		{
			_msg_handler.LogoutRequest(msg, pos);
			break;
		}
	case MsgType::SEND_MSG: // 发消息
		{
			_msg_handler.SendMsgRequest(msg, pos);
			break;
		}
	case MsgType::SEND_ALL: // 群发
		{
			_msg_handler.SendAllRequest(msg, pos);
			break;
		}
	case MsgType::QUIT: // 退出客户端
		{
			_msg_handler.QuitRequest();
			break;
		}

	default: // 非法消息
		{
			break;
		}
	}
}

void Client::RecvThread(Client* client)
{
	SOCKET socket = client->GetSocket();
	MsgHandler& msg_handler = client->GetMsgHandler();

	char buf[BUFF_SIZE] = {0};
	while (client->IsRuning())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		memset(buf, 0, BUFF_SIZE);

		int nRecv;
		nRecv = recv(socket, buf, BUFF_SIZE, 0);

		if (nRecv == SOCKET_ERROR)
		{
			int r = WSAGetLastError();
			if (r == WSAEWOULDBLOCK)
			{
				// 没有收到服务器返回的数据
				continue;
			}
			else // if (r == WSAENETDOWN)
			{
				std::cout << "Recv Fail ! Please Reconnect ! \n" << std::endl;
				std::cout << "Please input server address. Like: HI 127.0.0.1:8000" << std::endl;

				client->Stop();
				break;
			}
		}
		else if (nRecv == 0)
		{
			std::cout << "Server Closed !\n" << std::endl;
			std::cout << "Please input server address. Like: HI 127.0.0.1:8000" << std::endl;
			client->Stop();
			break;
		}
		else
		{
			MsgPacket* packet = new MsgPacket();
			packet->SetMsg(buf, nRecv);

			msg_handler.PushRecvPacket(packet);
			memset(buf, 0, BUFF_SIZE);
		}
	}
}

void Client::SendThread(Client* client)
{
	SOCKET socket = client->GetSocket();
	MsgHandler& msg_handler = client->GetMsgHandler();

	while (client->IsRuning())
	{
		// 读出一个消息包
		MsgPacket* packet = nullptr;
		while ((packet = msg_handler.PopSendPacket()) != nullptr)
		{
			MsgHead* msg_head = packet->GetMsgHead();
			if (!msg_head)
			{
				delete packet;
				continue;
			}

			int ret = send(socket, packet->GetMsg(), msg_head->_msg_len, 0);
			if (ret <= 0) //表示客服端断开链接
			{
				if (!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
				{
					printf("Send fail ! \n");

					client->Stop();
				}
			}

			delete packet;
			packet = nullptr;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
