
#include "MessageServer.h"
#include <functional>
#include <iostream>

#define BUFF_SIZE 1024
/*
*
表示链接ws2_32.lib这个库。和在工程设置里写上链入ws2_32.lib的效果一样（两种方式等价，或说一个隐式一个显式调用），
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

    //创建套接字
    // AF_INET :   表示使用 IPv4 地址		可选参数
    // SOCK_STREAM 表示使用面向连接的数据传输方式，
    // IPPROTO_TCP 表示使用 TCP 协议
    _server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_server_socket == INVALID_SOCKET)
    {
        std::cout << "Socket Error !" << std::endl;
        return false;
    }

    // int on = 1;//只有下方设置可重复性使用的端口用到
    // setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));//设置为可重复使用的端口

    //将套接字和IP、端口绑定
    SOCKADDR_IN serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    // serv_addr.sin_addr.s_addr = inet_addr(_ip.c_str());  //具体的IP地址
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(_port);  //端口

    if(bind(_server_socket, (LPSOCKADDR)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
    {
        std::cout << "Bind Error !" << std::endl;
        return false;
    }

    //进入监听状态，等待用户发起请求
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

	// 绑定socket
    if(InitSocket())
    {
    	// 开启select线程
        _select_thread = std::thread(SelectThread, this);

        // 开始消息处理
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

    //初始化两个集合 一个数组
    int client[FD_SETSIZE];//数组用于存放客服端fd 循环查询使用 FD_SETSIZE 是一个宏 默认已经最大1024
    fd_set rset;//放在select中 集合中都是有动静的fd 一般就一个
    fd_set allset;//只做添加fd
    FD_ZERO(&rset);//清空的动作
    FD_ZERO(&allset);//清空的动作

    //先将监听套接字放入
    FD_SET(sockfd, &allset);

    //初始化数组都为-1 应为标识符从0开始
    for (int i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;


   
    int nready = 0;
    while(message_server->IsRuning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        int maxfd = sockfd;
        //非常关键的一步
        rset = allset;//保证每次循环 select能监听所有的文件描述符 因为rset只会剩下有动静的
        timeval t = { 0,0 };        //完全非阻塞
        nready = select(maxfd + 1, &rset, NULL, NULL, &t);//参数一
        // nready = select(0, &rset, NULL, NULL, NULL);//参数一

        if (nready < 0) {
            perror("select异常，任务结束");
            break;
        }

        //新客户端
        if (FD_ISSET(sockfd, &rset))
        {
            SOCKADDR_IN client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            int len = sizeof(client_addr);

            int conn_fd = accept(sockfd, (struct sockaddr*)&client_addr, &len);

            std::cout << "Client connect success " << inet_ntoa(client_addr.sin_addr) << " port " << ntohs(client_addr.sin_port) << std::endl;

            //做的事情一：文件df放入数组
            for (int i = 0; i < FD_SETSIZE; i++)
            {
                if (client[i] < 0)
                {
                    client[i] = conn_fd;
                    break;
                }
            }
            //做的事情二：放入集合
            FD_SET(conn_fd, &allset);//放入集合
            //防止超出范围//select的第一个参数必须是监视的文件描述符+1 如果不断有新的客户连接 最大值不断变大 超出就赋值
            if (maxfd < conn_fd)
                maxfd = conn_fd;

            //下方表示 如果同一时刻只有 一个动静 就无需进入下方的else判断处理 如果不止一个 nready-1 再进入下方判断
            if (--nready <= 0)
                continue;

        }
        else
        {
            //已连接FD产生可读事件
            for (int i = 0; i < FD_SETSIZE; i++)
            {

                if (FD_ISSET(client[i], &rset))
                {
                    int conn_fd = client[i];
                    char buf[BUFF_SIZE] = { 0 };
                    int nRecv;
                    nRecv = recv(conn_fd, buf, sizeof(buf),0);
                    if (nRecv <= 0)//表示客服端断开链接
                    {
                        if (!(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
                        {
                            //打印说明 从集合中删除  从数组中删除 关闭客服端
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

                    //下方表示如果同意时刻如果可读事件只有一个 无需再将数组元素进行循环比对 直接跳出
                     //不必让循环走完 浪费时间
                    if (--nready <= 0)
                        break;
                }
            }//endfor
        }//endif
    }//endwhile
    

    // _clint_map
}
