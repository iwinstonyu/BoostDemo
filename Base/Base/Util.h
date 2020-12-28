//
//	<one line to give the program's name and a brief idea of what it does.>
//	Copyright (C) 2017. WenJin Yu. windpenguin@gmail.com.
//
//	Created at 2017/9/6 17:05:55
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

#include <iostream>
#include <string>
#include <thread>	// this_thread
#include <sstream> // ostringstream
#include <fstream>
//#include <Windows.h>
using namespace std;

namespace wind {

class MyCS {
public:
	MyCS() { InitializeCriticalSection(&cs_); }
	void Lock() { EnterCriticalSection(&cs_); }
	void Unlock() { LeaveCriticalSection(&cs_); }
private:
	CRITICAL_SECTION cs_;
};

MyCS gLogCS;

void LogSave(const char* filename, const char* pszFormat, ...) {
	ostringstream oss;
	oss << "[" << this_thread::get_id() << "][" << time(nullptr) << "]: ";
	char buffer[1024] = {0};
	va_list args;
	va_start(args, pszFormat);
	vsprintf_s(buffer, 1024, pszFormat, args);
	va_end(args);
	oss << buffer;

	gLogCS.Lock();
	cout << oss.str() << endl;
	fflush(stdout);
	ofstream ofs(filename, ios::app);
	ofs << oss.str() << endl;
	ofs.close();
	gLogCS.Unlock();
}

#define WD_IF(x)  if( (x) ? (assert(0), LogSave("base.log", "[%s][%d][%s]", __FILE__, __LINE__, #x ), 1) : 0 )

} // namespace wind