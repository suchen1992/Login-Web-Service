#ifndef LOGIN_CLIENT_H
#define LOGIN_CLIENT_H

#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>

#include <grpcpp/grpcpp.h>

#include "login.grpc.pb.h"

using grpc::Channel;
using grpc::ClientReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using login::LoginService;
using login::LoginReply;
using login::SignUpReply;
using login::CommonRequest;
using login::CheckRequest;
using login::CheckReply;
using namespace std;

class LoginClient {
  public:
    LoginClient(const string& cert,
	            const string& key,
                const string& root,
                const string& server );
    
    string SignUp(const string& user, const string& pwd);

    string Login(const string& user, const string& pwd);

    string CheckStatus(const string& user, const string& token);

  private:
    unique_ptr<LoginService::Stub> stub_;
};

#endif