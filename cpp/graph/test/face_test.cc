
#include "graph/face.h"

#include <cmath>
#include <sstream>
#include <string>

#include "utils/exception.h"
#include "utils/json.h"
#include "utils/log.h"
#include "gtest/gtest.h"

using utils::ConfigurationException;
using utils::Json;
using Face = ::graph::Face<double>;

TEST(Face, InitializesEmpty)
{
  Face face;
  EXPECT_EQ(face.edge_ids().size(), 0);
}

TEST(Face, InitializesWithCost)
{
  Face face(2.0);
  EXPECT_EQ(face.edges_count(), 0);
  EXPECT_EQ(face.cost(), 2.0);
}

TEST(Face, InitializesWithLists)
{
  std::vector<int> ids = {1};
  Face face(ids);
  EXPECT_EQ(face.edges_count(), 1);
  EXPECT_EQ(face.edge_ids().size(), 1);
  EXPECT_EQ(face.edge_ids()[0], 1);
}

TEST(Face, AddsId)
{
  std::vector<int> ids = {1};
  Face face(ids);
  face.add_edge_id(2);
  EXPECT_EQ(face.edge_ids().size(), 2);
  EXPECT_EQ(face.edge_ids()[1], 2);
}

TEST(Face, RemovesId)
{
  std::vector<int> ids = {1, 2};
  Face face(ids);
  face.remove_edge_id(1);
  EXPECT_EQ(face.edge_ids().size(), 1);
  EXPECT_EQ(face.edge_ids()[0], 2);
}

TEST(Face, Debugs)
{
  std::vector<int> ids = {1};
  Face face(ids);
  EXPECT_EQ(face.get_class_name(), "graph::Face<double>");
  std::stringstream s;
  s << face;
  EXPECT_EQ(s.str(), "<graph::Face<double>: [1]>");
}
