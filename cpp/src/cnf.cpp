#include "mfg/cnf.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mfg {
namespace {

std::string literal_text(const Literal literal) {
    std::ostringstream output;
    if (!literal.positive) {
        output << '~';
    }
    output << 'x' << literal.variable.row << '_' << literal.variable.col;
    return output.str();
}

} // namespace

Cnf::Cnf(std::vector<Clause> clauses, const std::function<bool()>& cancelled)
    : clauses_(std::move(clauses)) {
    normalize(cancelled);
}

void Cnf::normalize(const std::function<bool()>& cancelled) {
    const auto check_cancelled = [&] {
        if (cancelled && cancelled()) throw CnfCancelled();
    };
    check_cancelled();
    std::size_t work = 0;
    const auto checkpoint = [&] {
        if ((++work & 127U) == 0) check_cancelled();
    };
    const auto literal_less = [&](const Literal left, const Literal right) {
        checkpoint();
        return left < right;
    };
    const auto clause_less = [&](const Clause& left, const Clause& right) {
        checkpoint();
        return std::lexicographical_compare(left.begin(), left.end(),
                                            right.begin(), right.end(), literal_less);
    };
    const auto is_subset = [&](const Clause& subset, const Clause& superset) {
        std::size_t first = 0, second = 0;
        while (first < subset.size() && second < superset.size()) {
            checkpoint();
            if (subset[first] < superset[second]) return false;
            if (subset[first] == superset[second]) ++first;
            ++second;
        }
        return first == subset.size();
    };
    std::vector<Clause> cleaned;
    cleaned.reserve(clauses_.size());

    for (Clause& clause : clauses_) {
        checkpoint();
        std::sort(clause.begin(), clause.end(), literal_less);
        Clause normalized;
        normalized.reserve(clause.size());
        bool tautology = false;

        for (const Literal literal : clause) {
            checkpoint();
            if (!normalized.empty() && normalized.back().variable == literal.variable) {
                if (normalized.back().positive != literal.positive) {
                    tautology = true;
                    break;
                }
                continue;
            }
            normalized.push_back(literal);
        }
        if (!tautology) {
            cleaned.push_back(std::move(normalized));
        }
    }

    std::sort(cleaned.begin(), cleaned.end(), [&](const Clause& left, const Clause& right) {
        checkpoint();
        if (left.size() != right.size()) {
            return left.size() < right.size();
        }
        return clause_less(left, right);
    });
    cleaned.erase(std::unique(cleaned.begin(), cleaned.end(), [&](const Clause& left, const Clause& right) {
        checkpoint();
        if (left.size() != right.size()) return false;
        for (std::size_t index = 0; index < left.size(); ++index) {
            checkpoint();
            if (left[index] != right[index]) return false;
        }
        return true;
    }), cleaned.end());

    // The empty clause makes the entire conjunction false.
    if (!cleaned.empty() && cleaned.front().empty()) {
        check_cancelled();
        clauses_ = {Clause{}};
        return;
    }

    // Remove clauses subsumed by a shorter (or equal-sized earlier) clause.
    // Matrix clauses have at most four literals: looking up their at most 16
    // subsets avoids a quadratic scan over all earlier clauses. Keep the
    // general subset scan for longer clauses produced by projection.
    std::vector<Clause> irredundant;
    std::set<Clause> short_clauses;
    for (Clause& candidate : cleaned) {
        checkpoint();
        bool subsumed = false;
        if (candidate.size() <= 8) {
            const std::size_t subset_count = std::size_t{1} << candidate.size();
            for (std::size_t bits = 0; bits < subset_count && !subsumed; ++bits) {
                checkpoint();
                Clause subset;
                for (std::size_t index = 0; index < candidate.size(); ++index)
                    if ((bits >> index) & 1U) subset.push_back(candidate[index]);
                subsumed = short_clauses.contains(subset);
            }
        } else {
            for (const Clause& kept : irredundant) {
                checkpoint();
                if (is_subset(kept, candidate)) { subsumed = true; break; }
            }
        }
        if (!subsumed) {
            if (candidate.size() <= 8) short_clauses.insert(candidate);
            irredundant.push_back(std::move(candidate));
        }
    }
    std::sort(irredundant.begin(), irredundant.end(), clause_less);
    check_cancelled();
    clauses_ = std::move(irredundant);
}

std::set<Coord> Cnf::variables() const {
    std::set<Coord> result;
    for (const Clause& clause : clauses_) {
        for (const Literal literal : clause) {
            result.insert(literal.variable);
        }
    }
    return result;
}

bool Cnf::is_false() const noexcept {
    return clauses_.size() == 1U && clauses_.front().empty();
}

std::size_t Cnf::min_clause_size() const noexcept {
    if (clauses_.empty()) {
        return std::numeric_limits<std::size_t>::max();
    }
    std::size_t result = std::numeric_limits<std::size_t>::max();
    for (const Clause& clause : clauses_) {
        result = std::min(result, clause.size());
    }
    return result;
}

bool Cnf::evaluate(const std::map<Coord, bool>& assignment) const {
    for (const Clause& clause : clauses_) {
        bool clause_value = false;
        for (const Literal literal : clause) {
            const auto found = assignment.find(literal.variable);
            if (found == assignment.end()) {
                throw std::invalid_argument("CNF evaluation is missing a variable assignment");
            }
            if (found->second == literal.positive) {
                clause_value = true;
                break;
            }
        }
        if (!clause_value) {
            return false;
        }
    }
    return true;
}

std::string Cnf::key() const {
    if (clauses_.empty()) {
        return "TRUE";
    }
    if (is_false()) {
        return "FALSE";
    }

    std::ostringstream output;
    for (const Clause& clause : clauses_) {
        output << '[';
        for (const Literal literal : clause) {
            output << (literal.positive ? '+' : '-') << literal.variable.row << ','
                   << literal.variable.col << ';';
        }
        output << ']';
    }
    return output.str();
}

std::string Cnf::to_string() const {
    if (clauses_.empty()) {
        return "true";
    }
    if (is_false()) {
        return "false";
    }

    std::ostringstream output;
    for (std::size_t clause_index = 0; clause_index < clauses_.size(); ++clause_index) {
        if (clause_index != 0U) {
            output << " /\\ ";
        }
        output << '(';
        const Clause& clause = clauses_[clause_index];
        for (std::size_t literal_index = 0; literal_index < clause.size(); ++literal_index) {
            if (literal_index != 0U) {
                output << " \\/ ";
            }
            output << literal_text(clause[literal_index]);
        }
        output << ')';
    }
    return output.str();
}

Cnf rectangle_cnf(const Matrix& matrix) {
    std::vector<Clause> clauses;
    for (RectangleConstraint& constraint : rectangle_constraints(matrix)) {
        if (!constraint.satisfied_by_fixed_entry) {
            clauses.push_back(std::move(constraint.residual));
        }
    }
    return Cnf(std::move(clauses));
}

std::vector<RectangleConstraint> rectangle_constraints(const Matrix& matrix) {
    std::vector<RectangleConstraint> constraints;

    for (int top = 0; top < matrix.rows(); ++top) {
        for (int bottom = top + 1; bottom < matrix.rows(); ++bottom) {
            for (int left = 0; left < matrix.cols(); ++left) {
                for (int right = left + 1; right < matrix.cols(); ++right) {
                    Clause residual(4U);
                    std::size_t residual_size = 0U;
                    bool already_satisfied = false;

                    const auto add_literal = [&matrix, &residual, &residual_size,
                                              &already_satisfied](
                                                 const Coord position,
                                                 const bool positive) {
                        const Cell cell = matrix.at(position);
                        if (cell == Cell::Blank) {
                            residual.at(residual_size) = {position, positive};
                            ++residual_size;
                            return;
                        }
                        const bool value = cell == Cell::One;
                        if (value == positive) {
                            already_satisfied = true;
                        }
                    };

                    // not TL or TR or BL or not BR
                    add_literal({top, left}, false);
                    add_literal({top, right}, true);
                    add_literal({bottom, left}, true);
                    add_literal({bottom, right}, false);

                    residual.resize(residual_size);
                    constraints.push_back({
                        Rectangle{top, bottom, left, right},
                        std::move(residual),
                        already_satisfied,
                    });
                }
            }
        }
    }
    return constraints;
}

Cnf conjoin(const Cnf& first, const Cnf& second) {
    std::vector<Clause> clauses = first.clauses();
    clauses.insert(clauses.end(), second.clauses().begin(), second.clauses().end());
    return Cnf(std::move(clauses));
}

Cnf existentially_quantify(const Cnf& formula,
                           const std::vector<Coord>& eliminated_variables) {
    Cnf current = formula;
    std::vector<Coord> variables = eliminated_variables;
    std::sort(variables.begin(), variables.end());
    variables.erase(std::unique(variables.begin(), variables.end()), variables.end());

    for (const Coord variable : variables) {
        std::vector<Clause> positive;
        std::vector<Clause> negative;
        std::vector<Clause> unaffected;

        for (const Clause& clause : current.clauses()) {
            const auto found = std::find_if(
                clause.begin(), clause.end(),
                [variable](const Literal literal) { return literal.variable == variable; });
            if (found == clause.end()) {
                unaffected.push_back(clause);
            } else if (found->positive) {
                positive.push_back(clause);
            } else {
                negative.push_back(clause);
            }
        }

        // If one polarity is absent, choose the variable to satisfy every
        // clause containing it. Those clauses disappear under existential quantification.
        if (positive.empty() || negative.empty()) {
            current = Cnf(std::move(unaffected));
            continue;
        }

        std::vector<Clause> next = std::move(unaffected);
        for (const Clause& positive_clause : positive) {
            for (const Clause& negative_clause : negative) {
                Clause resolvent;
                resolvent.reserve(positive_clause.size() + negative_clause.size() - 2U);
                for (const Literal literal : positive_clause) {
                    if (literal.variable != variable) {
                        resolvent.push_back(literal);
                    }
                }
                for (const Literal literal : negative_clause) {
                    if (literal.variable != variable) {
                        resolvent.push_back(literal);
                    }
                }
                next.push_back(std::move(resolvent));
            }
        }
        current = Cnf(std::move(next));
    }
    return current;
}

Cnf rename_variables(const Cnf& formula, const std::map<Coord, Coord>& renaming) {
    std::vector<Clause> renamed;
    renamed.reserve(formula.clauses().size());
    for (const Clause& clause : formula.clauses()) {
        Clause renamed_clause;
        renamed_clause.reserve(clause.size());
        for (const Literal literal : clause) {
            const auto found = renaming.find(literal.variable);
            if (found == renaming.end()) {
                throw std::invalid_argument("CNF renaming is missing a variable");
            }
            renamed_clause.push_back({found->second, literal.positive});
        }
        renamed.push_back(std::move(renamed_clause));
    }
    return Cnf(std::move(renamed));
}

Cnf predecessor_condition(const SupportTransition& transition,
                          const Cnf& child_formula,
                          const Cnf& child_scheme) {
    const Matrix parent_matrix = strong_normal_form(transition.parent);
    const Matrix child_matrix = strong_normal_form(transition.child);

    std::map<Coord, Coord> child_to_parent;
    for (const Coord parent_blank : parent_matrix.blank_positions()) {
        const Coord child_position = transition.embed(parent_blank);
        if (child_matrix.at(child_position) != Cell::Blank) {
            throw std::logic_error("a parent blank did not remain blank in its child");
        }
        child_to_parent.emplace(child_position, parent_blank);
    }

    const Cnf combined = conjoin(child_formula, child_scheme);
    std::vector<Coord> eliminated;
    for (const Coord child_variable : combined.variables()) {
        if (!child_to_parent.contains(child_variable)) {
            eliminated.push_back(child_variable);
        }
    }

    const Cnf projected = existentially_quantify(combined, eliminated);
    return rename_variables(projected, child_to_parent);
}

Cnf predecessor_condition(const SupportTransition& transition,
                          const Cnf& child_formula) {
    return predecessor_condition(transition, child_formula, Cnf{});
}

SatResult solve_sat(const Cnf& formula) {
    const std::set<Coord> variable_set = formula.variables();
    const std::vector<Coord> variables(variable_set.begin(), variable_set.end());
    std::map<Coord, int> ids;
    for (std::size_t index = 0; index < variables.size(); ++index) {
        ids.emplace(variables[index], static_cast<int>(index));
    }

    std::vector<std::vector<int>> clauses;
    clauses.reserve(formula.clauses().size());
    for (const Clause& clause : formula.clauses()) {
        std::vector<int> encoded;
        encoded.reserve(clause.size());
        for (const Literal literal : clause) {
            const int id = ids.at(literal.variable) + 1;
            encoded.push_back(literal.positive ? id : -id);
        }
        clauses.push_back(std::move(encoded));
    }

    using Assignment = std::vector<std::int8_t>; // -1 unknown, 0 false, 1 true
    const auto propagate = [&clauses](Assignment& assignment) {
        bool changed = true;
        while (changed) {
            changed = false;
            for (const std::vector<int>& clause : clauses) {
                bool satisfied = false;
                int unassigned_count = 0;
                int unit_literal = 0;
                for (const int literal : clause) {
                    const int variable = std::abs(literal) - 1;
                    const std::int8_t value = assignment.at(static_cast<std::size_t>(variable));
                    if (value < 0) {
                        ++unassigned_count;
                        unit_literal = literal;
                    } else {
                        const bool boolean_value = value != 0;
                        if (boolean_value == (literal > 0)) {
                            satisfied = true;
                            break;
                        }
                    }
                }
                if (satisfied) {
                    continue;
                }
                if (unassigned_count == 0) {
                    return false;
                }
                if (unassigned_count == 1) {
                    const int variable = std::abs(unit_literal) - 1;
                    assignment.at(static_cast<std::size_t>(variable)) =
                        static_cast<std::int8_t>(unit_literal > 0 ? 1 : 0);
                    changed = true;
                }
            }
        }
        return true;
    };

    std::function<bool(Assignment&)> search = [&](Assignment& assignment) {
        if (!propagate(assignment)) {
            return false;
        }

        std::vector<int> scores(variables.size(), 0);
        bool all_clauses_satisfied = true;
        for (const std::vector<int>& clause : clauses) {
            bool satisfied = false;
            for (const int literal : clause) {
                const int variable = std::abs(literal) - 1;
                const std::int8_t value = assignment.at(static_cast<std::size_t>(variable));
                if (value >= 0 && (value != 0) == (literal > 0)) {
                    satisfied = true;
                    break;
                }
            }
            if (satisfied) {
                continue;
            }
            all_clauses_satisfied = false;
            for (const int literal : clause) {
                const int variable = std::abs(literal) - 1;
                if (assignment.at(static_cast<std::size_t>(variable)) < 0) {
                    ++scores.at(static_cast<std::size_t>(variable));
                }
            }
        }
        if (all_clauses_satisfied) {
            return true;
        }

        int choice = -1;
        int best_score = -1;
        for (std::size_t index = 0; index < scores.size(); ++index) {
            if (assignment[index] < 0 && scores[index] > best_score) {
                choice = static_cast<int>(index);
                best_score = scores[index];
            }
        }
        if (choice < 0) {
            return false;
        }

        const Assignment snapshot = assignment;
        for (const std::int8_t value : {std::int8_t{1}, std::int8_t{0}}) {
            assignment = snapshot;
            assignment.at(static_cast<std::size_t>(choice)) = value;
            if (search(assignment)) {
                return true;
            }
        }
        assignment = snapshot;
        return false;
    };

    Assignment assignment(variables.size(), std::int8_t{-1});
    SatResult result;
    result.satisfiable = search(assignment);
    if (result.satisfiable) {
        for (std::size_t index = 0; index < variables.size(); ++index) {
            const bool value = assignment[index] > 0;
            result.assignment.emplace(variables[index], value);
        }
    }
    return result;
}

} // namespace mfg
