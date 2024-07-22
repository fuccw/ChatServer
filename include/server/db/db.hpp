#ifndef DB_H
#define DB_H
#include<string>
#include<mysql/mysql.h>
#include<muduo/base/Logging.h>

static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat";

class MySQL{
public:
    MySQL(){
        _conn = mysql_init(nullptr);
    }
    ~MySQL(){
        if(_conn!=nullptr){
            mysql_close(_conn);
        }
    }
    bool connect(){
        MYSQL *p = mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),dbname.c_str(),3306,nullptr,0);
        if (p == nullptr) {
        LOG_ERROR << "MySQL connection failed: " << mysql_error(_conn);
        return false;
    }
        mysql_query(_conn,"set names gbk");
        return p;
    }
    bool update(std::string sql){
        if(mysql_query(_conn,sql.c_str())){
            LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"更新失败！";
            return false;
        }
        return true;
    }
    MYSQL_RES* query(std::string sql){
        if(mysql_query(_conn,sql.c_str())){
            LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"查询失败！";
            return nullptr;
        }
        return mysql_use_result(_conn);
    }
    MYSQL* getConnection(){
        return _conn;
    }

private:
    MYSQL* _conn;
};

#endif