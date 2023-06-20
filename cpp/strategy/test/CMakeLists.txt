include(googletest)

add_gtest(gd_parameters_test gd_parameters_test.cc)
target_link_libraries(gd_parameters_test strategy)

add_gtest(bayesian_test bayesian_test.cc)
target_link_libraries(bayesian_test strategy)

add_gtest(linear_search_test linear_search_test.cc)
target_link_libraries(linear_search_test strategy)

set_target_properties(gd_parameters_test bayesian_test linear_search_test PROPERTIES FOLDER "strategy/test")

