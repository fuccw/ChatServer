#include"offlinemessagemodel.hpp"
#include"db.hpp"

void offlineMessageModel::insert(int usrid,std::string msg){
    char sql[1024] = {0};
    sprintf(sql,"insert into OfflineMessage values(%d,'%s')",usrid,msg.c_str());
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return;
        }
    }
}

void offlineMessageModel::remove(int usrid){
    char sql[1024] = {0};
    sprintf(sql,"delete from OfflineMessage where usrid = %d",usrid);
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return;
        }
    }
}

std::vector<std::string> offlineMessageModel::query(int usrid){
    char sql[1024] = {0};
    sprintf(sql,"select message from OfflineMessage where usrid =%d",usrid);
    MySQL mysql;
    std::vector<std::string> vec;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row ;   
            while((row= mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}