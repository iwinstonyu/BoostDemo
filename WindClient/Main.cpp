// WindClient.cpp : 定义控制台应用程序的入口点。
//

#include "Client.h"

#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <fstream>	// ifstream
#include <sstream>
#include "RobotClient.h"

using namespace std;
using namespace wind;

map<int, string> gContents;
size_t gContentSize = 0;
CRITICAL_SECTION gLogCS;

//void LogSaveCS(const char* pszFormat, ...) {
//	EnterCriticalSection(&gLogCS);
//	va_list args;
//	va_start(args, pszFormat);
//	LogSave("client.log", pszFormat, args);
//	va_end(args);
//	LeaveCriticalSection(&gLogCS);
//}

string RandContent() {
	return gContents[rand() % gContentSize];
}

void ForkClient(int clientId, string ip, int port, std::atomic<bool>& running) {
	try {
		RobotClient robot(clientId, ip, port);
		robot.Start();

		while (robot.Alive()) {
			Sleep(100);
		}

		LogSave("client.log", "Robot[%d] dead", clientId);

// 		boost::asio::io_service io_service;
// 
// 		tcp::resolver resovler(io_service);
// 		auto endpoint_iterator = resovler.resolve({ "localhost", "13579" });
// 		Client client(io_service, endpoint_iterator, clientId);
// 
// 		io_service.run();

// 		std::thread t([&io_service]() { io_service.run(); });
// 		t.join();

// 		char szContent[Msg::MAX_BODY_LENGTH + 1] = "";
// 		while (std::cin.getline(szContent, Msg::MAX_BODY_LENGTH + 1)) {
// 			Msg msg;
// 			msg.SetBodyLength(strlen(szContent));
// 			memcpy(msg.Body(), szContent, msg.BodyLength());
// 			msg.EncodeHeader();
// 			client.SendMsg(msg);
// 		}

 		//while (true) {
  	//		if (robot.Online()) {
			//	int len = rand() % 10240;
			//	char arr[10240] = {0};
			//	for (int i = 0; i < len; ++i) {
			//		arr[i] = rand() % 26 + 'A';
			//	}

   //				Msg msg;
   //				msg.SetBodyLength(content.length());
   //				memcpy(msg.Body(), content.c_str(), msg.BodyLength());
   //				msg.EncodeHeader();
   //				client.SendMsg(msg);
  	//		}
  
  	//		Sleep(rand() % 30 * 1000);
 
 		//	//if (client.Online()) {
 		//	//	Sleep(5000);
 		//	//	client.Logout();
 		//	//	t.join();
 		//	//	break;
 		//	//}
 		//}
	}
	catch (std::exception& e) {
		cout << "Exception: " << e.what() << endl;
	}

	running = false;
}

int main(int argc, char* argv[])
{
	srand(static_cast<int>(time(nullptr)));

// 	ifstream ifs("Resource/sgyy.txt");
// 	if (ifs) {
// 		int lineno = 0;
// 		string content;
// 		while (!ifs.eof()) {
// 			getline(ifs, content);
// 			content = content.substr(0, 64);
// 			if( !content.empty())
// 				gContents[lineno++] = content;
// 		}
// 		gContentSize = gContents.size();
// 		ifs.close();
// 	}

	if (argc < 3)
		return 1;
	string ip(argv[1]);
	int port = atoi(argv[2]);
	//int userId = atoi(argv[3]);

	Sleep(3000);

	struct RobotDriver {
		thread thr_;
		std::atomic<bool> running_ = false;
	};

	vector<RobotDriver*> drivers;
	drivers.reserve(500);
	while (true) {
		for (auto it = drivers.begin(); it != drivers.end();) {
			if (!(*it)->running_) {
				if ((*it)->thr_.joinable())
					(*it)->thr_.join();
				delete *it;
				it = drivers.erase(it);
			}
			else {
				++it;
			}
		}

		int amount = rand() % 90 + 10;
		while (drivers.size() < amount) {
			RobotDriver* driverPtr = new RobotDriver;
			driverPtr->running_ = true;
			driverPtr->thr_ = std::thread(ForkClient, rand() % 100 + 1, ip, port, std::ref(driverPtr->running_));
			drivers.push_back(driverPtr);
		}

		Sleep(100);
	}


// 	map<int, ostringstream> ossMap;
// 	auto& oss1 = ossMap[1];
// 	auto& oss2 = ossMap[2];
// 	auto& oss3 = ossMap[3];
// 	std::thread t1([&oss1]() {RandContent(1, oss1); });
// 	std::thread t2([&oss2]() {RandContent(2, oss2); });
// 	std::thread t3([&oss3]() {RandContent(3, oss3); });

// 	for (int i = 0; i < 3; ++i) {
// 		auto& oss = ossMap[i];
// 	}

// 	for (int i = 0; i < 100; ++i) {
// 		int nRand = (rand() + time(nullptr)) % gContents.size();
// 		cout << gContents[nRand] << endl;
// 	}

	//for_each(clients.begin(), clients.end(), [](thread& t) {
	//	t.join();
	//});

	system("pause");
    return 0;
}

