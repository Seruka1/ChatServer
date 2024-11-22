#ifndef PUBLIC_H
#define PUBLIC_H

/*
    server 和 client的公共文件
*/
enum EnMsgType
{
    LOGIN_MSG = 1,   //登陆消息
    LOGIN_MSG_ACK,
    REG_MSG,         //注册消息
    REG_MSG_ACK
};

#endif