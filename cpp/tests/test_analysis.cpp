#include "mfg/analysis.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;
void check(bool condition, const std::string& message) {
    if (!condition) {
        if (failures < 20) std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool excluded(const mfg::Matrix& matrix, const mfg::IgnoreMask& mask, int row, int col) {
    return !mask.empty() && mask[static_cast<std::size_t>(row * matrix.cols() + col)];
}

// Independent four-loop oracle; deliberately does not use CNF or pair scans.
bool direct_bad(const mfg::Matrix& matrix, const mfg::IgnoreMask& mask = {}) {
    for (int r = 0; r < matrix.rows(); ++r)
        for (int s = r + 1; s < matrix.rows(); ++s)
            for (int c = 0; c < matrix.cols(); ++c)
                for (int d = c + 1; d < matrix.cols(); ++d) {
                    if (excluded(matrix, mask, r, c) || excluded(matrix, mask, r, d) ||
                        excluded(matrix, mask, s, c) || excluded(matrix, mask, s, d)) continue;
                    if (matrix.at({r, c}) == mfg::Cell::One && matrix.at({r, d}) == mfg::Cell::Zero &&
                        matrix.at({s, c}) == mfg::Cell::Zero && matrix.at({s, d}) == mfg::Cell::One)
                        return true;
                }
    return false;
}

std::set<mfg::Coord> direct_b0(const mfg::Matrix& matrix, const mfg::IgnoreMask& mask = {}) {
    std::set<mfg::Coord> result;
    for (int r = 0; r < matrix.rows(); ++r)
        for (int s = r + 1; s < matrix.rows(); ++s)
            for (int c = 0; c < matrix.cols(); ++c)
                for (int d = c + 1; d < matrix.cols(); ++d) {
                    if (excluded(matrix, mask, r, c) || excluded(matrix, mask, r, d) ||
                        excluded(matrix, mask, s, c) || excluded(matrix, mask, s, d)) continue;
                    if (matrix.at({r, c}) == mfg::Cell::Blank && matrix.at({r, d}) == mfg::Cell::Zero &&
                        matrix.at({s, c}) == mfg::Cell::Zero && matrix.at({s, d}) == mfg::Cell::Blank) {
                        result.insert({r, c});
                        result.insert({s, d});
                    }
                }
    return result;
}

std::vector<mfg::Coord> active_blanks(const mfg::Matrix& matrix, const mfg::IgnoreMask& mask) {
    std::vector<mfg::Coord> result;
    for (const auto p : matrix.blank_positions())
        if (!excluded(matrix, mask, p.row, p.col)) result.push_back(p);
    return result;
}

mfg::ForcedCellMaps direct_forced(const mfg::Matrix& matrix, const mfg::IgnoreMask& mask) {
    const auto area = static_cast<std::size_t>(matrix.rows() * matrix.cols());
    mfg::ForcedCellMaps result{std::vector<bool>(area), std::vector<bool>(area),
                               std::vector<bool>(area), std::vector<bool>(area)};
    const std::array<mfg::Cell, 4> pattern{mfg::Cell::One, mfg::Cell::Zero,
                                          mfg::Cell::Zero, mfg::Cell::One};
    for (int r = 0; r < matrix.rows(); ++r)
        for (int s = r + 1; s < matrix.rows(); ++s)
            for (int c = 0; c < matrix.cols(); ++c)
                for (int d = c + 1; d < matrix.cols(); ++d) {
                    const std::array<mfg::Coord, 4> positions{{{r, c}, {r, d}, {s, c}, {s, d}}};
                    int blank = -1;
                    bool possible = true;
                    for (std::size_t corner = 0; corner < 4; ++corner) {
                        const auto p = positions[corner];
                        if (excluded(matrix, mask, p.row, p.col)) { possible = false; break; }
                        if (matrix.at(p) == mfg::Cell::Blank) {
                            if (blank >= 0) { possible = false; break; }
                            blank = static_cast<int>(corner);
                        } else if (matrix.at(p) != pattern[corner]) { possible = false; break; }
                    }
                    if (!possible || blank < 0) continue;
                    const auto p = positions[static_cast<std::size_t>(blank)];
                    const auto index = static_cast<std::size_t>(p.row * matrix.cols() + p.col);
                    if (pattern[static_cast<std::size_t>(blank)] == mfg::Cell::Zero)
                        result.zero_forbidden[index] = true;
                    else result.one_forbidden[index] = true;
                }
    for (std::size_t index = 0; index < area; ++index) {
        result.forced[index] = result.zero_forbidden[index] || result.one_forbidden[index];
        result.conflict[index] = result.zero_forbidden[index] && result.one_forbidden[index];
    }
    return result;
}

std::uint64_t brute_count(const mfg::Matrix& input, const mfg::IgnoreMask& mask) {
    const auto blanks = active_blanks(input, mask);
    std::uint64_t valid = 0;
    for (std::uint64_t bits = 0; bits < (std::uint64_t{1} << blanks.size()); ++bits) {
        mfg::Matrix filled = input;
        for (std::size_t bit = 0; bit < blanks.size(); ++bit)
            filled.at(blanks[bit]) = ((bits >> bit) & 1U) ? mfg::Cell::One : mfg::Cell::Zero;
        if (!direct_bad(filled, mask)) ++valid;
    }
    return valid;
}

mfg::AnalysisOptions exact_options() {
    mfg::AnalysisOptions options;
    options.compute_estimate = false;
    options.compute_importance = false;
    return options;
}

void check_matrix(const mfg::Matrix& matrix, const mfg::IgnoreMask& mask) {
    const auto expected = brute_count(matrix, mask);
    const auto result = mfg::analyze_matrix(matrix, mask, exact_options());
    check(result.status == mfg::AnalysisStatus::Complete, "small analysis completes");
    check(result.exact.status == mfg::AnalysisStatus::Complete, "small exact count completes");
    check(result.exact.valid_count == std::to_string(expected), "exact count equals independent completions");
    const auto blanks = active_blanks(matrix, mask);
    const auto total = std::uint64_t{1} << blanks.size();
    check(result.exact.total_count == std::to_string(total), "total includes unconstrained blanks");
    check(result.exact.valid_ratio == static_cast<double>(expected) / static_cast<double>(total),
          "exact ratio equals independent completions");
    check(result.violation.has_value() == direct_bad(matrix, mask), "pair scan agrees with four-loop scan");
    if (result.violation) {
        const auto r = *result.violation;
        check(r.top < r.bottom && r.left < r.right &&
              matrix.at({r.top, r.left}) == mfg::Cell::One &&
              matrix.at({r.top, r.right}) == mfg::Cell::Zero &&
              matrix.at({r.bottom, r.left}) == mfg::Cell::Zero &&
              matrix.at({r.bottom, r.right}) == mfg::Cell::One,
              "witness is a real ordered 1001 rectangle");
    }
    const auto b0 = direct_b0(matrix, mask);
    check(std::set<mfg::Coord>(result.blanks.b0_positions.begin(), result.blanks.b0_positions.end()) == b0,
          "B0 pair scan equals independent diagonal-blank scan");
    check(result.blanks.blanks == blanks.size() && result.blanks.b0 == b0.size(), "B0/B excludes ignored cells");
    const auto forced = mfg::forced_cell_maps(matrix, mask);
    const auto expected_forced = direct_forced(matrix, mask);
    check(forced.zero_forbidden == expected_forced.zero_forbidden &&
          forced.one_forbidden == expected_forced.one_forbidden &&
          forced.forced == expected_forced.forced && forced.conflict == expected_forced.conflict,
          "forced/conflict maps agree with independent three-fixed-corner oracle");
    for (const auto method : {mfg::GreedyMethod::PairedZeros, mfg::GreedyMethod::ForcedImplications}) {
        const auto greedy = mfg::greedy_fill(matrix, method, mask);
        const bool completed = active_blanks(greedy.matrix, mask).empty();
        const bool valid = completed && !direct_bad(greedy.matrix, mask);
        check(greedy.status == mfg::AnalysisStatus::Complete, "greedy computation terminates");
        check(greedy.valid == valid, "greedy only claims success for an actually valid completion");
        if (greedy.valid) check(expected > 0, "greedy success has a satisfying completion");
        for (int row = 0; row < matrix.rows(); ++row)
            for (int col = 0; col < matrix.cols(); ++col)
                if (matrix.at({row, col}) != mfg::Cell::Blank || excluded(matrix, mask, row, col))
                    check(matrix.at({row, col}) == greedy.matrix.at({row, col}),
                          "greedy preserves fixed and excluded entries");
    }
}

void exhaustive_small_matrices() {
    // All ternary 3x3 matrices include existing violations, free variables,
    // unit clauses and inconsistent completions. Also cover both orientations
    // of the transposed scans, using deterministic scattered exclusions.
    for (const auto& [rows, cols] : {std::pair{2, 3}, std::pair{3, 2}, std::pair{3, 3}}) {
        int possibilities = 1;
        for (int cell = 0; cell < rows * cols; ++cell) possibilities *= 3;
        for (int word = 0; word < possibilities; ++word) {
            int value = word;
            mfg::Matrix matrix(rows, cols);
            for (int row = 0; row < rows; ++row)
                for (int col = 0; col < cols; ++col) {
                    matrix.at({row, col}) = static_cast<mfg::Cell>(value % 3);
                    value /= 3;
                }
            check_matrix(matrix, {});
            if (word % 7 == 0) {
                mfg::IgnoreMask mask(static_cast<std::size_t>(rows * cols), false);
                mask[static_cast<std::size_t>(word % (rows * cols))] = true;
                mask[static_cast<std::size_t>((word / 7) % (rows * cols))] = true;
                check_matrix(matrix, mask);
            }
        }
    }
}

void boundary_cases() {
    auto options = exact_options();
    options.max_exact_variables = 1;
    const auto free = mfg::analyze_matrix(mfg::Matrix(1, 100), {}, options);
    check(free.exact.valid_count == "1267650600228229401496703205376" &&
          free.exact.valid_count == free.exact.total_count && free.exact.valid_ratio == 1.0,
          "100 free blanks counted beyond 64-bit arithmetic without enumeration");
    const auto capped = mfg::analyze_matrix(mfg::Matrix(2, 2), {}, options);
    check(capped.status == mfg::AnalysisStatus::LimitReached &&
          capped.exact.status == mfg::AnalysisStatus::LimitReached && capped.exact.valid_count.empty(),
          "resource-limited enumeration is not an exact count");
    mfg::Matrix legal(2, 2, mfg::Cell::Zero);
    const auto legal_result = mfg::analyze_matrix(legal);
    check(legal_result.exact.valid_count == "1" && legal_result.exact.total_count == "1" &&
          legal_result.uniform.valid_ratio == 1.0 && legal_result.importance.valid_ratio == 1.0,
          "zero blanks has exactly one assignment and correct constant estimates");
    legal.at({0, 0}) = legal.at({1, 1}) = mfg::Cell::One;
    const auto illegal = mfg::analyze_matrix(legal);
    check(illegal.exact.valid_count == "0" && illegal.exact.total_count == "1" &&
          illegal.uniform.valid_ratio == 0.0 && illegal.importance.valid_ratio == 0.0 &&
          illegal.greedy && !illegal.greedy->valid,
          "fully filled forbidden rectangle cannot be called Works");
    const auto ignored = mfg::analyze_matrix(mfg::Matrix(3, 3), mfg::IgnoreMask(9, true));
    check(ignored.exact.valid_count == "1" && ignored.blanks.blanks == 0 && ignored.greedy->valid,
          "fully excluded board has empty completion universe");

    mfg::Matrix far(5, 7, mfg::Cell::One);
    far.at({0, 6}) = far.at({4, 0}) = mfg::Cell::Zero;
    const auto witness = mfg::find_forbidden_rectangle(far);
    check(witness && *witness == mfg::Rectangle{0, 4, 0, 6}, "witness detects nonadjacent four corners");
    mfg::IgnoreMask mask(35, false);
    mask[6] = true;
    check(!mfg::find_forbidden_rectangle(far, mask), "excluded witness corner removes its rectangle");
    bool caught = false;
    try { (void)mfg::analyze_matrix(far, mfg::IgnoreMask(1)); }
    catch (const std::invalid_argument&) { caught = true; }
    check(caught, "mismatched masks rejected");

    options = exact_options();
    options.max_display_clauses = 0;
    const auto hidden = mfg::analyze_matrix(mfg::Matrix(2, 2), {}, options);
    check(hidden.status == mfg::AnalysisStatus::LimitReached &&
          !hidden.cnf && !hidden.cnf_note.empty() && hidden.exact.valid_count == "15",
          "CNF display limit leaves computations available");
    options.max_analysis_clauses = 0;
    const auto limited = mfg::analyze_matrix(mfg::Matrix(2, 2), {}, options);
    check(limited.status == mfg::AnalysisStatus::LimitReached &&
          limited.exact.status == mfg::AnalysisStatus::LimitReached,
          "constraint resource limit is explicit");
}

void sampling_cases() {
    mfg::AnalysisOptions options;
    options.compute_greedy = false;
    options.sample_count = 50000;
    const auto single = mfg::analyze_matrix(mfg::Matrix(2, 2), {}, options);
    check(std::abs(single.uniform.valid_ratio - 15.0 / 16.0) < 0.01, "uniform sampling estimates valid completions");
    check(single.importance.valid_ratio == 15.0 / 16.0 && single.importance.standard_error == 0.0,
          "one DNF term has exact constant importance weight");

    mfg::Matrix unequal(3, 3);
    unequal.at({0, 0}) = unequal.at({2, 2}) = mfg::Cell::One;
    const auto result = mfg::analyze_matrix(unequal, {}, options);
    std::set<std::size_t> clause_sizes;
    for (const auto& clause : result.cnf->clauses()) clause_sizes.insert(clause.size());
    check(clause_sizes.size() >= 2, "importance test contains unequal clause lengths");
    const double exact = static_cast<double>(brute_count(unequal, {})) / 128.0;
    check(std::abs(result.uniform.valid_ratio - exact) < std::max(0.01, 6 * result.uniform.standard_error),
          "uniform estimate matches independent truth");
    check(std::abs(result.importance.valid_ratio - exact) < std::max(0.01, 6 * result.importance.standard_error),
          "importance estimator correctly handles unequal clause weights");
    const auto repeat = mfg::analyze_matrix(unequal, {}, options);
    check(repeat.uniform.valid_ratio == result.uniform.valid_ratio &&
          repeat.importance.valid_ratio == result.importance.valid_ratio,
          "seed produces repeatable sampling");
    options.sample_count = 0;
    const auto no_samples = mfg::analyze_matrix(unequal, {}, options);
    check(no_samples.status == mfg::AnalysisStatus::LimitReached &&
          no_samples.uniform.status == mfg::AnalysisStatus::LimitReached &&
          no_samples.importance.status == mfg::AnalysisStatus::LimitReached,
          "zero samples does not produce a made-up estimate");
}

void cancellation_cases() {
    auto options = exact_options();
    options.cancelled = [] { return true; };
    const auto early = mfg::analyze_matrix(mfg::Matrix(4, 4), {}, options);
    check(early.status == mfg::AnalysisStatus::Cancelled && early.exact.valid_count.empty(),
          "cancellation before analysis cannot expose a count");
    bool cancelled = false;
    options.cancelled = [&] { return cancelled; };
    options.compute_greedy = false;
    options.progress = [&](const std::string& stage, std::uint64_t done, std::uint64_t) {
        if (stage == "Exact count" && done >= 1024) cancelled = true;
    };
    const auto interrupted = mfg::analyze_matrix(mfg::Matrix(4, 4), {}, options);
    check(interrupted.status == mfg::AnalysisStatus::Cancelled &&
          interrupted.exact.status == mfg::AnalysisStatus::Cancelled &&
          interrupted.exact.assignments_examined >= 1024 && interrupted.exact.valid_count.empty(),
          "interrupted exact enumeration never publishes partial valid_count");
    cancelled = false;
    options.compute_exact = false;
    options.compute_estimate = true;
    options.progress = [&](const std::string& stage, std::uint64_t done, std::uint64_t) {
        if (stage == "Uniform estimate" && done >= 1024) cancelled = true;
    };
    const auto sampling = mfg::analyze_matrix(mfg::Matrix(3, 3), {}, options);
    check(sampling.status == mfg::AnalysisStatus::Cancelled &&
          sampling.uniform.status == mfg::AnalysisStatus::Cancelled,
          "interrupted estimate does not report completion");
    options = exact_options();
    std::size_t checks = 0;
    options.cancelled = [&] { return ++checks >= 5; };
    const auto greedy = mfg::greedy_fill(mfg::Matrix(15, 15),
        mfg::GreedyMethod::ForcedImplications, {}, options);
    check(greedy.status == mfg::AnalysisStatus::Cancelled && !greedy.valid,
          "greedy scan checks cancellation during bounded work");
}

void cnf_normalization_cases() {
    std::mt19937 random(731);
    for (int trial = 0; trial < 200; ++trial) {
        std::vector<mfg::Clause> clauses;
        for (int index = 0; index < 12; ++index) {
            mfg::Clause clause;
            for (int variable = 0; variable < 10; ++variable)
                if ((random() % 4) != 0)
                    clause.push_back({{0, variable}, (random() & 1U) != 0});
            clauses.push_back(std::move(clause));
        }
        const mfg::Cnf normalized(clauses);
        for (int bits = 0; bits < 1024; ++bits) {
            std::map<mfg::Coord, bool> assignment;
            for (int variable = 0; variable < 10; ++variable)
                assignment.emplace(mfg::Coord{0, variable}, ((bits >> variable) & 1) != 0);
            bool expected = true;
            for (const auto& clause : clauses) {
                bool satisfied = false;
                for (const auto literal : clause)
                    if (assignment.at(literal.variable) == literal.positive) satisfied = true;
                if (!satisfied) expected = false;
            }
            check(normalized.evaluate(assignment) == expected,
                  "fast short-clause normalization and long fallback preserve truth");
        }
    }
    mfg::Clause long_clause;
    for (int variable = 0; variable < 12; ++variable)
        long_clause.push_back({{0, variable}, true});
    const mfg::Clause short_clause{{{0, 2}, true}, {{0, 9}, true}};
    check(mfg::Cnf({long_clause, short_clause}).clauses() == std::vector<mfg::Clause>{short_clause},
          "short clause subsumes long clause in fallback");
    std::vector<mfg::Clause> many;
    for (int variable = 0; variable < 1000; ++variable)
        many.push_back({{{0, variable}, true}, {{1, variable}, false}});
    std::size_t checks = 0;
    bool interrupted = false;
    try { (void)mfg::Cnf(std::move(many), [&] { return ++checks >= 10; }); }
    catch (const mfg::CnfCancelled&) { interrupted = true; }
    check(interrupted && checks == 10, "CNF normalization itself checks cancellation");
}

} // namespace

int main() {
    exhaustive_small_matrices();
    boundary_cases();
    sampling_cases();
    cancellation_cases();
    cnf_normalization_cases();
    if (failures) {
        std::cerr << failures << " analysis checks failed\n";
        return 1;
    }
    std::cout << "All analysis checks passed\n";
    return 0;
}
