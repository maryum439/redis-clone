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
std::string handleCommand(std::vector<std::string>& args, std::unordered_map<std::string,std::string>& store){
    if (args.empty()){
        return "empty args";
    }
    if (args[0] == "SET"){
        if (args.size()!=3){
            return "-ERR wrong number of arguments for 'SET'\r\n";
        }
        store[args[1]] = args[2];
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
        std::string x = it->second;
        return "$"+std::to_string(x.length())+"\r\n"+x+"\r\n";
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
std::unordered_map<std::string, std::string> store;
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

