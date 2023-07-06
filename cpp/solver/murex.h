
#pragma once

#include <set>

#include "utils/operating_system.h"
#include "utils/random_generator.h"
#include "markov/metropolis.h"
#include "model/poly.h"
#include "schedule/schedule.h"
#include "solver/stepping_solver.h"

namespace solver
{
////////////////////////////////////////////////////////////////////////////////
/// Multi-Objective Replica Exchange -- "murex"
///
/// This solver implements a replica exchange approach whereby, in addition to
/// the variables ("spins"), the parameters are at different quenched values
/// at every node. As a result, the method explores the lowest cost value that
/// can be achieved at different "mixing rates" of the parameters.
///
/// The (currently hard-coded) arrangement of the nodes is in a triangle such
/// that the temperature of nodes in each diagonal agrees, while the parameters
/// x and y are swept from (1,0) to (0,1) in equal steps.
///
///  position   weights
///  5    j     ( 0, 1)
///  2 4   ↖    ( 0, 1)  (.5,.5)
///  0 1 3      (.5,.5)  ( 1, 0)  ( 1, 0)
///   → i
///
/// This setup is intended to analyze the effect of a multi-objective model
/// with different weights assigned to each objective:
///
/// \f[ \mathcal{H} = -x\mathcal{H}_x(\vec{s}) -y\mathcal{H}_y(\vec{s}) \f]
///
/// Replica exchanges happen between neighboring nodes (vertically and
/// horizontally) and take place at a rate which take into consideration
/// that the cost of the swapped states are modified by virtue of being
/// placed at a new node (and, thus, new set of parameters).
///
/// NOTE: The solver requires the model being simulated to support
/// parameters and have exactly two parameters (which are selected as
/// `x` and `y`). They do not necessarily need to split the cost function
/// as described. [At the time of this writing, only "poly" supports this].
///
template <class Model_T>
class Murex : public SteppingSolver<Model_T>
{
 public:
  using Base_T = SteppingSolver<Model_T>;
  using State_T = typename Model_T::State_T;

  Murex() : dimensions_(0), nodes_size_(0) {}
  Murex(const Murex&) = delete;
  Murex& operator=(const Murex&) = delete;

  using Weights = std::vector<double>;

  std::string get_identifier() const override
  {
    return "murex.qiotoolkit";
  }

  /// Representation of a node in the replica exchange grid.
  class Node
  {
   public:
    // Whether this node should perform replica exchanges in the
    // even or odd numbered steps.
    bool odd;
    // Coordinates of this node (i=diagonal, j=position in diagonal)
    size_t i, j;
    // Weights assigned to these coordinates
    Weights weights;
    // Metropolis walker at this node (contains a state)
    markov::Metropolis<Model_T> replica;
    // Indices of the children to the right and up from this node
    // (-1 if this node doesn't have children in this direction)
    std::vector<int> child_ids;

    // Lowest cost found so far (used to keep track of when to
    // reset the list of lowest states)
    double lowest_cost;
    // Keep track of the lowest states found so far
    // (this is cleaned if we find something lower by TOLERANCE)
    std::set<std::vector<bool>> lowest_states;

    static size_t memory_estimate(const Model_T& model)
    {
      size_t n_dimensions = model.get_parameters().size();
      size_t n_weights = 2;
      size_t n_states = 1;  // average lowest_states amount value during method
                            // run, starts with 1 at init
      size_t n_spins = model.get_sweep_size();
      return sizeof(Node) +  // memory consumption of empty class
             markov::Metropolis<Model_T>::memory_estimate(
                 model) +  // memory consumption of elements of replica
             utils::vector_values_memory_estimate<int>(
                 n_dimensions) +  // memory consumption of elements of child_ids
             utils::vector_values_memory_estimate<double>(
                 n_weights) +  // memory consumption of elements of weights
             utils::vector_values_memory_estimate<std::vector<bool>>(
                 n_states) +  // memory consumption of
             n_states * utils::vector_values_memory_estimate<bool>(
                            n_spins);  // elements of lowest_states
    }
  };

  std::string init_memory_check_error_message() const override
  {
    return (
        "Input problem is too large (too many terms, variables or "
        "temperatures), "
        "Expected to exceed machine's current available memory.");
  }

  size_t target_number_of_states() const override { return nodes_size_; }
  //////////////////////////////////////////////////////////////////////////////
  /// Initialize the solver
  ///
  /// Initialization entails creating the graph of nodes and initializing a
  /// replica (a metropolis walker containing a state) at each node.
  ///
  /// The set of nodes is currently hard coded do consist of nT diagonals with
  /// 1..nT nodes, respectively. Each of them is assigned a set of mixing
  /// weights such that the starting point (on the x axis) is [1,0] and the
  /// end point on the y axis is [0,1], with linear interpolation of the
  /// mixing weights in between:
  ///
  ///  position   weights
  ///  5    j     ( 0, 1)
  ///  2 4   ↖    ( 0, 1)  (.5,.5)
  ///  0 1 3      (.5,.5)  ( 1, 0)  ( 1, 0)
  ///   → i
  ///
  /// The temperature is highest in the lower left corner and decreases by
  /// one temperature step until the lowest on the furthest diagonal. We
  /// use the `i` index to refer to the diagonal and the `j` index for the
  /// position on that diagnoal. As such, the results for the pareto front
  /// are found on the last `nT` nodes in the node list, which represent
  /// the diagonal at i=nT-1, j=[0..nT].
  ///
  void init() override
  {
    // Construct the 2D node arrangement:
    auto temperatures = temperature_set_.get_discretized_values();
    std::sort(temperatures.begin(), temperatures.end());
    dimensions_ = this->model_->get_parameters().size();
    // Fix at 2d grid for now.
    assert(dimensions_ == 2);
    size_t nT = temperatures.size();
    size_t nodes_size_ = nT * (nT + 1) / 2;

    this->init_memory_check();
    // proceed to memory allocation
    nodes_.resize(nodes_size_);
    int node_id = 0;
    for (size_t i = 0; i < nT; i++)
    {
      for (size_t j = 0; j <= i; j++)
      {
        Weights weights = {0.5, 0.5};
        if (i != 0)
        {
          double ii = static_cast<double>(i);
          double jj = static_cast<double>(j);
          weights = {(ii - jj) / ii, jj / ii};
        }
        Node& node = nodes_[static_cast<size_t>(node_id)];
        node.odd = i & 1;
        node.i = i;
        node.j = j;
        node.weights = weights;
        node.replica.set_model(this->model_);
        node.replica.set_rng(this->rng_.get());
        node.replica.set_temperature(temperatures[nT - i - 1]);
        node.child_ids = std::vector<int>(dimensions_, -1);
        if (j != i)
        {
          nodes_[static_cast<size_t>(node_id) - i].child_ids[0] =
              node_id;  // horizontal
        }
        if (j != 0)
        {
          nodes_[static_cast<size_t>(node_id) - i - 1].child_ids[1] =
              node_id;  // vertical
        }
        node_id++;
      }
    }
    for (size_t i = 0; i < nodes_.size(); i++)
    {
      this->scoped_observable_label("node", i);
      auto& node = nodes_[i];
      assert(node.weights.size() == 2);
      auto& replica = node.replica;
      replica.init();
      node.lowest_cost = replica.cost();
      node.lowest_states.insert(replica.state().spins);
      auto transition = this->model_->create_parameter_change(node.weights);
      auto cost_difference =
          this->model_->calculate_cost_difference(replica.state(), transition);
      replica.apply_transition(transition, cost_difference);
      this->observe("avg_cost", replica.cost());
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  /// Make a step
  ///
  /// A step constitutes performing individual metropolis updates on the nodes,
  /// followed by measurements and replica exchange moves.
  ///
  /// We update exchange moves from even (odd) diagonals in even (odd) steps
  /// and switch the direction every 2 steps.
  ///
  void make_step(uint64_t step) override
  {
    // Metropolis updates to each replica (idependently)
    for (size_t i = 0; i < nodes_.size(); i++)
    {
      this->scoped_observable_label("node", i);
      auto& node = nodes_[i];
      node.replica.make_step();
      if (node.replica.cost() < node.lowest_cost + 1e-6)
      {
        if (node.replica.cost() < node.lowest_cost - 1e-6)
        {
          node.lowest_states.clear();
          node.lowest_cost = node.replica.cost();
        }
        node.lowest_states.insert(node.replica.state().spins);
      }
      this->observe("avg_cost", node.replica.cost());
      auto counter = node.replica.get_evaluation_counter();
      this->observe(
          "acc_rate",
          static_cast<double>(counter.get_accepted_transition_count()) /
              static_cast<double>(counter.get_difference_evaluation_count()));
    }

    // Find lowest outside multithreading
    for (auto& node : nodes_)
    {
      this->evaluation_counter_ += node.replica.get_evaluation_counter();
      node.replica.reset_evaluation_counter();
    }

    // Perform exchange moves
    bool odd = step & 1;
    size_t direction = (step / 2) % dimensions_;
    for (size_t i = 0; i < nodes_.size(); i++)
    {
      this->scoped_observable_label("node", i);
      this->scoped_observable_label("direction", direction);
      auto& n1 = nodes_[i];
      if (odd != n1.odd) continue;
      if (n1.child_ids[direction] == -1) continue;
      auto& n2 = nodes_[static_cast<size_t>(n1.child_ids[direction])];

      if ((step / 4) % 250 == 0)
      {
        const auto& s1 = n1.replica.state();
        const auto& s2 = n2.replica.state();
        this->observe("spin_overlap", this->model_->get_spin_overlap(s1, s2));
        this->observe("term_overlap", this->model_->get_term_overlap(s1, s2));
      }

      // NOTE: To satisfy detailed balance, we must consider the change
      // in costs induced by moving each state to a new sets of weights.
      //
      // Specifically we must satisfy:
      //
      // x-y = \beta_1(c_1-c_2') - \beta_2(c_2-c_1')
      //
      // with x,y both <= 0, c_1 and c_1' the costs before/after the
      // exchange and exp(x), exp(y) the transition probability
      // from either side of the exchange. One choice to satisfy this
      // is to have
      //
      // x = RHS, y = 0   when RHS < 0
      // x = 0, y = -RHS  when RHS > 0
      //
      auto& r1 = n1.replica;
      auto& r2 = n2.replica;
      auto c1 = r1.cost();
      auto c2 = r2.cost();
      auto t1 = this->model_->create_parameter_change(n1.weights);
      auto t2 = this->model_->create_parameter_change(n2.weights);
      auto c1p = c1 + this->model_->calculate_cost_difference(r1.state(), t2);
      auto c2p = c2 + this->model_->calculate_cost_difference(r2.state(), t1);

      double z = r1.beta() * (c1 - c2p) + r2.beta() * (c2 - c1p);
      if (z >= 0 || this->rng_->uniform() < exp(z))
      {
        r1.apply_transition(t2, c1p - c1);
        r2.apply_transition(t1, c2p - c2);
        r1.swap_state(&r2);
        this->observe("swap_rate", 1);
      }
      else
      {
        this->observe("swap_rate", 0);
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  /// Return the solutions found
  ///
  /// As the solution we return the lowest_cost found in the furthest diagonal
  /// (i.e., at lowest temperature) and the states corresponding to this lowest
  /// cost.
  utils::Structure get_solutions() const override
  {
    utils::Structure solutions;
    solutions["version"] = "1.0";
    int nT = temperature_set_.get_count();
    for (int i = 0; i < nT; i++)
    {
      auto& node = nodes_[nodes_.size() - nT + i];
      utils::Structure s;
      s["i"] = node.i;
      s["j"] = node.j;
      s["weights"] = node.weights;
      s["T"] = node.replica.temperature();
      // s["cost"] = node.avg_cost.render();
      s["min_cost"] = node.lowest_cost;
      for (const auto& state : node.lowest_states)
      {
        std::vector<int> tmp;
        for (const auto& spin : state)
        {
          tmp.push_back(spin ? -1 : 1);
        }
        s["states"].push_back(this->model_->create_state(tmp));
      }
      solutions.push_back(s);
    }
    return solutions;
  }

  void configure(const utils::Json& json) override
  {
    Base_T::configure(json);
    if (!json.IsObject() || !json.HasMember(utils::kParams))
    {
      THROW(utils::MissingInputException, "Input field `", utils::kParams,
            "` is required.");
    }
    const utils::Json& params = json[utils::kParams];
    this->param(params, "temperature_set", temperature_set_)
        .description("temperatures for the grid's diagonals.")
        .required();
  }

 private:
  size_t dimensions_;
  ::schedule::Schedule temperature_set_;
  std::vector<Node> nodes_;
  size_t nodes_size_;
};
REGISTER_SOLVER(Murex);

}  // namespace solver
