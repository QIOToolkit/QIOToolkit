include(googletest)

add_gtest(runner_test runner_test.cc)
target_link_libraries(runner_test runner  utils solver
  model
  rapidjson
  gpp
  strategy)

set_target_properties(runner_test PROPERTIES FOLDER "app/test")