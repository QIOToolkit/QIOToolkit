include(googletest)

add_gtest(soft_spin_test soft_spin_test.cc)
target_link_libraries(soft_spin_test example utils graph)

add_gtest(descent_test descent_test.cc)
target_link_libraries(descent_test example utils graph)

set_target_properties(soft_spin_test descent_test PROPERTIES FOLDER "examples/test")
