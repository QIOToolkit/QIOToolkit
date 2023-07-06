
#pragma once

#include <map>
#include <memory>
#include <vector>

#include "utils/component.h"
#include "omp.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// A population is a container for several instances of `Content_T`
///
/// Each instance of `Content_T` is internally wrapped in a `Citizen` wrapper,
/// which allows setting the desired duplicity count of the contained instance
/// after resampling. These new counts are, however, only applied after a call
/// to `resample()`.
///
/// Example:
///
///   Population<std::string> population;
///   population.insert("foo");
///   population.insert("bar");
///   population.insert("baz");
///   for (size_t i = 0; i < population.size(); i++) {
///     auto& citizen = population[i];
///     if (citizen.content() == "foo") citizen.kill()
///     if (citizen.content() == "baz") citizen.spawn(2)
///   }
///   // output before resampling: "foobarbaz"
///   for (size_t i = 0; i < population.size(); i++) {
///     std::cout << population[i].content();
///   }
///
///   population.resample();
///
///   // output after resampling: "bazbarbazbaz"
///   for (size_t i = 0; i < population.size(); i++) {
///     std::cout << population[i].content();
///   }
///
template <class Content_T>
class Population : public utils::Component
{
 public:
  using FamilyId = size_t;

  Population() {}

  class Citizen
  {
   public:
    // Create an uninitialized citizen
    Citizen() : count_(0), family_id_(-1) {}

    // Disallow copy and assign: This ensures iteration is done by
    // reference (otherwise the count is set on a copy and ignored).
    Citizen(const Citizen& other) = delete;

    // Create a new citizen for the content provided
    explicit Citizen(Content_T content)
        : count_(1), family_id_(0), content_(content)
    {
    }

    // Get the count currently assigned to this citizen.
    size_t get_count() const { return count_; }

    // Set the count (desired number of copies) after resampling.
    void set_count(size_t count) { count_ = count; }

    // Kill this citizen (shorthand for setting the count to 0)
    void kill() { set_count(0); }

    // Spawn `n` **additional** copies of this citizen.
    void spawn(size_t n) { set_count(count_ + n); }

    void set_family(FamilyId family_id) { family_id_ = family_id; }
    FamilyId get_family() const { return family_id_; }

    // Accessors for the content within this wrapper
    // (either with the `content()` method or by de-referencing.)
    const Content_T& content() const { return content_; }
    Content_T& content() { return content_; }
    const Content_T& operator*() const { return content_; }
    Content_T& operator*() { return content_; }
    const Content_T* operator->() const { return &content_; }
    Content_T* operator->() { return &content_; }

    // Comparator for sorting citizens
    static bool compare(const std::unique_ptr<Citizen>& c1,
                        const std::unique_ptr<Citizen>& c2)
    {
      return Content_T::compare(**c1, **c2);
    }

   private:
    friend class Population;
    Citizen& operator=(const Citizen& other)
    {
      count_ = other.count_;
      family_id_ = other.family_id_;
      content_ = other.content_;
      return *this;
    }

    size_t count_;
    FamilyId family_id_;
    Content_T content_;
  };

  void reserve(size_t size) { citizens_.reserve(size); }

  /// Add `content` to the population (with a count of 1).
  void insert(Content_T content)
  {
    std::unique_ptr<Citizen> citizen;
    create_citizen(citizen, content);
    citizens_.push_back(std::move(citizen));
  }

  void resize(size_t new_count)
  {
    if (new_count > citizens_.size())
    {
      LOG(WARN,
          "Population::resize is not intended to grow the population. "
          "It appends nullptrs.");
      citizens_.resize(new_count);
    }
    else
    {
      while (citizens_.size() > new_count)
      {
        remove_citizen(citizens_.back());
        citizens_.pop_back();
      }
    }
    families_.clear();
  }

  /// Get the current number of citizens.
  ///
  /// This does **not** consider current `count`s set on the citizens, so it
  /// should be called after `resample()`, prior to any `set_count()` on the
  /// citizens.
  size_t size() const { return citizens_.size(); }

  /// Check if the population is empty
  bool empty() { return size() == 0; }

  /// Positional access to citizens.
  ///
  /// They are in no particular order, but the order remains "stable" during
  /// resampling.
  const Citizen& operator[](size_t position) const
  {
    assert(position < citizens_.size());
    return *citizens_[position];
  }
  Citizen& operator[](size_t position)
  {
    assert(position < citizens_.size());
    return *citizens_[position];
  }

  /// Resample the population such that `count_` copies of each citizen in
  /// the current population are present in the resampled one.
  void resample()
  {
    families_.clear();  // force redoing the census.
    std::vector<std::unique_ptr<Citizen>> resampled;
#ifdef _DEBUG
    size_t vanished = 0;
#endif
    // First let us release all the citizens whose count() is 0
    // this can effective avoid the situation we have more than
    // target population citizen, since some citizen will be
    // repopulated more than 1.
    for (size_t i = 0; i < citizens_.size(); i++)
    {
      if (citizens_[i]->get_count() == 0)
      {
        remove_citizen(citizens_[i]);
#ifdef _DEBUG
        vanished++;
        assert(vanished < citizens_.size());
#endif
      }
    }
    for (size_t i = 0; i < citizens_.size(); i++)
    {
      if (citizens_[i].get() == nullptr)
      {
        // the citizen has been released since the count is 0
        continue;
      }
      const auto& citizen = *citizens_[i];
      // for each count larger than 1, create copies
      for (size_t count = 1; count < citizen.get_count(); count++)
      {
        std::unique_ptr<Citizen> new_citizen;
        create_citizen(new_citizen, citizen.content());
        new_citizen->set_family(citizen.get_family());
        resampled.push_back(std::move(new_citizen));
      }
      // if the count is non-zero, move the citizen
      // (i.e., move the first one, don't copy).
      if (citizen.get_count() > 0)
      {
        citizens_[i]->set_count(1);
        resampled.push_back(std::move(citizens_[i]));
      }
    }
    std::swap(citizens_, resampled);
  }

  /// Returns a map describing the sizes of the remaining families.
  const std::map<FamilyId, size_t>& get_families()
  {
    if (families_.empty()) do_census();
    return families_;
  }

  void partial_sort(unsigned solutions_to_sort)
  {
    std::partial_sort(citizens_.begin(), citizens_.begin() + solutions_to_sort,
                      citizens_.end(), Citizen::compare);
  }

  void clear()
  {
    families_.clear();
    for (auto& citizen : citizens_)
    {
      remove_citizen(citizen);
    }
    citizens_.clear();
    assert(citizens_.size() == 0);
  }

  void clear_cache_pool()
  {
    std::vector<std::unique_ptr<Citizen>> empty;
    std::swap(citizens_cache_pool_, empty);
  }

 private:
  void do_census()
  {
    for (const auto& citizen : citizens_)
    {
      {
        FamilyId family = citizen->get_family();
        auto it = families_.find(family);
        if (it == families_.end())
        {
          families_[family] = 1;
        }
        else
        {
          it->second++;
        }
      }
    }
  }

  // Take a Citizen from the pool (or allocate a new one) and place
  // it into `citizen` with `content` as its content.
  void create_citizen(std::unique_ptr<Citizen>& citizen,
                      const Content_T& content)
  {
    if (citizen != nullptr)
    {
      LOG(WARN, "Attempting to create citizen in non-empty pointer");
      return;
    }
    if (citizens_cache_pool_.empty())
    {
      citizen.reset(new Citizen(content));
    }
    else
    {
      std::swap(citizen, citizens_cache_pool_.back());
      citizens_cache_pool_.pop_back();
      citizen->content() = content;
    }
    citizen->set_count(1);
  }

  // Remove a citizen from the (active) population by moving the
  // pointer to the inactive citizen pool for later recycling.
  void remove_citizen(std::unique_ptr<Citizen>& citizen)
  {
    citizens_cache_pool_.push_back(nullptr);
    std::swap(citizen, citizens_cache_pool_.back());
  }

  std::vector<std::unique_ptr<Citizen>> citizens_;
  std::vector<std::unique_ptr<Citizen>> citizens_cache_pool_;
  std::map<FamilyId, size_t> families_;
};

}  // namespace solver
