//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2017. WenJin Yu. windpenguin@gmail.com.
//
//	Created at 2017/9/4 15:34:42
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
#include <iostream>
#include <set>
#include <deque>
#include <thread>
#include <Base/Util.h>
#include <Base/Msg.h>
#include "IServerPipe.h"

namespace wind {

using namespace std;
using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(uint32 scId, tcp::socket socket, IServerPipe* pipePtr)
		: scId_(scId)
		, socket_(std::move(socket))
		, pipePtr_(pipePtr)
	{
		IPInfo info;
		strncpy_s(info.ip_, socket_.remote_endpoint().address().to_string().c_str(), socket_.remote_endpoint().address().to_string().length());
		info.port_ = socket_.remote_endpoint().port();
		pipePtr_->OnCreate(scId_, info);
	}

	~Session() 
	{
		socket_.close();
		pipePtr_->OnDestroy(scId_);
	}

	uint32 ScId() { return scId_; }

	void DeliverMsg(const string& msgStr)
	{
		Msg msg;
		msg.Init(msgStr);

		outMsgs_.push_back(msg);
		if (outMsgs_.size() == 1) {
			WriteMsg();
		}
	}

	void Start() {
		ReadMsgHeader();
	}

private:
	void ReadMsgHeader() 
	{
		inMsg_.Clear();
		auto self(shared_from_this());
		boost::asio::async_read(socket_, boost::asio::buffer(inMsg_.Data(), Msg::HEADER_LENGTH),
			[this, self](boost::system::error_code ec, size_t length) {
			if (!ec && inMsg_.DecodeHeader()) {
				ReadMsgBody();
			}
			else {
				pipePtr_->OnError(scId_, ec.value(), ec.message());
			}
		});
	}

	void ReadMsgBody() 
	{
		auto self(shared_from_this());
		boost::asio::async_read(socket_, boost::asio::buffer(inMsg_.Body(), inMsg_.BodyLength()),
			[this, self](boost::system::error_code ec, size_t length) {
			if (!ec) {
				pipePtr_->OnMsg(scId_, inMsg_.Body(), inMsg_.BodyLength());
				ReadMsgHeader();
			}
			else {
				pipePtr_->OnError(scId_, ec.value(), ec.message());
			}
		});
	}

	void WriteMsg()
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(outMsgs_.front().Data(), outMsgs_.front().Length()),
			[this, self](boost::system::error_code ec, size_t length) {
			if (!ec) {
				outMsgs_.pop_front();
				if (!outMsgs_.empty())
					WriteMsg();
			}
			else {
				pipePtr_->OnError(scId_, ec.value(), ec.message());
			}
		});
	}

private:
	tcp::socket socket_;
	Msg inMsg_;
	deque<Msg> outMsgs_;
	uint32 scId_ = 0;
	IServerPipe* pipePtr_ = nullptr;
};
typedef shared_ptr<Session> SessionPtr;

class SessionPool
{
public:
	SessionPool() 
	{

	}

	void Join(SessionPtr sessionPtr) 
	{
		sessions_.insert({ sessionPtr->ScId(), sessionPtr });
	}

	void Leave(uint32 scId) 
	{
		if (!sessions_.count(scId)) {
			LogSave("Fail leave no socket id: [%d]", scId);
			return;
		}

		sessions_.erase(scId);
	}

	void LeaveAll() 
	{
		sessions_.clear();
	}

	void DeliverMsg(uint32 scId, const string& msg) 
	{
		if (!sessions_.count(scId)) {
			LogSave("Fail deliver msg no socket id: [%d]", scId);
			return;
		}

		sessions_.at(scId)->DeliverMsg(msg);
	}

private:
	std::map<uint32, SessionPtr> sessions_;
};

class Server 
{
public:
	Server(const tcp::endpoint& endpoint, IServerPipe* pipePtr)
		: acceptor_(ioService_, endpoint)
		, socket_(ioService_)
		, pipePtr_(pipePtr)
	{
		LogSave("Start server");
		AcceptClient();
	}

	void DeliverMsg(uint32 scId, const string& msg)
	{
		ioService_.post([this, scId, msg]()->void {
			pool_.DeliverMsg(scId, msg);
		});
	}

	void DeliverMsg(uint32 scId, const char* msg, size_t len) 
	{ 
		string msgStr(msg, len);
		DeliverMsg(scId, msgStr);
	}

	void CloseSocket(uint32 scId)
	{
		ioService_.post([this, scId]()->void {
			pool_.Leave(scId);
		});
	}

	void Start()
	{
		serviceThr_ = std::thread([this]()->void { 
			LogSave("Run io service");
			ioService_.run(); 
		});
	}

	void Stop()
	{
		ioService_.post([this]()->void {
			pool_.LeaveAll();
		});
		Sleep(1000);
		ioService_.stop();
	}

private:
	void AcceptClient() 
	{
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
			if (!ec) {
				LogSave("Accept socket: [%s:%d]", socket_.remote_endpoint().address().to_string().c_str(), socket_.remote_endpoint().port());
				auto sessionPtr = std::make_shared<Session>(++socketId_, std::move(socket_), pipePtr_);
				pool_.Join(sessionPtr);
				sessionPtr->Start();
			}
			else {
				LogSave("Accept socket error: %d", ec.value());
			}
			AcceptClient();
		});
	}

	boost::asio::io_service ioService_;
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	SessionPool pool_;
	uint32 socketId_ = 0;
	IServerPipe* pipePtr_ = nullptr;
	std::thread serviceThr_;
};

} // namespace wind
