licenses(["notice"])  # 3-clause BSD

package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
    name = "login_proto",
    srcs = ["login.proto"],
)

cc_proto_library(
    name = "login_cc_proto",
    deps = [":login_proto"],
)

cc_grpc_library(
    name = "login_cc_grpc",
    srcs = [":login_proto"],
    grpc_only = True,
    deps = [":login_cc_proto"],
    generate_mocks = True,
)

cc_library(
    name = "mysqlDriver",
    srcs = ["libmysqlclient.21.dylib"],
    hdrs = glob(["mysql/*.h"]),
)

cc_library(
    name = "mysqlpool",
    srcs = ["mysqlpool.cc"],
    hdrs = ["mysqlpool.h"],
    deps = [
        ":mysqlDriver",
    ],
    copts = ["-Imysql"],
)

cc_library(
    name = "clientlib",
    srcs = ["login_client.cc"],
    hdrs = ["login_client.h"],
    deps = [
        ":login_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

cc_binary(
    name = "login_client",
    srcs = ["login_client_main.cc"],
    deps = [
        ":clientlib",
    ],
)

cc_binary(
    name = "login_server",
    srcs = ["login_server.cc"],
    deps = [
        ":login_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        ":mysqlpool",
    ],
)

cc_test(
    name = "login_test",
    srcs = ["login_test.cc"],
    linkstatic = True,
    deps = [
        ":clientlib",
        "//external:gtest",
        "@com_github_grpc_grpc//:grpc++_test",
    ],
)