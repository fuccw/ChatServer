#include"groupmodel.hpp"
#include"db.hpp"

bool GroupModel::createGroup(Group& group){
    char sql[1024] = {0};
    sprintf(sql,"insert into AllGroup(groupname,groupdesc)values('%s','%s')",group.getName().c_str(),group.getDesc().c_str());
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

void GroupModel::addGroup(int userid,int groupid,std::string role){
    char sql[1024] = {0};
    sprintf(sql,"insert into GroupUservalues(%d,%d,'%s')",groupid,userid,role.c_str());
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

std::vector<Group> GroupModel::queryGroups(int userid){
    //1、查询用户所在的所有群组
    //2、分别查询各个群组包含的组员信息
    char sql[1024] = {0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on a.id = b.groupid where b.userid=%d",userid);
    std::vector<Group> vec;

    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    for(Group &group:vec){
        sprintf(sql,"select a.id,a.name,a.state,b.grouprole from User a \
        inner join GroupUser b on b.userid = a.id where b.groupid=%d",group.getId());
        MYSQL_RES* res = mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
  
std::vector<int> GroupModel::queryGroupUsers(int userid,int groupid){
    char sql[1024] = {0};
    sprintf(sql,"select userid from GroupUser where groupid =%d and userid!=%d",groupid,userid);
    std::vector<int> vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return vec;
}