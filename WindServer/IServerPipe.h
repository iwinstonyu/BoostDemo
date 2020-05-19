//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2020. Wenjin Yu. NetDragon. All rights reserved.
//
//	Created at 2020/5/12 17:09:01
//	Version 1.0
//
//	This program, including documentation, is protected by copyright controlled
//	by NetDragon. All rights are reserved.
//

#pragma once

namespace wind {

typedef unsigned int uint32;

struct IPInfo
{
	char proto_[256];
	char ip_[256];
	unsigned short port_ = 0;
};

class IServerPipe
{
public:
	virtual ~IServerPipe()
	{
	}

	virtual void OnCreate(uint32, IPInfo) = 0;
	virtual void OnMsg(uint32, const char*, unsigned short) = 0;
	virtual void OnError(uint32, int, const char*) = 0;
	virtual void OnDestroy(uint32) = 0;
	virtual void OnFlashDisconnect(uint32) = 0;
	virtual void OnFlashReconnect(uint32, IPInfo) = 0;
	virtual void OnMsgTimeout(uint32) = 0;
};

}
