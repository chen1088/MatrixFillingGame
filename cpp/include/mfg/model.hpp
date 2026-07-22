#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mfg {

enum class Cell : std::uint8_t {
    Zero = 0,
    One = 1,
    Blank = 2,
};

struct Coord {
    int row = 0;
    int col = 0;

    auto operator<=>(const Coord&) const = default;
};

class Matrix {
public:
    Matrix(int rows, int cols, Cell initial = Cell::Blank);

    [[nodiscard]] int rows() const noexcept { return rows_; }
    [[nodiscard]] int cols() const noexcept { return cols_; }

    [[nodiscard]] Cell at(Coord position) const;
    Cell& at(Coord position);

    [[nodiscard]] std::vector<Coord> blank_positions() const;
    [[nodiscard]] std::string to_string() const;

private:
    [[nodiscard]] std::size_t index(Coord position) const;

    int rows_;
    int cols_;
    std::vector<Cell> cells_;
};

// A normalized sparse support records exactly the entries fixed to 1.
// Empty rows and columns are deleted while their relative order is preserved.
class SparseSupport {
public:
    explicit SparseSupport(std::vector<Coord> ones);

    [[nodiscard]] int rows() const noexcept { return rows_; }
    [[nodiscard]] int cols() const noexcept { return cols_; }
    [[nodiscard]] std::size_t size() const noexcept { return ones_.size(); }
    [[nodiscard]] const std::vector<Coord>& ones() const noexcept { return ones_; }
    [[nodiscard]] bool contains(Coord position) const;
    [[nodiscard]] std::string key() const;

    auto operator<=>(const SparseSupport&) const = default;

private:
    int rows_ = 0;
    int cols_ = 0;
    std::vector<Coord> ones_;
};

// The unique parent is obtained by deleting the row-major last 1 and then
// deleting any newly empty row or column. row_map/col_map embed parent
// coordinates back into child coordinates.
struct SupportTransition {
    SparseSupport parent;
    SparseSupport child;
    Coord added;
    std::vector<int> row_map;
    std::vector<int> col_map;

    [[nodiscard]] Coord embed(Coord parent_position) const;
    [[nodiscard]] std::string key() const;
};

[[nodiscard]] std::optional<SupportTransition>
parent_transition(const SparseSupport& child);

[[nodiscard]] std::vector<SupportTransition>
canonical_children(const SparseSupport& parent);

[[nodiscard]] std::vector<std::vector<SparseSupport>>
enumerate_support_levels(int max_sparsity);

// Lemma-3/strong normalization: retain the given 1s. A non-1 entry remains
// blank exactly when it is an off-diagonal corner of a rectangle whose
// top-left and bottom-right corners are both retained 1s; all other cells are 0.
[[nodiscard]] Matrix strong_normal_form(const SparseSupport& support);

} // namespace mfg
