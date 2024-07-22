#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include"group.hpp"
#include<string>
#include<vector>
//群组信息的接口
class GroupModel{
public:
    //创建群组
    bool createGroup(Group& group);
    //加入群组
    void addGroup(int userid,int groupid,std::string role);
    //查询用户所在群组信息
    std::vector<Group> queryGroups(int userid);
    //查询组中其他成员id，用于批量发送消息
    std::vector<int> queryGroupUsers(int userid,int groupid);
private:

};
#endif