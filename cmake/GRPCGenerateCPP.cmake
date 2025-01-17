# Origin: https://raw.githubusercontent.com/NVIDIA/yais/master/cmake/GRPCGenerateCPP.cmake
#find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin) # Get full path to plugin
set(GRPC_CPP_PLUGIN $<TARGET_FILE:grpc_cpp_plugin>) # TODO: Need double check this, when TARGET_FILE is set?
#set(GRPC_CPP_PLUGIN ${CMAKE_BINARY_DIR}/grpc/grpc_cpp_plugin)

function(PROTOBUF_GENERATE_GRPC_CPP SRCS HDRS)
  cmake_parse_arguments(protobuf "" "EXPORT_MACRO;DESCRIPTORS" "" ${ARGN})

  set(PROTO_FILES "${protobuf_UNPARSED_ARGUMENTS}")
  if(NOT PROTO_FILES)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_GRPC_CPP() called without any proto files")
    return()
  endif()

  if(PROTOBUF_GENERATE_CPP_APPEND_PATH) # This variable is common for all types of output.
    # Create an include path for each file specified
    foreach(FIL ${PROTO_FILES})
      get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
      get_filename_component(ABS_PATH ${ABS_FIL} PATH)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  else()
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
    set(Protobuf_IMPORT_DIR "${PROTOBUF_IMPORT_DIRS")
  endif()

  if(DEFINED Protobuf_IMPORT_DIRS)
    foreach(DIR ${Protobuf_IMPORT_DIRS})
      get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  endif()

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${PROTO_FILES})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    if(NOT PROTOBUF_GENERATE_CPP_APPEND_PATH)
      get_filename_component(FIL_DIR ${FIL} DIRECTORY)
      if(FIL_DIR)
        set(FIL_WE "${FIL_DIR}/${FIL_WE}")
      endif()
    endif()

    set(_protobuf_grpc_src "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.cc")
    set(_protobuf_grpc_hdr "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.grpc.pb.h")
    list(APPEND ${SRCS} "${_protobuf_grpc_src}")
    list(APPEND ${HDRS} "${_protobuf_grpc_hdr}")

    add_custom_command(
      OUTPUT "${_protobuf_grpc_src}"
             "${_protobuf_grpc_hdr}"
      COMMAND ${Protobuf_PROTOC_EXECUTABLE}
              --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
              --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
              ${_protobuf_include_path} ${ABS_FIL}
      DEPENDS ${ABS_FIL} ${Protobuf_PROTOC_EXECUTABLE}
      COMMENT "Running gRPC C++ protocol buffer compiler on ${FIL}"
      VERBATIM)
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()
