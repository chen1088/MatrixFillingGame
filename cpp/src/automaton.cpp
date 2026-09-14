#include "mfg/automaton.hpp"

#include <algorithm>
#include <istream>
#include <limits>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mfg {
namespace {

std::uint64_t add_count(std::uint64_t left, std::uint64_t right) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        throw std::overflow_error("DFA word count exceeds uint64_t");
    }
    return left + right;
}

std::size_t read_index(std::istream& input, const char* description) {
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

} // namespace

Dfa::Dfa(std::size_t states, std::size_t alphabet) {
    if (states == 0 || alphabet == 0) {
        throw std::invalid_argument("A DFA needs at least one state and one alphabet symbol");
    }
    transitions_.assign(states, std::vector<State>(alphabet, 0));
    accepting_.assign(states, false);
}

Dfa::Dfa(Table transitions, std::vector<bool> accepting, State start)
    : transitions_(std::move(transitions)), accepting_(std::move(accepting)), start_(start) {
    if (transitions_.empty() || transitions_.front().empty() ||
        accepting_.size() != transitions_.size() || start_ >= transitions_.size()) {
        throw std::invalid_argument("Invalid DFA dimensions or start state");
    }
    for (const auto& row : transitions_) {
        if (row.size() != alphabet_size()) {
            throw std::invalid_argument("DFA transition rows have unequal alphabet sizes");
        }
        for (const State target : row) {
            if (target >= state_count()) {
                throw std::invalid_argument("DFA transition destination is out of range");
            }
        }
    }
}

std::size_t Dfa::state_count() const noexcept { return transitions_.size(); }
std::size_t Dfa::alphabet_size() const noexcept { return transitions_.front().size(); }
Dfa::State Dfa::start_state() const noexcept { return start_; }
const Dfa::Table& Dfa::transition_table() const noexcept { return transitions_; }
const std::vector<bool>& Dfa::accepting_states() const noexcept { return accepting_; }
bool Dfa::is_accepting(State state) const { return accepting_.at(state); }
Dfa::State Dfa::transition(State from, std::size_t symbol) const { return transitions_.at(from).at(symbol); }

void Dfa::ensure_state(State state) {
    if (state < state_count()) return;
    if (state == std::numeric_limits<State>::max()) {
        throw std::length_error("DFA state count is too large");
    }
    // Allocate both replacement tables before changing this automaton.
    auto transitions = transitions_;
    auto accepting = accepting_;
    transitions.resize(state + 1U, std::vector<State>(alphabet_size(), 0));
    accepting.resize(state + 1U, false);
    transitions_.swap(transitions);
    accepting_.swap(accepting);
}

void Dfa::set_transition(Transition edge) {
    if (edge.symbol >= alphabet_size()) {
        throw std::out_of_range("DFA alphabet symbol is out of range");
    }
    ensure_state(std::max(edge.from, edge.to));
    transitions_[edge.from][edge.symbol] = edge.to;
}

void Dfa::set_accepting(State state, bool accepting) {
    ensure_state(state);
    accepting_[state] = accepting;
}

void Dfa::set_start(State state) {
    if (state >= state_count()) throw std::out_of_range("DFA start state is out of range");
    start_ = state;
}

bool Dfa::evaluate(const std::vector<std::size_t>& word) const {
    State current = start_;
    for (const auto symbol : word) current = transition(current, symbol);
    return accepting_[current];
}

std::vector<Transition> Dfa::transitions() const {
    std::vector<Transition> result;
    for (State from = 0; from < state_count(); ++from) {
        for (std::size_t symbol = 0; symbol < alphabet_size(); ++symbol) {
            result.push_back({from, transitions_[from][symbol], symbol});
        }
    }
    return result;
}

Dfa::InverseMap Dfa::inverse_map() const {
    InverseMap result(state_count(), std::vector<std::vector<State>>(alphabet_size()));
    for (State from = 0; from < state_count(); ++from) {
        for (std::size_t symbol = 0; symbol < alphabet_size(); ++symbol) {
            result[transitions_[from][symbol]][symbol].push_back(from);
        }
    }
    return result;
}

Dfa Dfa::trim() const {
    const State absent = std::numeric_limits<State>::max();
    std::vector<State> mapping(state_count(), absent);
    std::vector<State> queue{start_};
    mapping[start_] = 0;
    for (std::size_t index = 0; index < queue.size(); ++index) {
        for (const State next : transitions_[queue[index]]) {
            if (mapping[next] == absent) {
                mapping[next] = queue.size();
                queue.push_back(next);
            }
        }
    }
    Table table(queue.size(), std::vector<State>(alphabet_size()));
    std::vector<bool> accepting(queue.size());
    for (State state = 0; state < queue.size(); ++state) {
        accepting[state] = accepting_[queue[state]];
        for (std::size_t symbol = 0; symbol < alphabet_size(); ++symbol) {
            table[state][symbol] = mapping[transitions_[queue[state]][symbol]];
        }
    }
    return Dfa(std::move(table), std::move(accepting));
}

Dfa Dfa::minimize() const {
    const Dfa reachable = trim();
    const auto n = reachable.state_count();
    std::vector<State> classes(n);
    for (State state = 0; state < n; ++state) classes[state] = reachable.accepting_[state] ? 1U : 0U;

    // Moore refinement uses signatures of the previous partition. Assigning
    // class IDs in first-state order makes the fixed-point check deterministic.
    for (;;) {
        std::map<std::vector<State>, State> signatures;
        std::vector<State> refined(n);
        for (State state = 0; state < n; ++state) {
            std::vector<State> signature{classes[state]};
            for (const State next : reachable.transitions_[state]) signature.push_back(classes[next]);
            const auto [entry, inserted] = signatures.emplace(std::move(signature), signatures.size());
            (void)inserted;
            refined[state] = entry->second;
        }
        if (refined == classes) break;
        classes.swap(refined);
    }
    const State class_count = *std::max_element(classes.begin(), classes.end()) + 1U;
    Table table(class_count, std::vector<State>(alphabet_size()));
    std::vector<bool> accepting(class_count);
    for (State state = 0; state < n; ++state) {
        accepting[classes[state]] = reachable.accepting_[state];
        for (std::size_t symbol = 0; symbol < alphabet_size(); ++symbol) {
            table[classes[state]][symbol] = classes[reachable.transitions_[state][symbol]];
        }
    }
    return Dfa(std::move(table), std::move(accepting), classes[0]).trim();
}

Dfa Dfa::intersect(const Dfa& other) const {
    if (alphabet_size() != other.alphabet_size()) {
        throw std::invalid_argument("DFA intersection requires equal alphabet sizes");
    }
    using Pair = std::pair<State, State>;
    std::vector<Pair> queue{{start_, other.start_}};
    std::map<Pair, State> index{{queue.front(), 0}};
    Table table;
    std::vector<bool> accepting;
    for (State current = 0; current < queue.size(); ++current) {
        const auto [left, right] = queue[current];
        std::vector<State> row;
        row.reserve(alphabet_size());
        accepting.push_back(accepting_[left] && other.accepting_[right]);
        for (std::size_t symbol = 0; symbol < alphabet_size(); ++symbol) {
            const Pair next{transitions_[left][symbol], other.transitions_[right][symbol]};
            const auto [entry, inserted] = index.emplace(next, queue.size());
            if (inserted) queue.push_back(next);
            row.push_back(entry->second);
        }
        table.push_back(std::move(row));
    }
    return Dfa(std::move(table), std::move(accepting));
}

Dfa::CountTable Dfa::count_stats(std::size_t max_length) const {
    if (max_length == std::numeric_limits<std::size_t>::max()) {
        throw std::length_error("DFA counting length is too large");
    }
    CountTable result(state_count(), std::vector<std::uint64_t>(max_length + 1U));
    result[start_][0] = 1;
    for (std::size_t length = 0; length < max_length; ++length) {
        for (State from = 0; from < state_count(); ++from) {
            for (const State to : transitions_[from]) {
                result[to][length + 1U] = add_count(result[to][length + 1U], result[from][length]);
            }
        }
    }
    return result;
}

std::vector<std::uint64_t> Dfa::accepted_counts(std::size_t max_length) const {
    const auto stats = count_stats(max_length);
    std::vector<std::uint64_t> result(max_length + 1U);
    for (State state = 0; state < state_count(); ++state) {
        if (accepting_[state]) {
            for (std::size_t length = 0; length <= max_length; ++length) {
                result[length] = add_count(result[length], stats[state][length]);
            }
        }
    }
    return result;
}

std::string Dfa::to_dot() const {
    std::ostringstream out;
    out << "digraph DFA {\n  rankdir=LR;\n  start [shape=point];\n  start -> q" << start_ << ";\n";
    for (State state = 0; state < state_count(); ++state) {
        out << "  q" << state << " [label=\"" << state << "\", shape="
            << (accepting_[state] ? "doublecircle" : "circle") << "];\n";
    }
    for (const auto& edge : transitions()) {
        out << "  q" << edge.from << " -> q" << edge.to << " [label=\"" << edge.symbol << "\"];\n";
    }
    out << "}\n";
    return out.str();
}

Dfa Dfa::read(std::istream& input) {
    const auto states = read_index(input, "state count");
    const auto alphabet = read_index(input, "alphabet size");
    const auto start = read_index(input, "start state");
    if (states == 0 || alphabet == 0 || start >= states) {
        throw std::invalid_argument("Invalid DFA dimensions or start state");
    }
    // Accumulate actual input rather than allocating from an untrusted header.
    // A truncated file claiming billions of states/symbols fails immediately.
    Table table;
    std::vector<bool> flags;
    for (State state = 0; state < states; ++state) {
        const auto accepting = read_index(input, "accepting flag");
        if (accepting > 1) throw std::invalid_argument("Accepting flag must be 0 or 1");
        flags.push_back(accepting != 0);
        std::vector<State> row;
        for (std::size_t symbol = 0; symbol < alphabet; ++symbol) {
            const auto to = read_index(input, "transition destination");
            if (to >= states) throw std::invalid_argument("DFA transition destination is out of range");
            row.push_back(to);
        }
        table.push_back(std::move(row));
    }
    std::string extra;
    if (input >> extra) throw std::invalid_argument("Unexpected data after DFA table");
    return Dfa(std::move(table), std::move(flags), start);
}

void Dfa::write(std::ostream& output) const {
    output << state_count() << ' ' << alphabet_size() << ' ' << start_ << '\n';
    for (State state = 0; state < state_count(); ++state) {
        output << (accepting_[state] ? 1 : 0);
        for (const auto to : transitions_[state]) output << ' ' << to;
        output << '\n';
    }
}

} // namespace mfg
