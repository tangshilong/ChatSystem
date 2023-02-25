#include "MsgHandler.h"
#include "Client.h"

#include <iostream>
#include <ostream>
#include <regex>


MsgPacket::MsgPacket()
{
	memset(_name, 0, sizeof(_name));
	memset(_msg, 0, sizeof(_msg));

	MsgHead* msg_head = (MsgHead*)_msg;
	msg_head->_time = time(0);
}


void MsgPacket::SetMsg(const char* msg, int len)
{
	memset(_msg, 0, sizeof(_msg));
	memcpy(_msg, msg, min(sizeof(_msg) - 1, len));
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


MsgHandler::MsgHandler() : _client(), _Msg_Deal_thread()
{

}

MsgHandler::~MsgHandler()
{
	Stop();
}

void MsgHandler::Start()
{
	_Msg_Deal_thread = new std::thread(MsgDealThread, this);
}

void MsgHandler::Stop()
{

	if(_Msg_Deal_thread != nullptr)
	{
		if (_Msg_Deal_thread->joinable())
		{
			_Msg_Deal_thread->detach();
			delete _Msg_Deal_thread;
		}
		
		_Msg_Deal_thread = nullptr;
	}

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

void MsgHandler::SetClient(Client* client)
{
	_client = client;
}

Client* MsgHandler::GetClient()
{
	return _client;
}

void MsgHandler::LoginRequest(std::string& msg, int pos)
{

	// 解析 账号密码
	int index = msg.find_first_of(':', pos + 1);
	std::string name = msg.substr(pos + 1, index - pos - 1);
	std::string password = msg.substr(index + 1);

	// 检查账号密码格式
	std::regex reg("^[A-Za-z0-9]+$");
	if(!(std::regex_match(name, reg) && std::regex_match(password, reg)))
	{
		std::cout << "Registration Failed!\n\nUsername and Password can only consist of numbers and letters!\n";
		return;
	}

	_client->SetName(std::move(name));
	_client->SetPassword(std::move(password));

	MsgPacket* send_packet = new MsgPacket();
	MsgHead* send_head = send_packet->GetMsgHead();

	strcpy(send_head->_name, _client->GetName().c_str());
	strcpy(send_head->_to_name, "Server");
	send_head->_type = MsgType::LOGIN;
	send_head->_msg_len = sizeof(MsgHead) + sizeof(LoginBody);

	LoginBody* send_body = (LoginBody*)send_packet->GetMsgBody();
	strcpy(send_body->_pwd, _client->GetPassword().c_str());

	// 添加到消息队列
	PushSendPacket(send_packet);

}


void MsgHandler::LogoutRequest(std::string& msg, int pos)
{
	if (_client->IsLogin())
	{
		MsgPacket* send_packet = new MsgPacket();
		MsgHead* send_head = send_packet->GetMsgHead();

		strcpy(send_head->_name, _client->GetName().c_str());
		strcpy(send_head->_to_name, "Server");
		send_head->_type = MsgType::LOGOUT;
		send_head->_msg_len = sizeof(MsgHead) + sizeof(LoginBody);

		LoginBody* send_body = (LoginBody*)send_packet->GetMsgBody();
		strcpy(send_body->_pwd, _client->GetPassword().c_str());

		// 添加到消息队列
		PushSendPacket(send_packet);
	}
	else
	{
		std::cout << "Client is not logged in !" << std::endl;
	}
}

void MsgHandler::SendMsgRequest(std::string& msg, int pos)
{
	if (_client->IsLogin())
	{
		std::string to_name = msg.substr(1, pos - 1);
		std::string str_msg = msg.substr(pos + 1);

		MsgPacket* send_packet = new MsgPacket();
		MsgHead* send_head = send_packet->GetMsgHead();

		strcpy(send_head->_name, _client->GetName().c_str());
		strcpy(send_head->_to_name, to_name.c_str());
		send_head->_type = MsgType::SEND_MSG;
		send_head->_msg_len = sizeof(MsgHead) + str_msg.length();

		char* send_body = send_packet->GetMsgBody();
		strcpy(send_body, str_msg.c_str());

		// 添加到消息队列
		PushSendPacket(send_packet);

		struct tm stime;
		localtime_s(&stime, &send_head->_time);

		char tmp[32] = { NULL };
		strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", &stime);
		std::cout << tmp << " " << _client->GetName() << ": " << send_body << std::endl;

		std::cout << _client->GetIP() << ":" << _client->GetServerPort() << "|" << _client->GetName() << " > ";
	}
	else
	{
		std::cout << "Client is not logged in !" << std::endl;
	}
}

void MsgHandler::SendAllRequest(std::string& msg, int pos)
{
	if (_client->IsLogin())
	{
		std::string str_msg = msg.substr(pos + 1);

		MsgPacket* send_packet = new MsgPacket();
		MsgHead* send_head = send_packet->GetMsgHead();

		strcpy(send_head->_name, _client->GetName().c_str());
		strcpy(send_head->_to_name, "Server");
		send_head->_type = MsgType::SEND_ALL;
		send_head->_msg_len = sizeof(MsgHead) + str_msg.length();

		char* send_body = send_packet->GetMsgBody();
		strcpy(send_body, str_msg.c_str());

		// 添加到消息队列
		PushSendPacket(send_packet);
	}
	else
	{
		std::cout << "Client is not logged in !" << std::endl;
	}
}

void MsgHandler::QuitRequest()
{
	exit(0);
}


void MsgHandler::RecvMsg(MsgPacket* packet)
{
	MsgHead* msg_head = packet->GetMsgHead();
	char* msg = packet->GetMsgBody();

	struct tm stime;
	localtime_s(&stime, &msg_head->_time);

	char tmp[32] = { NULL };
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", &stime);

	if (msg_head->_type == MsgType::SEND_MSG)
		std::cout << "\r" << tmp << " " << msg_head->_name << ": " << msg << std::endl;
	else
		std::cout << "\r" << tmp << " broadcast from " << msg_head->_name << ": " << msg << std::endl;

	std::cout << _client->GetIP() << ":" << _client->GetServerPort() << "|" << _client->GetName() << " > ";
}

void MsgHandler::LoginResponse(MsgPacket* packet)
{
	MsgHead* msg_head = packet->GetMsgHead();
	LoginBody* login_body = (LoginBody*)packet->GetMsgBody();

	if(login_body->_success)
	{
		std::cout << "Login Success !" << std::endl;
		_client->SetLogin();

		std::cout << "\nNow you can start chatting! Like: \"@Alice hello\" or \"@ALL Hello All guys!\"\n" << std::endl;

		std::cout << _client->GetIP() << ":" << _client->GetServerPort() << "|" << _client->GetName() << " > ";
	}
	else
	{
		_client->SetLogin(false);
		std::cout << "Login Fail !\n" << std::endl;
		std::cout << "Please input username and password. Like: LOGIN Alice:123456" << std::endl;
	}
}

void MsgHandler::LogoutResponse(MsgPacket* packet)
{
	MsgHead* msg_head = packet->GetMsgHead();
	LoginBody* login_body = (LoginBody*)packet->GetMsgBody();

	if (login_body->_success)
	{
		_client->SetLogin(false);
		std::cout << "Logout Success !\n" << std::endl;
		std::cout << "Please input username and password. Like: LOGIN Alice:123456" << std::endl;
	}
	else
	{
		std::cout << "Logout Fail !\n" << std::endl;
	}
}

void MsgHandler::MsgDealThread(MsgHandler* handler)
{
	Client* client = handler->GetClient();
	SOCKET socket = client->GetSocket();

	char buf[BUFF_SIZE] = { 0 };
	while (client->IsRuning())
	{
		// 读出一个消息包
		MsgPacket* packet = nullptr;
		while ((packet = handler->PopRecvPacket()) != nullptr)
		{
			MsgHead* msg_head = packet->GetMsgHead();
			if (!msg_head)
			{
				delete packet;
				continue;
			}
		
			switch (msg_head->_type)
			{
			case MsgType::SEND_MSG: //普通发送消息
			{
				handler->RecvMsg(packet);
				break;
			}
			case MsgType::SEND_ALL: //群发送消息
			{
				handler->RecvMsg(packet);
				break;
			}
			case MsgType::LOGIN: //登入返回
			{
				handler->LoginResponse(packet);
				break;
			}
			case MsgType::LOGOUT: //登出返回
			{
				handler->LogoutResponse(packet);
				break;
			}
			default:
				break;
			}

			delete packet;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
