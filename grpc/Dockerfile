FROM suchen92/login-server_grpctiny:latest

ADD . /greeter_server

WORKDIR /greeter_server

RUN bazel build src:greeter_server

RUN apt-get install build-essential  