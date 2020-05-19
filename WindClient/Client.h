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

class Client {
public:
	Client(IClientPipe* pipePtr)
		: socket_(ioService_)
		, pipePtr_(pipePtr)
	{
		tcp::resolver resovler(ioService_);
		auto endpointIterator = resovler.resolve({ "localhost", "13579" });

		ConnectServer(endpointIterator);
	}

	virtual ~Client() 
	{
	}

	void Start()
	{
		serviceThr_ = std::thread([this]()->void {
			ioService_.run();
		});
	}

	void SendMsg(const string& msgStr) 
	{
		ioService_.post([this, msgStr]() {
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
		LogSave("Close client");
		ioService_.post([this]() { socket_.close(); });
		ioService_.stop();
		serviceThr_.join();
	}

private:
	void ConnectServer(tcp::resolver::iterator endpointIterator) 
	{
		LogSave("Client connecting");
		boost::asio::async_connect(socket_, endpointIterator,
			[this](boost::system::error_code ec, tcp::resolver::iterator) {
			if (!ec) {
				pipePtr_->OnConnect();

				ReadMsgHeader();
			}
			else {
				pipePtr_->OnError(ec.value(), "Fail connect server");
			}
		});
	}

	void ReadMsgHeader() 
	{
		inMsg_.Clear();
		boost::asio::async_read(socket_, boost::asio::buffer(inMsg_.Data(), Msg::HEADER_LENGTH),
			[this](boost::system::error_code ec, size_t length) {
			if (!ec && inMsg_.DecodeHeader()) {
				ReadMsgBody();
			}
			else {
				pipePtr_->OnError(ec.value(), "Fail read msg header");
			}
		});
	}

	void ReadMsgBody() 
	{
		boost::asio::async_read(socket_, boost::asio::buffer(inMsg_.Body(), inMsg_.BodyLength()),
			[this](boost::system::error_code ec, size_t length) {
			if (!ec) {
				pipePtr_->OnMsg(inMsg_.Body(), inMsg_.BodyLength());

				ReadMsgHeader();
			}
			else {
				pipePtr_->OnError(ec.value(), "Fail read msg body");
			}
		});
	}

	void WriteMsg() 
	{
		boost::asio::async_write(socket_, boost::asio::buffer(outMsgs_.front().Data(), outMsgs_.front().Length()),
			[this](boost::system::error_code ec, size_t length) {
			if (!ec) {
				outMsgs_.pop_front();
				if (!outMsgs_.empty())
					WriteMsg();
			}
			else {
				pipePtr_->OnError(ec.value(), "Fail write msg");
			}
		});
	}

private:
	boost::asio::io_service ioService_;
	tcp::socket socket_;
	IClientPipe* pipePtr_ = nullptr;
	Msg inMsg_;
	deque<Msg> outMsgs_;
	std::thread serviceThr_;
};
typedef shared_ptr<Client> ClientPtr;

} // namespace wind