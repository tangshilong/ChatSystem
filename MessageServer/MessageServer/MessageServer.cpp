
#include "MessageServer.h"
#include <functional>
#include <iostream>

#define BUFF_SIZE 1024
/*
*
��ʾ����ws2_32.lib����⡣���ڹ���������д������ws2_32.lib��Ч��һ�������ַ�ʽ�ȼۣ���˵һ����ʽһ����ʽ���ã���
*/
#pragma comment(lib,"ws2_32.lib")

// #define _WINSOCK_DEPRECATED_NO_WARNINGS


MessageServer::MessageServer() :
	_is_running(false),
	_ip("127.0.0.1"),
	_port(8000),
    _server_socket(INVALID_SOCKET)
{
	
}

MessageServer::~MessageServer()
{
	if(_server_socket != -1)
	{
        closesocket(_server_socket);
	}
}

bool MessageServer::InitSocket()
{
    WSADATA data_;
    if (WSAStartup(MAKEWORD(2, 2), &data_) != 0)
    {
        std::cout << "WSAStartup() init error " << GetLastError() << std::endl;
        return false;
    }

    //�����׽���
    // AF_INET :   ��ʾʹ�� IPv4 ��ַ		��ѡ����
    // SOCK_STREAM ��ʾʹ���������ӵ����ݴ��䷽ʽ��
    // IPPROTO_TCP ��ʾʹ�� TCP Э��
    _server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_server_socket == INVALID_SOCKET)
    {
        std::cout << "Socket Error !" << std::endl;
        return false;
    }

    // int on = 1;//ֻ���·����ÿ��ظ���ʹ�õĶ˿��õ�
    // setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));//����Ϊ���ظ�ʹ�õĶ˿�

    //���׽��ֺ�IP���˿ڰ�
    SOCKADDR_IN serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;  //ʹ��IPv4��ַ
    // serv_addr.sin_addr.s_addr = inet_addr(_ip.c_str());  //�����IP��ַ
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(_port);  //�˿�

    if(bind(_server_socket, (LPSOCKADDR)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
    {
        std::cout << "Bind Error !" << std::endl;
        return false;
    }

    //�������״̬���ȴ��û���������
    if(listen(_server_socket, 128) == SOCKET_ERROR)
    {
        std::cout << "Listen Error !" << std::endl;
        return false;
    }


    return true;
}

bool MessageServer::StartServer()
{
    SetRunning();

    _client_manager.Init();

	// ��socket
    if(InitSocket())
    {
    	// ����select�߳�
        _select_thread = std::thread(SelectThread, this);

        // ��ʼ��Ϣ����
        _msg_handler.SetClientManager(&_client_manager);
        _msg_handler.SetMessageServer(this);
        _msg_handler.Start();
        
        return true;
    }

    return false;
}

bool MessageServer::IsRuning()
{
	return _is_running;
}

void MessageServer::SetRunning(bool status)
{
    _is_running = true;
}

SOCKET MessageServer::GetServerSocekt()
{
    return _server_socket;
}

ClientMananger& MessageServer::GetClientManger()
{
    return _client_manager;
}

MsgHandler& MessageServer::GetMessageHandler()
{
    return _msg_handler;
}

void MessageServer::SelectThread(MessageServer* message_server)
{
    ClientMananger& client_manager = message_server->GetClientManger();
    MsgHandler& message_handler = message_server->GetMessageHandler();


    int sockfd = message_server->GetServerSocekt();

    //��ʼ���������� һ������
    int client[FD_SETSIZE];//�������ڴ�ſͷ���fd ѭ����ѯʹ�� FD_SETSIZE ��һ���� Ĭ���Ѿ����1024
    fd_set rset;//����select�� �����ж����ж�����fd һ���һ��
    fd_set allset;//ֻ�����fd
    FD_ZERO(&rset);//��յĶ���
    FD_ZERO(&allset);//��յĶ���

    //�Ƚ������׽��ַ���
    FD_SET(sockfd, &allset);

    //��ʼ�����鶼Ϊ-1 ӦΪ��ʶ����0��ʼ
    for (int i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;


   
    int nready = 0;
    while(message_server->IsRuning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        int maxfd = sockfd;
        //�ǳ��ؼ���һ��
        rset = allset;//��֤ÿ��ѭ�� select�ܼ������е��ļ������� ��Ϊrsetֻ��ʣ���ж�����
        timeval t = { 0,0 };        //��ȫ������
        nready = select(maxfd + 1, &rset, NULL, NULL, &t);//����һ
        // nready = select(0, &rset, NULL, NULL, NULL);//����һ

        if (nready < 0) {
            perror("select�쳣���������");
            break;
        }

        //�¿ͻ���
        if (FD_ISSET(sockfd, &rset))
        {
            SOCKADDR_IN client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            int len = sizeof(client_addr);

            int conn_fd = accept(sockfd, (struct sockaddr*)&client_addr, &len);

            std::cout << "Client connect success " << inet_ntoa(client_addr.sin_addr) << " port " << ntohs(client_addr.sin_port) << std::endl;

            //��������һ���ļ�df��������
            for (int i = 0; i < FD_SETSIZE; i++)
            {
                if (client[i] < 0)
                {
                    client[i] = conn_fd;
                    break;
                }
            }
            //��������������뼯��
            FD_SET(conn_fd, &allset);//���뼯��
            //��ֹ������Χ//select�ĵ�һ�����������Ǽ��ӵ��ļ�������+1 ����������µĿͻ����� ���ֵ���ϱ�� �����͸�ֵ
            if (maxfd < conn_fd)
                maxfd = conn_fd;

            //�·���ʾ ���ͬһʱ��ֻ�� һ������ ����������·���else�жϴ��� �����ֹһ�� nready-1 �ٽ����·��ж�
            if (--nready <= 0)
                continue;

        }
        else
        {
            //������FD�����ɶ��¼�
            for (int i = 0; i < FD_SETSIZE; i++)
            {

                if (FD_ISSET(client[i], &rset))
                {
                    int conn_fd = client[i];
                    char buf[BUFF_SIZE] = { 0 };
                    int nRecv;
                    nRecv = recv(conn_fd, buf, sizeof(buf),0);
                    if (nRecv <= 0)//��ʾ�ͷ��˶Ͽ�����
                    {
                        if (!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
                        {
                            //��ӡ˵�� �Ӽ�����ɾ��  ��������ɾ�� �رտͷ���
                            FD_CLR(conn_fd, &allset);
                            client[i] = -1;
                            closesocket(conn_fd);

                            Client* client = client_manager.GetClient(conn_fd);
                            if (client && strlen(client->_name) > 0)
                                std::cout << client->_name << " is closed.." << std::endl;
                            else
                                std::cout << "client is closed.." << std::endl;

                            client_manager.DeleteClient(conn_fd);
                        }

                    }
                    else //if (nRecv > 0)
                    {
                        MsgPacket* packet = new MsgPacket();
                        packet->SetSocket(conn_fd);
                        packet->SetMsg(buf, nRecv);

                        message_handler.PushRecvPacket(packet);
                        memset(buf, 0, BUFF_SIZE);
                    }

                    //�·���ʾ���ͬ��ʱ������ɶ��¼�ֻ��һ�� �����ٽ�����Ԫ�ؽ���ѭ���ȶ� ֱ������
                     //������ѭ������ �˷�ʱ��
                    if (--nready <= 0)
                        break;
                }
            }//endfor
        }//endif
    }//endwhile
    

    // _clint_map
}
