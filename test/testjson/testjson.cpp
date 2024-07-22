#include"json.hpp"
using json = nlohmann::json;
#include<iostream>
#include<vector>
#include<map>
#include<string>
using namespace std;


string func1(){
    json js;
    js["msg_type"] = 2;
    js["from"]="zhang san";
    js["to"] = "li si";
    js["msg"] = "hello";
    string sendBuf = js.dump();
    return sendBuf;
}

void func2(){
    json js;
    js["id"] = {1,2,3,4,5};
    js["msg"]["zhang san"] = "hello";
    js["msg"]["li si"] = "world";
}

int main(){
    string receiveBuf = func1();
    json jsbuf = json::parse(receiveBuf);
    cout<<jsbuf["from"]<<endl;
    return 0;
}