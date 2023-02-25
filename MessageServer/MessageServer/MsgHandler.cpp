#include <direct.h>
#include <iostream>
#include <ostream>
#include <fstream>

#include "MsgHandler.h"
#include "MessageServer.h"

MsgPacket::MsgPacket():
	_socket(-1)
{
	memset(_name, 0, sizeof(_name));
	memset(_msg, 0, sizeof(_msg));

	MsgHead* msg_head = (MsgHead*)_msg;
	msg_head->_time = time(0);
}

void MsgPacket::SetSocket(SOCKET socket)
{
	_socket = socket;
}

SOCKET MsgPacket::GetSocket()
{
	return _socket;
}

void MsgPacket::SetMsg(const char* msg, int len)
{
	memset(_msg, 0, sizeof(_msg));
	memcpy(_msg, msg, min(sizeof(_msg)-1, len));
}

char* MsgPacket::GetMsg()
{
	return _msg;
}

MsgHead* MsgPacket::GetMsgHead()
{
	MsgHead* msg_head = (MsgHead*)_msg;
	return msg_head;
}

char* MsgPacket::GetMsgBody()
{
	return _msg + sizeof(MsgHead);
}

MsgHandler::MsgHandler(): _client_manager(), _message_server()
{
}

MsgHandler::~MsgHandler()
{
	std::lock_guard<std::mutex> lock(_recv_mutex);
	while (!_recv_queue.empty())
	{
		auto packet = _recv_queue.front();
		delete packet;
		_recv_queue.pop();
	}
	

	std::lock_guard<std::mutex> recv_lock(_send_mutex);
	while (!_send_queue.empty())
	{
		auto packet = _send_queue.front();
		delete packet;
		_send_queue.pop();
	}
}

void MsgHandler::Start()
{
	_send_thread = std::thread(MsgSendThread, this);
	_recv_thread = std::thread(MsgRecvThread, this);
}

MsgPacket* MsgHandler::PopRecvPacket()
{
	std::lock_guard<std::mutex> lock(_recv_mutex);
	if (_recv_queue.empty())
		return nullptr;

	auto packet = _recv_queue.front();
	_recv_queue.pop();
	return packet;
}

void MsgHandler::PushRecvPacket(MsgPacket* packet)
{
	std::lock_guard<std::mutex> lock(_recv_mutex);
	_recv_queue.emplace(packet);
}

MsgPacket* MsgHandler::PopSendPacket()
{
	std::lock_guard<std::mutex> lock(_send_mutex);
	if (_send_queue.empty())
		return nullptr;

	auto packet = _send_queue.front();
	_send_queue.pop();
	return packet;
}

void MsgHandler::PushSendPacket(MsgPacket* packet)
{
	std::lock_guard<std::mutex> lock(_send_mutex);
	_send_queue.emplace(packet);
}

void MsgHandler::SetClientManager(ClientMananger* client_manager)
{
	_client_manager = client_manager;
}

ClientMananger* MsgHandler::GetClientManager()
{
	return _client_manager;
}

void MsgHandler::SetMessageServer(MessageServer* message_server)
{
	_message_server = message_server;
}

MessageServer* MsgHandler::GetMessageServer()
{
	return _message_server;
}
void MsgHandler::LoginFunc(MsgPacket* packet)
{
	MsgHead* msg_head = packet->GetMsgHead();
	ClientMananger* client_manager = GetClientManager();

	MsgPacket* send_packet = new MsgPacket();

	bool is_success = false;

	if (msg_head->_type == MsgType::LOGIN)
	{
		Client* client = client_manager->GetClient(msg_head->_name);

		if (client)
		{
			std::cout << msg_head->_name << " has already logged in !" << std::endl;
		}
		else
		{
			LoginBody* login_body = (LoginBody*)packet->GetMsgBody();

			// 检查账号密码
			if (client_manager->CheckAccount(msg_head->_name, login_body->_pwd))
			{
				is_success = true;

				Client* _new_client = new Client();
				_new_client->_socket = packet->GetSocket();
				strcpy(_new_client->_name, msg_head->_name);

				client_manager->AddClient(_new_client);
			}
		}

		// 发送登录消息回执
		MsgHead* send_head = send_packet->GetMsgHead();
		LoginBody* send_body = (LoginBody*)send_packet->GetMsgBody();

		strcpy(send_head->_name, "Server");
		strcpy(send_head->_to_name, msg_head->_name);
		send_head->_type = MsgType::LOGIN;
		send_head->_msg_len = sizeof(MsgHead) + sizeof(LoginBody);
		send_body->_success = is_success;

		send_packet->SetSocket(packet->GetSocket());

	}
	else // 没登录时，只处理登录请求
	{
		MsgHead* send_head = send_packet->GetMsgHead();
		char* send_body = send_packet->GetMsgBody();

		strcpy(send_head->_name, "Server");
		strcpy(send_head->_to_name, msg_head->_name);
		send_head->_type = MsgType::SEND_MSG;

		std::string msg = "client need login !";
		send_head->_msg_len = sizeof(MsgHead) + msg.length();

		send_packet->SetMsg(send_packet->GetMsg(), send_head->_msg_len);
		send_packet->SetSocket(packet->GetSocket());
	}


	// 添加到消息队列
	PushSendPacket(send_packet);

	// 发送离线消息
	if(is_success)
		SendOfflineMsg(msg_head->_name, packet->GetSocket());
}

// 处理登出
void MsgHandler::LogOutFunc(MsgPacket* packet)
{
	ClientMananger* client_manager = GetClientManager();
	Client* client = client_manager->GetClient(packet->GetSocket());
	MsgHead* msg_head = packet->GetMsgHead();

	if (!client)
	{
		std::cout << msg_head->_name << " has already logged out !" << std::endl;
		return;
	}

	MsgPacket* send_packet = new MsgPacket();
	MsgHead* send_head = send_packet->GetMsgHead();
	LoginBody* send_body = (LoginBody*)send_packet->GetMsgBody();

	strcpy(send_head->_name, "Server");
	strcpy(send_head->_to_name, msg_head->_name);
	send_head->_type = MsgType::LOGOUT;
	send_head->_msg_len = sizeof(MsgHead) + sizeof(LoginBody);
	send_body->_success = true;

	send_packet->SetSocket(packet->GetSocket());

	// 添加到消息队列
	PushSendPacket(send_packet);

	GetClientManager()->DeleteClient(packet->GetSocket());

	std::cout << msg_head->_name << " Logout !" << std::endl;
}

void MsgHandler::SaveOfflineMsg(MsgPacket* packet)
{
	MsgHead* msg_head = packet->GetMsgHead();
	// 客户端不在线，记录离线消息
	char* path = getcwd(NULL, 0);
	std::string file_path = path;
	file_path.append("\\");
	file_path.append(msg_head->_to_name);
	file_path.append(".txt");

	// 以写模式打开文件
	std::ofstream outfile;
	outfile.open(file_path, std::ios::app);
	// 向文件写入用户输入的数据
	outfile.write(packet->GetMsg(), msg_head->_msg_len);
	
	// 关闭打开的文件
	outfile.close();
}

// 发送离线消息
void MsgHandler::SendOfflineMsg(char* name, SOCKET socket)
{
	// 读取已经记录的账号密码
	char* path = getcwd(NULL, 0);
	std::string file_path = path;
	file_path.append("\\");
	file_path.append(name);
	file_path.append(".txt");

	std::ifstream file;
	file.open(file_path, std::ios::in);

	if (!file.is_open())
		return;

	char buffer[2048] = { 0 };
	int head_size = sizeof(MsgHead);

	while (!file.eof())
	{
		memset(buffer,0,sizeof(buffer));
		// 先读取消息头
		file.read(buffer, head_size);

		// 再读取消息体
		MsgHead* msg_head = (MsgHead*)buffer;
		file.read(buffer+ head_size, msg_head->_msg_len - head_size);

		if(msg_head->_msg_len == 0)
			continue;

		// 发送消息
		MsgPacket* send_packet = new MsgPacket();
		// 发送登录消息回执
		send_packet->SetMsg(buffer, msg_head->_msg_len);
		send_packet->SetSocket(socket);
		PushSendPacket(send_packet);
	}

	file.close();

	// 清空
	std::remove(file_path.c_str());
}

// 普通发送消息
void MsgHandler::SendMsgFunc(MsgPacket* packet)
{
	ClientMananger* client_manager = GetClientManager();

	MsgHead* msg_head = packet->GetMsgHead();

	MsgPacket* send_packet = new MsgPacket();
	Client* client = client_manager->GetClient(msg_head->_to_name);
	if(client == nullptr)
	{
		// 记录离线消息
		SaveOfflineMsg(packet);

		MsgHead* send_head = send_packet->GetMsgHead();
		char* send_body = send_packet->GetMsgBody();

		strcpy(send_head->_name, "Server");
		strcpy(send_head->_to_name, msg_head->_to_name);
		send_head->_type = MsgType::SEND_MSG;

		std::string msg = "User " + std::string(msg_head->_to_name) + " not Online !";
		send_head->_msg_len = sizeof(MsgHead) + msg.length();

		memcpy(send_body, msg.c_str(), msg.length());
		
		send_packet->SetSocket(packet->GetSocket());

	}else
	{
		send_packet->SetMsg(packet->GetMsg(), packet->GetMsgHead()->_msg_len);
		send_packet->SetSocket(client->_socket);
	}


	// 添加到消息队列
	PushSendPacket(send_packet);
}

// 群发消息
void MsgHandler::SendAllFunc(MsgPacket* packet)
{
	ClientMananger* client_manager = GetClientManager();
	MsgHead* msg_head = packet->GetMsgHead();

	auto client_map = client_manager->GetAllClient();

	for (const auto & iter : client_map)
	{
		MsgPacket* send_packet = new MsgPacket();

		send_packet->SetMsg(packet->GetMsg(), packet->GetMsgHead()->_msg_len);
		send_packet->SetSocket(iter.second->_socket);

		// 添加到消息队列
		PushSendPacket(send_packet);
	}

}

void MsgHandler::ReadPacket()
{
	ClientMananger* client_manager = GetClientManager();

	// 读出一个消息包
	MsgPacket* packet = nullptr;
	while((packet = PopRecvPacket()) != nullptr)
	{
		// 找到对应的客户端
		Client* client = client_manager->GetClient(packet->GetSocket());
		MsgHead* msg_head = packet->GetMsgHead();
		if (!msg_head)
		{
			delete packet;
			continue;
		}

		// 判断是否登录，没登录只处理登录请求
		if (client == nullptr)
		{
			LoginFunc(packet);
		}
		else
		{
			switch (msg_head->_type)
			{
			case MsgType::SEND_MSG: //普通发送消息
			{
				SendMsgFunc(packet);
				break;
			}
			case MsgType::SEND_ALL: //群发
			{
				SendAllFunc(packet);
				break;
			}
			case MsgType::LOGIN: //重复登录
			{
				LoginFunc(packet);
				break;
			}
			case MsgType::LOGOUT: //登出
			{
				LogOutFunc(packet);
				break;
			}
			default:
				break;
			}
		}

		delete packet;
	}

}

void MsgHandler::MsgRecvThread(MsgHandler* handler)
{
	MessageServer* message_server = handler->GetMessageServer();

	while (message_server->IsRuning())
	{
		handler->ReadPacket();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void MsgHandler::MsgSendThread(MsgHandler* handler)
{
	MessageServer* message_server = handler->GetMessageServer();
	ClientMananger* client_manager = handler->GetClientManager();

	while (message_server->IsRuning())
	{
		// 读出一个消息包
		MsgPacket* packet = nullptr;
		while ((packet = handler->PopSendPacket()) != nullptr)
		{
			MsgHead* msg_head = packet->GetMsgHead();
			if (!msg_head)
			{
				delete packet;
				continue;
			}

			int ret = send(packet->GetSocket(), packet->GetMsg(), msg_head->_msg_len,0);
			if (ret <= 0)//表示客服端断开链接
			{
				if (!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
				{
					std::cout << msg_head->_to_name << " is closed.." << std::endl;
					client_manager->DeleteClient(packet->GetSocket());
				}
			}

			delete packet;
			packet = nullptr;

			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}