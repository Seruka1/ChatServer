#include "friendmodel.hpp"
#include "db.h"

void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024];
    sprintf(sql, "insert into friend values('%d','%d')", userid, friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return;
        }
    }
}

// 拉取好友列表
vector<User> FriendModel::query(int userid)
{
    vector<User> vec;
    char sql[1024];
    sprintf(sql, "select t1.friendid,t2.name,t2.state from friend t1 Join user t2 On t1.friendid=t2.id where userid=%d", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid用户的所有离线消息放入vec中
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}