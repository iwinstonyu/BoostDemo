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
#include <string>

namespace wind {

typedef unsigned int uint32;
typedef unsigned short uint16;

class Msg 
{
public:
	static const uint16 HEADER_LENGTH = 4;
	static const uint16 MAX_BODY_LENGTH = 32768;

	Msg() {}

	const char* Data() const { return data_; }
	char* Data() { return data_; }
	uint16 Length() { return HEADER_LENGTH + bodyLength_; }
	const char* Body() const { return data_ + HEADER_LENGTH; }
	char* Body() { return data_ + HEADER_LENGTH; }
	uint16 BodyLength() { return bodyLength_; }
	void SetBodyLength(uint16 length) { bodyLength_ = length; }
	void Clear() { bodyLength_ = 0;  memset(data_, 0, sizeof(data_)); }

	bool Init(const std::string& msg)
	{
		bodyLength_ = static_cast<uint16>(msg.length());
		memcpy(data_ + HEADER_LENGTH, msg.c_str(), msg.length());
		EncodeHeader();

		return true;
	}

	bool DecodeHeader() 
	{
		char header[HEADER_LENGTH + 1] = "";
		strncat_s(header, data_, HEADER_LENGTH);
		bodyLength_ = atoi(header);
		if (bodyLength_ > MAX_BODY_LENGTH) {
			bodyLength_ = 0;
			return false;
		}
		return true;
	}

	void EncodeHeader() 
	{
		char header[HEADER_LENGTH + 1] = "";
		sprintf_s(header, HEADER_LENGTH + 1, "%4d", static_cast<int>(bodyLength_));
		memcpy(data_, header, HEADER_LENGTH);
	}

private:
	uint16 bodyLength_ = 0;
	char data_[HEADER_LENGTH + MAX_BODY_LENGTH] = "";
};

} // namespace wind