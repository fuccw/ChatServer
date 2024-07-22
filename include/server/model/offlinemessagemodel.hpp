#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include<string>
#include<vector>
class offlineMessageModel{
public:
    //存储消息
    void insert(int usrid,std::string msg);
    //删除消息
    void remove(int usrid);
    //查询消息
    std::vector<std::string> query(int usrid);
private:

};

#endif