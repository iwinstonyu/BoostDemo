#pragma once

namespace wind {

class IClientPipe
{
public:
	virtual ~IClientPipe()
	{
	}

	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnMsg(const char*, unsigned short) = 0;
	virtual void OnError(int, std::string msg) = 0;
	virtual void OnReconnect() = 0;
	virtual void OnReconnected() = 0;
};

}