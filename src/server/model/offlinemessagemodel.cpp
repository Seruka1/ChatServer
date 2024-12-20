#include "offlinemessagemodel.hpp"
#include "db.h"
// 存储离线消息
void OfflineMessageModel::insert(int userid, string msg)
{
    // 1.组装sql语句
    char sql[1024];
    sprintf(sql, "insert into offlinemessage values('%d','%s')", userid,msg.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return;
        }
    }
}

// 移除离线消息
void OfflineMessageModel::remove(int userid)
{
    // 1.组装sql语句
    char sql[1024];
    sprintf(sql, "delete from offlinemessage where userid='%d'",userid);
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return;
        }
    }
}

// 查询用户的离线消息
vector<string> OfflineMessageModel::query(int userid)
{
    vector<string> messageVec;
    // 1.组装sql语句
    char sql[1024];
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            //把userid用户的所有离线消息放入vec中
            MYSQL_ROW row ;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                messageVec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return messageVec;
}