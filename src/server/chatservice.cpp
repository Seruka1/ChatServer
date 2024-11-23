#include "chatservice.hpp"
#include "public.hpp"



#include <muduo/base/Logging.h>
#include <string>
#include <vector>
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
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});
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
            // 登陆成功，记录用户连接信息
            {
                lock_guard<mutex> lk(_connMtx);
                _userConnMap.insert({id, conn});
            }
            // 登陆成功，更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            //查询用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }
            //查询用户的好友信息并返回
            vector<User> vecUser = _friendModel.query(id);
            if(!vecUser.empty())
            {
                vector<string> vec2;
                for(auto &user:vecUser)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

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

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toId = js["to"].get<int>();
    //检查对端是否在线
    {
        lock_guard<mutex> lock(_connMtx);
        if(_userConnMap.count(toId))
        {
            //toid在线，转发消息
            _userConnMap[toId]->send(js.dump());
            return;
        }
    }
    //toId不在线，存储离线消息
    _offlineMsgModel.insert(toId, js.dump());
}

//添加好友业务   msgid  id  friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //添加好友信息
    _friendModel.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard lk(_connMtx);
    for(int id:useridVec)
    {
        if(_userConnMap.count(id))
        {
            //转发消息
            _userConnMap[id]->send(js.dump());
        }
        else
        {
            //存储离线消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
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

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        // 把用户设为离线
        lock_guard<mutex> lock(_connMtx);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的连接信息
                _userConnMap.erase(it);
                user.setId(it->first);
                break;
            }
        }
    }
    //更新用户的状态信息
    if(user.getId()!=-1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}