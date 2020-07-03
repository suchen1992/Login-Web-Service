- [题目](#题目)
- [运行环境](#运行环境)
- [方案设计](#方案设计)
  - [模块设计](#模块设计)
  - [安全性设计（完成一半）](#安全性设计完成一半)
  - [单元测试（未实现）](#单元测试未实现)
- [运行命令](#运行命令)


## 题目

设计单终端登录系统，具备：
* 注册登录功能，单终端登录
* 使用`C++`+`grpc`实现，数据存储使用`mysql`，注意数据安全与传输安全

## 运行环境

```
OS: MacOS 10.15.5

Bazel: 3.3.0-homebrew

gcc: Apple clang version 11.0.3 (clang-1103.0.32.62)

docker: 19.03.8

mysql-client: 5.7
```

## 方案设计

### 模块设计

系统主要分为两个模块：

1. 命令行客户端：启动后用于发送注册、登录、持续检查登录状态等功能
2. 服务端：接收客户端请求，与数据库进行交互，提供注册、登录、单终端登录校验等接口


注册接口流程图如下所示

 ![image](img/signup-flowchart.png)

登录接口流程图如下所示

 ![image](img/login-flowchart.png)

登录校验接口流程图如下所示

 ![image](img/checkstatus-flowchart.png)


* 用户登陆成功，server端会返回一个token值，同时根据username做key保存在map中，当有其他终端登录，则覆盖token；用户登陆成功后，根据用户名请求登录校验接口，server端持续返回对应token，客户端获取到token进行对比，不一致则断开连接

本机环境下操作录屏：
https://v.youku.com/v_show/id_XNDczNjMzMDE1Mg==.html


1. 数据库连接模块
   数据库连接部分， 在mac环境下启动时遇到过一个问题，找不到安装之后的mysql-client库，查过资料，原因可能是由于mac环境下启动扫描的是xcode安装路径，而不是从/usr路径下扫描依赖，解决方案一个可以在xcode对应路径下创建软连接，这里采用的是另一种较为简单粗暴的方案，就是把dylib文件与对应头文件放在项目路径下


### 安全性设计（完成一半）

1. 数据安全：
   server端接到密码后，使用PBKDF2SHA1加密，salt值随机生成，并迭代10000次，得到密文，db中保存格式为： `PBKDF2SHA1$10000$salt$cypher_text`
2. 传输安全（未实现）：
   1. 传输安全方面，初始方案设计时准备使用grpc + ssl，证书与密钥生成脚本见`generate_ca.sh`，但server端在加载启动过程中，会有报错，查过资料说在使用openssl生成证书时应当确保CN的地址与代码中地址完全一致，但多次检查没有问题的情况下依旧启动失败，不论更换localhost还是使用ip并修改hosts文件地址映射
   2. 方案1尝试过几版失败后考虑发送post请求，但在proto文件中`import google/api/*.proto`会报文件不存在错误，但查过`/usr/local/include/google/protobuf/` 路径下对应头文件存在，由于时间原因没有进行进一步尝试

### 单元测试（未实现）

测试样例编写过程中，BUILD文件中cc_test块在deps部分引入grpc依赖时会报错，在github看过grpc官方test example与其他一些人的代码，发觉BUILD文件没有一个明确固定的书写结构，较为灵活，但在尝试过程中始终没能成功引入依赖，可能还是自己对于BUILD文件的依赖了解不够深入，时间原因没有在进行进一步尝试


## 运行命令
1. ${project}项目根目录下运行`docker-compose up`
2. `${project}/grpc/src`路径下运行`bazel run :login_server`启动服务端
3. `${project}/grpc/src`路径下运行`bazel run :login_client`启动客户端