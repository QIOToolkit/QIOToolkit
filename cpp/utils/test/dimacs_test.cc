
#include "utils/dimacs.h"

#include <string>

#include "utils/json.h"
#include "gtest/gtest.h"

using utils::Dimacs;

TEST(Dimacs, Unweighted)
{
  Dimacs dimacs;
  dimacs.read(R"(c
c
c Unweighted Max-SAT
c
p cnf 2 3
-1 0
1 2 0
-2 0
)");

  EXPECT_EQ(dimacs.get_type(), "cnf");
  EXPECT_EQ(dimacs.get_nvar(), 2);
  EXPECT_EQ(dimacs.get_ncl(), 3);
  EXPECT_EQ(dimacs.get_variables(), std::vector<int>({1, 2}));
  const auto& clauses = dimacs.get_clauses();
  EXPECT_EQ(clauses[0].variables, std::vector<int>({-1}));
  EXPECT_EQ(clauses[1].variables, std::vector<int>({1, 2}));
  EXPECT_EQ(clauses[2].variables, std::vector<int>({-2}));
  EXPECT_EQ(clauses[0].weight, 1);
  EXPECT_EQ(clauses[1].weight, 1);
  EXPECT_EQ(clauses[2].weight, 1);
  EXPECT_EQ(dimacs.get_top(), 0);
}

TEST(Dimacs, Weighted)
{
  Dimacs dimacs;
  dimacs.read(R"(c
c
c Weighted Max-SAT
c
p wcnf 2 3
1 -1 0
4 1 2 0
2 -2 0
)");

  EXPECT_EQ(dimacs.get_type(), "wcnf");
  EXPECT_EQ(dimacs.get_nvar(), 2);
  EXPECT_EQ(dimacs.get_ncl(), 3);
  EXPECT_EQ(dimacs.get_variables(), std::vector<int>({1, 2}));
  const auto& clauses = dimacs.get_clauses();
  EXPECT_EQ(clauses[0].variables, std::vector<int>({-1}));
  EXPECT_EQ(clauses[1].variables, std::vector<int>({1, 2}));
  EXPECT_EQ(clauses[2].variables, std::vector<int>({-2}));
  EXPECT_EQ(clauses[0].weight, 1);
  EXPECT_EQ(clauses[1].weight, 4);
  EXPECT_EQ(clauses[2].weight, 2);
  EXPECT_EQ(dimacs.get_top(), 0);
}

TEST(Dimacs, Parial)
{
  Dimacs dimacs;
  dimacs.read(R"(c
c
c Weighted Max-SAT
c
p wcnf 2 3 4
1 -1 0
4 1 2 0
2 -2 0
)");

  EXPECT_EQ(dimacs.get_type(), "wcnf");
  EXPECT_EQ(dimacs.get_nvar(), 2);
  EXPECT_EQ(dimacs.get_ncl(), 3);
  EXPECT_EQ(dimacs.get_variables(), std::vector<int>({1, 2}));
  const auto& clauses = dimacs.get_clauses();
  EXPECT_EQ(clauses[0].variables, std::vector<int>({-1}));
  EXPECT_EQ(clauses[1].variables, std::vector<int>({1, 2}));
  EXPECT_EQ(clauses[2].variables, std::vector<int>({-2}));
  EXPECT_EQ(clauses[0].weight, 1);
  EXPECT_EQ(clauses[1].weight, 4);
  EXPECT_EQ(clauses[2].weight, 2);
  EXPECT_EQ(dimacs.get_top(), 4);
}
