#pragma once
#include <map>
#include <mutex>
#include <string>
#include <winsock2.h>

// 客户端类
struct Client
{

public:
	Client();

	SOCKET _socket;
	char _name[32];
};


// 客户端管理类
class ClientMananger
{
public:
	ClientMananger();
	~ClientMananger();

	bool Init();

	// 添加新账号
	bool AddAccount(std::string& account);
	// 校验账号密码
	int CheckAccount(char* name, char* pwd);

	bool AddClient(Client* client);
	bool DeleteClient(SOCKET socket);

	// 通过 socket 和 name 查找客户端
	Client* GetClient(SOCKET socket);
	Client* GetClient(char* name);

	std::map<SOCKET, Client*>& GetAllClient();

private:

	std::map<std::string, std::string> _record_account; // 记录账号密码

	std::mutex _clients_lock;				// 客户端管理锁
	std::map<SOCKET, Client*> _clients;	// 存储客户端信息
	std::map<std::string, Client*> _name_clients;	// 存储客户端信息
};