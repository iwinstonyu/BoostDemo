#pragma once

#include "Client.h"
#include "IClientPipe.h"
#include "SingleQueue.h"
#include <Base/JsonTool.h>
#include <Base/MsgType.h>
#include <Base/MsgItem.h>
#include <Base/Util.h>

namespace wind {

struct JMsgItem
{
	JMsgItem(EMsgType msgType, JValue& val)
		: msgType_(msgType)
		, val_(val)
	{

	}

	EMsgType msgType_;
	JValue val_;
};
typedef std::shared_ptr<JMsgItem> JMsgItemPtr;

class RobotClient : public IClientPipe
{
public:
	RobotClient(uint32 userId, string ip, int port)
		: userId_(userId)
	{
		clientPtr_ = new Client(this, ip, port);
		clientPtr_->Start();
	}

	virtual void OnConnect()
	{
		JValue val;
		inMsgs_.Write(make_shared<JMsgItem>(EMsgType::ConnectSocket, val));
	}
	virtual void OnDisconnect()
	{
		JValue val;
		inMsgs_.Write(make_shared<JMsgItem>(EMsgType::DisconnectSocket, val));
	}
	virtual void OnMsg(const char* msg, unsigned short len)
	{
		MsgItem msgItem;
		WD_IF (!msgItem.Init(0, msg, len)) {
			LogSave("Fail init msg");
			return;
		}

		JValue val;
		JReader reader;
		WD_IF (!reader.parse(msgItem.GetBody(), msgItem.GetBody() + msgItem.GetBodySize(), val))
			return;

		inMsgs_.Write(make_shared<JMsgItem>(static_cast<EMsgType>(msgItem.head_.msgType_), val));
	}
	virtual void OnError(int, const char*)
	{
		JValue val;
		inMsgs_.Write(make_shared<JMsgItem>(EMsgType::SocketError, val));
	}
	virtual void OnReconnect()
	{

	}
	virtual void OnReconnected()
	{

	}

	void SendMsg(EMsgType msgType, JValue& val)
	{
		MsgItem msgItem;
		WD_IF (!msgItem.Init(0, msgType, val)) {
			return;
		}

		clientPtr_->SendMsg(msgItem.GetBuff(), msgItem.GetBuffSize());
	}

	void Start()
	{
		runThr_ = std::thread([this]()->void {
			while(true) {
				JMsgItemPtr inMsgPtr = nullptr;
				while (inMsgPtr = inMsgs_.Read()) {
					auto& inMsg = *inMsgPtr;
					auto& val = inMsg.val_;

					switch (inMsg.msgType_) {
					case EMsgType::ConnectSocket:
					{
						isConnecting_ = true;

						LogSave("Connect socket: [%d]", userId_);

						JValue val;
						val["userId"] = userId_;
						SendMsg(EMsgType::Login, val);
					}
					break;
					case EMsgType::LoginAck:
					{
						isLogin_ = true;

						LogSave("Login ack: [%d]", userId_);

						JValue val;
						val["content"] = "Hello world";
						SendMsg(EMsgType::Talk, val);
					}
					break;
					case EMsgType::TalkAck:
					{
						LogSave("Talk ack: [%d][%d]", userId_, val["result"].asUInt());
					}
					break;
					case EMsgType::Reset:
					{
						LogSave("Reset user: [%d]", userId_);
						return;
					}
					break;
					case EMsgType::SocketError:
					{
						LogSave("Socket error: [%d]", userId_);
						return;
					}
					break;
					default:
					{
					}
					break;
					}
				}
			}
		});
	}

private:
	Client* clientPtr_ = nullptr;
	std::thread runThr_;
	uint32 userId_ = 0;
	bool isConnecting_ = false;
	bool isLogin_ = false;
	SingleQueue<JMsgItem> inMsgs_;
};

}
