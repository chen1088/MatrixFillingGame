#include "mfg/document.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mfg {
namespace {

bool valid_cell(const Cell cell) {
    return cell == Cell::Zero || cell == Cell::One || cell == Cell::Blank;
}

void validate_size(const int rows, const int cols) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("matrix dimensions must be positive");
    }
    if (static_cast<std::size_t>(rows) >
        Document::maximum_cells / static_cast<std::size_t>(cols)) {
        throw std::invalid_argument("editable documents are limited to 1,000,000 cells");
    }
}

Matrix checked_matrix(const int rows, const int cols) {
    validate_size(rows, cols);
    return Matrix(rows, cols);
}

void validate_matrix(const Matrix& matrix) {
    validate_size(matrix.rows(), matrix.cols());
    for (int row = 0; row < matrix.rows(); ++row) {
        for (int col = 0; col < matrix.cols(); ++col) {
            if (!valid_cell(matrix.at({row, col}))) {
                throw std::invalid_argument("matrix contains an invalid cell value");
            }
        }
    }
}

bool equal_matrix(const Matrix& first, const Matrix& second) {
    if (first.rows() != second.rows() || first.cols() != second.cols()) {
        return false;
    }
    for (int row = 0; row < first.rows(); ++row) {
        for (int col = 0; col < first.cols(); ++col) {
            if (first.at({row, col}) != second.at({row, col})) {
                return false;
            }
        }
    }
    return true;
}

std::string_view trim(std::string_view text) {
    const auto space = [](const char ch) {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    };
    while (!text.empty() && space(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && space(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

std::vector<std::string_view> split(const std::string_view text, const char separator) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (true) {
        const auto end = text.find(separator, start);
        if (end == std::string_view::npos) {
            fields.push_back(trim(text.substr(start)));
            return fields;
        }
        fields.push_back(trim(text.substr(start, end - start)));
        start = end + 1;
    }
}

int integer(std::string_view text) {
    text = trim(text);
    int result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::invalid_argument("expected an integer in matrix document");
    }
    return result;
}

} // namespace

Document::Document(const int rows, const int cols)
    : state_{checked_matrix(rows, cols), {}, {}, {}} {}

Document::Document(Matrix matrix)
    : state_{std::move(matrix), {}, {}, {}} {
    validate_matrix(state_.matrix);
}

bool Document::contains(const Coord position) const noexcept {
    return position.row >= 0 && position.row < state_.matrix.rows() &&
           position.col >= 0 && position.col < state_.matrix.cols();
}

bool Document::commit(State next) {
    if (state_.rows == next.rows && state_.columns == next.columns &&
        state_.cells == next.cells && equal_matrix(state_.matrix, next.matrix)) {
        return false;
    }
    // Save the complete value before replacing it; no mutable entries are
    // shared with either undo history or background computation snapshots.
    undo_.push_back(state_);
    state_ = std::move(next);
    redo_.clear();
    ++revision_;
    return true;
}

bool Document::set_cell(const Coord position, const Cell value) {
    if (!contains(position) || !valid_cell(value) || state_.matrix.at(position) == value) {
        return false;
    }
    State next = state_;
    next.matrix.at(position) = value;
    return commit(std::move(next));
}

bool Document::toggle_cell(const Coord position) {
    if (!contains(position)) {
        return false;
    }
    // Maximal configuration never changes a fixed 1 or creates new 1s, so
    // consulting the raw cell is exactly the displayed click semantics.
    return set_cell(position, state_.matrix.at(position) == Cell::One
                                 ? Cell::Blank : Cell::One);
}

Matrix Document::maximal_configuration() const {
    const Matrix& raw = state_.matrix;
    Matrix result = raw;
    for (const Coord blank : raw.blank_positions()) {
        result.at(blank) = Cell::Zero;
    }
    // A raw blank survives precisely when it is an off-diagonal corner of a
    // rectangle with fixed diagonal 1s and TWO raw blank off-diagonal corners.
    // Pair the shorter axis, for O(rows*cols*min(rows,cols)) time and
    // O(rows*cols) total space. Transposition preserves the forbidden pattern.
    const bool transpose = raw.cols() > raw.rows();
    const int paired = transpose ? raw.rows() : raw.cols();
    const int scanned = transpose ? raw.cols() : raw.rows();
    const auto coord = [transpose](const int row, const int column) {
        return transpose ? Coord{column, row} : Coord{row, column};
    };
    for (int left = 0; left < paired; ++left) {
        for (int right = left + 1; right < paired; ++right) {
            bool northwest = false;
            for (int row = 0; row < scanned; ++row) {
                const Cell a = raw.at(coord(row, left));
                const Cell b = raw.at(coord(row, right));
                if (a == Cell::One && b == Cell::Blank) {
                    northwest = true;
                } else if (a == Cell::Blank && b == Cell::One && northwest) {
                    result.at(coord(row, left)) = Cell::Blank;
                }
            }
            bool southeast = false;
            for (int row = scanned - 1; row >= 0; --row) {
                const Cell a = raw.at(coord(row, left));
                const Cell b = raw.at(coord(row, right));
                if (a == Cell::Blank && b == Cell::One) {
                    southeast = true;
                } else if (a == Cell::One && b == Cell::Blank && southeast) {
                    result.at(coord(row, right)) = Cell::Blank;
                }
            }
        }
    }
    return result;
}

bool Document::resize_axis(const int index, const bool row_axis, const bool insert) {
    const int length = row_axis ? state_.matrix.rows() : state_.matrix.cols();
    if (index < 0 || (insert ? index > length : index >= length) ||
        (insert ? length == std::numeric_limits<int>::max() : length <= 1)) {
        return false;
    }
    const int delta = insert ? 1 : -1;
    State next{checked_matrix(state_.matrix.rows() + (row_axis ? delta : 0),
                      state_.matrix.cols() + (row_axis ? 0 : delta)), {}, {}, {}};
    // -1 means the row/column was deleted. All three mask kinds follow the
    // same coordinate map as the data, including masks on otherwise blank cells.
    const auto mapped_index = [index, insert](const int old) {
        if (insert) {
            return old >= index ? old + 1 : old;
        }
        return old == index ? -1 : old > index ? old - 1 : old;
    };
    const auto mapped_coord = [row_axis, &mapped_index](Coord position) {
        if (row_axis) {
            position.row = mapped_index(position.row);
        } else {
            position.col = mapped_index(position.col);
        }
        return position;
    };
    for (int row = 0; row < state_.matrix.rows(); ++row) {
        for (int col = 0; col < state_.matrix.cols(); ++col) {
            const Coord dest = mapped_coord({row, col});
            if (dest.row >= 0 && dest.col >= 0) {
                next.matrix.at(dest) = state_.matrix.at({row, col});
            }
        }
    }
    for (const int row : state_.rows) {
        const int dest = row_axis ? mapped_index(row) : row;
        if (dest >= 0) {
            next.rows.insert(dest);
        }
    }
    for (const int column : state_.columns) {
        const int dest = row_axis ? column : mapped_index(column);
        if (dest >= 0) {
            next.columns.insert(dest);
        }
    }
    for (const Coord position : state_.cells) {
        const Coord dest = mapped_coord(position);
        if (dest.row >= 0 && dest.col >= 0) {
            next.cells.insert(dest);
        }
    }
    return commit(std::move(next));
}

bool Document::insert_row(const int index) { return resize_axis(index, true, true); }
bool Document::insert_column(const int index) { return resize_axis(index, false, true); }
bool Document::remove_row(const int index) { return resize_axis(index, true, false); }
bool Document::remove_column(const int index) { return resize_axis(index, false, false); }

bool Document::undo() {
    if (undo_.empty()) {
        return false;
    }
    redo_.push_back(state_);
    state_ = std::move(undo_.back());
    undo_.pop_back();
    ++revision_;
    return true;
}

bool Document::redo() {
    if (redo_.empty()) {
        return false;
    }
    undo_.push_back(state_);
    state_ = std::move(redo_.back());
    redo_.pop_back();
    ++revision_;
    return true;
}

bool Document::replace(Matrix matrix) {
    validate_matrix(matrix);
    return commit(State{std::move(matrix), {}, {}, {}});
}

bool Document::clear(const int rows, const int cols) {
    if (rows <= 0 || cols <= 0) {
        return false;
    }
    return replace(checked_matrix(rows, cols));
}

bool Document::set_row_ignored(const int row, const bool value) {
    if (row < 0 || row >= state_.matrix.rows() || state_.rows.contains(row) == value) {
        return false;
    }
    State next = state_;
    if (value) {
        next.rows.insert(row);
    } else {
        next.rows.erase(row);
    }
    return commit(std::move(next));
}

bool Document::set_column_ignored(const int column, const bool value) {
    if (column < 0 || column >= state_.matrix.cols() || state_.columns.contains(column) == value) {
        return false;
    }
    State next = state_;
    if (value) {
        next.columns.insert(column);
    } else {
        next.columns.erase(column);
    }
    return commit(std::move(next));
}

bool Document::set_cell_ignored(const Coord position, const bool value) {
    if (!contains(position) || state_.cells.contains(position) == value) {
        return false;
    }
    State next = state_;
    if (value) {
        next.cells.insert(position);
    } else {
        next.cells.erase(position);
    }
    return commit(std::move(next));
}

bool Document::ignored(const Coord position) const {
    return contains(position) && (state_.rows.contains(position.row) ||
           state_.columns.contains(position.col) || state_.cells.contains(position));
}

std::string Document::serialize() const {
    std::ostringstream output;
    output << state_.matrix.rows() << ';' << state_.matrix.cols() << ';';
    for (int row = 0; row < state_.matrix.rows(); ++row) {
        for (int col = 0; col < state_.matrix.cols(); ++col) {
            const Cell value = state_.matrix.at({row, col});
            if (value != Cell::Blank) {
                output << row << ',' << col << ',' << static_cast<int>(value) << '/';
            }
        }
    }
    output << ';';
    for (const Coord position : state_.cells) {
        output << position.row << ',' << position.col << '/';
    }
    output << ';';
    for (const int row : state_.rows) {
        output << row << '/';
    }
    output << ';';
    for (const int column : state_.columns) {
        output << column << '/';
    }
    return output.str();
}

Document::State Document::parse(const std::string& input) {
    const std::string_view text = trim(input);
    if (text.empty()) {
        throw std::invalid_argument("matrix document is empty");
    }
    if (text.find(';') == std::string_view::npos) {
        std::vector<std::vector<Cell>> rows;
        for (const auto line : split(text, '\n')) {
            std::vector<Cell> row;
            for (const char ch : line) {
                if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == ',') {
                    continue;
                }
                switch (ch) {
                case '0': row.push_back(Cell::Zero); break;
                case '1': row.push_back(Cell::One); break;
                case '2': case 'b': case 'B': case '.': case '?':
                    row.push_back(Cell::Blank); break;
                default:
                    throw std::invalid_argument("plain matrix accepts only 0, 1, and b/2/./? cells");
                }
                if (row.size() > maximum_cells ||
                    (!rows.empty() && rows.size() >= maximum_cells / rows.front().size())) {
                    throw std::invalid_argument("editable documents are limited to 1,000,000 cells");
                }
            }
            if (!row.empty()) {
                if (!rows.empty() && rows.front().size() != row.size()) {
                    throw std::invalid_argument("plain matrix rows have different lengths");
                }
                rows.push_back(std::move(row));
            }
        }
        if (rows.empty()) {
            throw std::invalid_argument("plain matrix has no cells");
        }
        if (rows.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            rows.front().size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::invalid_argument("plain matrix dimensions are too large");
        }
        State result{checked_matrix(static_cast<int>(rows.size()), static_cast<int>(rows.front().size())), {}, {}, {}};
        for (int row = 0; row < result.matrix.rows(); ++row) {
            for (int col = 0; col < result.matrix.cols(); ++col) {
                result.matrix.at({row, col}) = rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            }
        }
        return result;
    }

    const auto fields = split(text, ';');
    if (fields.size() != 6U) {
        throw std::invalid_argument("matrix document requires six fields: rows;columns;data;cells;rows;columns");
    }
    const int height = integer(fields[0]);
    const int width = integer(fields[1]);
    if (height <= 0 || width <= 0) {
        throw std::invalid_argument("matrix dimensions must be positive");
    }
    State result{checked_matrix(height, width), {}, {}, {}};
    const auto coordinate = [height, width](const std::vector<std::string_view>& parts) {
        const Coord position{integer(parts[0]), integer(parts[1])};
        if (position.row < 0 || position.row >= height || position.col < 0 || position.col >= width) {
            throw std::invalid_argument("matrix document coordinate is out of range");
        }
        return position;
    };
    std::set<Coord> supplied;
    for (const auto entry : split(fields[2], '/')) {
        if (entry.empty()) {
            continue;
        }
        const auto parts = split(entry, ',');
        if (parts.size() != 3U) {
            throw std::invalid_argument("matrix data entry must be row,column,value");
        }
        const Coord position = coordinate(parts);
        const int value = integer(parts[2]);
        if (value < 0 || value > 2 || !supplied.insert(position).second) {
            throw std::invalid_argument("invalid or duplicate matrix data entry");
        }
        result.matrix.at(position) = static_cast<Cell>(value);
    }
    for (const auto entry : split(fields[3], '/')) {
        if (entry.empty()) {
            continue;
        }
        const auto parts = split(entry, ',');
        if (parts.size() != 2U) {
            throw std::invalid_argument("matrix cell mask must be row,column");
        }
        result.cells.insert(coordinate(parts));
    }
    for (const auto entry : split(fields[4], '/')) {
        if (!entry.empty()) {
            const int row = integer(entry);
            if (row < 0 || row >= height) {
                throw std::invalid_argument("ignored row is out of range");
            }
            result.rows.insert(row);
        }
    }
    for (const auto entry : split(fields[5], '/')) {
        if (!entry.empty()) {
            const int column = integer(entry);
            if (column < 0 || column >= width) {
                throw std::invalid_argument("ignored column is out of range");
            }
            result.columns.insert(column);
        }
    }
    return result;
}

bool Document::load(const std::string& input) {
    return commit(parse(input));
}

} // namespace mfg
