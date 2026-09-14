#pragma once

#include "mfg/cnf.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mfg {

// Empty means no exclusions. Otherwise row-major, rows()*cols() entries.
// Ignored cells are outside the completion universe, and a rectangle touching
// any ignored cell contributes no constraint.
using IgnoreMask = std::vector<bool>;

enum class AnalysisStatus { NotRequested, Complete, Cancelled, LimitReached };

struct AnalysisOptions {
    bool compute_exact = true;
    bool compute_estimate = true;
    bool compute_importance = true;
    bool compute_greedy = true;
    bool compute_cnf = true;
    std::uint64_t sample_count = 100000;
    std::uint64_t seed = 1;
    // Limits only variables actually occurring in constraints. Free blanks
    // are counted exactly by multiplication, including beyond 64-bit counts.
    std::optional<std::size_t> max_exact_variables = 24;
    std::size_t max_analysis_clauses = 250000;
    std::size_t max_display_clauses = 250000;
    std::function<bool()> cancelled;
    std::function<void(const std::string&, std::uint64_t, std::uint64_t)> progress;
};

class AnalysisCancelled : public std::runtime_error {
public:
    AnalysisCancelled() : std::runtime_error("analysis cancelled") {}
};

struct ExactCompletionCount {
    AnalysisStatus status = AnalysisStatus::NotRequested;
    std::size_t variables = 0;
    std::size_t constrained_variables = 0;
    // Empty unless status==Complete. Decimal strings never overflow.
    std::string valid_count;
    std::string total_count;
    double valid_ratio = 0.0;
    std::uint64_t assignments_examined = 0;
    std::string note;
};

struct CompletionEstimate {
    AnalysisStatus status = AnalysisStatus::NotRequested;
    double valid_ratio = 0.0;
    double standard_error = 0.0;
    std::uint64_t samples = 0;
    std::string note;
};

struct BlankStatistics {
    AnalysisStatus status = AnalysisStatus::NotRequested;
    std::size_t blanks = 0; // B
    std::size_t b0 = 0;     // diagonally paired blanks with opposite corners 0
    std::vector<Coord> b0_positions;
};

struct ForcedCellMaps {
    // Row-major maps. A forbidden value would finish a 1001 rectangle using
    // three already fixed entries. Forced is their union; conflict is their
    // intersection. Ignored and nonblank cells are always false.
    std::vector<bool> zero_forbidden;
    std::vector<bool> one_forbidden;
    std::vector<bool> forced;
    std::vector<bool> conflict;
};

enum class GreedyMethod { PairedZeros, ForcedImplications };

struct GreedyResult {
    Matrix matrix;
    AnalysisStatus status = AnalysisStatus::NotRequested;
    bool completed = false;
    // True only if completed and the resulting matrix avoids every 1001.
    bool valid = false;
    std::optional<Rectangle> violation;
    std::optional<Coord> conflict;
};

struct AnalysisResult {
    AnalysisStatus status = AnalysisStatus::NotRequested;
    std::optional<Cnf> cnf;
    std::string cnf_note;
    std::size_t rectangle_clause_count = 0;
    std::optional<Rectangle> violation;
    ExactCompletionCount exact;
    CompletionEstimate uniform;
    CompletionEstimate importance;
    BlankStatistics blanks;
    std::optional<GreedyResult> greedy;
    std::string note;
};

// Utility scans throw AnalysisCancelled when interrupted. Rectangle indices
// are arbitrary ordered pairs, never restricted to adjacent rows/columns.
[[nodiscard]] std::optional<Rectangle> find_forbidden_rectangle(
    const Matrix&, const IgnoreMask& = {}, const AnalysisOptions& = {});
[[nodiscard]] BlankStatistics blank_statistics(
    const Matrix&, const IgnoreMask& = {}, const AnalysisOptions& = {});
[[nodiscard]] ForcedCellMaps forced_cell_maps(
    const Matrix&, const IgnoreMask& = {}, const AnalysisOptions& = {});
[[nodiscard]] GreedyResult greedy_fill(
    const Matrix&, GreedyMethod = GreedyMethod::ForcedImplications,
    const IgnoreMask& = {}, const AnalysisOptions& = {});

// All computations operate on the matrix supplied; normalization is a caller
// choice. Interrupted routines never label a partial count/estimate Complete.
[[nodiscard]] AnalysisResult analyze_matrix(
    const Matrix&, const IgnoreMask& = {}, const AnalysisOptions& = {});

} // namespace mfg
