#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace mfg {

struct Transition {
    std::size_t from;
    std::size_t to;
    std::size_t symbol;
    bool operator==(const Transition&) const = default;
};

// A complete DFA. The empty language is represented by a rejecting state,
// never an invalid automaton with no start state. Mutators keep both tables
// the same size; newly introduced states reject and transition to state 0.
class Dfa {
public:
    using State = std::size_t;
    using Table = std::vector<std::vector<State>>;
    using CountTable = std::vector<std::vector<std::uint64_t>>;
    using InverseMap = std::vector<std::vector<std::vector<State>>>;

    explicit Dfa(std::size_t states = 1, std::size_t alphabet = 2);
    Dfa(Table transitions, std::vector<bool> accepting, State start = 0);

    [[nodiscard]] std::size_t state_count() const noexcept;
    [[nodiscard]] std::size_t alphabet_size() const noexcept;
    [[nodiscard]] State start_state() const noexcept;
    [[nodiscard]] const Table& transition_table() const noexcept;
    [[nodiscard]] const std::vector<bool>& accepting_states() const noexcept;
    [[nodiscard]] bool is_accepting(State state) const;
    [[nodiscard]] State transition(State from, std::size_t symbol) const;
    void set_transition(Transition transition);
    void set_accepting(State state, bool accepting);
    void set_start(State state);

    [[nodiscard]] bool evaluate(const std::vector<std::size_t>& word) const;
    [[nodiscard]] std::vector<Transition> transitions() const;
    [[nodiscard]] InverseMap inverse_map() const;
    [[nodiscard]] Dfa trim() const;
    [[nodiscard]] Dfa minimize() const;
    [[nodiscard]] Dfa intersect(const Dfa& other) const;

    // Result[state][length], including length zero and max_length.
    // Throws overflow_error rather than returning wrapped word counts.
    [[nodiscard]] CountTable count_stats(std::size_t max_length) const;
    [[nodiscard]] std::vector<std::uint64_t> accepted_counts(std::size_t max_length) const;
    [[nodiscard]] std::string to_dot() const;

    // Plain text: states alphabet start, then one row per state containing
    // accepting(0/1) and one destination per alphabet symbol. Indices are 0-based.
    static Dfa read(std::istream& input);
    void write(std::ostream& output) const;

private:
    Table transitions_;
    std::vector<bool> accepting_;
    State start_ = 0;
    void ensure_state(State state);
};

} // namespace mfg
