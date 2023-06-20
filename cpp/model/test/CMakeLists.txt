include(googletest)

add_gtest(ising_test ising_test.cc)
target_link_libraries(ising_test model utils solver)

add_gtest(ising_term_cached_test ising_term_cached_test.cc)
target_link_libraries(ising_term_cached_test model utils)

add_gtest(ising_grouped_test ising_grouped_test.cc)
target_link_libraries(ising_grouped_test model utils)

add_gtest(pubo_test pubo_test.cc)
target_link_libraries(pubo_test model utils solver)

add_gtest(pubo_with_counter_test pubo_with_counter_test.cc)
target_link_libraries(pubo_with_counter_test model utils)

add_gtest(pubo_grouped_test pubo_grouped_test.cc)
target_link_libraries(pubo_grouped_test model utils)

add_gtest(pubo_adaptive_test pubo_adaptive_test.cc)
target_link_libraries(pubo_adaptive_test model utils)

add_gtest(clock_test clock_test.cc)
target_link_libraries(clock_test model utils)

add_gtest(permutation_test permutation_test.cc)
target_link_libraries(permutation_test model)

add_gtest(partition_test partition_test.cc)
target_link_libraries(partition_test model)

add_gtest(poly_test poly_test.cc)
target_link_libraries(poly_test model utils)

add_gtest(max_sat_test max_sat_test.cc)
target_link_libraries(max_sat_test model utils solver)

add_gtest(model_registry_test model_registry_test.cc)
target_link_libraries(model_registry_test model utils)

set_target_properties(ising_test ising_term_cached_test ising_grouped_test pubo_test pubo_with_counter_test pubo_grouped_test 
    pubo_adaptive_test clock_test permutation_test partition_test poly_test max_sat_test model_registry_test PROPERTIES FOLDER "model/test")
