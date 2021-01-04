//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2020. Wenjin Yu. NetDragon. All rights reserved.
//
//	Created at 2020/5/12 19:08:04
//	Version 1.0
//
//	This program, including documentation, is protected by copyright controlled
//	by NetDragon. All rights are reserved.
//

#pragma once
#include <memory>
#include <map>
#include <json/json.h>
#include <Base/Util.h>
#include <Base/MsgType.h>
#include <Base/JsonTool.h>
#include <Base/MsgItem.h>
#include "SingleQueue.h"
#include "Server.h"
#include "IServerPipe.h"

using boost::asio::ip::tcp;

namespace wind {

struct JMsgItem
{
	JMsgItem(uint32 scId, EMsgType msgType, JValue& val)
		: scId_(scId)
		, msgType_(msgType)
		, val_(val)
	{

	}

	uint32 scId_;
	uint32 userId_ = 0;
	EMsgType msgType_;
	JValue val_;
};
typedef std::shared_ptr<JMsgItem> JMsgItemPtr;

struct Client
{
	Client(uint32 scId) 
		: scId_(scId)
	{

	}

	uint32 scId_ = 0;
	uint32 userId_ = 0;
};
typedef std::shared_ptr<Client> ClientPtr;

class Proxy : public wind::IServerPipe
{
public:
	Proxy(int port) 
	{
		endPointPtr_ = new tcp::endpoint(tcp::v4(), port);
		serverPtr_ = new Server(*endPointPtr_, this);
		serverPtr_->Start();
	}

	void Start()
	{
		runThr_ = std::thread([this]()->void {
			while (true) {
				LogSave("server.log", "Socket size: %d", scClientMap_.size());

				for (auto it = disconnectScMap_.begin(); it != disconnectScMap_.end(); ++it) {
					if (!it->second.closeSocket_ && it->second.closeTime_ <= time(nullptr)) {
						uint32 scId = it->first;

						it->second.closeSocket_ = true;
						serverPtr_->CloseSocket(scId);
					}
				}

				JMsgItemPtr inMsgPtr = nullptr;
				while (inMsgPtr = inMsgs_.Read()) {
					auto& inMsg = *inMsgPtr;
					auto& val = inMsg.val_;

					if (disconnectScMap_.count(inMsg.scId_)) {
						LogSave("server.log", "Socket already in disconnect map: [%d][%d][%d]", inMsg.userId_, inMsg.scId_, inMsg.msgType_);
						continue;
					}

					switch (inMsg.msgType_) {
					case EMsgType::CreateSocket:
					{
						WD_IF (scClientMap_.count(inMsg.scId_)) {
							LogSave("server.log", "Duplicate socket id on create: [%d]", inMsg.scId_);
							break;
						}

						scClientMap_.insert({ inMsg.scId_, make_shared<Client>(inMsg.scId_) });

						LogSave("server.log", "Create socket: [%d]", inMsg.scId_);
					}
					break;
					case EMsgType::SocketError:
					{
						if (disconnectScMap_.count(inMsg.scId_)) {
							LogSave("server.log", "Socket error already in disconnect map: [%d][%d][%s]", inMsg.scId_, val["code"].asInt(), val["msg"].asCString());
							break;
						}

						if (!scClientMap_.count(inMsg.scId_)) {
							LogSave("server.log", "Fail find socket: [%d]", inMsg.scId_);
							break;
						}
						auto clientPtr = scClientMap_.at(inMsg.scId_);

						if (clientPtr->userId_) {
							JValue valLogout;
							JMsgItemPtr msgItemLogoutPtr = make_shared<JMsgItem>(inMsg.scId_, EMsgType::Logout, valLogout);
							msgItemLogoutPtr->userId_ = clientPtr->userId_;
							userMsgs_.Write(msgItemLogoutPtr);
						}

						disconnectScMap_.insert({ inMsg.scId_, time(nullptr) + 1 });

						LogSave("server.log", "Socket error: [%d][%d][%d][%s]", clientPtr->userId_, inMsg.scId_, val["code"].asInt(), val["msg"].asCString());
					}
					break;
					default:
					{
						if (!scClientMap_.count(inMsg.scId_)) {
							LogSave("server.log", "Socket already destroyed: [%d][%d]", inMsg.scId_, inMsg.msgType_);
							break;
						}
						auto clientPtr = scClientMap_.at(inMsg.scId_);

						inMsg.userId_ = clientPtr->userId_;
						userMsgs_.Write(inMsgPtr);
					}
					break;
					}
				}

				JMsgItemPtr jMsgItemPtr = nullptr;
				while (jMsgItemPtr = outMsgs_.Read()) {
					auto& jMsgItem = *jMsgItemPtr;
					auto& val = jMsgItem.val_;

					if (disconnectScMap_.count(jMsgItem.scId_) && disconnectScMap_.at(jMsgItem.scId_).closeSocket_) {
						LogSave("server.log", "Fail send msg with socket already closed: [%d][%d]", jMsgItem.scId_, jMsgItem.msgType_);
						continue;
					}

					if (!scClientMap_.count(jMsgItem.scId_)) {
						LogSave("server.log", "Fail send msg out with socket not found: [%d][%d]", jMsgItem.scId_, jMsgItem.msgType_);
						continue;
					}
					auto clientPtr = scClientMap_.at(jMsgItem.scId_);

					switch (jMsgItem.msgType_) {
					case EMsgType::LoginAck:
					{
						ELoginAck result = static_cast<ELoginAck>(val["result"].asUInt());
						if (result == ELoginAck::Ok) {
							clientPtr->userId_ = val["userId"].asUInt();
						}
					}
					break;
					case EMsgType::Reset:
					{
						if (disconnectScMap_.count(jMsgItem.scId_)) {
							LogSave("server.log", "Reset already in disconnect map: [%d]", jMsgItem.scId_);
							break;
						}

						disconnectScMap_.insert({ jMsgItem.scId_, time(nullptr) + 1 });
						LogSave("server.log", "Proxy reset user msg: %d", jMsgItem.scId_);
					}
					break;
					default:
						break;
					}

					MsgItem msgItem;
					WD_IF (!msgItem.Init(jMsgItem.scId_, jMsgItem.msgType_, val))
						continue;
					serverPtr_->DeliverMsg(jMsgItem.scId_, msgItem.GetBuff(), msgItem.GetBuffSize());
				}

				Sleep(100);
			}
		});
	}

	virtual void OnCreate(uint32 scId, wind::IPInfo)
	{
		JValue val;
		val["scId"] = scId;
		inMsgs_.Write(make_shared<JMsgItem>(scId, EMsgType::CreateSocket, val));
	}

	virtual void OnMsg(uint32 scId, const char* msg, uint16 len)
	{
		MsgItem msgItem;
		WD_IF (!msgItem.Init(scId, msg, len)) {
			LogSave("server.log", "Fail init msg: [%d]", scId);
			return;
		}

		JValue val;
		JReader reader;
		WD_IF (!reader.parse(msgItem.GetBody(), msgItem.GetBody() + msgItem.GetBodySize(), val))
			return;

		inMsgs_.Write(make_shared<JMsgItem>(scId, static_cast<EMsgType>(msgItem.head_.msgType_), val));
	}

	virtual void OnError(uint32 scId, int code, std::string msg)
	{
		JValue val;
		val["scId"] = scId;
		val["code"] = code;
		val["msg"] = msg;
		inMsgs_.Write(make_shared<JMsgItem>(scId, EMsgType::SocketError, val));
	}

	virtual void OnDestroy(uint32 scId)
	{
		LogSave("server.log", "Destroy socket: [%d]", scId);

		WD_IF (!scClientMap_.count(scId))
			return;
		auto clientPtr = scClientMap_.at(scId);

		LogSave("server.log", "Destroy socket next: [%d][%d]", clientPtr->userId_, scId);

		userClientMap_.erase(clientPtr->userId_);
		scClientMap_.erase(scId);
		disconnectScMap_.erase(scId);
	}

	virtual void OnFlashDisconnect(uint32 scId)
	{

	}

	virtual void OnFlashReconnect(uint32 scId, wind::IPInfo)
	{

	}

	virtual void OnMsgTimeout(uint32 scId)
	{

	}

	void SendMsg(MsgItemPtr msgItemPtr)
	{

	}

	void SendMsg(uint32 scId, EMsgType msgType, JValue& val)
	{
		outMsgs_.Write(make_shared<JMsgItem>(scId, msgType, val));
	}

	JMsgItemPtr ReadUserMsg()
	{
		return userMsgs_.Read();
	}

private:
	std::map<uint32, ClientPtr> scClientMap_;
	std::map<uint32, ClientPtr> userClientMap_;
	struct DisconnectInfo {
		DisconnectInfo(time_t closeTime) : closeTime_(closeTime), closeSocket_(false) {}

		time_t closeTime_ = 0;
		bool closeSocket_ = false;
	};
	map<uint32, DisconnectInfo> disconnectScMap_;
	SingleQueue<JMsgItem> inMsgs_;
	SingleQueue<JMsgItem> outMsgs_;
	SingleQueue<JMsgItem> userMsgs_;
	tcp::endpoint* endPointPtr_;
	Server* serverPtr_;
	std::thread runThr_;
};

}