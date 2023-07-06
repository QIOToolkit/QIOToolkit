
#pragma once

#include <cmath>
#include <memory>
#include <queue>

#include "utils/component.h"

namespace observe
{
/// Observable Interface
///
/// An observable can be reset and allows recoding of values with individual
/// weights (the default weight being 1.0). Depending on the type of the
/// observable, it will keep track of the last recorded value, the mean since
/// the last reset or a full histogram of values (see implementations).
///
class Observable : public utils::Component
{
 public:
  virtual ~Observable() {}

  virtual void reset() = 0;
  void record(double value);
  void record(int value);
  void record(size_t value);
  virtual void record(double value, double weight) = 0;
};

/// Constant observable
///
/// Keeps track of the last value (the assumption is that this observable is
/// used to indicate a constant associated with other observables, hence the
/// name). It does not keep track of previously recorded values.
class Constant : public Observable
{
 public:
  Constant() { reset(); }
  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 private:
  double value_;
};

/// Average observable
///
/// Keeps track of the sum of values and total weight in order to produce the
/// weighted average of recorded data points since the last reset.
class Average : public Observable
{
 public:
  Average() { reset(); }
  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  double weight_;
  double sum_;
};

class AverageAndCount : public Average
{
 public:
  utils::Structure render() const override;
};

/// MinMax observable
///
/// In addition to the average value, keeps track of the smallest and largest
/// value recorded.
class MinMax : public AverageAndCount
{
 public:
  MinMax() { reset(); }
  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  double min_;
  double max_;
};

/// Average2 observable
///
/// Additionally keeps track of the sum of squares to produce the
/// standard deviation of recorded data points.
class Average2 : public MinMax
{
 public:
  Average2() { reset(); }
  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  double sum2_;
};

/// RangedDistribution observable
///
/// In addition to the mean and standard deviation, keeps track of binned
/// counts of the values within `low` and `high` (divided into `bins` bins).
/// Values outside the specified range affect the mean and stddev, but are
/// not recorded in the histogram. This observable is useful if you're only
/// interested in the counts for a specific (known) value range.
class RangedDistribution : public Average2
{
 public:
  RangedDistribution(double low, double high, int bins)
      : low_(low), high_(high), bins_((size_t)bins)
  {
    reset();
  }
  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  double low_, high_;
  std::vector<double> bins_;
};

/// BinWidthDistribution observable
///
/// In addition to the mean and standard deviation, keeps track of the full
/// histogram of recorded values in bins of the specified `bin_width`. This
/// histogram can become quite large if you specify a small bin width in
/// relation to the range of recorded values.
///
/// NOTE: There is a bin centered around 0 and bins are directly adjacent
/// to each other.
class BinWidthDistribution : public Average2
{
 public:
  BinWidthDistribution(double bin_width) : bin_width_(bin_width) { reset(); }
  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  double bin_width_;
  std::map<int, double> bins_;
};

/// Windowed observable
///
/// This creates a "windowed" version of any observable such that the current
/// state of the contained observable always reflects the last `window_size`
/// values recorded. This is implemented by keeping track of the recorded
/// values in the queue and removing them when the `window_size` is exceeded.
///
/// NOTE: The `window_size` is implemented as a weight; if all your recorded
/// values have a weight of 1, this translates to `window_size` observables
/// being tracked; if they differ it can be more or less, accordingly.
class Windowed : public Observable
{
 public:
  Windowed(std::unique_ptr<Observable>&& observable, double window_size)
      : observable_(std::move(observable)), window_size_(window_size)
  {
    reset();
  }
  Windowed(const Windowed&) = delete;
  Windowed& operator=(const Windowed&) = delete;

  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  class ValueAndWeight : public utils::Component
  {
   public:
    ValueAndWeight() : value(0), weight(0) {}
    ValueAndWeight(double value_, double weight_)
        : value(value_), weight(weight_)
    {
    }
    utils::Structure render() const override
    {
      utils::Structure rendered;
      rendered["value"] = value;
      rendered["weight"] = weight;
      return rendered;
    }
    double value;
    double weight;
  };

  std::unique_ptr<Observable> observable_;
  double weight_;
  double window_size_;
  std::deque<ValueAndWeight> queue_;
  std::multiset<double> sorted_;
};

/// Batched observable
///
/// This creates a "batched" version of any observable which records
/// a timeline with batches of size `batch_size`. I.e., after a total
/// weight of `batch_size` has been recorded, the observable is rendered
/// to the timeline of batches and the observable itself is reset.
///
/// Example: Batched(BinWidthDistribution(0.2), 1000) records the
/// evolution of a histogram:
///
///   - 1000 values are recorded into each histogram
///   - the histogram is grouped into bins of width 0.2
///   - after each 1000, the histogram is stored and a new one started.
///
class Batched : public Observable
{
 public:
  Batched(std::unique_ptr<Observable>&& observable, double batch_size)
      : observable_(std::move(observable)), batch_size_(batch_size)
  {
    reset();
  }
  Batched(Observable* observable, double batch_size)
      : Batched(std::unique_ptr<Observable>(observable), batch_size)
  {
  }
  Batched(const Batched&) = delete;
  Batched& operator=(const Batched&) = delete;

  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  std::unique_ptr<Observable> observable_;
  double weight_;
  double batch_size_;
  utils::Structure batches_;
};

/// LograithmicBatched
///
/// This creates a "logarithmically batched" version of any observable which
/// records a timeline with batches doubling in size over time. I.e., after
/// a total weight of 32 has been recorded, the observable is rendered
/// to the timeline of batches and the observable itself is reset with a
/// new batch size of 64.
///
/// Example: LogBatched(BinWidthDistribution(0.2), 16) records the
/// evolution of a histogram:
///
///   - 16 values are recorded into the first batch
///   - 32 values will be recorded into the next batch
///   - the histogram is grouped into bins of width 0.2
///
class LogBatched : public Observable
{
 public:
  LogBatched(std::unique_ptr<Observable>&& observable,
             double initial_batch_size = 1)
      : observable_(std::move(observable)),
        current_batch_size_(initial_batch_size)
  {
    reset();
  }
  LogBatched(const LogBatched&) = delete;
  LogBatched& operator=(const LogBatched&) = delete;

  void reset() override;
  using Observable::record;
  void record(double value, double weight) override;
  utils::Structure render() const override;

 protected:
  std::unique_ptr<Observable> observable_;
  double weight_;
  double current_batch_size_;
  utils::Structure batches_;
};

/// Record when new values are reached
///
/// This records a timeline of when a new value is recorded. If the recorded
/// values are monotonically decreasing (or increasing), this translates to
/// a timeline of when a new lowest (or highest) value is found.
class Milestone : public Observable
{
 public:
  Milestone(const std::string& key_label = "key",
            const std::string& value_label = "value")
      : key_label_(key_label), value_label_(value_label)
  {
    reset();
  }

  void reset() override;
  using Observable::record;
  void record(double value_label, double weight) override;
  utils::Structure render() const override;

 protected:
  std::string key_label_;
  std::string value_label_;
  double key_total_;
  std::vector<double> keys_;
  std::vector<double> values_;
};

}  // namespace observe
