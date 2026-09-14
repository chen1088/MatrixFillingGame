#pragma once

#include "mfg/model.hpp"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace mfg {

// Editable raw cells are separate from the maximal configuration shown by the
// application. Every successful change, including masks and file loads, is one
// undoable transaction. Invalid/no-op edits return false and preserve history.
class Document {
public:
    // Limits allocations from interactive edits and untrusted document files;
    // the general Matrix and search engine are not subject to this limit.
    static constexpr std::size_t maximum_cells = 1'000'000;
    explicit Document(int rows = 6, int cols = 6);
    explicit Document(Matrix matrix);

    [[nodiscard]] const Matrix& matrix() const noexcept { return state_.matrix; }
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }
    [[nodiscard]] Matrix maximal_configuration() const;
    bool set_cell(Coord position, Cell value);
    // Legacy left-click behavior: a displayed 1 becomes raw Blank; any other
    // displayed value becomes 1. Explicit 0/Blank editing uses set_cell().
    bool toggle_cell(Coord position);
    bool insert_row(int index);
    bool insert_column(int index);
    bool remove_row(int index);
    bool remove_column(int index);

    [[nodiscard]] bool can_undo() const noexcept { return !undo_.empty(); }
    [[nodiscard]] bool can_redo() const noexcept { return !redo_.empty(); }
    bool undo();
    bool redo();
    bool replace(Matrix matrix);
    bool clear(int rows = 6, int cols = 6);

    bool set_row_ignored(int row, bool ignored);
    bool set_column_ignored(int column, bool ignored);
    bool set_cell_ignored(Coord position, bool ignored);
    [[nodiscard]] bool ignored(Coord position) const;
    [[nodiscard]] const std::set<int>& ignored_rows() const noexcept {
        return state_.rows;
    }
    [[nodiscard]] const std::set<int>& ignored_columns() const noexcept {
        return state_.columns;
    }
    [[nodiscard]] const std::set<Coord>& ignored_cells() const noexcept {
        return state_.cells;
    }

    // rows;cols;r,c,value/...;r,c/...;row/...;column/...
    // Coordinates are zero-based. Unlisted cells are Blank. Loading also
    // accepts rectangular plain text using 0, 1, and b/B/2/./? for Blank.
    // A malformed load throws invalid_argument without changing the document.
    // Allocations beyond maximum_cells also throw invalid_argument atomically.
    [[nodiscard]] std::string serialize() const;
    bool load(const std::string& input);

private:
    struct State {
        Matrix matrix;
        std::set<int> rows;
        std::set<int> columns;
        std::set<Coord> cells;
    };

    [[nodiscard]] bool contains(Coord position) const noexcept;
    bool commit(State state);
    bool resize_axis(int index, bool row_axis, bool insert);
    static State parse(const std::string& input);

    State state_;
    std::vector<State> undo_;
    std::vector<State> redo_;
    std::uint64_t revision_ = 0;
};

} // namespace mfg
