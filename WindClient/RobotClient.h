#pragma once

#include "Client.h"
#include "IClientPipe.h"
#include "SingleQueue.h"
#include <Base/JsonTool.h>
#include <Base/MsgType.h>
#include <Base/MsgItem.h>
#include <Base/Util.h>
#include <random>

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
		std::uniform_int_distribution<int> distrib(0, 120);
		liveTime_ = time(nullptr) + distrib(engine_);

		clientPtr_ = make_shared<Client>(this, ip, port);
		clientPtr_->Start();
	}

	~RobotClient() {
		if (runThr_.joinable())
			runThr_.join();
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
			LogSave("client.log", "Fail init msg");
			return;
		}

		JValue val;
		JReader reader;
		WD_IF (!reader.parse(msgItem.GetBody(), msgItem.GetBody() + msgItem.GetBodySize(), val))
			return;

		inMsgs_.Write(make_shared<JMsgItem>(static_cast<EMsgType>(msgItem.head_.msgType_), val));
	}
	virtual void OnError(int code, string msg)
	{
		JValue val;
		val["code"] = code;
		val["msg"] = msg;
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

	bool Online() { return isConnecting_ && isLogin_ && !isStop_; }
	bool Alive() { return alive_; }

	void Start()
	{
		runThr_ = std::thread([this]()->void {
			while(alive_) {
				JMsgItemPtr inMsgPtr = nullptr;
				while (inMsgPtr = inMsgs_.Read()) {
					auto& inMsg = *inMsgPtr;
					auto& val = inMsg.val_;

					switch (inMsg.msgType_) {
					case EMsgType::ConnectSocket:
					{
						isConnecting_ = true;

						LogSave("client.log", "Connect socket: [%d]", userId_);

						JValue val;
						val["userId"] = userId_;
						SendMsg(EMsgType::Login, val);
					}
					break;
					case EMsgType::LoginAck:
					{
						isLogin_ = true;

						LogSave("client.log", "Login ack: [%d]", userId_);
					}
					break;
					case EMsgType::TalkAck:
					{
						string talkEcho = val["content"].asString();
						assert(talkEcho == talkContent_);
						LogSave("client.log", "Talk echo: [%d][%d] %.6s...%.6s", userId_, talkEcho.length(),
							talkEcho.substr(0, 6).c_str(), talkEcho.substr(talkEcho.length() - 6, 6).c_str());
						talkContent_ = "";
					}
					break;
					case EMsgType::Reset:
					{
						LogSave("client.log", "Reset user: [%d]", userId_);
						isStop_ = true;
					}
					break;
					case EMsgType::SocketError:
					{
						LogSave("client.log", "Socket error: [%d] %d %s", userId_, val["code"].asInt(), val["msg"].asCString());
						clientPtr_->Close();
						isStop_ = true;
					}
					break;
					case EMsgType::DisconnectSocket:
					{
						LogSave("client.log", "Disconnect socket: [%d]", userId_);
						isStop_ = true;
						alive_ = false;
					}
					break;
					default:
					{
					}
					break;
					}
				}

				if (liveTime_ && liveTime_ < time(nullptr)) {
					liveTime_ = 0;
					LogSave("client.log", "Time out for live: %d", userId_);
					clientPtr_->Close();
					isStop_ = true;
				}

				if (Online()) {
					if (talkTime_ <= time(nullptr) && talkContent_.empty()) {
						std::uniform_int_distribution<int> distrib(0, 30);
						talkTime_ = distrib(engine_) + time(nullptr);

						int len = 0;
						distrib.param(std::uniform_int_distribution<int>::param_type{ 0, 10 });
						int lenType = distrib(engine_);
						if (lenType < 5) {
							distrib.param(std::uniform_int_distribution<int>::param_type{ 0, 64 });
							len = distrib(engine_) + 13;
						}
						else if (lenType < 8) {
							distrib.param(std::uniform_int_distribution<int>::param_type{ 0, 1024 });
							len = distrib(engine_) + 13;
						}
						else {
							distrib.param(std::uniform_int_distribution<int>::param_type{ 0, 8000 });
							len = distrib(engine_) + 13;
						}

						char arr[10013] = { 0 };
						for (int i = 0; i < len; ++i) {
							distrib.param(std::uniform_int_distribution<int>::param_type{ 0, 25 });
							arr[i] = distrib(engine_) + 'A';
						}

						talkContent_ = arr;
						JValue val;
						val["content"] = talkContent_;
						SendMsg(EMsgType::Talk, val);

						LogSave("client.log", "Client talk: [%d][%d] %.6s...%.6s", userId_, talkContent_.length(),
							talkContent_.substr(0, 6).c_str(), talkContent_.substr(talkContent_.length() - 6, 6).c_str());
					}
				}

				Sleep(100);
			}
		});
	}

private:
	shared_ptr<Client> clientPtr_ = nullptr;
	std::thread runThr_;
	uint32 userId_ = 0;
	bool isConnecting_ = false;
	bool isLogin_ = false;
	SingleQueue<JMsgItem> inMsgs_;
	bool isStop_ = false;
	time_t talkTime_ = time(nullptr) + 5;
	string talkContent_;
	atomic<bool> alive_ = true;
	std::mt19937 engine_{ std::random_device{}() };
	time_t liveTime_ = 0;
};

}
