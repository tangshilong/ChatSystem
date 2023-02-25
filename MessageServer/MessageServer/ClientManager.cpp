#include "ClientManager.h"

#include <iostream>
#include <ostream>
#include <direct.h>
#include <fstream>


Client::Client():
	_socket(INVALID_SOCKET)
{
	memset(_name,0,sizeof(_name));
}

ClientMananger::ClientMananger()
{
	
}

ClientMananger::~ClientMananger()
{
	
}

bool ClientMananger::Init()
{
	// 读取已经记录的账号密码
	char* path = getcwd(NULL, 0);
	std::string file_path = path;
	file_path.append("\\Account.txt");

	std::ifstream file;
	file.open(file_path, std::ios::in);

	if (!file.is_open())
		return false;

	std::cout << "Account list:" << std::endl;
	std::string strLine;
	while (getline(file, strLine))
	{
		if (strLine.empty())
			continue;

		AddAccount(strLine);
	}
	std::cout << std::endl;

	file.close();
}

bool ClientMananger::AddAccount(std::string& account)
{
	std::cout << account << std::endl;
	int pos = account.find_first_of(':');
	_record_account.insert(std::make_pair(account.substr(0,pos), account.substr(pos+1)));
	return true;
}

int ClientMananger::CheckAccount(char* name, char* pwd)
{
	auto iter = _record_account.find(name);
	// 没找到，是新账号，则直接记录
	if(iter == _record_account.end())
	{
		_record_account.insert(std::make_pair(name, pwd));

		char* path = getcwd(NULL, 0);
		std::string file_path = path;
		file_path.append("\\Account.txt");

		 // 以写模式打开文件
		std::ofstream outfile;
		outfile.open(file_path, std::ios::app);
		// 向文件写入用户输入的数据
		outfile << name << ":" << pwd << std::endl;
		// 关闭打开的文件
		outfile.close();
		return true;
	}
	else
	{
		// 找到了则校验密码
		if (strcmp(iter->second.c_str(), pwd) == 0)
			return true;
		else
			return false;
	}
}

bool ClientMananger::AddClient(Client* client)
{
	std::lock_guard<std::mutex> lock(_clients_lock);

	if(_clients.find(client->_socket) == _clients.end())
	{
		_clients.insert(std::make_pair(client->_socket, client));
	}
	else
	{
		_clients[client->_socket] = client;
	}

	if (_name_clients.find(client->_name) == _name_clients.end())
	{
		_name_clients.insert(std::make_pair(client->_name, client));
	}
	else
	{
		_name_clients[client->_name] = client;
	}

	return false;
}

bool ClientMananger::DeleteClient(SOCKET socket)
{
	std::lock_guard<std::mutex> lock(_clients_lock);
	auto client_iter = _clients.find(socket);
	char name[32] = { 0 };
	if (client_iter != _clients.end())
	{
		strcpy(name, client_iter->second->_name);
		_clients.erase(client_iter);
	}

	auto name_client_iter = _name_clients.find(name);
	if (name_client_iter != _name_clients.end())
	{
		_name_clients.erase(name_client_iter);
		return true;
	}

	return false;
}

Client* ClientMananger::GetClient(SOCKET socket)
{
	std::lock_guard<std::mutex> lock(_clients_lock);
	auto client_iter = _clients.find(socket);
	if (client_iter != _clients.end())
	{
		return client_iter->second;
	}
	return nullptr;
}

Client* ClientMananger::GetClient(char* name)
{
	std::lock_guard<std::mutex> lock(_clients_lock);
	auto client_iter = _name_clients.find(name);
	if (client_iter != _name_clients.end())
	{
		return client_iter->second;
	}
	return nullptr;
}

std::map<SOCKET, Client*>& ClientMananger::GetAllClient()
{
	return _clients;
}