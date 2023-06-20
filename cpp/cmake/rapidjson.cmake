include(FetchContent)

FetchContent_Declare(
  rapidjson
  GIT_REPOSITORY https://github.com/Tencent/rapidjson
  GIT_TAG        17aa824c928ea111e9b12a61e06d98335ce98f15
)

FetchContent_GetProperties(rapidjson)
if(NOT rapidjson_POPULATED)
  FetchContent_Populate(rapidjson)
  add_library(rapidjson INTERFACE)
  target_include_directories(rapidjson INTERFACE ${rapidjson_SOURCE_DIR}/include)
endif()
