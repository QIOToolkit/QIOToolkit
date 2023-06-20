include(googletest)

add_gtest(state_test state_test.cc)
target_link_libraries(state_test markov utils)

add_gtest(transition_test transition_test.cc)
target_link_libraries(transition_test markov utils)

add_gtest(model_test model_test.cc)
target_link_libraries(model_test markov utils)

add_gtest(metropolis_test metropolis_test.cc)
target_link_libraries(metropolis_test markov solver utils test_model)

set_target_properties(state_test transition_test model_test metropolis_test PROPERTIES FOLDER "markov/test")


