include(googletest)

add_gtest(bit_stream_test bit_stream_test.cc)
target_link_libraries(bit_stream_test utils)

add_gtest(optional_test optional_test.cc)
target_link_libraries(optional_test utils)

add_gtest(json_test json_test.cc)
target_link_libraries(json_test utils)

add_gtest(component_test component_test.cc)
target_link_libraries(component_test utils)

add_gtest(language_test language_test.cc)
target_link_libraries(language_test utils)

add_gtest(log_test log_test.cc)
target_link_libraries(log_test utils)

add_gtest(structure_test structure_test.cc)
target_link_libraries(structure_test utils)

add_gtest(parameter_test parameter_test.cc)
target_link_libraries(parameter_test utils)

add_gtest(random_generator_test random_generator_test.cc)
target_link_libraries(random_generator_test utils)

add_gtest(random_selector_test random_selector_test.cc)
target_link_libraries(random_selector_test utils)

add_gtest(config_test config_test.cc)
target_link_libraries(config_test utils)

add_gtest(exception_test exception_test.cc)
target_link_libraries(exception_test utils)

add_gtest(stream_test stream_test.cc)
target_link_libraries(stream_test utils)

add_gtest(stream_proto_test stream_proto_test.cc)
target_link_libraries(stream_proto_test utils)

add_gtest(utils_test utils_test.cc)
target_link_libraries(utils_test utils)

add_gtest(dimacs_test dimacs_test.cc)
target_link_libraries(dimacs_test model utils)

set_target_properties(bit_stream_test optional_test json_test component_test language_test log_test structure_test 
    parameter_test random_generator_test random_generator_test random_selector_test config_test 
    exception_test stream_test stream_proto_test utils_test dimacs_test PROPERTIES FOLDER "utils/test")

