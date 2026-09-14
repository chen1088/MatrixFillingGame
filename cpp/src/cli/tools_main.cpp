#include "mfg/automaton.hpp"
#include "mfg/polynomial.hpp"

#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* usage = R"(Usage: mfg_tools COMMAND ARGUMENTS

DFA commands:
  dfa-dot FILE                    Export a graph in Graphviz DOT format
  dfa-trim FILE                   Remove states unreachable from the start
  dfa-minimize FILE               Minimize without changing the input file
  dfa-intersect LEFT RIGHT        Reachable synchronized intersection
  dfa-evaluate FILE [SYMBOL ...]  Print accepted or rejected (no symbols = empty word)
  dfa-count FILE MAX_LENGTH       CSV: length,accepted,state_0,...,state_n

Polynomial commands:
  poly-cyclic STEP MAX_CYCLE      Generate 2 + monomial + inverse(monomial)
  poly-single STEP SIZE [SIZE...] Generate 1 + monomial for cyclic blocks
  poly-add LEFT RIGHT             Add Laurent polynomials
  poly-multiply LEFT RIGHT        Multiply Laurent polynomials
  poly-print FILE                 Print a readable algebraic expression
  poly-constant FILE              Print the constant coefficient
  poly-square-sum FILE            Print sum of squared coefficients

All results go to stdout. Input files are never overwritten.
DFA file: states alphabet start, then one row per state containing a 0/1
accepting flag followed by one destination per symbol. All indices are 0-based.
Polynomial file: dimension term_count, then coefficient followed by one signed
exponent per dimension for every term. Duplicate monomials are summed.
Counts are checked uint64_t; coefficients and exponents are checked int64_t.
Overflow and invalid input produce an error and nonzero exit status.
)";

std::size_t number(const char* argument) {
    const std::string text(argument);
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument("Expected a nonnegative integer: " + text);
    }
    std::size_t result = 0;
    for (char digit : text) {
        const auto value = static_cast<std::size_t>(digit - '0');
        if (result > (std::numeric_limits<std::size_t>::max() - value) / 10U) {
            throw std::out_of_range("Integer is too large: " + text);
        }
        result = result * 10U + value;
    }
    return result;
}

template <typename T>
T load(const char* filename) {
    std::ifstream input(filename);
    if (!input) throw std::runtime_error(std::string("Cannot open input file: ") + filename);
    return T::read(input);
}

void require_arguments(bool valid) {
    if (!valid) throw std::invalid_argument("Incorrect arguments; use mfg_tools --help");
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1 || (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"))) {
            std::cout << usage;
            return 0;
        }
        const std::string command(argv[1]);
        if (command == "dfa-dot" || command == "dfa-trim" || command == "dfa-minimize") {
            require_arguments(argc == 3);
            const auto dfa = load<mfg::Dfa>(argv[2]);
            if (command == "dfa-dot") std::cout << dfa.to_dot();
            else if (command == "dfa-trim") dfa.trim().write(std::cout);
            else dfa.minimize().write(std::cout);
        } else if (command == "dfa-intersect") {
            require_arguments(argc == 4);
            load<mfg::Dfa>(argv[2]).intersect(load<mfg::Dfa>(argv[3])).write(std::cout);
        } else if (command == "dfa-evaluate") {
            require_arguments(argc >= 3);
            const auto dfa = load<mfg::Dfa>(argv[2]);
            std::vector<std::size_t> word;
            for (int index = 3; index < argc; ++index) word.push_back(number(argv[index]));
            std::cout << (dfa.evaluate(word) ? "accepted\n" : "rejected\n");
        } else if (command == "dfa-count") {
            require_arguments(argc == 4);
            const auto dfa = load<mfg::Dfa>(argv[2]);
            const auto length_limit = number(argv[3]);
            const auto stats = dfa.count_stats(length_limit);
            // Compute and validate the accepted sums before printing any CSV.
            std::vector<std::uint64_t> accepted(length_limit + 1U);
            for (std::size_t state = 0; state < dfa.state_count(); ++state) {
                if (!dfa.is_accepting(state)) continue;
                for (std::size_t length = 0; length <= length_limit; ++length) {
                    if (accepted[length] > std::numeric_limits<std::uint64_t>::max() - stats[state][length]) {
                        throw std::overflow_error("Accepted word count exceeds uint64_t");
                    }
                    accepted[length] += stats[state][length];
                }
            }
            std::cout << "length,accepted";
            for (std::size_t state = 0; state < dfa.state_count(); ++state) std::cout << ",state_" << state;
            std::cout << '\n';
            for (std::size_t length = 0; length <= length_limit; ++length) {
                std::cout << length << ',' << accepted[length];
                for (const auto& row : stats) std::cout << ',' << row[length];
                std::cout << '\n';
            }
        } else if (command == "poly-cyclic") {
            require_arguments(argc == 4);
            mfg::Polynomial::cyclic_step(number(argv[2]), number(argv[3])).write(std::cout);
        } else if (command == "poly-single") {
            require_arguments(argc >= 4);
            std::vector<std::size_t> sizes;
            for (int index = 3; index < argc; ++index) sizes.push_back(number(argv[index]));
            mfg::Polynomial::single_cyclic_step(sizes, number(argv[2])).write(std::cout);
        } else if (command == "poly-add" || command == "poly-multiply") {
            require_arguments(argc == 4);
            const auto left = load<mfg::Polynomial>(argv[2]);
            const auto right = load<mfg::Polynomial>(argv[3]);
            (command == "poly-add" ? left.add(right) : left.multiply(right)).write(std::cout);
        } else if (command == "poly-print" || command == "poly-constant" || command == "poly-square-sum") {
            require_arguments(argc == 3);
            const auto polynomial = load<mfg::Polynomial>(argv[2]);
            if (command == "poly-print") std::cout << polynomial.to_string() << '\n';
            else if (command == "poly-constant") std::cout << polynomial.constant() << '\n';
            else std::cout << polynomial.sum_squared_coefficients() << '\n';
        } else {
            throw std::invalid_argument("Unknown command: " + command + "; use mfg_tools --help");
        }
        if (!std::cout) throw std::runtime_error("Unable to write command output");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "mfg_tools: " << error.what() << '\n';
        return 1;
    }
}
