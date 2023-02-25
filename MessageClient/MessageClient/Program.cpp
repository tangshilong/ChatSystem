// MessageClient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <conio.h>
#include <iostream>

#include "Client.h"

#pragma comment (lib,"ws2_32")

int main()
{

	Client client;
	client.SetRunning();

	std::cout << "Please input server address. Like: HI 127.0.0.1:8000" << std::endl;

	std::string str;
	while (true)
	{
		str.clear();

		char ch;
		while((ch = _getche()))
		{
			// 退格
			if(ch == '\b')
			{
				if(!str.empty())
				{
					str.pop_back();
					printf(" \b");
				}
			}
			// 换行
			else if (ch == '\\')
			{

				printf("\b \n");
				// std::cout << std::endl;
				str += "\n";
			}
			else if (ch == '\n' || ch == '\r')
			{
				printf("\n");
				break;
			}else
				str += ch;

		}

		// 处理指令
		client.DealInputMsg(str);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
