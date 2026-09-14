#include "mfg/automaton.hpp"
#include "mfg/polynomial.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

template <typename Exception = std::exception, typename Function>
void throws(Function operation, const char* message) {
    bool expected = false;
    try { operation(); }
    catch (const Exception&) { expected = true; }
    catch (...) {}
    check(expected, message);
}

// Explore the pair automaton directly, independently of intersect/minimize.
bool equivalent(const mfg::Dfa& left, const mfg::Dfa& right,
                std::size_t left_start, std::size_t right_start) {
    using Pair = std::pair<std::size_t, std::size_t>;
    std::vector<Pair> queue{{left_start, right_start}};
    std::set<Pair> visited{queue.front()};
    for (std::size_t index = 0; index < queue.size(); ++index) {
        const auto [a, b] = queue[index];
        if (left.is_accepting(a) != right.is_accepting(b)) return false;
        for (std::size_t symbol = 0; symbol < left.alphabet_size(); ++symbol) {
            const Pair next{left.transition(a, symbol), right.transition(b, symbol)};
            if (visited.insert(next).second) queue.push_back(next);
        }
    }
    return true;
}

void test_dfa_invariants() {
    mfg::Dfa grown;
    grown.set_transition({0, 5, 1});
    check(grown.state_count() == 6 && grown.accepting_states().size() == 6,
          "transition growth also grows accepting-state storage");
    check(!grown.evaluate({1}), "new states safely reject by default");
    grown.set_accepting(8, true);
    check(grown.state_count() == 9 && grown.transition(8, 1) == 0,
          "accepting-state growth also grows transition storage");
    throws<std::out_of_range>([&] { grown.set_transition({0, 100, 2}); },
                              "out-of-alphabet transition is rejected before growing state storage");
    check(grown.state_count() == 9, "invalid transition does not mutate DFA");
    throws<std::out_of_range>([&] { (void)grown.evaluate({2}); }, "invalid word symbol is rejected");
    throws<std::invalid_argument>([] { (void)mfg::Dfa(0, 2); }, "zero-state DFA is rejected");
    throws<std::invalid_argument>([] { (void)mfg::Dfa({{0}, {0, 1}}, {true, false}); },
                                  "ragged transition table is rejected");
    throws<std::invalid_argument>([] { (void)mfg::Dfa({{1}}, {true}); },
                                  "out-of-range transition is rejected");
}

void test_dfa_trim_minimize() {
    const mfg::Dfa source({{1, 2}, {1, 3}, {1, 3}, {3, 3}, {4, 4}},
                          {false, true, true, false, true});
    const auto trimmed = source.trim();
    const auto minimal = source.minimize();
    check(trimmed.state_count() == 4, "trim removes unreachable states only");
    check(minimal.state_count() == 3, "minimization merges equivalent reachable states");
    check(source.state_count() == 5 && source.transition(0, 1) == 2,
          "minimize does not mutate its input");
    check(equivalent(source, minimal, source.start_state(), minimal.start_state()),
          "minimized DFA recognizes exactly the original language");
    const mfg::Dfa accepting({{1, 1}, {0, 0}}, {true, true});
    const mfg::Dfa rejecting({{1, 1}, {0, 0}}, {false, false});
    check(accepting.minimize().state_count() == 1 && accepting.minimize().evaluate({}),
          "all-accepting DFA minimizes to one accepting state");
    check(rejecting.minimize().state_count() == 1 && !rejecting.minimize().evaluate({}),
          "all-rejecting DFA minimizes to one rejecting state");
    const mfg::Dfa nonzero_start({{0, 0}, {2, 2}, {2, 1}}, {false, true, false}, 1);
    check(nonzero_start.trim().state_count() == 2 && nonzero_start.trim().evaluate({}),
          "trim starts at the designated nonzero start state");

    std::mt19937 random(431);
    for (std::size_t alphabet = 1; alphabet <= 3; ++alphabet) {
        for (std::size_t n = 1; n <= 7; ++n) {
            for (int trial = 0; trial < 30; ++trial) {
                mfg::Dfa::Table table(n, std::vector<std::size_t>(alphabet));
                std::vector<bool> flags(n);
                for (std::size_t state = 0; state < n; ++state) {
                    flags[state] = random() % 2U != 0;
                    for (auto& next : table[state]) next = random() % n;
                }
                const mfg::Dfa original(table, flags, random() % n);
                const auto minimized = original.minimize();
                check(equivalent(original, minimized, original.start_state(), minimized.start_state()),
                      "random DFA language equivalence is preserved for all word lengths");
                for (std::size_t left = 0; left < minimized.state_count(); ++left) {
                    for (std::size_t right = left + 1; right < minimized.state_count(); ++right) {
                        check(!equivalent(minimized, minimized, left, right),
                              "every pair of minimized states has a distinguishing word");
                    }
                }
            }
        }
    }
}

void test_dfa_intersection_counts_dot() {
    const mfg::Dfa even_zero({{1, 0, 0}, {0, 1, 1}}, {true, false});
    const mfg::Dfa even_two({{0, 0, 1}, {1, 1, 0}}, {true, false});
    const auto product = even_zero.intersect(even_two);
    check(product.alphabet_size() == 3 && product.state_count() == 4,
          "intersection handles nonbinary alphabets and dense reachable states");
    const auto stats = product.count_stats(7);
    const auto accepted = product.accepted_counts(7);
    std::uint64_t words = 1;
    for (std::size_t length = 0; length <= 7; ++length) {
        std::uint64_t enumerated = 0;
        for (std::uint64_t encoding = 0; encoding < words; ++encoding) {
            auto value = encoding;
            std::vector<std::size_t> word(length);
            for (auto& symbol : word) { symbol = value % 3U; value /= 3U; }
            const bool expected = even_zero.evaluate(word) && even_two.evaluate(word);
            check(product.evaluate(word) == expected, "synchronized intersection accepts exactly both languages");
            if (expected) ++enumerated;
        }
        std::uint64_t total = 0;
        for (const auto& row : stats) total += row[length];
        check(total == words && accepted[length] == enumerated,
              "per-state and accepted counts agree with exhaustive enumeration");
        words *= 3U;
    }
    check(even_zero.intersect(even_zero).state_count() == 2,
          "intersection materializes only reachable pairs");
    throws<std::invalid_argument>([&] { (void)even_zero.intersect(mfg::Dfa()); },
                                  "intersection rejects incompatible alphabets");
    mfg::Dfa full;
    full.set_accepting(0, true);
    check(full.accepted_counts(63)[63] == (std::uint64_t{1} << 63U),
          "64-bit counts remain exact beyond Java signed int range");
    throws<std::overflow_error>([&] { (void)full.count_stats(64); },
                               "DFA count overflow is explicit");
    const auto inverse = even_zero.inverse_map();
    check(inverse[0][0] == std::vector<std::size_t>{1} && inverse[0][2] == std::vector<std::size_t>{0},
          "inverse map includes every alphabet symbol");
    const auto dot = even_zero.to_dot();
    check(dot.find("q0 [label=\"0\", shape=doublecircle]") != std::string::npos &&
              dot.find("q1 [label=\"1\", shape=circle]") != std::string::npos &&
              dot.find("[label=\"2\"]") != std::string::npos,
          "DOT uses each state's own acceptance and includes all symbols");
    std::stringstream serialized;
    product.write(serialized);
    const auto restored = mfg::Dfa::read(serialized);
    check(restored.transition_table() == product.transition_table() && restored.accepting_states() == product.accepting_states(),
          "DFA serialization round trips without losing transitions or acceptance");
    throws<std::invalid_argument>([] { std::istringstream input("-1 2 0"); (void)mfg::Dfa::read(input); },
                                  "negative state counts are rejected");
    throws<std::invalid_argument>([] { std::istringstream input("999999999999 999999999999 0\n0\n"); (void)mfg::Dfa::read(input); },
                                  "huge malformed DFA header fails without allocating its claimed table");
}

void test_polynomial() {
    mfg::Polynomial left(2), right(2);
    left.set({200, 0}, 7);
    left.set({0, 0}, 3);
    right.set({0, -2}, -5);
    const auto sum = left.add(right);
    check(sum.get({200, 0}) == 7 && sum.get({0, -2}) == -5 && sum.constant() == 3,
          "addition retains left-only, right-only, and constant terms");
    right.set({200, 0}, -7);
    check(left.add(right).terms().size() == 2 && left.add(right).get({200, 0}) == 0,
          "equal large exponents combine by value and cancellation removes zero terms");
    const auto cyclic = mfg::Polynomial::cyclic_step(1, 1);
    const auto square = cyclic.multiply(cyclic);
    check(square.constant() == 6 && square.get({-2}) == 1 && square.get({-1}) == 4 &&
              square.get({1}) == 4 && square.get({2}) == 1 && square.terms().size() == 5,
          "Laurent multiplication computes (2+x+x^-1)^2 exactly");
    check(cyclic.sum_squared_coefficients() == 6, "sum of squared coefficients is exact");
    const auto single = mfg::Polynomial::single_cyclic_step({2, 3}, 4);
    check(single.dimension() == 5 && single.get({0, 1, 1, 0, 0}) == 1 && single.constant() == 1,
          "cyclic single-step generation uses correct 1-based step residues");
    const auto cyclic_blocks = mfg::Polynomial::cyclic_step(5, 3);
    check(cyclic_blocks.dimension() == 6 && cyclic_blocks.get({1, 1, 0, 0, 1, 0}) == 1,
          "all-cycle generator preserves triangular block layout");
    mfg::Polynomial constant;
    constant.set({}, -3);
    check(constant.multiply(constant).constant() == 9, "zero-dimensional constant polynomials work");
    throws<std::invalid_argument>([&] { (void)left.add(constant); }, "mismatched dimensions are rejected");
    throws<std::invalid_argument>([&] { left.set({1}, 3); }, "malformed monomial is rejected");
    throws<std::invalid_argument>([] { (void)mfg::Polynomial::single_cyclic_step({0}, 1); }, "zero cycle size is rejected");
    std::stringstream serialized;
    sum.write(serialized);
    check(mfg::Polynomial::read(serialized) == sum, "polynomial serialization round trips");
    std::istringstream repeated("1 3\n2 200\n3 200\n-1 -1\n");
    check(mfg::Polynomial::read(repeated).get({200}) == 5, "repeated input monomials are added");
    throws<std::invalid_argument>([] { std::istringstream input("999999999999 1\n1\n"); (void)mfg::Polynomial::read(input); },
                                  "huge malformed polynomial header fails without allocating its claimed exponent vector");

    constexpr auto maximum = std::numeric_limits<std::int64_t>::max();
    constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
    mfg::Polynomial extreme(1), unit(1);
    extreme.set({0}, maximum);
    unit.set({0}, 1);
    throws<std::overflow_error>([&] { (void)extreme.add(unit); }, "coefficient addition overflow is explicit");
    extreme.set({0}, minimum);
    unit.set({0}, -1);
    throws<std::overflow_error>([&] { (void)extreme.multiply(unit); }, "minimum signed coefficient times -1 is rejected");
    check(extreme.to_string() == "-9223372036854775808", "minimum signed coefficient formats without undefined behavior");
    extreme.set({0}, -maximum);
    check(extreme.multiply(unit).constant() == maximum, "largest representable negative product stays valid");
    extreme.set({0}, 0);
    extreme.set({maximum}, 1);
    unit.set({0}, 0);
    unit.set({1}, 1);
    throws<std::overflow_error>([&] { (void)extreme.multiply(unit); }, "exponent overflow is explicit");
    unit.set({1}, std::int64_t{1} << 32U);
    throws<std::overflow_error>([&] { (void)unit.sum_squared_coefficients(); }, "squared coefficient overflow is explicit");
}

} // namespace

int main() {
    test_dfa_invariants();
    test_dfa_trim_minimize();
    test_dfa_intersection_counts_dot();
    test_polynomial();
    if (failures != 0) {
        std::cerr << failures << " utility test(s) failed\n";
        return 1;
    }
    std::cout << "all utility tests passed\n";
    return 0;
}
