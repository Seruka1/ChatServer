#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>

#include <muduo/net/TcpConnection.h>
#include "json.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService *instance();
    //处理登陆业务
    void login(const TcpConnection &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnection &conn, json &js, Timestamp time);

private:
    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;

    ChatService();
};

#endif