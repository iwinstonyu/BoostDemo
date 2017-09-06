// WindClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Client.h"

#include <iostream>
#include <boost/asio.hpp>
#include <thread>

using namespace std;
using namespace wind;

int main()
{
	try {
		boost::asio::io_service io_service;
		
		tcp::resolver resovler(io_service);
		auto endpoint_iterator = resovler.resolve({ "localhost", "13579" });
		Client client(io_service, endpoint_iterator);

		std::thread t([&io_service]() { io_service.run(); });

		char szContent[Msg::MAX_BODY_LENGTH + 1] = "";
		while (std::cin.getline(szContent, Msg::MAX_BODY_LENGTH + 1)) {
			Msg msg;
			msg.SetBodyLength(strlen(szContent));
			memcpy(msg.Body(), szContent, msg.BodyLength());
			msg.EncodeHeader();
			client.SendMsg(msg);
		}

	}
	catch (std::exception& e) {
		cout << "Exception: " << e.what() << endl;
	}
	system("pause");
    return 0;
}

