#ifndef GROUPUSER_H
#define GROUPUSER_H
#include"user.hpp"
#include<string>
//群组用户，比User多了一个Role信息
class GroupUser:public User{
public:
    void setRole(std::string role){this->role = role;}
    std::string getRole(){return this->role;}

private:
    std::string role;

};

#endif