include(googletest)

add_library(test_model test_model.h test_model.cc)
target_link_libraries(test_model model)

add_gtest(population_test population_test.cc ../partition.h)
target_link_libraries(population_test model utils markov test_model solver)

add_gtest(estimator_test estimator_test.cc)
target_link_libraries(estimator_test model utils markov test_model solver)

add_gtest(parallel_tempering_test parallel_tempering_test.cc ../parallel_tempering.h) 
target_link_libraries(parallel_tempering_test test_model utils schedule markov solver)

add_gtest(simulated_annealing_test simulated_annealing_test.cc ../simulated_annealing.h)
target_link_libraries(simulated_annealing_test test_model model utils schedule markov solver)

add_gtest(population_annealing_test population_annealing_test.cc ../population_annealing.h)
target_link_libraries(population_annealing_test test_model utils schedule markov solver)

add_gtest(tabu_test tabu_test.cc ../tabu.h)
target_link_libraries(tabu_test test_model utils schedule markov solver)

add_gtest(substochastic_monte_carlo_test substochastic_monte_carlo_test.cc ../substochastic_monte_carlo.h)
target_link_libraries(substochastic_monte_carlo_test test_model utils schedule markov solver)

add_gtest(quantum_monte_carlo_test quantum_monte_carlo_test.cc ../quantum_monte_carlo.h)
target_link_libraries(quantum_monte_carlo_test test_model utils schedule markov solver)

add_gtest(ssmc_pf_test ssmc_pf_test.cc ../parameter_free_solver.h ../parameter_free_adapter.h ../ssmc_parameter_free.h)
target_link_libraries(ssmc_pf_test test_model utils schedule markov solver strategy gpp)

add_gtest(pa_pf_test pa_pf_test.cc ../parameter_free_solver.h ../parameter_free_adapter.h ../pa_parameter_free.h)
target_link_libraries(pa_pf_test test_model utils schedule markov solver strategy gpp)

add_gtest(sa_pf_test sa_pf_test.cc ../parameter_free_linear_solver.h ../parameter_free_linear_adapter.h ../sa_parameter_free.h)
target_link_libraries(sa_pf_test test_model utils schedule markov solver strategy)

add_gtest(pt_pf_test pt_pf_test.cc  ../parameter_free_linear_solver.h ../parameter_free_linear_adapter.h ../pt_parameter_free.h)
target_link_libraries(pt_pf_test test_model utils schedule markov solver strategy)

add_gtest(tabu_pf_test tabu_pf_test.cc ../parameter_free_linear_solver.h ../parameter_free_linear_adapter.h ../tabu_parameter_free.h)
target_link_libraries(tabu_pf_test test_model utils schedule markov solver strategy)

add_gtest(solver_registry_test solver_registry_test.cc)
target_link_libraries(solver_registry_test test_model utils schedule markov solver)

set_target_properties(test_model population_test estimator_test parallel_tempering_test  
    simulated_annealing_test  population_annealing_test tabu_test 
    substochastic_monte_carlo_test substochastic_monte_carlo_test quantum_monte_carlo_test  
    ssmc_pf_test pa_pf_test sa_pf_test pt_pf_test tabu_pf_test solver_registry_test 
    PROPERTIES FOLDER "solver/test")
