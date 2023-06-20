include(googletest)

add_gtest(observable_test observable_test.cc)
target_link_libraries(observable_test observe)

set_target_properties(observable_test PROPERTIES FOLDER "observe/test")

