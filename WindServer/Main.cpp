// WindServer.cpp : 定义控制台应用程序的入口点。
//

#include "Server.h"
#include <thread>
#include "UserMgr.h"
#include "Proxy.h"

using boost::asio::ip::tcp;
using namespace wind;

int main(int argc, char* argv[])
{
	if (argc < 2)
		return 1;
	int port = atoi(argv[1]);

	try {
		Proxy proxy(port);
		proxy.Start();

		UserMgr userMgr(&proxy);
		userMgr.Start();

		char c;
		cin >> c;

// 		thread t([&io_service, &server]() {
// 			while (true) {
// 				io_service.post([&server]() { 
// 					char szContent[1024] = "";
// 					sprintf_s(szContent, 1023, "hello world %I64d", ::time(NULL));
// 					Msg msg;
// 					msg.SetBodyLength(strlen(szContent));
// 					memcpy(msg.Body(), szContent, msg.BodyLength());
// 					msg.EncodeHeader();
// 
// 					server.DeliverMsg(msg); 
// 
// 					LogSave("Deliver msg: %s", szContent);
// 				} );
// 
// 				Sleep(rand() % 30 * 1000);
// 			}
// 		});

		//thread t([&server]() { Sleep(5000); LogSave("Close server..."); server.Close(); });

		//io_service.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what();
	}

	system("pause");
    return 0;
}

