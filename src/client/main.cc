#include"json.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
#include<ctime>
#include<semaphore.h>
#include<atomic>
using json = nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include"group.hpp"
#include"user.hpp"
#include"public.hpp"

//当前用户
User _currentUser;
//当前用户好友
std::vector<User> _currentUserFriendList;
//当前用户群组
std::vector<Group> _currentUserGroupList;

bool _isMainMenuRunning = false;//标记菜单运行状态
sem_t rwsem;//线程通信信号量
std::atomic_bool _isLoginSuccess{false};

void showCurrentUserData();//显示当前用户信息
void readTaskHandler(int clientfd);//接收子线程
std::string getCurrentTime();//获取时间
void mainMenu(int);//聊天页面

void doLoginResponse(json&); // 登录响应
void doRegResponse(json&);//注册响应

//主线程发送消息，子线程接收消息
int main(int argc,char** argv){
    if(argc<3){
        std::cerr<<"command invalid!example: ./ChatClient 127.0.0.1 6000" << std::endl;
        exit(-1);
    }
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);
    //创建socket
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1==clientfd){
        std::cerr<<"socket create error!"<<std::endl;
        exit(-1);
    }
    //server信息
    sockaddr_in server;
    memset(&server,0,sizeof server);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //client与server连接
    if(-1==connect(clientfd,(sockaddr*)&server,sizeof(sockaddr_in))){
        std::cerr<<"connect server error!"<<std::endl;
        close(clientfd);
        exit(-1);
    }
    //初始化线程通信信号量
    sem_init(&rwsem,0,0);
    //启动子线程
    std::thread readTask(readTaskHandler,clientfd);
    readTask.detach();

    for(;;){
        // 显示首页面菜单 登录、注册、退出
        std::cout << "========================" << std::endl;
        std::cout << "1. login" << std::endl;
        std::cout << "2. register" << std::endl;
        std::cout << "3. quit" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << "choice:";
        int choice = 0;
        std::cin >> choice;
        std::cin.get(); // 读掉缓冲区残留的回车
        switch (choice)
        {
        case 1://login
            {
                int id = 0;
                char pwd[50] = {0};
                std::cout << "userid:";
                std::cin >> id;
                std::cin.get(); // 读掉缓冲区残留的回车
                std::cout << "userpassword:";
                std::cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = (int)id;
                js["password"] = pwd;
                std::string request = js.dump();

                _isLoginSuccess = false;

                int len = send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                if(len==-1){
                    std::cerr<<"send login msg error:"<<request<<std::endl;
                }
                sem_wait(&rwsem);
                if(_isLoginSuccess){
                    _isMainMenuRunning = true;
                    mainMenu(clientfd);
                }
            }
            break;
        case 2://register
            {
                char name[50] = {0};
                char pwd[50] = {0};
                std::cout<<"username:";
                std::cin.getline(name,50);
                std::cout<<"password:";
                std::cin.getline(pwd,50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                std::string request = js.dump();
                
                int len = send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                if(len==-1){
                    std::cerr<<"send reg msg error:"<<request<<std::endl;
                }
                sem_wait(&rwsem);
            }
            break;
        case 3://quit
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            std::cerr << "invalid input!" << std::endl;
            break;
        }
    }
    return 0;
}

//子线程
void readTaskHandler(int clientfd){
    for(;;){
        char buffer[1024] = {0};
        int len =  recv(clientfd,buffer,1024,0);//阻塞
        if(-1==len||0==len){
            close(clientfd);
            exit(-1);
        }
        //反序列化接收到的数据
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        //-----------------------------------
        if(msgtype == ONE_CHAT_MESSAGE){
            std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                 << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }
        if (GROUP_CHAT_MSG == msgtype)
        {
            std::cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<std::string>() 
            << " [" << js["id"] << "]" << js["name"].get<std::string>()
                 << " said: " << js["msg"].get<std::string>() << std::endl;
            continue;
        }
        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成
            continue;
        }
        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);    // 通知主线程，注册结果处理完成
            continue;
        }
    }
}   

void doLoginResponse(json &responsejs){
    if(0!=responsejs["errno"].get<int>()){
        std::cerr<<responsejs["errmsg"]<<std::endl;
        _isLoginSuccess = false;
    }
    else{
        _currentUser.setId(responsejs["id"]);
        _currentUser.setName(responsejs["name"]);
        if(responsejs.contains("friends")){
            _currentUserFriendList.clear();
            std::vector<std::string> vec = responsejs["friends"];
            for(std::string&str:vec){
                json js = json::parse(str);
                User user;
                user.setId(js["id"]);
                user.setName(js["name"]);
                user.setState(js["state"]);
                _currentUserFriendList.push_back(user);
            }
        }
        if (responsejs.contains("groups"))
        {
            // 初始化
            _currentUserGroupList.clear();

            std::vector<std::string> vec1 = responsejs["groups"];
            for (std::string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                std::vector<std::string> vec2 = grpjs["users"];
                for (std::string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                _currentUserGroupList.push_back(group);
            }
        }
        showCurrentUserData();
        // 显示当前用户的离线消息  个人聊天信息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            std::vector<std::string> vec = responsejs["offlinemsg"];
            for (std::string &str : vec)
            {
                json js = json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MESSAGE == js["msgid"].get<int>())
                {
                    std::cout << js["time"].get<std::string>() << " [" << js["id"] << "]" << js["name"].get<std::string>()
                    << " said: " << js["msg"].get<std::string>() << std::endl;
                }
                else
                {
                    std::cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<std::string>() 
                    << " [" << js["id"] << "]" << js["name"].get<std::string>()
                    << " said: " << js["msg"].get<std::string>() << std::endl;
                }
            }
        }

        _isLoginSuccess = true;
    }
}
void doRegResponse(json &responsejs){
    if (0 != responsejs["errno"].get<int>()) // 注册失败
    {
        std::cerr << "name is already exist, register error!" << std::endl;
    }
    else // 注册成功
    {
        std::cout << "name register success, userid is " << responsejs["id"]
                << ", do not forget it!" << std::endl;
    }
}

void showCurrentUserData(){
    std::cout << "======================login user======================" << std::endl;
    std::cout << "current login user => id:" << _currentUser.getId() << " name:" << _currentUser.getName() << std::endl;
    std::cout << "----------------------friend list---------------------" << std::endl;
    if (!_currentUserFriendList.empty())
    {
        for (User &user : _currentUserFriendList)
        {
            std::cout << user.getId() << " " << user.getName() << " " << user.getState() << std::endl;
        }
    }
    std::cout << "----------------------group list----------------------" << std::endl;
    if (!_currentUserGroupList.empty())
    {
        for (Group &group : _currentUserGroupList)
        {
            std::cout << group.getId() << " " << group.getName() << " " << group.getDesc() << std::endl;
            for (GroupUser &user : group.getUsers())
            {
                std::cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << std::endl;
            }
        }
    }
    std::cout << "======================================================" << std::endl;
}


std::string getCurrentTime()//获取系统时间
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// "help" command handler
void help(int fd = 0, std::string str = "");
// "chat" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "loginout" command handler
void loginout(int, std::string);

// 系统支持的客户端命令列表
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
    };

    // 主聊天页面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (_isMainMenuRunning)
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command; // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            std::cerr << "invalid input command!" << std::endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void help(int, std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap)
    {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}
// "addfriend" command handler
void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = _currentUser.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error -> " << buffer << std::endl;
    }
}
// "chat" command handler
void chat(int clientfd, std::string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MESSAGE;
    js["id"] = _currentUser.getId();
    js["name"] = _currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}
// "creategroup" command handler  groupname:groupdesc
void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = _currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = _currentUser.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}
// "groupchat" command handler   groupid:message
void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = _currentUser.getId();
    js["name"] = _currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
}
// "loginout" command handler
void loginout(int clientfd, std::string)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = _currentUser.getId();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send loginout msg error -> " << buffer << std::endl;
    }
    else
    {
        _isMainMenuRunning = false;
    }   
}

