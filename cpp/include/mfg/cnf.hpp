#pragma once

#include "mfg/model.hpp"

#include <compare>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mfg {

struct Literal {
    Coord variable;
    bool positive = true;

    auto operator<=>(const Literal&) const = default;
};

using Clause = std::vector<Literal>;

struct Rectangle {
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;

    auto operator<=>(const Rectangle&) const = default;
};

// Keeps the geometric premise even when the logical CNF is later normalized.
struct RectangleConstraint {
    Rectangle source;
    Clause residual;
    bool satisfied_by_fixed_entry = false;
};

class Cnf {
public:
    Cnf() = default; // The empty conjunction is true.
    explicit Cnf(std::vector<Clause> clauses);

    [[nodiscard]] const std::vector<Clause>& clauses() const noexcept { return clauses_; }
    [[nodiscard]] std::set<Coord> variables() const;
    [[nodiscard]] bool is_false() const noexcept;
    [[nodiscard]] std::size_t min_clause_size() const noexcept;
    [[nodiscard]] bool evaluate(const std::map<Coord, bool>& assignment) const;
    [[nodiscard]] std::string key() const;
    [[nodiscard]] std::string to_string() const;

    auto operator<=>(const Cnf&) const = default;

private:
    void normalize();

    std::vector<Clause> clauses_;
};

// Residual CNF for avoiding the ordered 1001 pattern. Variables retain their
// matrix coordinates; fixed entries simplify each rectangle clause exactly.
[[nodiscard]] std::vector<RectangleConstraint>
rectangle_constraints(const Matrix& matrix);

[[nodiscard]] Cnf rectangle_cnf(const Matrix& matrix);

[[nodiscard]] Cnf conjoin(const Cnf& first, const Cnf& second);

// Davis-Putnam elimination computes an exact CNF for existential projection.
[[nodiscard]] Cnf existentially_quantify(
    const Cnf& formula, const std::vector<Coord>& eliminated_variables);

[[nodiscard]] Cnf rename_variables(
    const Cnf& formula, const std::map<Coord, Coord>& renaming);

// Pull a child condition back to the parent's blank variables:
//   exists Y (F_child(X,Y) and Gamma_child(X,Y)).
// Child variables not inherited from blanks of M(parent) are eliminated.
[[nodiscard]] Cnf predecessor_condition(
    const SupportTransition& transition,
    const Cnf& child_formula,
    const Cnf& child_scheme);

[[nodiscard]] Cnf predecessor_condition(
    const SupportTransition& transition,
    const Cnf& child_formula);

struct SatResult {
    bool satisfiable = false;
    std::map<Coord, bool> assignment;
};

[[nodiscard]] SatResult solve_sat(const Cnf& formula);

} // namespace mfg
