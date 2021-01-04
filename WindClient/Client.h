//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2017. WenJin Yu. windpenguin@gmail.com.
//
//	Created at 2017/9/6 13:39:10
//	Version 1.0
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <Base/Msg.h>
#include <Base/Util.h>
#include "IClientPipe.h"

namespace wind {

using boost::asio::ip::tcp;
using namespace std;

class Client : public std::enable_shared_from_this<Client> {
public:
	Client(IClientPipe* pipePtr, string ip, int port)
		: socket_(ioService_)
		, pipePtr_(pipePtr)
		, ip_(ip)
		, port_(port)
	{
	}

	virtual ~Client()
	{
		ioService_.stop();
		if (serviceThr_.joinable())
			serviceThr_.join();
	}

	void Start()
	{
		tcp::resolver resovler(ioService_);
		auto endpointIterator = resovler.resolve({ ip_, std::to_string(port_) });
		ConnectServer(endpointIterator);

		serviceThr_ = std::thread([this]()->void {
			LogSave("client.log", "Running io service");
			ioService_.run();
			LogSave("client.log", "Stop io service");
		});
	}

	void SendMsg(const string& msgStr) 
	{
		auto self(shared_from_this());
		ioService_.post([this, self, msgStr]() {
			if (isStop_)
				return;

			Msg msg;
			msg.Init(msgStr);

			outMsgs_.push_back(msg);
			if (outMsgs_.size() == 1) {
				WriteMsg();
			}
		});
	}

	void SendMsg(const char* msg, size_t len)
	{
		string msgStr(msg, len);
		SendMsg(msgStr);
	}

	void Close()
	{
		LogSave("client.log", "Close client");
		ioService_.post([this]() { 
			LogSave("client.log", "Close socket");
			isStop_ = true;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both); 
			socket_.close();
			pipePtr_->OnDisconnect();
		});
	}

private:
	void ConnectServer(tcp::resolver::iterator endpointIterator) 
	{
		LogSave("client.log", "Client connecting");
		auto self(shared_from_this());
		boost::asio::async_connect(socket_, endpointIterator,
			[this, self](boost::system::error_code ec, tcp::resolver::iterator) {
			if (isStop_)
				return;

			if (!ec) {
				pipePtr_->OnConnect();
				ReadMsgHeader();
			}
			else {
				if (!isStop_) {
					isStop_ = true;
					pipePtr_->OnError(ec.value(), ec.message());
					boost::system::error_code ec;
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
					if (ec) {
						LogSave("client.log", "Shutdown error: [%d][%s]", ec.value(), ec.message());
					}
					socket_.close();
					pipePtr_->OnDisconnect();
				}
			}
		});
	}

	void ReadMsgHeader() 
	{
		inMsg_.Clear();
		auto self(shared_from_this());
		boost::asio::async_read(socket_, boost::asio::buffer(inMsg_.Data(), Msg::HEADER_LENGTH),
			[this, self](boost::system::error_code ec, size_t length) {
			if (isStop_)
				return;

			if (!ec && inMsg_.DecodeHeader()) {
				ReadMsgBody();
			}
			else {
				if (!isStop_) {
					isStop_ = true;
					pipePtr_->OnError(ec.value(), ec.message());
				}
			}
		});
	}

	void ReadMsgBody() 
	{
		auto self(shared_from_this());
		boost::asio::async_read(socket_, boost::asio::buffer(inMsg_.Body(), inMsg_.BodyLength()),
			[this, self](boost::system::error_code ec, size_t length) {
			if (isStop_)
				return;

			if (!ec) {
				pipePtr_->OnMsg(inMsg_.Body(), inMsg_.BodyLength());
				ReadMsgHeader();
			}
			else {
				if (!isStop_) {
					isStop_ = true;
					pipePtr_->OnError(ec.value(), ec.message());
				}
			}
		});
	}

	void WriteMsg() 
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(outMsgs_.front().Data(), outMsgs_.front().Length()),
			[this, self](boost::system::error_code ec, size_t length) {
			if (isStop_)
				return;

			if (!ec) {
				outMsgs_.pop_front();
				if (!outMsgs_.empty())
					WriteMsg();
			}
			else {
				if (!isStop_) {
					isStop_ = true;
					pipePtr_->OnError(ec.value(), ec.message());
				}
			}
		});
	}

private:
	boost::asio::io_service ioService_;
	tcp::socket socket_;
	IClientPipe* pipePtr_ = nullptr;
	string ip_;
	int port_ = 0;
	Msg inMsg_;
	deque<Msg> outMsgs_;
	std::thread serviceThr_;
	bool isStop_ = false;
};
typedef shared_ptr<Client> ClientPtr;

} // namespace wind