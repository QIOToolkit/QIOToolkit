include(googletest)

add_gtest(schedule_test schedule_test.cc)
target_link_libraries(schedule_test schedule utils)

add_gtest(constant_test constant_test.cc)
target_link_libraries(constant_test schedule utils)

add_gtest(linear_test linear_test.cc)
target_link_libraries(linear_test schedule utils)

add_gtest(geometric_test geometric_test.cc)
target_link_libraries(geometric_test schedule utils)

add_gtest(segments_test segments_test.cc)
target_link_libraries(segments_test schedule utils)

set_target_properties(schedule_test constant_test linear_test geometric_test segments_test PROPERTIES FOLDER "schedule/test")

