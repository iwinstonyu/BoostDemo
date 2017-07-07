#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

//
// using a timer synchronously
//
void SyncTimer()
{
	boost::asio::io_service io;

	boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));
	t.wait();

	std::cout << "Hello, SyncTimer!" << std::endl;
}




//
// using a timer asynchronously
//
// void print(const boost::system::error_code& /*e*/)
// {
// 	std::cout << "Hello, world!" << std::endl;
// }
// 
// int main()
// {
// 	boost::asio::io_service io;
// 
// 	boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));
// 	t.async_wait(&print);
// 
// 	io.run();
// 
// 	return 0;
// }
void AsyncTimer()
{
// 	void print(const boost::system::error_code& /*e*/)
// 	{
// 		std::cout << "Hello, world!" << std::endl;
// 	}

	boost::asio::io_service io;

	boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));
	t.async_wait([](const boost::system::error_code& /*e*/)->void
	{
		std::cout << "Hello, AsyncTimer!" << std::endl;
	});

	io.run();

	std::cout << "I'm hero!" << std::endl;
}

#include <boost/bind.hpp>

void print(const boost::system::error_code& /*e*/,
	boost::asio::deadline_timer* t, int* count)
{
	if (*count < 5)
	{
		std::cout << *count << std::endl;
		++(*count);

		t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
		t->async_wait(boost::bind(print,
			boost::asio::placeholders::error, t, count));
	}
}

void Timer3()
{
	boost::asio::io_service io;

	int count = 0;
	boost::asio::deadline_timer t(io, boost::posix_time::seconds(1));
	t.async_wait(boost::bind(print,
		boost::asio::placeholders::error, &t, &count));

	io.run();

	std::cout << "Final count is " << count << std::endl;
}