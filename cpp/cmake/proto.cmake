# For some reason, Windows requires this to be all lower-cased and
# Linux needs it to be capitalized in some environments...
IF (WIN32)
  find_package(protobuf CONFIG REQUIRED)
ELSE()
  find_package(Protobuf REQUIRED)
ENDIF()

function(add_proto name source)
  protobuf_generate(
      LANGUAGE cpp
      PROTOS ${source}
      OUT_VAR problem
      )
  get_filename_component(proto_name ${source} NAME_WE)
  add_library(${name} ${proto_name}.pb.h ${proto_name}.pb.cc)
  target_link_libraries(${name} PUBLIC protobuf::libprotobuf)

  target_include_directories(${name} PUBLIC protobuf::libprotobuf_includes)
  target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endfunction()