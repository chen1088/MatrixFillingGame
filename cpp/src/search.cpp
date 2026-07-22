#include "mfg/search.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace mfg {

SearchReport run_bounded_search(const int max_sparsity, const bool compute_projections) {
    const auto support_levels = enumerate_support_levels(max_sparsity);
    SearchReport report;
    report.levels.reserve(support_levels.size());

    for (std::size_t level_index = 0; level_index < support_levels.size(); ++level_index) {
        const auto& supports = support_levels[level_index];
        LevelSummary summary;
        summary.sparsity = static_cast<int>(level_index) + 1;
        summary.support_count = supports.size();
        summary.transition_count = level_index == 0U ? 0U : supports.size();

        std::set<std::string> formula_types;
        std::set<std::string> projection_types;

        for (const SparseSupport& support : supports) {
            Matrix matrix = strong_normal_form(support);
            Cnf formula = rectangle_cnf(matrix);
            formula_types.insert(formula.key());
            summary.max_blank_count =
                std::max(summary.max_blank_count, matrix.blank_positions().size());
            summary.max_clause_count =
                std::max(summary.max_clause_count, formula.clauses().size());

            const bool viable = formula.min_clause_size() >= 2U;
            if (!viable) {
                summary.all_strong_forms_viable = false;
                if (!report.first_normalization_error.has_value()) {
                    report.first_normalization_error.emplace(
                        Counterexample{support, matrix, formula});
                }
            }

            const SatResult sat = solve_sat(formula);
            if (!sat.satisfiable) {
                summary.all_formulas_satisfiable = false;
                if (!report.first_counterexample.has_value()) {
                    report.first_counterexample.emplace(
                        Counterexample{support, std::move(matrix), std::move(formula)});
                }
            }

            if (compute_projections && level_index != 0U) {
                const auto transition = parent_transition(support);
                if (!transition.has_value()) {
                    throw std::logic_error("non-root support has no canonical parent");
                }
                const Cnf projected = predecessor_condition(*transition, formula);
                projection_types.insert(projected.key());
            }
        }

        summary.distinct_formula_count = formula_types.size();
        summary.distinct_projection_count = projection_types.size();
        report.levels.push_back(summary);
    }
    return report;
}

} // namespace mfg
