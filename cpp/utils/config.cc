
#include "utils/config.h"

#include <bitset>
#include <cmath>
#include <vector>

#include "utils/exception.h"

namespace utils
{
class FeatureConfigs
{
 public:
  FeatureConfigs()
  {
    std::bitset<64> initial(0);
    m_features.resize((Features::FEATURE_COUNT + 63) / 64, initial);
  };

  void set_disabled_feature(int feature_id)
  {
    size_t id = static_cast<size_t>(feature_id);
    m_features[id / 64].set(id % 64, false);
  };

  void set_enabled_feature(int feature_id)
  {
    size_t id = static_cast<size_t>(feature_id);
    m_features[id / 64].set(id % 64, true);
  };

  bool feature_disabled(int feature_id)
  {
    return !(feature_enabled(feature_id));
  };

  bool feature_enabled(int feature_id)
  {
    size_t id = static_cast<size_t>(feature_id);
    return m_features[id / 64][id % 64];
  };

 private:
  std::vector<std::bitset<64>> m_features;
};

FeatureConfigs g_features;

void set_disabled_feature(const std::set<int>& features)
{
  for (auto id : features)
  {
    if (id < 0 || id >= Features::FEATURE_COUNT)
    {
      throw ValueException("Wrong feature id for disabled features!");
    }
    g_features.set_disabled_feature(id);
  }
}

void set_enabled_feature(const std::set<int>& features)
{
  for (auto id : features)
  {
    if (id < 0 || id >= Features::FEATURE_COUNT)
    {
      throw ValueException("Wrong feature id for disabled features!");
    }
    g_features.set_enabled_feature(id);
  }
}

void initialize_features()
{
  std::set<int> features = {Features::FEATURE_COMPACT_TERMS,
                            Features::FEATURE_ACCURATE_TIMEOUT};
  set_enabled_feature(features);
}

bool feature_disabled(int feature_id)
{
  return g_features.feature_disabled(feature_id);
}

bool feature_enabled(int feature_id)
{
  return g_features.feature_enabled(feature_id);
}

}  // namespace utils
