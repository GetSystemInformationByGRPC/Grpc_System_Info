cmake_minimum_required(VERSION 3.28)
project(Project1)

set(CMAKE_CXX_STANDARD 17)

#set(PROTO_FILES "/home/mohammad/Desktop/Grpc_System_Info/Project1/SysInfo.proto")
set(PROTO_FILES "/home/mohammadmahdi/Desktop/Grpc_System_Info/Project1/SysInfo.proto")

include_directories("/usr/include")
link_directories("/usr/lib")

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)



#include_directories("//usr//include")
#include_directories("//usr//local//include")
set(PROTOC "/usr/bin/protoc")
set(GRPC_CPP_PLUGIN "/bin/grpc_cpp_plugin")
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

set(PROTO_SRC SysInfo.proto)
find_package(gRPC REQUIRED)

#execute_process(COMMAND ${PROTOC} --proto_path=. --proto_path=${CMAKE_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} --grpc_out=${CMAKE_CURRENT_BINARY_DIR} --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin SysInfo.proto
#
#        RESULT_VARIABLE proto_result
#        ERROR_VARIABLE proto_error)
#
#if(NOT ${proto_result} EQUAL 0)
#    message(FATAL_ERROR "running grpc protoc on controller.proto failed: ${proto_error}")
#endif()
#find_package(Protobuf  REQUIRED)
#set(Protobuf_INCLUDE_DIR "//usr//include")
#set(Protobuf_LIBRARIES "//lib//x86_64-linux-gnu//libprotobuf.so")
#set(Protobuf_PROTOC_EXECUTABLE "//bin//protoc")
add_executable(Project1 main.cpp Logger.cpp ${PROTOBUF_LIBRARY}
        sysinfo.cpp
        sysinfo.h
        SysInfo.grpc.pb.h
        SysInfo.grpc.pb.cc
        SysInfo.pb.h
        SysInfo.pb.cc
                )
target_link_libraries(Project1
        PUBLIC
        gRPC::grpc++
        fmt::fmt
        spdlog::spdlog
        ${PROTOBUF_LIBRARIES}
)