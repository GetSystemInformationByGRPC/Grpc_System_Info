cmake_minimum_required(VERSION 3.28)
project(Project1)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf 3.21.12 REQUIRED)

execute_process(COMMAND ${PROTOC} --proto_path=. --proto_path=${CMAKE_SOURCE_DIR}/../../common --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/grpc --grpc_out=${CMAKE_CURRENT_BINARY_DIR}/grpc --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin SysInfo.proto
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../ControllerApi
        RESULT_VARIABLE proto_result
        ERROR_VARIABLE proto_error)

if(NOT ${proto_result} EQUAL 0)
    message(FATAL_ERROR "running grpc protoc on failed: ${proto_error}")
endif()



#find_package(Protobuf  REQUIRED)
#set(Protobuf_INCLUDE_DIR "//usr//include")
#set(Protobuf_LIBRARIES "//lib//x86_64-linux-gnu//libprotobuf.so")
#set(Protobuf_PROTOC_EXECUTABLE "//bin//protoc")
add_executable(Project1 clipp.h  main.cpp Logger.h Logger.cpp sysinfo.h sysinfo.cpp )
