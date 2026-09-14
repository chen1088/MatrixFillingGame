#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace mfg {

// Sparse multivariate Laurent polynomial: exponents may be negative.
// Coefficients and exponents use checked int64_t arithmetic. A failed
// operation throws overflow_error and leaves its operands unchanged.
class Polynomial {
public:
    using Coefficient = std::int64_t;
    using Monomial = std::vector<std::int64_t>;
    using Terms = std::map<Monomial, Coefficient>;

    explicit Polynomial(std::size_t dimension = 0);
    [[nodiscard]] std::size_t dimension() const noexcept;
    [[nodiscard]] const Terms& terms() const noexcept;
    void set(const Monomial& powers, Coefficient coefficient);
    [[nodiscard]] Coefficient get(const Monomial& powers) const;
    [[nodiscard]] Coefficient constant() const;
    [[nodiscard]] Polynomial add(const Polynomial& other) const;
    [[nodiscard]] Polynomial multiply(const Polynomial& other) const;
    [[nodiscard]] std::uint64_t sum_squared_coefficients() const;
    [[nodiscard]] std::string to_string() const;
    bool operator==(const Polynomial&) const = default;

    // 2 + monomial + inverse(monomial), with one selected coordinate in
    // every cyclic block of sizes 1,...,max_cycle. Steps are 1-based.
    static Polynomial cyclic_step(std::size_t step, std::size_t max_cycle);
    // 1 + monomial for the supplied positive cyclic block sizes.
    static Polynomial single_cyclic_step(const std::vector<std::size_t>& sizes, std::size_t step);

    // Plain text: dimension term_count, then coefficient followed by one
    // signed exponent per dimension. Duplicate monomials are added.
    static Polynomial read(std::istream& input);
    void write(std::ostream& output) const;

private:
    std::size_t dimension_;
    Terms terms_;
    void validate(const Monomial& powers) const;
};

} // namespace mfg
