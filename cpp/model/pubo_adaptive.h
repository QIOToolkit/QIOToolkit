
#pragma once

#include <cstring>
#include <functional>
#include <sstream>

#include "utils/operating_system.h"
#include "utils/timing.h"
#include "markov/model.h"
#include "model/terms.h"

namespace model
{
////////////////////////////////////////////////////////////////////////////////
/// Adaptive Binary State
///
/// This adaptive state stores its state in a contigous (self allocated) chunk
/// of heap memory (pointed to by data_). This chunk is organized as follows:
///
/// +--------------------------------------------------------------------------+
/// | Binary values stored in uint64_t   | 32-bit counters | 8-bit counters    |
/// +------------------------------------+--------+--------+---+---+---+---+---+
/// | 0101010100001010011111011110101101 |    256 |     42 | 3 | 0 | 7 | 2 | 1 |
/// +------------------------------------+--------+--------+---+---+---+---+---+
///
///   * The first section consists of as many uint64_t as needed to store the
///     state of each variable in the problem
///   * Counters are used to cache the number of zeros in a given term. Their
///     usage is allocated problem-dependent to the highest-locality terms and
///     according to memory allowance passed during configuration.
///
/// NOTE: In addition to limiting per-state memory, PuboAdaptive can be
/// templated for the index type (i.e. uint16_t). Because chache ids are
/// appended to the variable ids; they must fit within the numeric range of the
/// type used for indexing as well (see below).
class PuboBinaryAdaptive : public markov::State
{
 public:
  /// Create an empty adaptive state (used by std::vector)
  PuboBinaryAdaptive() : data_(nullptr), size_(0) {}

  /// Create an adaptive state of the desired size and initialize it to zero.
  PuboBinaryAdaptive(size_t size) : size_(std::max(size, size_t(1)))
  {
    if (size_ > std::numeric_limits<size_t>::max() / sizeof(uint64_t))
    {
      LOG(FATAL, "Trying to allocate state of ", size_,
          " uint64_t's causes overflow");
      throw utils::MemoryLimitedException("size overflow in state allocation");
    }
    data_.reset(new uint64_t[size_]);
#ifdef __STDC_LIB_EXT1__
    errno_t errno = memset_s(data_.get(), size_ * sizeof(uint64_t), 0,
                             size_ * sizeof(uint64_t));
    if (errno)
    {
      THROW(utils::MemoryLimitedException, "memset_s error ", errno);
    }
#else
    memset(data_.get(), 0, size_ * sizeof(uint64_t));
#endif
  }

  /// Copy another state (allocating memory if it is empty).
  PuboBinaryAdaptive& operator=(const PuboBinaryAdaptive& other)
  {
    if (other.size_ == 0)
    {
      data_.reset();
      size_ = 0;
      return *this;
    }
    if (size_ != other.size_)
    {
      if (size_ > 0)
      {
        // We don't expect this to happen; a model should always
        // create states of equal size and allocating memory
        // should only be necessary when assigning to an empty
        // state.
        LOG(ERROR, "Assigning states of non-equivalent size.");
      }
      size_ = other.size_;
      if (size_ > std::numeric_limits<size_t>::max() / sizeof(uint64_t))
      {
        LOG(FATAL, "Trying to allocate state of ", size_,
            " uint64_t's causes overflow");
        throw utils::MemoryLimitedException("size overflow in state allocation");
      }
      data_.reset(new uint64_t[size_]);
    }
#ifdef __STDC_LIB_EXT1__
    errno_t errno =
        memcpy_s(data_.get(), size_ * sizeof(uint64_t) other.data_.get(),
                 size_ * sizeof(uint64_t));
    if (errno)
    {
      THROW(utils::MemoryLimitedException("memcpy_s error ", errno);
    }
#else
    memcpy(data_.get(), other.data_.get(), size_ * sizeof(uint64_t));
#endif
    return *this;
  }

  /// Move assignment can be done without copy.
  PuboBinaryAdaptive& operator=(PuboBinaryAdaptive&& other)
  {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
    return *this;
  }

  PuboBinaryAdaptive(const PuboBinaryAdaptive& other) : data_(nullptr), size_(0)
  {
    *this = other;
  }
  PuboBinaryAdaptive(PuboBinaryAdaptive&& other) : data_(nullptr), size_(0)
  {
    *this = std::move(other);
  }

  /// We need to copy the whole state (including caches); the model cannot
  /// compute the cost function without them. This functionality is exclusively
  /// used to keep track of the best cost milestones -- I expect this to happen
  /// rarely enough that we can afford the overhead.
  void copy_state_only(const PuboBinaryAdaptive& other) { *this = other; }

  static size_t memory_estimate(size_t size)
  {
    return sizeof(PuboBinaryAdaptive) + sizeof(uint64_t) * size;
  }

  static size_t state_only_memory_estimate(size_t size)
  {
    return memory_estimate(size);
  }

  std::unique_ptr<uint64_t[]> data_;
  size_t size_;
};

////////////////////////////////////////////////////////////////////////////////
/// Adaptive Pubo Model
///
/// This pubo implementation aims for adaptability to per-state memory
/// constraints while using a cache-friendly graph representation.
///
/// 1) The numeric type used for indexing can be templated. This type needs to
///    be chosen such that #variables < std::numeric_limits<T>::max() - 2.
///    (the last two values of the range are used as sentinels in the graph
///    representation).
///
/// 2) If the graph has high-locality terms, there should (ideally) be some
///    unused range between #variables and that upper limit; this range is
///    used to index into the cache. The number of terms for which caching
///    can be used is limited by the width of that empty range.
///
/// 3) The graph representation uses a contigous chunk of memory initialized
///    with an adjacency list for each node. For each term, the first 8 bytes
///    contain the coefficient, followed by a sentinel terminated list of
///    variable_ids (or cache_ids) participating in the term. The sentinel type
///    (NEXT_TERM or NEXT_VAR) denotes when the last term of a variable is
///    reached.
///
/// 4) During configuration of the graph representaion, the maximum number
///    of bytes for each state can be specified. If this number exceeds the
///    required bytes to store the boolean variables, the extra bytes are
///    used to preferentially cache the number of zeros in high-locality
///    terms (using either 32-bit or 8-bit counters). This has the effect
///    of both reducing the graph size and time to compute a term.
///
template <typename Index>
class PuboAdaptive : public markov::LinearSweepModel<PuboBinaryAdaptive>
{
 public:
  using State_T = PuboBinaryAdaptive;
  using Base_T = markov::LinearSweepModel<State_T>;
  // Interface for external model configuration functions.
  using Configuration_T = Terms::TermsConfiguration;

  /// Create an unconfigured PuboAdaptive model.
  PuboAdaptive()
      : constant_cost_(0),
        is_empty_(false),
        var_count_(0),
        state_size_(0),
        cache32_count_(0),
        cache_count_(0),
        cache_offset_(0)
  {
  }

  /// Return the identifier of this model.
  std::string get_identifier() const override { return "pubo"; }

  /// Return the version of this model.
  std::string get_version() const override { return "1.1"; }

  /// Accept both version 1.0 and 1.1.
  void match_version(const std::string& version) override
  {
    if (version.compare("1.0") == 0 || version.compare(get_version()) == 0)
    {
      return;
    }
    THROW(utils::ValueException,
          "Expecting `version` equals 1.0 or 1.1, but found: ", version);
  }

  /// Configure the model from input
  void configure(const utils::Json& json) override
  {
    Base_T::configure(json);
    // PuboAdaptive doesn't need to configure anything other than terms.
    if (!json.IsObject() || !json.HasMember(utils::kCostFunction))
    {
      THROW(utils::MissingInputException,
            "The configuration of a pubo model must contain a `cost_function` "
            "entry.");
    }
    Terms terms;
    {
      utils::ScopedTimer terms_timer("Terms");
      set_from_json(terms, json[utils::kCostFunction]);
    }
    utils::ScopedTimer graph_timer("Graph");
    configure(std::move(terms), std::numeric_limits<size_t>::max());
  }

  void configure(Configuration_T& config)
  {
    Base_T::configure(config);
    Terms terms;
    terms.configure(config);
    configure(std::move(terms), std::numeric_limits<size_t>::max());
  }

  double get_const_cost() const override { return constant_cost_; }

  bool is_empty() const override { return is_empty_; }

  void configure(model::Terms&& terms, size_t max_state_size_in_bytes)
  {
    // Remember the constant cost (needed for whole cost function eval)
    constant_cost_ = terms.stats.constant_cost;
    is_empty_ = (terms.stats.terms == terms.stats.constant_terms);
    // Determine how to size the states (i.e., how many caching counters
    // to use).
    configure_state_size(terms, max_state_size_in_bytes);
    // Build the adjanceny list representation.
    configure_graph(terms);
    // Extract the original variable names for rendering the solution
    // (terms will be destructed).
    names_.reserve(var_count_);
    for (size_t i = 0; i < var_count_; i++)
    {
      names_.push_back(terms.variables[i].name);
    }
  }

  /// Create a random initial state.
  State_T get_random_state(utils::RandomGenerator& rng) const override
  {
    PuboBinaryAdaptive state(state_size_);
    uint32_t bits = 0;
    for (size_t i = 0; i < var_count_; i++)
    {
      if (i % 32 == 0) bits = rng.uint32();
      if (bits & (1UL << (i & 31))) apply_transition(i, state);
    }
    return state;
  }

  /// Get memory estimation for state
  size_t state_memory_estimate() const override
  {
    return State_T::memory_estimate(state_size_);
  }

  size_t state_only_memory_estimate() const override
  {
    return State_T::state_only_memory_estimate(state_size_);
  }

  /// Create a random transition.
  Transition_T get_random_transition(const State_T&,
                                     utils::RandomGenerator& rng) const override
  {
    return Transition_T(rng.uniform() * double(var_count_));
  }

  // Shared setup for acessing the state data.
#define SETUP_POINTERS(state, const)                               \
  const uint64_t* spins = state.data_.get();                       \
  const uint32_t* cache32 =                                        \
      reinterpret_cast<const uint32_t*>(spins + cache_offset_);    \
  const uint8_t* cache8 =                                          \
      reinterpret_cast<const uint8_t*>(cache32 + cache32_count_) - \
      cache32_count_;

  /// Applying a random transition is done by advancing the graph pointer
  /// to the offset corresponding to the variable being flipped.
  void apply_transition(const Transition_T& transition,
                        State_T& state) const override
  {
    SETUP_POINTERS(state, );
    uint8_t* p = graph_.get() + offsets_[transition];
    bool spin_value = get_spin_value(spins, transition);
    flip_spin(spins, transition);
    update_caches(p, spin_value, cache32, cache8);
  }

  /// Evaluate the whole cost function
  ///
  /// NOTE: The adjacency list representation is inefficient for this purpose,
  /// but we don't expect to evaluate this frequently.
  ///
  /// 1) For non-cached terms, double counting is avoided by ensuring
  ///    neighbor ids are larger than the variable we are processing
  ///
  /// 2) For cached terms we use a std::vector<bool> to keep track which ones
  ///    have been counted.
  double calculate_cost(const State_T& state) const override
  {
    SETUP_POINTERS(state, const)
    std::vector<bool> computed(cache_count_, false);
    double cost =
        0;  // we do add const cost here since const cost shall not affect the
            // search process, but add the chance of floating point overflow.
    uint8_t* p = graph_.get();
    for (Index variable_id = 0; variable_id < var_count_; variable_id++)
    {
      bool spin = get_spin_value(spins, variable_id);
      if (spin)
      {
        // This spin is zero, none of the terms will be active.
        if (variable_id == var_count_ - 1) break;
        p = graph_.get() + offsets_[variable_id + 1];
        continue;
      }
      while (true)  // over terms
      {
        double coefficient = take_double(p);
        Index index = take_index(p);
        if (index >= NEXT_TERM)
        {
          // The first index value after the coefficient is already
          // a sentinel. This means there are no other variables in
          // this term. It's a "local field" whose activation always
          // changes with this variable.
          if (!spin) cost += coefficient;
        }
        else if (index < var_count_)  // a non cached term
        {
          // The following check avoids double counting of terms with multiple
          // variables. The graph representation lists all the OTHER variables
          // (besides variable_id) participating in this term. We count the
          // term only if variable_id is the lower than all these other
          // variables listed.
          //
          // We require that this list of other variables is sorted, such that
          // we only need to test variable_id against the first one listed
          // (i.e., `index`, the one we just read).
          if (variable_id < index)
          {
            // We want to know if any variable in the term is zero.  Since the
            // boolean representation used 'true' for zero, we therefore OR all
            // the values together.
            // NOTE: This is different from calculate_cost_diff where we need
            // to know if any OTHER variable (besides variable_id) is zero
            // to know whether the cost CHANGES with variable_id. This is why
            // we initialize any_zero with spin here.
            bool any_zero = spin;
            while (index < NEXT_TERM && !any_zero)
            {
              any_zero = get_spin_value(spins, index);
              index = take_index(p);
            }
            if (!any_zero) cost += coefficient;
          }
        }
        else
        {
          // ids larger than var_count_ are used for indexing into the cache.
          size_t cache_id = static_cast<size_t>(index) - var_count_;
          assert(cache_id < cache_count_);
          if (!computed[cache_id])
          {
            computed[cache_id] = true;
            if (get_cache(cache_id, cache32, cache8) == 0) cost += coefficient;
          }
        }
        while (index < NEXT_TERM) index = take_index(p);
        if (index == NEXT_VAR) break;
      }
    }
    return cost;
  }

  /// The cost difference of an arbitrary transition is evaluated by setting
  /// up the state pointers and advancing the graph pointer to the offset
  /// corresponding to the variable being flipped.
  double calculate_cost_difference(
      const State_T& state, const Transition_T& transition) const override
  {
    SETUP_POINTERS(state, const)
    uint8_t* p = graph_.get() + offsets_[transition];
    bool spin_value = get_spin_value(spins, transition);
    return calculate_term_difference(p, spin_value, spins, cache32, cache8);
  }

  /// For a linear sweep, we can avoid the overhead of repeatedly setting up
  /// state pointers and jumping to specific offsets in the graph; it can
  /// simply be processed in order.
  void make_linear_sweep(double& cost, State_T& state,
                         std::function<bool(double)> accept,
                         std::function<void(void)> check_lowest) const override
  {
    SETUP_POINTERS(state, )
    uint8_t* p = graph_.get();
    for (Index variable_id = 0; variable_id < var_count_; variable_id++)
    {
      // Store where this variable started (we will need to go back to this
      // point in case we need to update caches).
      uint8_t* rewind = p;
      bool spin_value = get_spin_value(spins, variable_id);
      // NOTE: The following call also advances p. After the call, it points
      // to the coefficient of the first term of the next variable.
      double diff =
          calculate_term_difference(p, spin_value, spins, cache32, cache8);

      if (accept(diff))
      {
        cost += diff;
        flip_spin(spins, variable_id);
        update_caches(rewind, spin_value, cache32, cache8);
        check_lowest();
      }
    }
  }

// don't need this any more.
#undef SETUP_POINTERS

  /// Render the state as a structure mapping (original) names to
  /// pubo values. NOTE: the mapping is true=zero, false=1
  utils::Structure render_state(const State_T& state) const override
  {
    utils::Structure s;
    for (size_t variable_id = 0; variable_id < names_.size(); variable_id++)
    {
      s[std::to_string(names_[variable_id])] =
          get_spin_value(state.data_.get(), variable_id) ? 0 : 1;
    }
    return s;
  }

  /// One sweep consists of attempting to update each variable once.
  size_t get_sweep_size() const override { return var_count_; }

 private:
  /// Flip a spin by applying xor with a shifted bit.
  inline void flip_spin(uint64_t* spins, Index index) const
  {
    assert(index < var_count_);
    spins[size_t(index) >> 6] ^= (1ULL << (index & 63));
  }

  /// Extract a bit by masking with a shifted bit.
  inline bool get_spin_value(const uint64_t* spins, Index index) const
  {
    assert(index < var_count_);
    return spins[size_t(index) >> 6] & (1ULL << (index & 63));
  }

  /// Select the appropriate (type and index) of the cache for a
  /// given cache_id.
  /// NOTE: the cache_id=index-var_count_ is in [0..cache_count_).
  inline uint32_t get_cache(size_t cache_id, const uint32_t* cache32,
                            const uint8_t* cache8) const
  {
    assert(cache_id < cache_count_);
    if (cache_id < cache32_count_)
    {
      return cache32[cache_id];
    }
    else
    {
      return static_cast<uint32_t>(cache8[cache_id]);
    }
  }

  /// Add the cache_change to the appropriate (type and index) of the cache.
  inline void update_cache(size_t cache_id, uint32_t* cache32, uint8_t* cache8,
                           uint8_t cache_change) const
  {
    assert(cache_id < cache_count_);
    if (cache_id < cache32_count_)
    {
      cache32[cache_id] += static_cast<uint32_t>(cache_change);
    }
    else
    {
      cache8[cache_id] += cache_change;
    }
  }

  /// Interpret the next sizeof(Index) bytes as an Index and advance the
  /// pointer.
  static inline Index take_index(uint8_t*& p)
  {
    Index index = *reinterpret_cast<Index*>(p);
    p += sizeof(Index);
    return index;
  }

  /// Interpret the next 8 bytes as a double and advance the pointer.
  static inline double take_double(uint8_t*& p)
  {
    double d = *reinterpret_cast<double*>(p);
    p += sizeof(double);
    return d;
  }

  static inline void put_double(uint8_t*& p, double d)
  {
    *reinterpret_cast<double*>(p) = d;
    p += sizeof(double);
  }

  static inline void put_index(uint8_t*& p, Index index)
  {
    *reinterpret_cast<Index*>(p) = index;
    p += sizeof(Index);
  }

  /// Calculate how many terms we want to and can afford to cache.
  /// After a call to this method, state_size_ and the cache counters/offsets
  /// have been initialized to the correct values.
  void configure_state_size(const model::Terms& terms,
                            size_t max_state_size_in_bytes)
  {
    // We use only multiples of 64bit.
    size_t remaining_64bit = max_state_size_in_bytes / 8;
    var_count_ = terms.variables.size();
    if (var_count_ > NEXT_TERM)
    {
      throw utils::MemoryLimitedException(
          "cannot index variables with Index type");
    }

    // Allocate storage for the binary variables
    // (how many uint64_t to store one boolean for each pubo variable)
    state_size_ = (var_count_ + 63) / 64;
    if (state_size_ > remaining_64bit)
    {
      LOG(WARN, "cannot fit variables in requested state size ",
          utils::print_bytes(max_state_size_in_bytes), ", using ",
          utils::print_bytes(state_size_ * sizeof(uint64_t)), " instead");
      remaining_64bit = 0;
    }
    else
    {
      remaining_64bit -= state_size_;
    }

    // If space remains, preferentially cache higher order terms
    // (in the remaining bytes, place 32bit counters to track the highest
    // locality terms)
    cache_offset_ = state_size_;  // Where caching counters start in each state
    cache32_count_ = terms.stats.terms32;  // How many 32bit counters we need
    size_t fit_in_state = remaining_64bit * 2;
    size_t fit_in_index = NEXT_TERM - var_count_;  // How many we can index
    if (cache32_count_ > std::min(fit_in_state, fit_in_index))
    {
      // We would need more 32bit counters than fit in the desired state size.
      // (=> Track as many as we can)
      cache_count_ = cache32_count_ = std::min(fit_in_state, fit_in_index);
      // Allocate state memory exhausted, don't track any lower locality terms.
      // Add the chaching variables to the state size.
      // NOTE: This could be odd if we hit the fit_in_index limit above, or if
      // there is an odd number of terms with >255 variables.
      state_size_ += (cache32_count_ + 1) / 2;  // two uint32_t to a uint64_t
    }
    else
    {
      // Then fill with lower order terms
      size_t cache8_count =
          terms.stats.terms8;  // How many 8bit counters we need
      fit_in_state = remaining_64bit * 8 - cache32_count_ * 4;
      fit_in_index = NEXT_TERM - var_count_ - cache32_count_;
      if (cache8_count > std::min(fit_in_state, fit_in_index))
      {
        cache8_count = std::min(fit_in_state, fit_in_index);
      }
      state_size_ += (cache32_count_ * 4 + cache8_count + 7) / 8;
      cache_count_ = cache32_count_ + cache8_count;
    }

    LOG(INFO, "Using ", utils::print_bytes(state_size_ * 8),
        " per state, caching ", cache_count_, " term",
        cache_count_ == 1 ? "" : "s");
  }

  /// Allocate and fill the adjacency list graph representation.
  void configure_graph(const model::Terms& terms)
  {
    size_t offset = 0;
    // We are storing the offset for each variable into the graph
    // representation (i.e., where the term-list for variable_id starts).
    // This is NOT needed when doing a linear sweep, but allows performing
    // individual updates without traversing the whole list.
    offsets_.resize(terms.variables.size());

    // This first loop simulates the generation to calculate the required
    // size for the memory.
    for (Index variable_id = 0; variable_id < var_count_; variable_id++)
    {
      offsets_[(size_t)variable_id] = offset;
      const auto& variable = terms.variables[(size_t)variable_id];
      for (size_t term_id : variable.term_ids)
      {
        const auto& term = terms.terms[term_id];
        offset += sizeof(double);
        if (term_id < cache_count_)
        {
          // This is one of the cache terms
          // (the second one is the NEXT_TERM or NEXT_VAR)
          offset += 2 * sizeof(Index);
        }
        else
        {
          // This one is not cached, spell out the other variables
          // in this term (not including the variable itself).
          // However, the list is terminated by one of the sentinels
          // (either NEXT_TERM or NEXT_VAR) so the length is still
          // term.variable_ids.size().
          offset += term.variable_ids.size() * sizeof(Index);
        }
      }
    }

    LOG(INFO, "Using a graph representation of ", utils::print_bytes(offset),
        ".");
    graph_.reset(new uint8_t[offset]);
    uint8_t* p = graph_.get();
    for (Index variable_id = 0; variable_id < var_count_; variable_id++)
    {
      assert(offsets_[(size_t)variable_id] ==
             static_cast<size_t>(p - graph_.get()));
      const auto& variable = terms.variables[(size_t)variable_id];
      for (size_t term_id : variable.term_ids)
      {
        const auto& term = terms.terms[term_id];
        put_double(p, term.cost);
        if (term_id < cache_count_)  // We are caching this term.
        {
          Index cache_id = static_cast<Index>(var_count_ + term_id);
          put_index(p, cache_id);
          // We can't really skip this sentinel because this might be the
          // last term and then the NEXT_VAR sentinel is required.
          put_index(p, NEXT_TERM);
        }
        else
        {
          for (auto i : term.variable_ids)
          {
            if (static_cast<Index>(i) == variable_id) continue;
            put_index(p, static_cast<Index>(i));
          }
          put_index(p, NEXT_TERM);
        }
      }
      // Change the last sentinel to a NEXT_VAR.
      *(reinterpret_cast<Index*>(p) - 1) = NEXT_VAR;
    }
  }

  inline double calculate_term_difference(uint8_t*& p, bool spin_value,
                                          const uint64_t* spins,
                                          const uint32_t* cache32,
                                          const uint8_t* cache8) const
  {
    // To calculate the difference, we need to identify which of the terms
    // this variable participates in are 'critical'. By critical, we mean
    // that all other variables are 1 and, therefore, activation hinges on
    // whether this variable's value changes.
    double diff = 0;
    uint32_t critical = spin_value ? 1 : 0;
    while (true)  // over terms
    {
      // NOTE: We will be adding up the coefficients of each critical term
      // regardless of the spin_value. The sign is applied only in the return
      // statement at the bottom of this function. This works because the
      // spin_value has the same sign effect on all terms (i.e., changing from
      // 0->1 will activate all critical terms, 1->0 will deactivate them all).
      double coefficient = take_double(p);
      Index index = take_index(p);
      if (index >= NEXT_TERM)
      {
        // This term does not contain any other variables, its activation
        // always changes with variable_id.
        diff += coefficient;
      }
      else if (index < var_count_)  // a non cached term
      {
        // A non-cached term: We need to know if any of the OTHER variables
        // participating in this term are zero (in which case, changing
        // variable_id has no effect on the cost).
        bool any_zero = false;
        while (index < NEXT_TERM && !any_zero)
        {
          // the boolean representation for pubo_variable=zero
          // is 'true', so taking the logical OR of all the
          // spins participating in the term tells us whether
          // there is a zero.
          any_zero = get_spin_value(spins, index);
          index = take_index(p);
        }
        if (!any_zero)
        {
          // None of the OTHER variables participating in this
          // term are zero. This means the term is 'critical', i.e.,
          // its activation changes with the variable being changed.
          diff += coefficient;
        }
      }
      else  // a cached term
      {
        size_t cache_id = index - var_count_;
        if (get_cache(cache_id, cache32, cache8) == critical)
          diff += coefficient;
      }
      while (index < NEXT_TERM) index = take_index(p);
      if (index == NEXT_VAR) break;
    }
    return spin_value ? diff : -diff;
  }

  inline void update_caches(uint8_t*& p, bool spin, uint32_t* cache32,
                            uint8_t* cache8) const
  {
    if (cache_count_ == 0) return;  // nothign to update
    uint8_t cache_change = spin ? -1 : 1;
    while (true)  // over terms
    {
      p += sizeof(double);
      Index index = take_index(p);
      if (index >= var_count_ && index < NEXT_TERM)
      {
        size_t cache_id = static_cast<size_t>(index) - var_count_;
        update_cache(cache_id, cache32, cache8, cache_change);
      }
      while (index < NEXT_TERM) index = take_index(p);
      if (index == NEXT_VAR) break;
    }
  }

  /// Sentinel value denoting the end of the last term of a variable.
  const static Index NEXT_VAR = std::numeric_limits<Index>::max();
  /// Sentinel value denoting the end of a term.
  const static Index NEXT_TERM = NEXT_VAR - 1;

  double constant_cost_;
  bool is_empty_;
  size_t var_count_, state_size_;
  size_t cache32_count_, cache_count_;
  size_t cache_offset_;
  std::unique_ptr<uint8_t[]> graph_;
  std::vector<int> names_;
  std::vector<size_t> offsets_;
};  // namespace model

}  // namespace model
