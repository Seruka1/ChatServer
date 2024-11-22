#include "chatservice.hpp"
#include "public.hpp"



#include <muduo/base/Logging.h>
#include <string>
using namespace muduo;
using namespace std;

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

//注册消息以及对应的处理函数
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
}

//实现登陆业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId()==id&&user.getPwd()==pwd)
    {
        if(user.getState()=="online")
        {
            //该用户已经登陆，不允许重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登陆，不允许重复登陆";
            conn->send(response.dump());
        }
        else
        {
            //登陆成功，更新用户状态信息
            user.setState("online");
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            conn->send(response.dump());
        }
    }
    else
    {
        //用户不存在/密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户不存在/密码错误";
        conn->send(response.dump());
    }
}

//实现注册业务  name，password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    json response;
    response["msgid"] = REG_MSG_ACK;
    if (_userModel.insert(user))
    {
        response["errno"] = 0;
        response["id"] = user.getId();
    }
    else
    {
        response["errno"] = 1;
    }
    conn->send(response.dump());
}

MsgHandler ChatService::getHandler(int msgId)
{
    //记录错误日志，msgId没有对应的事件处理回调
    if(!_msgHandlerMap.count(msgId))
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgId << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgId];
    }
}