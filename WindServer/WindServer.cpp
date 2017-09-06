// WindServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Server.h"

using boost::asio::ip::tcp;
using namespace wind;

int main()
{
	try {
		boost::asio::io_service io_service;

		tcp::endpoint endpoint(tcp::v4(), 13579);
		Server server(io_service, endpoint);

		io_service.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what();
	}

	system("pause");
    return 0;
}

