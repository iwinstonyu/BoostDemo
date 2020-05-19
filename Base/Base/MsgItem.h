//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2017. WenJin Yu. windpenguin@gmail.com.
//
//	Created at 2017/9/6 17:04:36
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
#include "MsgType.h"
#include "JsonTool.h"

namespace wind {

typedef unsigned int uint32;
typedef unsigned short uint16;

struct MsgItem
{
	static const uint32 MAX_MSG_SIZE = 32 * 1024;

	struct MsgHead
	{
		uint16 msgSize_ = 0;
		uint16 msgType_ = 0;
	};
	static const uint32 MSG_HEAD_SIZE = sizeof(MsgHead);

	MsgItem()
	{
	}

	const char* GetBuff() { return buff_; }
	uint32 GetBuffSize() { return head_.msgSize_; }
	const char* GetBody() { return buff_ + MSG_HEAD_SIZE; }
	uint32 GetBodySize() { return head_.msgSize_ - MSG_HEAD_SIZE; }

	bool Init(uint32 scId, const char* msg, uint16 len)
	{
		if (len > MAX_MSG_SIZE)
			return false;

		scId_ = scId;
		memcpy(buff_, msg, len);

		if (head_.msgType_ == static_cast<uint16>(EMsgType::None))
			return false;

		if (head_.msgSize_ != len)
			return false;

		return true;
	}

	bool Init(uint32 scId, EMsgType msgType, JValue& val)
	{
		scId = scId_;

		JWriter writer;
		string buff = writer.write(val);

		head_.msgType_ = static_cast<uint16>(msgType);
		head_.msgSize_ = static_cast<uint16>(buff.length()) + MSG_HEAD_SIZE;

		memcpy(buff_ + MSG_HEAD_SIZE, buff.c_str(), buff.length());

		return true;
	}

	uint32 scId_ = 0;
	union
	{
		char buff_[MAX_MSG_SIZE] = { 0 };
		MsgHead head_;
	};
};
typedef std::shared_ptr<MsgItem> MsgItemPtr;

} // namespace wind