#include "login_client.h"

LoginClient::LoginClient(const string& cert,
	              const string& key,
                const string& root,
                const string& server ) {
  grpc::SslCredentialsOptions opts ={ root, key, cert };

  stub_ = LoginService::NewStub (grpc::CreateChannel (
    server, grpc::SslCredentials(opts)));
}

string LoginClient::SignUp(const string& user, const string& pwd) {
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

string LoginClient::Login(const string& user, const string& pwd) {
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

string LoginClient::CheckStatus(const string& user, const string& token) {
  CheckRequest request;
  ClientContext context;
  request.set_user_name(user);
  request.set_token(token);

  shared_ptr<ClientReader<CheckReply> > reader(stub_->CheckStatus(&context, request));
  CheckReply reply;
  bool loopFlag = true;
  while (reader->Read(&reply) && loopFlag) {
    string tokenResp = reply.token();
    loopFlag = token.compare(tokenResp) == 0;
  }
  return "exit";
}