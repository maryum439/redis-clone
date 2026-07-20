#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>  
#include <vector>     
#include <string>     
#include <cstdlib> 
#include <unordered_map>
struct Value {
    enum Type {
        STRING,
        LIST
    };
    Type type;
    std::string stringValue;
    std::vector<std::string> listValue;
};
std::string handleCommand(std::vector<std::string>& args, std :: unordered_map<std::string, Value> &store){
    if (args.empty()){
        return "empty args";
    }
    if (args[0] == "SET"){
        if (args.size()!=3){
            return "-ERR wrong number of arguments for 'SET'\r\n";
        }
        Value value;
        value.type = Value::STRING;
        value.stringValue = args[2];
        store[args[1]] = value;
        return "+OK\r\n";
    }
    else if (args[0] == "GET"){
        if (args.size()!=2){
            return "-ERR wrong number of arguments for 'GET'\r\n";
        }
        auto it = store.find(args[1]);
         if (it == store.end()){
            return "$-1\r\n";
        }
        if (it->second.type != Value::STRING){
            return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
        }
        std::string x = it->second.stringValue;
        return "$"+std::to_string(x.length())+"\r\n"+x+"\r\n";
    }
    else if (args[0]=="PING"){
        if (args.size()!=1){
            return "-ERR wrong number of arguments for 'PING'\r\n";
        }
        return "+PONG\r\n";
    }
    else if (args[0]=="DEL"){
        if (args.size()<2){
            return "-ERR wrong number of arguments for 'DEL'\r\n";
        }
        int deleted = 0;
        for (int i =1; i < args.size();i++){
            deleted += store.erase(args[i]);
        }
        return ":"+std::to_string(deleted)+"\r\n";
    }
    else if (args[0]=="EXISTS"){
        if (args.size()<2){
            return "-ERR wrong number of arguments for 'EXISTS'\r\n";
        }
        int existing = 0;
        for (int i = 1; i< args.size();i++){
            existing += store.count(args[i]);
        }
        return ":"+std::to_string(existing)+"\r\n";
    }
    else if (args[0]=="DBSIZE"){
        if (args.size()!=1){
            return "-ERR wrong number of arguments for 'DBSIZE'\r\n";
        }
        return ":"+std::to_string(store.size()) + "\r\n";
    }
    else if (args[0]=="LPUSH"){
        if (args.size() < 3){
            return "-ERR wrong number of arguments for 'LPUSH'\r\n";
        }
        auto it = store.find(args[1]);
        if (it == store.end()){
            Value value;
            value.type = Value::LIST;
            for (int i = 2; i < args.size(); i++){
                value.listValue.insert(value.listValue.begin(),args[i]);
            }
            store[args[1]]=value;
            return ":" + std::to_string(value.listValue.size()) + "\r\n";
        }
        else if (it->second.type != Value::LIST){
            return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
        }
        else{
            for (int i = 2;  i < args.size(); i++){
                it->second.listValue.insert(it->second.listValue.begin(),args[i]);
            }
            return ":" + std::to_string(it->second.listValue.size()) + "\r\n";
        }
    }
    else if (args[0]=="LRANGE"){
        if (args.size() < 4){
            return "-ERR wrong number of arguments for 'LRANGE'\r\n";
        }
        auto it = store.find(args[1]);
        if (it == store.end()){
            return "*0\r\n";
        }
        if (it->second.type!=Value::LIST){
            return "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
        }
        int start = std:: stoi(args[2]);
        if (start<0){
            start = it->second.listValue.size()+start;
        }
        if (start <0){
            start = 0;
        }
        else if (start >= it->second.listValue.size()){
            return "*0\r\n";
        }
        int end = std:: stoi(args[3]);
        if (end <0){
            end = it->second.listValue.size()+end;
        }
        if (end >=it->second.listValue.size()){
            end = it->second.listValue.size()-1;
        }
        if (start > end){
            return "*0\r\n";
        }
        int count = 0;
        for (int i = start; i <= end; i++){
            count ++;
        }
        std:: string str = "*" + std::to_string(end - start+1) + "\r\n";
        for (int i = start; i <= end; i++) {
            std::string element = it->second.listValue[i];
            str += "$" + std::to_string(element.length()) + "\r\n";
            str += element + "\r\n";
        }
        return str;
    }
    else{
        return "error";
    }
}   
std::vector<std::string> parseRESP(char* buffer, ssize_t bytes){
    std::vector<std::string> args;
    if (buffer[0]!='*'){
        printf("not parseable\n");
        return args;
    }
    char* pointer = buffer+1;
    char* end;
    int argc = strtol(pointer,&end,10);
    if (argc <= 0){
        printf("no strings found");
        return args;
    }
    pointer = end+2;
    for (int i = 0; i < argc; i++){
        if (*pointer != '$'){
            printf("Expected bulk string\n");
            return args;
        }
        pointer++;
        int len = strtol(pointer,&end,10);
        pointer = end+2;
        std :: string arg(pointer,len);
        args.push_back(arg);
        pointer = pointer + len;
        pointer = pointer + 2;
    }
    return args;
}
int main (){
std::unordered_map<std::string, Value> store;
char buffer[1024];
int server_fd = socket(AF_INET,SOCK_STREAM,0);
if (server_fd < 0){
    perror("Socket failed");
    return 1;
}
struct sockaddr_in server ;
server.sin_family = AF_INET;
server.sin_port = htons(6379);
server.sin_addr.s_addr = htonl(INADDR_ANY);
if (bind(server_fd,(struct sockaddr*)&server,sizeof(server))<0){
    perror("Bind failed");
    return 1;
}
if (listen (server_fd,10)<0){
    perror("listen failed");
    return 1;
}
while (1){
int client_fd = accept (server_fd,NULL,NULL);
if (client_fd <0){
    perror("Accepting failed");
    continue;
}
while(1){
ssize_t bytes = read (client_fd,buffer,sizeof(buffer));
if (bytes == 0){
    printf("client disconnected\n");
    break;
}
if (bytes < 0){
    perror("reading failed");
    break;
}
printf("Received %zd bytes: %.*s\n", bytes, (int)bytes, buffer);
std::vector<std::string> args = parseRESP(buffer, bytes);
for (size_t i = 0; i < args.size(); i++) {
    printf("arg[%zu] = %s\n", i, args[i].c_str());
}
std::string reply = handleCommand(args, store);
if (write(client_fd, reply.c_str(), reply.size())<0){
    perror("writing failed");
    break;
}
}
close(client_fd);

}
close(server_fd);
}

