#ifndef USERMODEL_H
#define USERMODEL_H
#include"user.hpp"
//表User的数据操作类
class UserModel{
public:
    //增加方法
    bool insert(User& user);
    User query(int id);
    bool updateState(User& user);
    void resetState();//重置用户状态信息
private:
};

#endif