// MessageServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <chrono>
#include <thread>

#include "MessageServer.h"

int main()
{
    std::cout << "Message Server Start !\n";

    MessageServer MessageServer;

    // 启动服务
    if(!MessageServer.StartServer())
    {
        return -1;
    }
    
    while(MessageServer.IsRuning())
    {
	    std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Message Server Closed !\n";
}
