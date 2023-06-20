include(FetchContent)

FetchContent_Declare(
  pcg
  GIT_REPOSITORY https://github.com/imneme/pcg-cpp
  GIT_TAG        ffd522e7188bef30a00c74dc7eb9de5faff90092
)

FetchContent_GetProperties(pcg)
if(NOT pcg_POPULATED)
  FetchContent_Populate(pcg)
  add_library(pcg INTERFACE)
  target_include_directories(pcg INTERFACE ${pcg_SOURCE_DIR}/include)
endif()
