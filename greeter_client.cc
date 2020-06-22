/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "login.grpc.pb.h"
#else
#include "login.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string SayHelloAgain(const std::string& user) {
    // Follows the same pattern as SayHello.
    HelloRequest request;
    request.set_name(user);
    HelloReply reply;
    ClientContext context;

    // Here we can use the stub's newly available method we just added.
    Status status = stub_->SayHelloAgain(&context, request, &reply);
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv) {
  std::string address = "localhost";
  std::string port = "50051";
  std::string server_address = address + ":" + port;
  std::cout << "Client querying server address: " << server_address << std::endl;
  GreeterClient greeter(grpc::CreateChannel(
      server_address, grpc::InsecureChannelCredentials()));
  bool flag = true;
  while (flag) {
    std::cout << "Please choose service: 1. Sign in; 2. Sign up; 0. quit" << std::endl;
    int num = 0;
    std::cin >> num;
    if (num == 1) {
      std::cout << "Enter your username: " << std::endl;
      std::string user;
      std::cin >> user;
      std::string reply = greeter.SayHello(user);
      std::cout << "Greeter hello received: " << reply << std::endl;
    } else if (num == 2) {
      std::cout << "Enter username: " << std::endl;
      std::string user;
      std::cin >> user;
      std::string reply = greeter.SayHelloAgain(user);
      std::cout << "Greeter hello again received: " << reply << std::endl;
    } else if (num == 0) {
      flag = false;
    } else {
      std::cout << "Error number" << std::endl;
    }
  }
  return 0;
}
