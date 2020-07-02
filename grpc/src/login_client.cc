#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "login.grpc.pb.h"
#else
#include "login.grpc.pb.h"
#endif

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
    // LoginClient(const string& cert,
	  //             const string& key,
    //             const string& root,
    //             const string& server ) {
		// grpc::SslCredentialsOptions opts ={
		// 	root,
		// 	key,
		// 	cert
		// };

		// stub_ = LoginService::NewStub (grpc::CreateChannel (
    //   server, grpc::SslCredentials(opts)));
    // }
    LoginClient(std::shared_ptr<Channel> channel)
      : stub_(LoginService::NewStub(channel)) {}
    
    string SignUp(const string& user, const string& pwd) {
      CommonRequest request;
      SignUpReply reply;
      request.set_user_name(user);
      request.set_password(pwd);

      ClientContext context;
      Status status = stub_->SignUp(&context, request, &reply);
      if (status.ok()) {
        return reply.message();
      } else {
        cout << status.error_code() << ": " << status.error_message()
                  << endl;
        return "RPC failed";
      }
    }

    string Login(const string& user, const string& pwd) {
      CommonRequest request;
      LoginReply reply;
      request.set_user_name(user);
      request.set_password(pwd);

      ClientContext context;
      Status status = stub_->Login(&context, request, &reply);
      if (status.ok()) {
        cout << "loginResp:"<<reply.resp_code()<<endl;
        cout << "token:" <<reply.token() <<endl;
        cout << "message:"<<reply.message()<<endl;
        if (reply.resp_code() == 200) {
          return CheckStatus(user, reply.token());
        } else {
          return reply.message();
        }
      } else {
        cout << status.error_code() << ": " << status.error_message()
                  << endl;
        return "RPC failed";
      }
    }

    string CheckStatus(const string& user, const string& token) {
      CheckRequest request;
      ClientContext context;
      request.set_user_name(user);

      shared_ptr<ClientReader<CheckReply> > reader(stub_->CheckStatus(&context, request));
      CheckReply reply;
      bool loopFlag = true;
      while (reader->Read(&reply) && loopFlag) {
        string tokenResp = reply.token();
        loopFlag = token.compare(tokenResp) == 0;
      }
      return "exit";
    }

  private:
    unique_ptr<LoginService::Stub> stub_;
};

void read ( const string& filename, string& data ) {
	ifstream file ( filename.c_str (), ios::in );
	if ( file.is_open () )
	{
		stringstream ss;
		ss << file.rdbuf ();

		file.close ();

		data = ss.str ();
	}

	return;
}

int main(int argc, char** argv) {
  string cert;
	string key;
	string root;
	string server_address {"localhost:50051"};

	// read ( "client.crt", cert );
	// read ( "client.key", key );
	// read ( "ca.crt", root );
  // LoginClient client(cert, key, root, server);
  LoginClient client(grpc::CreateChannel(
      server_address, grpc::InsecureChannelCredentials()));
  bool flag = true;
  while (flag) {
    cout << "Please choose service: 1. Sign up; 2. Log in; 0. quit" << endl;
    int num = 0;
    cin >> num;
    if (num == 1) {
      cout << "Enter your username: " << endl;
      string user;
      cin >> user;
      cout << "Enter your password: " << endl;
      string pwd;
      cin >> pwd;
      string reply = client.SignUp(user, pwd);
      cout << "loginer hello received: " << reply << endl;
    } else if (num == 2) {
      cout << "Enter your username: " << endl;
      string user;
      cin >> user;
      cout << "Enter your password: " << endl;
      string pwd;
      cin >> pwd;
      string reply = client.Login(user, pwd);
      cout << "loginer hello received: " << reply << endl;
    } else if (num == 0) {
      flag = false;
    } else {
      cout << "Error number" << endl;
    }
  }
  return 0;
}