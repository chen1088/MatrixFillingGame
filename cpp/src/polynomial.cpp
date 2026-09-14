#include "mfg/polynomial.hpp"

#include <istream>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace mfg {
namespace {

std::int64_t checked_add(std::int64_t left, std::int64_t right) {
    constexpr auto low = std::numeric_limits<std::int64_t>::min();
    constexpr auto high = std::numeric_limits<std::int64_t>::max();
    if ((right > 0 && left > high - right) || (right < 0 && left < low - right)) {
        throw std::overflow_error("Polynomial sum exceeds int64_t");
    }
    return left + right;
}

std::int64_t checked_multiply(std::int64_t left, std::int64_t right) {
    constexpr auto low = std::numeric_limits<std::int64_t>::min();
    constexpr auto high = std::numeric_limits<std::int64_t>::max();
    if (left > 0) {
        if ((right > 0 && left > high / right) || (right < 0 && right < low / left)) {
            throw std::overflow_error("Polynomial product exceeds int64_t");
        }
    } else if (left < 0) {
        if ((right > 0 && left < low / right) || (right < 0 && left < high / right)) {
            throw std::overflow_error("Polynomial product exceeds int64_t");
        }
    }
    return left * right;
}

std::uint64_t magnitude(std::int64_t value) {
    return value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1U : static_cast<std::uint64_t>(value);
}

std::size_t read_size(std::istream& input, const char* description) {
    std::string token;
    if (!(input >> token) || token.empty() || token.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument(std::string("Expected nonnegative integer: ") + description);
    }
    std::size_t result = 0;
    for (char digit : token) {
        const auto value = static_cast<std::size_t>(digit - '0');
        if (result > (std::numeric_limits<std::size_t>::max() - value) / 10U) {
            throw std::out_of_range(std::string("Integer too large: ") + description);
        }
        result = result * 10U + value;
    }
    return result;
}

Polynomial::Monomial cyclic_monomial(const std::vector<std::size_t>& sizes, std::size_t step) {
    if (step == 0 || sizes.empty()) {
        throw std::invalid_argument("Cyclic polynomials need a positive step and nonempty cycle sizes");
    }
    std::size_t dimension = 0;
    for (auto size : sizes) {
        if (size == 0 || size > std::numeric_limits<std::size_t>::max() - dimension) {
            throw std::invalid_argument("Invalid cyclic block size");
        }
        dimension += size;
    }
    Polynomial::Monomial powers(dimension, 0);
    std::size_t offset = 0;
    for (auto size : sizes) {
        powers[offset + (step - 1U) % size] = 1;
        offset += size;
    }
    return powers;
}

} // namespace

Polynomial::Polynomial(std::size_t dimension) : dimension_(dimension) {}
std::size_t Polynomial::dimension() const noexcept { return dimension_; }
const Polynomial::Terms& Polynomial::terms() const noexcept { return terms_; }

void Polynomial::validate(const Monomial& powers) const {
    if (powers.size() != dimension_) throw std::invalid_argument("Polynomial monomial dimension mismatch");
}

void Polynomial::set(const Monomial& powers, Coefficient coefficient) {
    validate(powers);
    if (coefficient == 0) terms_.erase(powers);
    else terms_[powers] = coefficient;
}

Polynomial::Coefficient Polynomial::get(const Monomial& powers) const {
    validate(powers);
    const auto entry = terms_.find(powers);
    return entry == terms_.end() ? 0 : entry->second;
}

Polynomial::Coefficient Polynomial::constant() const {
    if (terms_.empty()) return 0;
    return get(Monomial(dimension_, 0));
}

Polynomial Polynomial::add(const Polynomial& other) const {
    if (dimension_ != other.dimension_) throw std::invalid_argument("Polynomial dimensions must match");
    Polynomial result = *this;
    for (const auto& [powers, coefficient] : other.terms_) {
        result.set(powers, checked_add(result.get(powers), coefficient));
    }
    return result;
}

Polynomial Polynomial::multiply(const Polynomial& other) const {
    if (dimension_ != other.dimension_) throw std::invalid_argument("Polynomial dimensions must match");
    Polynomial result(dimension_);
    for (const auto& [left, left_coefficient] : terms_) {
        for (const auto& [right, right_coefficient] : other.terms_) {
            Monomial powers(dimension_);
            for (std::size_t index = 0; index < dimension_; ++index) {
                powers[index] = checked_add(left[index], right[index]);
            }
            result.set(powers, checked_add(result.get(powers), checked_multiply(left_coefficient, right_coefficient)));
        }
    }
    return result;
}

std::uint64_t Polynomial::sum_squared_coefficients() const {
    std::uint64_t result = 0;
    constexpr auto high = std::numeric_limits<std::uint64_t>::max();
    for (const auto& [powers, coefficient] : terms_) {
        (void)powers;
        const auto value = magnitude(coefficient);
        if (value != 0 && value > high / value) throw std::overflow_error("Squared coefficient exceeds uint64_t");
        const auto square = value * value;
        if (result > high - square) throw std::overflow_error("Squared coefficient sum exceeds uint64_t");
        result += square;
    }
    return result;
}

std::string Polynomial::to_string() const {
    if (terms_.empty()) return "0";
    std::ostringstream output;
    bool first = true;
    for (const auto& [powers, coefficient] : terms_) {
        if (!first) output << (coefficient < 0 ? " - " : " + ");
        else if (coefficient < 0) output << '-';
        output << magnitude(coefficient);
        for (std::size_t index = 0; index < dimension_; ++index) {
            if (powers[index] != 0) {
                output << "*x" << index + 1U;
                if (powers[index] != 1) output << '^' << powers[index];
            }
        }
        first = false;
    }
    return output.str();
}

Polynomial Polynomial::cyclic_step(std::size_t step, std::size_t max_cycle) {
    if (max_cycle == 0 || max_cycle == std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("Maximum cycle size must be positive and representable");
    }
    // Check the triangular dimension before attempting to allocate its blocks.
    auto left = max_cycle;
    auto right = max_cycle + 1U;
    if (left % 2U == 0) left /= 2U;
    else right /= 2U;
    if (left > std::numeric_limits<std::size_t>::max() / right) {
        throw std::length_error("Cyclic polynomial dimension is too large");
    }
    std::vector<std::size_t> sizes(max_cycle);
    for (std::size_t index = 0; index < max_cycle; ++index) sizes[index] = index + 1U;
    auto powers = cyclic_monomial(sizes, step);
    Polynomial result(powers.size());
    result.set(Monomial(powers.size(), 0), 2);
    result.set(powers, 1);
    for (auto& power : powers) power = -power;
    result.set(powers, 1);
    return result;
}

Polynomial Polynomial::single_cyclic_step(const std::vector<std::size_t>& sizes, std::size_t step) {
    const auto powers = cyclic_monomial(sizes, step);
    Polynomial result(powers.size());
    result.set(Monomial(powers.size(), 0), 1);
    result.set(powers, 1);
    return result;
}

Polynomial Polynomial::read(std::istream& input) {
    const auto dimension = read_size(input, "polynomial dimension");
    const auto count = read_size(input, "polynomial term count");
    Polynomial result(dimension);
    for (std::size_t term = 0; term < count; ++term) {
        Coefficient coefficient = 0;
        if (!(input >> coefficient)) throw std::invalid_argument("Invalid polynomial coefficient");
        Monomial powers;
        for (std::size_t index = 0; index < dimension; ++index) {
            std::int64_t power = 0;
            if (!(input >> power)) throw std::invalid_argument("Invalid polynomial exponent");
            powers.push_back(power);
        }
        result.set(powers, checked_add(result.get(powers), coefficient));
    }
    std::string extra;
    if (input >> extra) throw std::invalid_argument("Unexpected data after polynomial terms");
    return result;
}

void Polynomial::write(std::ostream& output) const {
    output << dimension_ << ' ' << terms_.size() << '\n';
    for (const auto& [powers, coefficient] : terms_) {
        output << coefficient;
        for (auto power : powers) output << ' ' << power;
        output << '\n';
    }
}

} // namespace mfg
