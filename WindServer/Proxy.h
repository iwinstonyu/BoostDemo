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
	Proxy() 
	{
		endPointPtr_ = new tcp::endpoint(tcp::v4(), 13579);
		serverPtr_ = new Server(*endPointPtr_, this);
		serverPtr_->Start();
	}

	void Start()
	{
		runThr_ = std::thread([this]()->void {
			while (true) {
				for (auto it = disconnectScMap_.begin(); it != disconnectScMap_.end(); ) {
					if (it->second < time(nullptr)) {
						serverPtr_->CloseSocket(it->first);
						it = disconnectScMap_.erase(it);
					}
					else {
						++it;
					}
				}

				JMsgItemPtr inMsgPtr = nullptr;
				while (inMsgPtr = inMsgs_.Read()) {
					auto& inMsg = *inMsgPtr;
					auto& val = inMsg.val_;

					switch (inMsg.msgType_) {
					case EMsgType::CreateSocket:
					{
						if (scClientMap_.count(inMsg.scId_)) {
							LogSave("Duplicate socket id on create: [%d]", inMsg.scId_);
							continue;
						}

						scClientMap_.insert({ inMsg.scId_, make_shared<Client>(inMsg.scId_) });

						LogSave("Create socket: [%d]", inMsg.scId_);
					}
					break;
					default:
					{
						if (!scClientMap_.count(inMsg.scId_)) {
							continue;
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

					if (!scClientMap_.count(jMsgItem.scId_)) {
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
						disconnectScMap_.insert({ jMsgItem.scId_, time(nullptr) + 1 });
					}
					break;
					default:
						break;
					}

					MsgItem msgItem;
					if (!msgItem.Init(jMsgItem.scId_, jMsgItem.msgType_, val))
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
		if (!msgItem.Init(scId, msg, len)) {
			LogSave("Fail init msg: [%d]", scId);
			return;
		}

		JValue val;
		JReader reader;
		if (!reader.parse(msgItem.GetBody(), msgItem.GetBody() + msgItem.GetBodySize(), val))
			return;

		inMsgs_.Write(make_shared<JMsgItem>(scId, static_cast<EMsgType>(msgItem.head_.msgType_), val));
	}

	virtual void OnError(uint32 scId, int, const char*)
	{

	}

	virtual void OnDestroy(uint32 scId)
	{
		LogSave("Destroy socket: [%d]", scId);

		scClientMap_.erase(scId);
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
	map<uint32, time_t> disconnectScMap_;
	SingleQueue<JMsgItem> inMsgs_;
	SingleQueue<JMsgItem> outMsgs_;
	SingleQueue<JMsgItem> userMsgs_;
	tcp::endpoint* endPointPtr_;
	Server* serverPtr_;
	std::thread runThr_;
};

}