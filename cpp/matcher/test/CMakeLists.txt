include(googletest)

add_gtest(matchers_test matchers_test.cc)
target_link_libraries(matchers_test matchers)

set_target_properties(matchers_test PROPERTIES FOLDER "matcher/test")

