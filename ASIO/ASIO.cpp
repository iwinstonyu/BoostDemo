// ASIO.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


void AsioTimer()
{
	boost::asio::io_service io;

	boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));
	t.wait();

	std::cout << "Hello, world!" << std::endl;
}

int main()
{
	AsioTimer();

	system("pause");
    return 0;
}

