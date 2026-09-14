#include "mfg/analysis.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <utility>

namespace mfg {
namespace {

class Monitor {
public:
    explicit Monitor(const AnalysisOptions& options) : options_(options) { check(); }
    void tick() { if ((++ticks_ & 127U) == 0) check(); }
    void check() const {
        if (options_.cancelled && options_.cancelled()) throw AnalysisCancelled();
    }
    void progress(const std::string& stage, std::uint64_t done,
                  std::uint64_t total) const {
        check();
        if (options_.progress) options_.progress(stage, done, total);
    }
private:
    const AnalysisOptions& options_;
    std::uint64_t ticks_ = 0;
};

std::size_t offset(const Matrix& matrix, Coord p) {
    return static_cast<std::size_t>(p.row) * static_cast<std::size_t>(matrix.cols()) +
           static_cast<std::size_t>(p.col);
}

void validate_mask(const Matrix& matrix, const IgnoreMask& mask) {
    const auto area = static_cast<std::size_t>(matrix.rows()) *
                      static_cast<std::size_t>(matrix.cols());
    if (!mask.empty() && mask.size() != area)
        throw std::invalid_argument("ignore mask must match matrix dimensions");
}

bool ignored(const Matrix& matrix, const IgnoreMask& mask, Coord p) {
    return !mask.empty() && mask[offset(matrix, p)];
}

std::array<Coord, 4> corners(Rectangle rectangle) {
    return {{{rectangle.top, rectangle.left}, {rectangle.top, rectangle.right},
             {rectangle.bottom, rectangle.left}, {rectangle.bottom, rectangle.right}}};
}

constexpr std::array<Cell, 4> forbidden = {
    Cell::One, Cell::Zero, Cell::Zero, Cell::One};

struct FormulaLimit : std::runtime_error {
    FormulaLimit() : std::runtime_error("rectangle clause limit reached") {}
};

std::vector<Clause> build_clauses(const Matrix& matrix, const IgnoreMask& mask,
                                const AnalysisOptions& options, Monitor& monitor) {
    std::vector<Clause> clauses;
    // A set avoids duplicate residual clauses before any quadratic display
    // normalization, without changing the probability union used below.
    std::set<Clause> seen;
    for (int top = 0; top < matrix.rows(); ++top) {
        monitor.progress("Building constraints", static_cast<std::uint64_t>(top),
                         static_cast<std::uint64_t>(matrix.rows()));
        for (int bottom = top + 1; bottom < matrix.rows(); ++bottom)
            for (int left = 0; left < matrix.cols(); ++left)
                for (int right = left + 1; right < matrix.cols(); ++right) {
                    monitor.tick();
                    Clause clause;
                    bool skip = false;
                    const auto positions = corners({top, bottom, left, right});
                    for (std::size_t index = 0; index < positions.size(); ++index) {
                        const Coord p = positions[index];
                        const Cell cell = matrix.at(p);
                        if (ignored(matrix, mask, p) ||
                            (cell != Cell::Blank && cell != forbidden[index])) {
                            skip = true;
                            break;
                        }
                        if (cell == Cell::Blank)
                            clause.push_back({p, forbidden[index] == Cell::Zero});
                    }
                    if (skip) continue;
                    if (clause.empty()) return {Clause{}};
                    if (!seen.insert(clause).second) continue;
                    if (clauses.size() >= options.max_analysis_clauses) throw FormulaLimit();
                    clauses.push_back(std::move(clause));
                }
    }
    monitor.progress("Building constraints", static_cast<std::uint64_t>(matrix.rows()),
                     static_cast<std::uint64_t>(matrix.rows()));
    return clauses;
}

struct EncodedLiteral { std::size_t variable; bool positive; };
using EncodedClause = std::vector<EncodedLiteral>;
struct EncodedFormula {
    std::vector<EncodedClause> clauses;
    std::size_t variables = 0;
};

EncodedFormula encode(const std::vector<Clause>& clauses, Monitor& monitor) {
    EncodedFormula result;
    std::map<Coord, std::size_t> ids;
    result.clauses.reserve(clauses.size());
    for (const auto& clause : clauses) {
        monitor.tick();
        EncodedClause encoded;
        for (const auto literal : clause) {
            auto [it, inserted] = ids.try_emplace(literal.variable, ids.size());
            (void)inserted;
            encoded.push_back({it->second, literal.positive});
        }
        result.clauses.push_back(std::move(encoded));
    }
    result.variables = ids.size();
    return result;
}

bool satisfied(const EncodedFormula& formula, const std::vector<bool>& assignment,
               Monitor& monitor) {
    for (const auto& clause : formula.clauses) {
        monitor.tick();
        bool clause_value = false;
        for (const auto literal : clause) {
            if (assignment[literal.variable] == literal.positive) {
                clause_value = true;
                break;
            }
        }
        if (!clause_value) return false;
    }
    return true;
}

std::string times_power_of_two(std::uint64_t number, std::size_t exponent,
                              Monitor& monitor) {
    if (number == 0) return "0";
    std::string digits = std::to_string(number);
    std::reverse(digits.begin(), digits.end());
    for (std::size_t power = 0; power < exponent; ++power) {
        int carry = 0;
        for (char& digit : digits) {
            monitor.tick();
            const int value = 2 * (digit - '0') + carry;
            digit = static_cast<char>('0' + value % 10);
            carry = value / 10;
        }
        if (carry) digits.push_back(static_cast<char>('0' + carry));
    }
    std::reverse(digits.begin(), digits.end());
    return digits;
}

ExactCompletionCount count_completions(const EncodedFormula& formula,
                                      std::size_t all_variables,
                                      const AnalysisOptions& options) {
    ExactCompletionCount result;
    result.variables = all_variables;
    result.constrained_variables = formula.variables;
    if (formula.variables >= 63 || (options.max_exact_variables &&
                                   formula.variables > *options.max_exact_variables)) {
        result.status = AnalysisStatus::LimitReached;
        result.note = "Exact enumeration needs " + std::to_string(formula.variables) +
                      " constrained variables; " + (formula.variables >= 63
                          ? "this enumerator supports at most 62 constrained variables."
                          : "increase the exact-variable limit to run it.");
        return result;
    }
    try {
        Monitor monitor(options);
        const std::uint64_t total = std::uint64_t{1} << formula.variables;
        std::uint64_t valid = 0;
        std::vector<bool> assignment(formula.variables, false);
        for (std::uint64_t word = 0; word < total; ++word) {
            monitor.tick();
            if ((word & 1023U) == 0) monitor.progress("Exact count", word, total);
            if (satisfied(formula, assignment, monitor)) ++valid;
            ++result.assignments_examined;
            for (std::size_t bit = 0; bit < assignment.size(); ++bit) {
                assignment[bit] = !assignment[bit];
                if (assignment[bit]) break;
            }
        }
        result.valid_count = times_power_of_two(valid, all_variables - formula.variables, monitor);
        result.total_count = times_power_of_two(1, all_variables, monitor);
        result.valid_ratio = static_cast<double>(valid) / static_cast<double>(total);
        monitor.progress("Exact count", total, total);
        result.status = AnalysisStatus::Complete;
    } catch (const AnalysisCancelled&) {
        result.status = AnalysisStatus::Cancelled;
        result.valid_count.clear();
        result.total_count.clear();
        result.note = "Exact enumeration cancelled; partial counts are not exact results.";
    }
    return result;
}

CompletionEstimate estimate(const EncodedFormula& formula,
                            const AnalysisOptions& options, bool importance) {
    CompletionEstimate result;
    try {
        Monitor monitor(options);
        const std::string stage = importance ? "Importance estimate" : "Uniform estimate";
        // Constant formulas are known without sampling, including zero blanks.
        if (formula.clauses.empty() ||
            (formula.clauses.size() == 1 && formula.clauses.front().empty())) {
            result.valid_ratio = formula.clauses.empty() ? 1.0 : 0.0;
            result.status = AnalysisStatus::Complete;
            result.note = "Constant formula; probability is exact.";
            return result;
        }
        if (options.sample_count == 0) {
            result.status = AnalysisStatus::LimitReached;
            result.note = "At least one sample is required.";
            return result;
        }
        std::mt19937_64 random(options.seed);
        std::vector<bool> assignment(formula.variables, false);
        std::vector<double> weights;
        double weight_sum = 0.0;
        if (importance) {
            weights.reserve(formula.clauses.size());
            for (const auto& clause : formula.clauses) {
                monitor.tick();
                // A violated CNF clause is a DNF term of probability 2^-k.
                const double weight = std::ldexp(1.0, -static_cast<int>(clause.size()));
                weights.push_back(weight);
                weight_sum += weight;
            }
        }
        std::discrete_distribution<std::size_t> choose(weights.begin(), weights.end());
        double mean = 0.0;
        double sum_squares = 0.0;
        for (std::uint64_t sample = 0; sample < options.sample_count; ++sample) {
            monitor.tick();
            if ((sample & 1023U) == 0) monitor.progress(stage, sample, options.sample_count);
            for (std::size_t bit = 0; bit < assignment.size(); ++bit) {
                monitor.tick();
                assignment[bit] = (random() & 1U) != 0;
            }
            double value = 0.0;
            if (!importance) {
                value = satisfied(formula, assignment, monitor) ? 1.0 : 0.0;
            } else {
                const auto& selected = formula.clauses[choose(random)];
                for (const auto literal : selected)
                    assignment[literal.variable] = !literal.positive;
                std::size_t covering_terms = 0;
                for (const auto& clause : formula.clauses) {
                    monitor.tick();
                    bool violated = true;
                    for (const auto literal : clause) {
                        if (assignment[literal.variable] == literal.positive) {
                            violated = false;
                            break;
                        }
                    }
                    if (violated) ++covering_terms;
                }
                // The sampled assignment has mass covering_terms/(2^n*S).
                // S/covering_terms is an unbiased estimate of the union of
                // bad assignments, even when term lengths are unequal.
                value = 1.0 - weight_sum / static_cast<double>(covering_terms);
            }
            ++result.samples;
            const double delta = value - mean;
            mean += delta / static_cast<double>(result.samples);
            sum_squares += delta * (value - mean);
        }
        result.valid_ratio = mean;
        result.standard_error = result.samples > 1
            ? std::sqrt(std::max(0.0, sum_squares) /
                        (static_cast<double>(result.samples - 1) *
                         static_cast<double>(result.samples)))
            : std::numeric_limits<double>::quiet_NaN();
        if (importance)
            result.note = "Unbiased complement-of-union estimate; sampling noise may put it outside [0,1].";
        monitor.progress(stage, options.sample_count, options.sample_count);
        result.status = AnalysisStatus::Complete;
    } catch (const AnalysisCancelled&) {
        result.status = AnalysisStatus::Cancelled;
        result.note = "Sampling cancelled; no completed estimate is available.";
    }
    return result;
}

// Transposition preserves the forbidden pattern and reduces pair-table memory
// to the square of the smaller dimension.
struct ScanView {
    const Matrix& matrix;
    bool transpose;
    int height;
    int width;
    explicit ScanView(const Matrix& m)
        : matrix(m), transpose(m.rows() < m.cols()),
          height(transpose ? m.cols() : m.rows()),
          width(transpose ? m.rows() : m.cols()) {}
    Coord coord(int row, int col) const { return transpose ? Coord{col, row} : Coord{row, col}; }
    Cell at(int row, int col) const { return matrix.at(coord(row, col)); }
    Rectangle rectangle(int top, int bottom, int left, int right) const {
        return transpose ? Rectangle{left, right, top, bottom}
                         : Rectangle{top, bottom, left, right};
    }
};

} // namespace

std::optional<Rectangle> find_forbidden_rectangle(const Matrix& matrix,
    const IgnoreMask& mask, const AnalysisOptions& options) {
    validate_mask(matrix, mask);
    Monitor monitor(options);
    const ScanView view(matrix);
    if (view.width < 2) return std::nullopt;
    const auto width = static_cast<std::size_t>(view.width);
    std::vector<int> first_row(width * width, -1);
    for (int row = 0; row < view.height; ++row)
        for (int left = 0; left < view.width; ++left)
            for (int right = left + 1; right < view.width; ++right) {
                monitor.tick();
                if (ignored(matrix, mask, view.coord(row, left)) ||
                    ignored(matrix, mask, view.coord(row, right))) continue;
                int& seen = first_row[static_cast<std::size_t>(left) * width +
                                      static_cast<std::size_t>(right)];
                if (view.at(row, left) == Cell::Zero && view.at(row, right) == Cell::One && seen >= 0)
                    return view.rectangle(seen, row, left, right);
                if (view.at(row, left) == Cell::One && view.at(row, right) == Cell::Zero && seen < 0)
                    seen = row;
            }
    monitor.check();
    return std::nullopt;
}

BlankStatistics blank_statistics(const Matrix& matrix, const IgnoreMask& mask,
                                const AnalysisOptions& options) {
    validate_mask(matrix, mask);
    Monitor monitor(options);
    BlankStatistics result;
    const ScanView view(matrix);
    const auto width = static_cast<std::size_t>(view.width);
    std::vector<bool> marked(static_cast<std::size_t>(matrix.rows()) *
                             static_cast<std::size_t>(matrix.cols()), false);
    for (bool reverse : {false, true}) {
        std::vector<bool> seen(width * width, false);
        for (int step = 0; step < view.height; ++step) {
            const int row = reverse ? view.height - 1 - step : step;
            for (int left = 0; left < view.width; ++left)
                for (int right = left + 1; right < view.width; ++right) {
                    monitor.tick();
                    if (ignored(matrix, mask, view.coord(row, left)) ||
                        ignored(matrix, mask, view.coord(row, right))) continue;
                    const auto pair = static_cast<std::size_t>(left) * width +
                                      static_cast<std::size_t>(right);
                    const Cell a = view.at(row, left), b = view.at(row, right);
                    if (!reverse) {
                        if (a == Cell::Blank && b == Cell::Zero) seen[pair] = true;
                        else if (a == Cell::Zero && b == Cell::Blank && seen[pair])
                            marked[offset(matrix, view.coord(row, right))] = true;
                    } else {
                        if (a == Cell::Zero && b == Cell::Blank) seen[pair] = true;
                        else if (a == Cell::Blank && b == Cell::Zero && seen[pair])
                            marked[offset(matrix, view.coord(row, left))] = true;
                    }
                }
        }
    }
    for (int row = 0; row < matrix.rows(); ++row)
        for (int col = 0; col < matrix.cols(); ++col) {
            monitor.tick();
            const Coord p{row, col};
            if (!ignored(matrix, mask, p) && matrix.at(p) == Cell::Blank) ++result.blanks;
            if (marked[offset(matrix, p)]) result.b0_positions.push_back(p);
        }
    result.b0 = result.b0_positions.size();
    monitor.check();
    result.status = AnalysisStatus::Complete;
    return result;
}

ForcedCellMaps forced_cell_maps(const Matrix& matrix, const IgnoreMask& mask,
                               const AnalysisOptions& options) {
    validate_mask(matrix, mask);
    Monitor monitor(options);
    const auto area = static_cast<std::size_t>(matrix.rows()) *
                      static_cast<std::size_t>(matrix.cols());
    ForcedCellMaps result{std::vector<bool>(area), std::vector<bool>(area),
                          std::vector<bool>(area), std::vector<bool>(area)};
    const ScanView view(matrix);
    const auto width = static_cast<std::size_t>(view.width);
    for (const bool reverse : {false, true}) {
        std::vector<bool> seen(width * width, false);
        for (int step = 0; step < view.height; ++step) {
            monitor.tick();
            const int row = reverse ? view.height - 1 - step : step;
            for (int left = 0; left < view.width; ++left)
                for (int right = left + 1; right < view.width; ++right) {
                    monitor.tick();
                    const Coord a = view.coord(row, left), b = view.coord(row, right);
                    if (ignored(matrix, mask, a) || ignored(matrix, mask, b)) continue;
                    const auto pair = static_cast<std::size_t>(left) * width +
                                      static_cast<std::size_t>(right);
                    const Cell av = matrix.at(a), bv = matrix.at(b);
                    if (!reverse) {
                        if (av == Cell::One && bv == Cell::Zero) seen[pair] = true;
                        else if (seen[pair] && av == Cell::Blank && bv == Cell::One)
                            result.zero_forbidden[offset(matrix, a)] = true;
                        else if (seen[pair] && av == Cell::Zero && bv == Cell::Blank)
                            result.one_forbidden[offset(matrix, b)] = true;
                    } else {
                        if (av == Cell::Zero && bv == Cell::One) seen[pair] = true;
                        else if (seen[pair] && av == Cell::Blank && bv == Cell::Zero)
                            result.one_forbidden[offset(matrix, a)] = true;
                        else if (seen[pair] && av == Cell::One && bv == Cell::Blank)
                            result.zero_forbidden[offset(matrix, b)] = true;
                    }
                }
        }
    }
    for (std::size_t index = 0; index < area; ++index) {
        monitor.tick();
        result.forced[index] = result.zero_forbidden[index] || result.one_forbidden[index];
        result.conflict[index] = result.zero_forbidden[index] && result.one_forbidden[index];
    }
    monitor.check();
    return result;
}

GreedyResult greedy_fill(const Matrix& input, GreedyMethod method,
                        const IgnoreMask& mask, const AnalysisOptions& options) {
    validate_mask(input, mask);
    GreedyResult result{input, AnalysisStatus::NotRequested, false, false, {}, {}};
    try {
        Monitor monitor(options);
        if (method == GreedyMethod::PairedZeros) {
            const auto stats = blank_statistics(input, mask, options);
            for (const Coord p : stats.b0_positions) {
                monitor.tick();
                result.matrix.at(p) = Cell::Zero;
            }
            for (int row = 0; row < input.rows(); ++row)
                for (int col = 0; col < input.cols(); ++col) {
                    monitor.tick();
                    const Coord p{row, col};
                    if (!ignored(input, mask, p) && result.matrix.at(p) == Cell::Blank)
                        result.matrix.at(p) = Cell::One;
                }
        } else {
            result.violation = find_forbidden_rectangle(input, mask, options);
            if (result.violation) {
                result.completed = true;
                for (int row = 0; row < input.rows(); ++row)
                    for (int col = 0; col < input.cols(); ++col) {
                        monitor.tick();
                        if (!ignored(input, mask, {row, col}) && input.at({row, col}) == Cell::Blank)
                            result.completed = false;
                    }
                result.status = AnalysisStatus::Complete;
                return result;
            }
            // Preserve the Java heuristic's column-major choice order, while
            // treating a conflicting position as failure rather than skipping
            // it and accidentally claiming success later.
            for (int col = 0; col < input.cols(); ++col) {
                monitor.progress("Greedy completion", static_cast<std::uint64_t>(col),
                                 static_cast<std::uint64_t>(input.cols()));
                for (int row = 0; row < input.rows(); ++row) {
                    monitor.tick();
                    const Coord target{row, col};
                    if (ignored(input, mask, target) || result.matrix.at(target) != Cell::Blank) continue;
                    bool zero_forbidden = false, one_forbidden = false, one_implies = false;
                    for (int other_row = 0; other_row < input.rows(); ++other_row) {
                        if (other_row == row) continue;
                        for (int other_col = 0; other_col < input.cols(); ++other_col) {
                            monitor.tick();
                            if (other_col == col) continue;
                            const auto positions = corners({std::min(row, other_row), std::max(row, other_row),
                                                            std::min(col, other_col), std::max(col, other_col)});
                            std::size_t blanks = 0;
                            bool possible = true;
                            Cell target_forbidden = Cell::Blank;
                            for (std::size_t index = 0; index < positions.size(); ++index) {
                                const Coord p = positions[index];
                                if (ignored(input, mask, p)) { possible = false; break; }
                                if (p == target) { target_forbidden = forbidden[index]; continue; }
                                const Cell value = result.matrix.at(p);
                                if (value == Cell::Blank) ++blanks;
                                else if (value != forbidden[index]) { possible = false; break; }
                            }
                            if (!possible) continue;
                            if (blanks == 0) {
                                if (target_forbidden == Cell::Zero) zero_forbidden = true;
                                else one_forbidden = true;
                            }
                            if (blanks == 1 && target_forbidden == Cell::One) one_implies = true;
                        }
                    }
                    if (zero_forbidden && one_forbidden) {
                        result.conflict = target;
                        result.status = AnalysisStatus::Complete;
                        return result;
                    }
                    result.matrix.at(target) = zero_forbidden ? Cell::One
                        : (one_forbidden || one_implies) ? Cell::Zero : Cell::One;
                }
            }
        }
        result.completed = true;
        for (int row = 0; row < input.rows(); ++row)
            for (int col = 0; col < input.cols(); ++col) {
                monitor.tick();
                if (!ignored(input, mask, {row, col}) && result.matrix.at({row, col}) == Cell::Blank)
                    result.completed = false;
            }
        result.violation = find_forbidden_rectangle(result.matrix, mask, options);
        result.valid = result.completed && !result.violation;
        monitor.progress("Greedy completion", static_cast<std::uint64_t>(input.cols()),
                         static_cast<std::uint64_t>(input.cols()));
        result.status = AnalysisStatus::Complete;
    } catch (const AnalysisCancelled&) {
        result.status = AnalysisStatus::Cancelled;
        result.valid = false;
        result.completed = false;
    }
    return result;
}

AnalysisResult analyze_matrix(const Matrix& matrix, const IgnoreMask& mask,
                              const AnalysisOptions& options) {
    validate_mask(matrix, mask);
    AnalysisResult result;
    try {
        Monitor monitor(options);
        result.blanks = blank_statistics(matrix, mask, options);
        result.violation = find_forbidden_rectangle(matrix, mask, options);
        if (options.compute_greedy) {
            result.greedy = greedy_fill(matrix, GreedyMethod::ForcedImplications, mask, options);
            if (result.greedy->status == AnalysisStatus::Cancelled) throw AnalysisCancelled();
        }
        if (options.compute_exact || options.compute_estimate || options.compute_importance || options.compute_cnf) {
            auto clauses = result.violation ? std::vector<Clause>{Clause{}}
                                            : build_clauses(matrix, mask, options, monitor);
            result.rectangle_clause_count = clauses.size();
            const auto formula = encode(clauses, monitor);
            if (options.compute_cnf) {
                if (clauses.size() <= options.max_display_clauses) {
                    monitor.check();
                    try {
                        result.cnf.emplace(std::move(clauses), options.cancelled);
                    } catch (const CnfCancelled&) {
                        throw AnalysisCancelled();
                    }
                    monitor.check();
                } else {
                    result.cnf_note = "CNF display omitted: " + std::to_string(clauses.size()) +
                        " residual clauses exceed the display limit of " +
                        std::to_string(options.max_display_clauses) + ". Computations remain available.";
                }
            }
            if (options.compute_exact) {
                result.exact = count_completions(formula, result.blanks.blanks, options);
                if (result.exact.status == AnalysisStatus::Cancelled) throw AnalysisCancelled();
            }
            if (options.compute_estimate) {
                result.uniform = estimate(formula, options, false);
                if (result.uniform.status == AnalysisStatus::Cancelled) throw AnalysisCancelled();
            }
            if (options.compute_importance) {
                result.importance = estimate(formula, options, true);
                if (result.importance.status == AnalysisStatus::Cancelled) throw AnalysisCancelled();
            }
        }
        monitor.check();
        result.status = AnalysisStatus::Complete;
        if (result.exact.status == AnalysisStatus::LimitReached ||
            result.uniform.status == AnalysisStatus::LimitReached ||
            result.importance.status == AnalysisStatus::LimitReached ||
            (options.compute_cnf && !result.cnf)) {
            result.status = AnalysisStatus::LimitReached;
            result.note = "Some requested results reached a resource limit.";
            if (!result.exact.note.empty()) result.note += " " + result.exact.note;
            if (!result.cnf_note.empty()) result.note += " " + result.cnf_note;
        }
    } catch (const AnalysisCancelled&) {
        result.status = AnalysisStatus::Cancelled;
        result.note = "Analysis cancelled.";
        if (options.compute_exact && result.exact.status == AnalysisStatus::NotRequested)
            result.exact.status = AnalysisStatus::Cancelled;
        if (options.compute_estimate && result.uniform.status == AnalysisStatus::NotRequested)
            result.uniform.status = AnalysisStatus::Cancelled;
        if (options.compute_importance && result.importance.status == AnalysisStatus::NotRequested)
            result.importance.status = AnalysisStatus::Cancelled;
    } catch (const FormulaLimit& error) {
        result.status = AnalysisStatus::LimitReached;
        result.note = std::string(error.what()) + "; increase max_analysis_clauses to continue.";
        result.cnf_note = result.note;
        if (options.compute_exact) { result.exact.status = AnalysisStatus::LimitReached; result.exact.note = result.note; }
        if (options.compute_estimate) { result.uniform.status = AnalysisStatus::LimitReached; result.uniform.note = result.note; }
        if (options.compute_importance) { result.importance.status = AnalysisStatus::LimitReached; result.importance.note = result.note; }
    }
    return result;
}

} // namespace mfg
