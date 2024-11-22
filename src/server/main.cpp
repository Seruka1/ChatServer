#include "chatserver.hpp"
#include <iostream>
using namespace std;



int main()
{
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();  //listenfd注册到epoll
    loop.loop();  //epoll_wait

    return 0;
}