include(FetchContent)

FetchContent_Declare(
  picojson
  GIT_REPOSITORY https://github.com/kazuho/picojson
  GIT_TAG        111c9be5188f7350c2eac9ddaedd8cca3d7bf394s
)

FetchContent_GetProperties(picojson)
if(NOT picojson_POPULATED)
  FetchContent_Populate(picojson)
  add_library(picojson INTERFACE)
  target_include_directories(picojson INTERFACE ${picojson_SOURCE_DIR})
endif()
