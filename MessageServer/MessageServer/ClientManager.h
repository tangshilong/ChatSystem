#pragma once
#include <map>
#include <mutex>
#include <string>
#include <winsock2.h>

// �ͻ�����
struct Client
{

public:
	Client();

	SOCKET _socket;
	char _name[32];
};


// �ͻ��˹�����
class ClientMananger
{
public:
	ClientMananger();
	~ClientMananger();

	bool Init();

	// ������˺�
	bool AddAccount(std::string& account);
	// У���˺�����
	int CheckAccount(char* name, char* pwd);

	bool AddClient(Client* client);
	bool DeleteClient(SOCKET socket);

	// ͨ�� socket �� name ���ҿͻ���
	Client* GetClient(SOCKET socket);
	Client* GetClient(char* name);

	std::map<SOCKET, Client*>& GetAllClient();

private:

	std::map<std::string, std::string> _record_account; // ��¼�˺�����

	std::mutex _clients_lock;				// �ͻ��˹�����
	std::map<SOCKET, Client*> _clients;	// �洢�ͻ�����Ϣ
	std::map<std::string, Client*> _name_clients;	// �洢�ͻ�����Ϣ
};