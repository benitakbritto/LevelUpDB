cmake_minimum_required(VERSION 3.5.1)

project(BlockStorage C CXX)
add_subdirectory(third_party/leveldb)
add_executable(runscript util/levelDBWrapper.cc)
target_link_libraries(runscript PUBLIC leveldb)
include(${CMAKE_CURRENT_SOURCE_DIR}/../common.cmake)

# Proto files
# Proto file 1
get_filename_component(comm_proto "../src/protos/servercomm.proto" ABSOLUTE)
get_filename_component(comm_proto_path "${comm_proto}" PATH)
# Proto file 2
get_filename_component(lb_proto "../src/protos/lb.proto" ABSOLUTE)
get_filename_component(lb_proto_path "${lb_proto}" PATH)

# Generated sources
# Proto file 1
set(comm_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.pb.cc")
set(comm_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.pb.h")
set(comm_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.grpc.pb.cc")
set(comm_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/servercomm.grpc.pb.h")
add_custom_command(
      OUTPUT "${comm_proto_srcs}" "${comm_proto_hdrs}" "${comm_grpc_srcs}" "${comm_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${comm_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${comm_proto}"
      DEPENDS "${comm_proto}")

# Proto file 2
set(lb_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/lb.pb.cc")
set(lb_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/lb.pb.h")
set(lb_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/lb.grpc.pb.cc")
set(lb_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/lb.grpc.pb.h")
add_custom_command(
      OUTPUT "${lb_proto_srcs}" "${lb_proto_hdrs}" "${lb_grpc_srcs}" "${lb_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${lb_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${lb_proto}"
      DEPENDS "${lb_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Proto file 1
add_library(comm_grpc_proto
  ${comm_grpc_srcs}
  ${comm_grpc_hdrs}
  ${comm_proto_srcs}
  ${comm_proto_hdrs})
target_link_libraries(comm_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Proto file 2
add_library(lb_grpc_proto
  ${lb_grpc_srcs}
  ${lb_grpc_hdrs}
  ${lb_proto_srcs}
  ${lb_proto_hdrs})
target_link_libraries(lb_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})


# add_library(client_lib "client.cc")
# target_link_libraries(client_lib 
#   hw_grpc_proto
# )

# Client and LB
# foreach(_target
#   blockstorage_client load_balancer)
#   add_executable(${_target} "${_target}.cc")
#   target_link_libraries(${_target}
#     hw_grpc_proto
#     lb_grpc_proto
#     client_lib
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()


# Server
# foreach(_target
#   blockstorage_server )
#   add_library(atl "util/address_translation.cc")
#   add_library(wal "util/wal.cc")
#   add_library(txn "util/txn.cc")
#   add_library(kv "util/kv_store.cc")
#   add_library(locks "util/locks.cc")
#   add_library(cacheUtil "util/cache.cc")
#   add_executable(${_target} "${_target}.cc")
#   target_link_libraries(${_target}
#     atl
#     wal
#     txn
#     kv
#     cr
#     locks
#     cacheUtil
#     hw_grpc_proto
#     comm_grpc_proto
#     lb_grpc_proto
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()