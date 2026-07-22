#include "mfg/cnf.hpp"
#include "mfg/model.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(const bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::int64_t binomial(const int n, const int k) {
    if (k < 0 || k > n) {
        return 0;
    }
    const int reduced = std::min(k, n - k);
    std::int64_t result = 1;
    for (int index = 1; index <= reduced; ++index) {
        result = result * (n - reduced + index) / index;
    }
    return result;
}

std::int64_t independent_support_count(const int sparsity) {
    std::int64_t total = 0;
    for (int rows = 1; rows <= sparsity; ++rows) {
        for (int cols = 1; cols <= sparsity; ++cols) {
            std::int64_t shape_count = 0;
            for (int empty_rows = 0; empty_rows <= rows; ++empty_rows) {
                for (int empty_cols = 0; empty_cols <= cols; ++empty_cols) {
                    const int available =
                        (rows - empty_rows) * (cols - empty_cols);
                    const std::int64_t term =
                        binomial(rows, empty_rows) * binomial(cols, empty_cols) *
                        binomial(available, sparsity);
                    if ((empty_rows + empty_cols) % 2 == 0) {
                        shape_count += term;
                    } else {
                        shape_count -= term;
                    }
                }
            }
            total += shape_count;
        }
    }
    return total;
}

void test_rectangle_clause_polarity() {
    const mfg::Matrix matrix(2, 2, mfg::Cell::Blank);
    const auto constraints = mfg::rectangle_constraints(matrix);
    check(constraints.size() == 1U, "a 2x2 matrix retains one rectangle premise");
    check(constraints.front().source == mfg::Rectangle{0, 1, 0, 1},
          "the rectangle premise retains all four source indices");
    check(!constraints.front().satisfied_by_fixed_entry,
          "the all-blank rectangle is not simplified away");

    const mfg::Cnf formula = mfg::rectangle_cnf(matrix);
    check(formula.clauses().size() == 1U, "a blank 2x2 matrix has one clause");
    check(formula.clauses().front() == mfg::Clause({
              {{0, 0}, false}, {{0, 1}, true},
              {{1, 0}, true}, {{1, 1}, false}}),
          "the 1001-avoidance clause has the exact polarity");

    const std::map<mfg::Coord, bool> forbidden = {
        {{0, 0}, true}, {{0, 1}, false},
        {{1, 0}, false}, {{1, 1}, true}};
    check(!formula.evaluate(forbidden), "the 1001 assignment falsifies its clause");

    auto safe = forbidden;
    safe[{0, 1}] = true;
    check(formula.evaluate(safe), "changing an off-diagonal corner avoids 1001");
}

void test_strong_normal_form() {
    const mfg::SparseSupport diagonal({{0, 0}, {1, 1}});
    const mfg::Matrix matrix = mfg::strong_normal_form(diagonal);
    check(matrix.to_string() == "1 b\nb 1",
          "diagonal support keeps exactly its two witness blanks");

    const mfg::Cnf formula = mfg::rectangle_cnf(matrix);
    check(formula.clauses().size() == 1U, "the diagonal witness has one residual clause");
    check(formula.clauses().front() == mfg::Clause({
              {{0, 1}, true}, {{1, 0}, true}}),
          "the diagonal witness clause is x(0,1) OR x(1,0)");
    check(mfg::solve_sat(formula).satisfiable, "the diagonal witness is satisfiable");

    const mfg::SparseSupport anti_diagonal({{0, 1}, {1, 0}});
    const mfg::Matrix anti_matrix = mfg::strong_normal_form(anti_diagonal);
    check(anti_matrix.to_string() == "0 1\n1 0",
          "an anti-diagonal pair creates no 1001 witness blanks");
    check(mfg::rectangle_cnf(anti_matrix).clauses().empty(),
          "the anti-diagonal strong form is unconditionally safe");
}

void test_support_enumeration_and_parents() {
    const auto levels = mfg::enumerate_support_levels(5);
    check(levels.size() == 5U, "five support levels were generated");
    check(levels[0].size() == 1U, "there is one normalized 1-sparse support");
    check(levels[1].size() == 4U, "there are four ordered normalized 2-sparse supports");
    check(levels[2].size() == 24U, "there are 24 ordered normalized 3-sparse supports");

    for (std::size_t level = 0; level < levels.size(); ++level) {
        const int sparsity = static_cast<int>(level) + 1;
        check(static_cast<std::int64_t>(levels[level].size()) ==
                  independent_support_count(sparsity),
              "support-tree count matches independent inclusion-exclusion");
    }

    for (std::size_t level = 1; level < levels.size(); ++level) {
        std::set<std::string> previous;
        for (const mfg::SparseSupport& support : levels[level - 1U]) {
            previous.insert(support.key());
        }
        for (const mfg::SparseSupport& support : levels[level]) {
            const auto transition = mfg::parent_transition(support);
            check(transition.has_value(), "every non-root support has a parent");
            if (transition.has_value()) {
                check(previous.contains(transition->parent.key()),
                      "every canonical parent lies in the preceding level");
                check(transition->child == support,
                      "the parent transition retains the exact child");
            }
        }
    }
}

void test_exact_elimination() {
    const mfg::Coord x{0, 0};
    const mfg::Coord y{0, 1};
    const mfg::Coord z{0, 2};
    const mfg::Cnf formula({
        mfg::Clause{{x, true}, {y, true}},
        mfg::Clause{{y, false}, {z, true}},
    });
    const mfg::Cnf projected = mfg::existentially_quantify(formula, {y});
    const mfg::Cnf expected({mfg::Clause{{x, true}, {z, true}}});
    check(projected == expected,
          "Davis-Putnam elimination gives exists y F = (x OR z)");

    const mfg::Cnf contradiction({
        mfg::Clause{{x, true}}, mfg::Clause{{x, false}},
    });
    check(!mfg::solve_sat(contradiction).satisfiable,
          "the SAT solver rejects x AND not-x");
    check(mfg::solve_sat(formula).satisfiable,
          "the SAT solver accepts a satisfiable CNF");
}

void test_predecessor_projection_semantically() {
    const auto levels = mfg::enumerate_support_levels(3);
    for (std::size_t level = 1; level < levels.size(); ++level) {
        for (const mfg::SparseSupport& child : levels[level]) {
            const auto transition = mfg::parent_transition(child);
            check(transition.has_value(), "projection test child has a parent");
            if (!transition.has_value()) {
                continue;
            }

            const mfg::Matrix parent_matrix = mfg::strong_normal_form(transition->parent);
            const mfg::Matrix child_matrix = mfg::strong_normal_form(child);
            const mfg::Cnf child_formula = mfg::rectangle_cnf(child_matrix);
            const mfg::Cnf projected =
                mfg::predecessor_condition(*transition, child_formula);

            const std::vector<mfg::Coord> parent_variables = parent_matrix.blank_positions();
            std::set<mfg::Coord> inherited_child_variables;
            for (const mfg::Coord parent_variable : parent_variables) {
                inherited_child_variables.insert(transition->embed(parent_variable));
            }
            std::vector<mfg::Coord> new_child_variables;
            for (const mfg::Coord child_variable : child_formula.variables()) {
                if (!inherited_child_variables.contains(child_variable)) {
                    new_child_variables.push_back(child_variable);
                }
            }

            check(parent_variables.size() < 63U && new_child_variables.size() < 63U,
                  "small semantic projection test fits bit masks");
            const std::uint64_t parent_assignments =
                std::uint64_t{1} << parent_variables.size();
            const std::uint64_t new_assignments =
                std::uint64_t{1} << new_child_variables.size();

            for (std::uint64_t parent_mask = 0; parent_mask < parent_assignments;
                 ++parent_mask) {
                std::map<mfg::Coord, bool> parent_assignment;
                std::map<mfg::Coord, bool> inherited_assignment;
                for (std::size_t index = 0; index < parent_variables.size(); ++index) {
                    const bool value = ((parent_mask >> index) & 1U) != 0U;
                    parent_assignment.emplace(parent_variables[index], value);
                    inherited_assignment.emplace(
                        transition->embed(parent_variables[index]), value);
                }

                bool extension_exists = false;
                for (std::uint64_t new_mask = 0; new_mask < new_assignments; ++new_mask) {
                    auto child_assignment = inherited_assignment;
                    for (std::size_t index = 0; index < new_child_variables.size(); ++index) {
                        child_assignment.emplace(
                            new_child_variables[index],
                            ((new_mask >> index) & 1U) != 0U);
                    }
                    if (child_formula.evaluate(child_assignment)) {
                        extension_exists = true;
                        break;
                    }
                }
                check(projected.evaluate(parent_assignment) == extension_exists,
                      "projected CNF exactly matches existential child extensions");
            }
        }
    }
}

} // namespace

int main() {
    test_rectangle_clause_polarity();
    test_strong_normal_form();
    test_support_enumeration_and_parents();
    test_exact_elimination();
    test_predecessor_projection_semantically();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "all tests passed\n";
    return 0;
}
