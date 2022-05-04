cmake_minimum_required(VERSION 3.5.1)

project(kvstore C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common" ${CMAKE_MODULE_PATH})
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


include(${CMAKE_CURRENT_SOURCE_DIR}/common/common.cmake)

# Proto files
# Proto file 1
get_filename_component(keyvalueops_proto "../src/protos/keyvalueops.proto" ABSOLUTE)
get_filename_component(keyvalueops_proto_path "${keyvalueops_proto}" PATH)

# Generated sources
# Proto file 1
set(keyvalueops_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/keyvalueops.pb.cc")
set(keyvalueops_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/keyvalueops.pb.h")
set(keyvalueops_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/keyvalueops.grpc.pb.cc")
set(keyvalueops_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/keyvalueops.grpc.pb.h")
add_custom_command(
      OUTPUT "${keyvalueops_proto_srcs}" "${keyvalueops_proto_hdrs}" "${keyvalueops_grpc_srcs}" "${keyvalueops_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${keyvalueops_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${keyvalueops_proto}"
      DEPENDS "${keyvalueops_proto}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Proto file 1
add_library(keyvalueops_grpc_proto
  ${keyvalueops_grpc_srcs}
  ${keyvalueops_grpc_hdrs}
  ${keyvalueops_proto_srcs}
  ${keyvalueops_proto_hdrs})
target_link_libraries(keyvalueops_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(client_lib "../src/client/client.cc")
target_link_libraries(client_lib 
  keyvalueops_grpc_proto
)

# For client tests  
foreach(_target
  read)
  add_executable(${_target} "./${_target}.cc")
  target_link_libraries(${_target}
    keyvalueops_grpc_proto
    client_lib
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()