// ASIO.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <iostream>
#include "AsioTimer.h"
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>


int main()
{
	//SyncTimer();
	//AsyncTimer();
	Timer3();

	boost::asio::async_write()

	system("pause");
    return 0;
}

