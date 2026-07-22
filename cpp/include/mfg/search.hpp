#pragma once

#include "mfg/cnf.hpp"
#include "mfg/model.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace mfg {

struct LevelSummary {
    int sparsity = 0;
    std::size_t support_count = 0;
    std::size_t transition_count = 0;
    std::size_t distinct_formula_count = 0;
    std::size_t distinct_projection_count = 0;
    std::size_t max_blank_count = 0;
    std::size_t max_clause_count = 0;
    bool all_strong_forms_viable = true;
    bool all_formulas_satisfiable = true;
};

struct Counterexample {
    SparseSupport support;
    Matrix matrix;
    Cnf formula;
};

struct SearchReport {
    std::vector<LevelSummary> levels;
    std::optional<Counterexample> first_counterexample;
    std::optional<Counterexample> first_normalization_error;
};

[[nodiscard]] SearchReport run_bounded_search(
    int max_sparsity, bool compute_projections = true);

} // namespace mfg
