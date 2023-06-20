include(googletest)

add_gtest(graph_compact_test graph_compact_test.cc)
target_link_libraries(graph_compact_test graph utils)

add_gtest(graph_test graph_test.cc)
target_link_libraries(graph_test graph utils)

add_gtest(edge_test edge_test.cc)
target_link_libraries(edge_test graph utils)

add_gtest(cost_edge_test cost_edge_test.cc)
target_link_libraries(cost_edge_test graph utils)

add_gtest(node_test node_test.cc)
target_link_libraries(node_test graph utils)

add_gtest(face_test face_test.cc)
target_link_libraries(face_test graph utils)

add_gtest(properties_test properties_test.cc)
target_link_libraries(properties_test graph utils)

set_target_properties(graph_compact_test graph_test edge_test cost_edge_test node_test face_test properties_test PROPERTIES FOLDER "graph/test")
