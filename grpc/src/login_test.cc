#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>
#include <grpcpp/test/mock_stream.h>
#include <grpcpp/channel.h>
#include <grpc++/grpc++.h>

#include "login_client.h"
#include "login_mock.grpc.pb.h"
#include <iostream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using grpc::ClientReaderInterface;
using grpc::testing::MockClientReader;
using login::LoginService;
using login::CommonRequest;
using login::SignUpReply;
using login::LoginReply;
using login::CheckRequest;
using login::CheckReply;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::Return;
using ::testing::InSequence;

class FakeClient {
  public:
    explicit FakeClient(LoginService::StubInterface* stub) : stub_(stub) {}

  void DoSignUp(string username, string password) {
    ClientContext context;
    CommonRequest request;
    SignUpReply reply;
    request.set_user_name(username);
    request.set_password(password);
    Status s = stub_->SignUp(&context, request, &reply);
    if (username.compare("test") == 0) {
      // 模拟用户名已存在
      EXPECT_EQ(1001, reply.resp_code());
      EXPECT_EQ("already exist", reply.message());
    } else if (username.compare("test1") == 0) {
      // 模拟插库失败
      EXPECT_EQ(1002, reply.resp_code());
      EXPECT_EQ("sign up failed", reply.message());
    } else {
      // 模拟成功
      EXPECT_EQ(200, reply.resp_code());
      EXPECT_EQ("succ", reply.message());
    }
    EXPECT_TRUE(s.ok());
  }

  string DoLogin(string username, string password) {
    ClientContext context;
    CommonRequest request;
    LoginReply reply;
    request.set_user_name(username);
    request.set_password(password);
    Status s = stub_->Login(&context, request, &reply);
    string token;
    if (username.compare("test") == 0) {
      // 模拟用户名已存在
      EXPECT_EQ(1003, reply.resp_code());
      EXPECT_EQ("user not exists", reply.message());
    } else if (username.compare("test1") == 0 && password.compare("test123") != 0) {
      // 模拟密码错误
      EXPECT_EQ(1004, reply.resp_code());
      EXPECT_EQ("pwd is wrong", reply.message());
    } else if (username.compare("test1") == 0 && password.compare("test123") == 0) {
      // 模拟成功
      EXPECT_EQ(200, reply.resp_code());
      EXPECT_EQ("succ", reply.message());
      token = reply.token();
    }
    EXPECT_TRUE(s.ok());
    return token;
  }

  void DoCheckStatus(string username, string token) {
    CheckRequest request;
    CheckReply reply;
    ClientContext context;  
    request.set_user_name(username);
    unique_ptr<ClientReaderInterface<CheckReply>> cstream =
        stub_->CheckStatus(&context, request);

    EXPECT_TRUE(cstream->Read(&reply));
    EXPECT_EQ(token, reply.token());

    EXPECT_TRUE(cstream->Read(&reply));
    EXPECT_NE(token, reply.token());
  }

  void ResetStub(LoginService::StubInterface* stub) { stub_ = stub; }

  private:
    LoginService::StubInterface* stub_;
};

class LoginServiceImpl final : public LoginService::Service {
  public:
    Status SignUp(ServerContext* context, const CommonRequest* request, SignUpReply* reply) {
      string username = request->user_name();
      string password = request->password();
      if (username.compare("test") == 0) {
        // 模拟用户名已存在
        reply->set_resp_code(1001);
        reply->set_message("already exist");
      } else if (username.compare("test1") == 0) {
        // 模拟插库失败
        reply->set_resp_code(1002);
        reply->set_message("sign up failed");
      } else {
        // 模拟成功
        reply->set_resp_code(200);
        reply->set_message("succ");
      }
      return Status::OK;
    }
  
  Status Login(ServerContext* context, const CommonRequest* request, LoginReply* reply) {
    string username = request->user_name();
    string password = request->password();
    if (username.compare("test") == 0) {
        // 模拟用户不存在
        reply->set_resp_code(1003);
        reply->set_message("user not exists");
    } else if (username.compare("test1") == 0 && password.compare("test123") != 0) {
      // 模拟密码错误
      reply->set_resp_code(1004);
      reply->set_message("pwd is wrong");
    } else if (username.compare("test1") == 0 && password.compare("test123") == 0) {
      // 模拟成功
      reply->set_resp_code(200);
      reply->set_message("succ");
      string token = "someToken";
      reply->set_token(token);
      tokenMap[username] = token;
    }
    return Status::OK;
  }

  Status CheckStatus(ServerContext* context, const CheckRequest* request, ServerWriter<CheckReply>* writer) {
    CheckReply reply;
    string username = request->user_name();
    map<string, string>::iterator iter = tokenMap.find(username);
    if (iter != tokenMap.end()) {
      reply.set_token(iter -> second);
    }
    writer->Write(reply);
    // 模拟有其他客户端更新了token
    reply.set_token("otherToken");
    writer->Write(reply);
    return Status::OK;
  }

  private:
    map<string, string> tokenMap;
};

class MockTest : public ::testing::Test {
  protected:
    MockTest() {}
  
  void SetUp() override {
    server_address_ << "localhost:50051";
    ServerBuilder builder;
    builder.AddListeningPort(server_address_.str(), grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    server_ = builder.BuildAndStart();
  }

  void TearDown() override { server_ -> Shutdown(); }

  void ResetStub() {
    std::shared_ptr<Channel> channel = grpc::CreateChannel(
        server_address_.str(), grpc::InsecureChannelCredentials());
    stub_ = LoginService::NewStub(channel);
  }

  unique_ptr<LoginService::Stub> stub_;
  unique_ptr<Server> server_;
  ostringstream server_address_;
  LoginServiceImpl service_;
};

TEST_F(MockTest, testSignUpExist) {
  ResetStub();
  FakeClient client(stub_.get());
  client.DoSignUp("test", "test123");

  login::MockLoginServiceStub stub;
  SignUpReply reply;
  reply.set_resp_code(1001);
  reply.set_message("already exist");
  EXPECT_CALL(stub, SignUp(_, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(reply), Return(Status::OK)));
  client.ResetStub(&stub);
  client.DoSignUp("test", "test123");
}

TEST_F(MockTest, testSignUpFailed) {
  ResetStub();
  FakeClient client(stub_.get());
  client.DoSignUp("test1", "test123");

  login::MockLoginServiceStub stub;
  SignUpReply reply;
  reply.set_resp_code(1002);
  reply.set_message("sign up failed");
  EXPECT_CALL(stub, SignUp(_, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(reply), Return(Status::OK)));
  client.ResetStub(&stub);
  client.DoSignUp("test1", "test123");
}

TEST_F(MockTest, testSignUpSucc) {
  ResetStub();
  FakeClient client(stub_.get());
  client.DoSignUp("test2", "test123");

  login::MockLoginServiceStub stub;
  SignUpReply reply;
  reply.set_resp_code(200);
  reply.set_message("succ");
  EXPECT_CALL(stub, SignUp(_, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(reply), Return(Status::OK)));
  client.ResetStub(&stub);
  client.DoSignUp("test2", "test123");
}

TEST_F(MockTest, testLoginNotExist) {
  ResetStub();
  FakeClient client(stub_.get());
  client.DoLogin("test", "test123");

  login::MockLoginServiceStub stub;
  LoginReply reply;
  reply.set_resp_code(1003);
  reply.set_message("user not exists");
  EXPECT_CALL(stub, Login(_, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(reply), Return(Status::OK)));
  client.ResetStub(&stub);
  client.DoLogin("test", "test123");
}

TEST_F(MockTest, testLoginPwdWrong) {
  ResetStub();
  FakeClient client(stub_.get());
  client.DoLogin("test1", "123456");

  login::MockLoginServiceStub stub;
  LoginReply reply;
  reply.set_resp_code(1004);
  reply.set_message("pwd is wrong");
  EXPECT_CALL(stub, Login(_, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(reply), Return(Status::OK)));
  client.ResetStub(&stub);
  client.DoLogin("test1", "123456");
}

TEST_F(MockTest, testLoginSucc) {
  ResetStub();
  FakeClient client(stub_.get());
  client.DoLogin("test1", "test123");

  login::MockLoginServiceStub stub;
  LoginReply reply;
  reply.set_resp_code(200);
  reply.set_message("succ");
  EXPECT_CALL(stub, Login(_, _, _))
      .Times(AtLeast(1))
      .WillOnce(DoAll(SetArgPointee<2>(reply), Return(Status::OK)));
  client.ResetStub(&stub);
  client.DoLogin("test1", "test123");
}

TEST_F(MockTest, testCheckStatus) {
  ResetStub();
  FakeClient client(stub_.get());
  string username = "test1";
  string token = client.DoLogin(username, "test123");
  client.DoCheckStatus(username, token);

  login::MockLoginServiceStub stub;
  auto vec = new MockClientReader<CheckReply>();
  CheckReply r1;
  r1.set_token("someToken");
  CheckReply r2;
  r2.set_token("otherToken");
  EXPECT_CALL(*vec, Read(_))
    .Times(2)
    .WillOnce(DoAll(SetArgPointee<0>(r1), Return(true)))
    .WillOnce(DoAll(SetArgPointee<0>(r2), Return(true)))
    .WillOnce(Return(false));
  EXPECT_CALL(*vec, Finish()).Times(0).WillOnce(Return(Status::OK));

  EXPECT_CALL(stub, CheckStatusRaw(_, _))
    .WillOnce(Return(vec));

  client.ResetStub(&stub);
  client.DoCheckStatus(username, token);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}