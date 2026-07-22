#include "mfg/model.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mfg {
namespace {

std::size_t checked_area(const int rows, const int cols) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("matrix dimensions must be positive");
    }
    return static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols);
}

} // namespace

Matrix::Matrix(const int rows, const int cols, const Cell initial)
    : rows_(rows), cols_(cols),
      cells_(checked_area(rows, cols), initial) {}

std::size_t Matrix::index(const Coord position) const {
    if (position.row < 0 || position.row >= rows_ ||
        position.col < 0 || position.col >= cols_) {
        throw std::out_of_range("matrix coordinate is out of range");
    }
    return static_cast<std::size_t>(position.row) * static_cast<std::size_t>(cols_) +
           static_cast<std::size_t>(position.col);
}

Cell Matrix::at(const Coord position) const {
    return cells_.at(index(position));
}

Cell& Matrix::at(const Coord position) {
    return cells_.at(index(position));
}

std::vector<Coord> Matrix::blank_positions() const {
    std::vector<Coord> result;
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const Coord position{row, col};
            if (at(position) == Cell::Blank) {
                result.push_back(position);
            }
        }
    }
    return result;
}

std::string Matrix::to_string() const {
    std::ostringstream output;
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            if (col != 0) {
                output << ' ';
            }
            switch (at({row, col})) {
            case Cell::Zero:
                output << '0';
                break;
            case Cell::One:
                output << '1';
                break;
            case Cell::Blank:
                output << 'b';
                break;
            }
        }
        if (row + 1 != rows_) {
            output << '\n';
        }
    }
    return output.str();
}

SparseSupport::SparseSupport(std::vector<Coord> ones) {
    if (ones.empty()) {
        throw std::invalid_argument("a sparse support must contain at least one 1");
    }
    for (const Coord position : ones) {
        if (position.row < 0 || position.col < 0) {
            throw std::invalid_argument("support coordinates must be nonnegative");
        }
    }

    std::sort(ones.begin(), ones.end());
    if (std::adjacent_find(ones.begin(), ones.end()) != ones.end()) {
        throw std::invalid_argument("a sparse support cannot repeat a coordinate");
    }

    std::vector<int> used_rows;
    std::vector<int> used_cols;
    used_rows.reserve(ones.size());
    used_cols.reserve(ones.size());
    for (const Coord position : ones) {
        used_rows.push_back(position.row);
        used_cols.push_back(position.col);
    }
    std::sort(used_rows.begin(), used_rows.end());
    used_rows.erase(std::unique(used_rows.begin(), used_rows.end()), used_rows.end());
    std::sort(used_cols.begin(), used_cols.end());
    used_cols.erase(std::unique(used_cols.begin(), used_cols.end()), used_cols.end());

    std::map<int, int> row_rank;
    std::map<int, int> col_rank;
    for (std::size_t index = 0; index < used_rows.size(); ++index) {
        row_rank.emplace(used_rows[index], static_cast<int>(index));
    }
    for (std::size_t index = 0; index < used_cols.size(); ++index) {
        col_rank.emplace(used_cols[index], static_cast<int>(index));
    }

    ones_.reserve(ones.size());
    for (const Coord position : ones) {
        ones_.push_back({row_rank.at(position.row), col_rank.at(position.col)});
    }
    std::sort(ones_.begin(), ones_.end());
    rows_ = static_cast<int>(used_rows.size());
    cols_ = static_cast<int>(used_cols.size());
}

bool SparseSupport::contains(const Coord position) const {
    return std::binary_search(ones_.begin(), ones_.end(), position);
}

std::string SparseSupport::key() const {
    std::ostringstream output;
    output << rows_ << 'x' << cols_ << ':';
    for (const Coord position : ones_) {
        output << position.row << ',' << position.col << ';';
    }
    return output.str();
}

Coord SupportTransition::embed(const Coord parent_position) const {
    if (parent_position.row < 0 ||
        parent_position.row >= static_cast<int>(row_map.size()) ||
        parent_position.col < 0 ||
        parent_position.col >= static_cast<int>(col_map.size())) {
        throw std::out_of_range("parent coordinate is out of transition range");
    }
    return {row_map.at(static_cast<std::size_t>(parent_position.row)),
            col_map.at(static_cast<std::size_t>(parent_position.col))};
}

std::string SupportTransition::key() const {
    std::ostringstream output;
    output << parent.key() << " -> " << child.key() << " +(" << added.row << ','
           << added.col << ')';
    return output.str();
}

std::optional<SupportTransition> parent_transition(const SparseSupport& child) {
    if (child.size() <= 1U) {
        return std::nullopt;
    }

    const Coord added = child.ones().back();
    std::vector<Coord> remaining(child.ones().begin(), child.ones().end() - 1);

    std::set<int> used_rows;
    std::set<int> used_cols;
    for (const Coord position : remaining) {
        used_rows.insert(position.row);
        used_cols.insert(position.col);
    }

    std::vector<int> row_map(used_rows.begin(), used_rows.end());
    std::vector<int> col_map(used_cols.begin(), used_cols.end());
    SparseSupport parent(std::move(remaining));
    return SupportTransition{std::move(parent), child, added,
                             std::move(row_map), std::move(col_map)};
}

std::vector<SupportTransition> canonical_children(const SparseSupport& parent) {
    std::map<std::string, SupportTransition> unique;

    const auto consider = [&parent, &unique](std::vector<Coord> ones,
                                              const Coord candidate_added) {
        if (std::find(ones.begin(), ones.end(), candidate_added) != ones.end()) {
            return;
        }
        ones.push_back(candidate_added);
        SparseSupport child(std::move(ones));
        const auto transition = parent_transition(child);
        if (!transition.has_value() || transition->parent != parent ||
            transition->added != candidate_added) {
            return;
        }
        unique.emplace(child.key(), *transition);
    };

    // Add a 1 in the existing rectangle.
    for (int row = 0; row < parent.rows(); ++row) {
        for (int col = 0; col < parent.cols(); ++col) {
            consider(parent.ones(), {row, col});
        }
    }

    // Insert a new ordered row, then add its unique initial 1.
    for (int inserted_row = 0; inserted_row <= parent.rows(); ++inserted_row) {
        std::vector<Coord> shifted;
        shifted.reserve(parent.size());
        for (Coord position : parent.ones()) {
            if (position.row >= inserted_row) {
                ++position.row;
            }
            shifted.push_back(position);
        }
        for (int col = 0; col < parent.cols(); ++col) {
            consider(shifted, {inserted_row, col});
        }
    }

    // Insert a new ordered column, then add its unique initial 1.
    for (int inserted_col = 0; inserted_col <= parent.cols(); ++inserted_col) {
        std::vector<Coord> shifted;
        shifted.reserve(parent.size());
        for (Coord position : parent.ones()) {
            if (position.col >= inserted_col) {
                ++position.col;
            }
            shifted.push_back(position);
        }
        for (int row = 0; row < parent.rows(); ++row) {
            consider(shifted, {row, inserted_col});
        }
    }

    // Insert a new ordered row and column whose intersection contains the new 1.
    for (int inserted_row = 0; inserted_row <= parent.rows(); ++inserted_row) {
        for (int inserted_col = 0; inserted_col <= parent.cols(); ++inserted_col) {
            std::vector<Coord> shifted;
            shifted.reserve(parent.size());
            for (Coord position : parent.ones()) {
                if (position.row >= inserted_row) {
                    ++position.row;
                }
                if (position.col >= inserted_col) {
                    ++position.col;
                }
                shifted.push_back(position);
            }
            consider(shifted, {inserted_row, inserted_col});
        }
    }

    std::vector<SupportTransition> result;
    result.reserve(unique.size());
    for (auto& [unused_key, transition] : unique) {
        static_cast<void>(unused_key);
        result.push_back(std::move(transition));
    }
    return result;
}

std::vector<std::vector<SparseSupport>> enumerate_support_levels(const int max_sparsity) {
    if (max_sparsity < 1) {
        throw std::invalid_argument("maximum sparsity must be positive");
    }

    std::vector<std::vector<SparseSupport>> levels;
    levels.reserve(static_cast<std::size_t>(max_sparsity));
    levels.push_back({SparseSupport({{0, 0}})});

    for (int sparsity = 2; sparsity <= max_sparsity; ++sparsity) {
        std::map<std::string, SparseSupport> next;
        for (const SparseSupport& parent : levels.back()) {
            for (const SupportTransition& transition : canonical_children(parent)) {
                next.emplace(transition.child.key(), transition.child);
            }
        }
        std::vector<SparseSupport> level;
        level.reserve(next.size());
        for (auto& [unused_key, support] : next) {
            static_cast<void>(unused_key);
            level.push_back(std::move(support));
        }
        levels.push_back(std::move(level));
    }
    return levels;
}

Matrix strong_normal_form(const SparseSupport& support) {
    Matrix result(support.rows(), support.cols(), Cell::Zero);
    for (const Coord position : support.ones()) {
        result.at(position) = Cell::One;
    }

    const auto& ones = support.ones();
    for (std::size_t first = 0; first < ones.size(); ++first) {
        for (std::size_t second = first + 1; second < ones.size(); ++second) {
            const Coord northwest = ones[first];
            const Coord southeast = ones[second];
            if (northwest.row >= southeast.row || northwest.col >= southeast.col) {
                continue;
            }
            const Coord northeast{northwest.row, southeast.col};
            const Coord southwest{southeast.row, northwest.col};
            if (result.at(northeast) != Cell::One) {
                result.at(northeast) = Cell::Blank;
            }
            if (result.at(southwest) != Cell::One) {
                result.at(southwest) = Cell::Blank;
            }
        }
    }
    return result;
}

} // namespace mfg
