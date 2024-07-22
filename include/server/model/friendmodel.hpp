#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include"user.hpp"
#include<vector>
class friendModel{
public:
    void insert(int usrid,int friendid);//添加好友关系
    std::vector<User> query(int usrid);//返回好友列表
};

#endif