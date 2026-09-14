#include "mfg/document.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(const bool result, const std::string& message) {
    if (!result) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

template<class Function>
void rejects(Function function, const std::string& message) {
    try {
        function();
        check(false, message);
    } catch (const std::invalid_argument&) {
        // Invalid input must be rejected before any document state changes.
    }
}

void test_value_history() {
    mfg::Document document;
    const auto original = document.serialize();
    check(document.set_cell({0, 0}, mfg::Cell::One), "first edit changes cell");
    check(document.set_cell({0, 0}, mfg::Cell::Zero), "second edit changes same cell");
    check(document.set_cell({0, 0}, mfg::Cell::Blank), "third edit changes same cell");
    const auto revision = document.revision();
    check(document.undo() && document.matrix().at({0, 0}) == mfg::Cell::Zero,
          "undo restores prior value rather than shared mutable entry");
    check(document.undo() && document.matrix().at({0, 0}) == mfg::Cell::One,
          "second undo restores first value");
    check(document.undo() && document.serialize() == original, "third undo restores original document");
    check(!document.undo() && !document.can_undo(), "undo of empty history is a no-op");
    check(document.revision() == revision + 3, "undo revisions increase monotonically");
    check(document.redo() && document.matrix().at({0, 0}) == mfg::Cell::One, "redo restores edit");
    check(!document.set_cell({0, 0}, mfg::Cell::One) && document.can_redo(),
          "no-op edit preserves redo history");
    check(document.set_cell({1, 1}, mfg::Cell::One) && !document.can_redo(),
          "new edit after undo clears redo history");
    check(!document.redo(), "cleared redo is safe");

    const mfg::Document snapshot = document;
    const mfg::Matrix matrix_snapshot = document.matrix();
    document.set_cell({0, 0}, mfg::Cell::Zero);
    check(snapshot.matrix().at({0, 0}) == mfg::Cell::One &&
          matrix_snapshot.at({0, 0}) == mfg::Cell::One,
          "document and matrix copies are independent background snapshots");
}

void test_structural_masks_and_history() {
    mfg::Document document(3, 5);
    std::vector<std::string> states{document.serialize()};
    const auto save = [&] { states.push_back(document.serialize()); };
    document.set_cell({2, 4}, mfg::Cell::One); save();
    document.set_cell({1, 2}, mfg::Cell::Zero); save();
    document.set_row_ignored(2, true); save();
    document.set_column_ignored(4, true); save();
    document.set_cell_ignored({1, 3}, true); save();
    document.set_cell_ignored({2, 1}, true); save();
    document.insert_row(1); save();
    check(document.matrix().rows() == 4 && document.matrix().cols() == 5 &&
          document.matrix().at({3, 4}) == mfg::Cell::One &&
          document.ignored_rows().contains(3) && document.ignored_cells().contains({2, 3}),
          "row insertion moves cells and row/cell masks together");
    document.insert_column(2); save();
    check(document.matrix().at({3, 5}) == mfg::Cell::One &&
          document.ignored_columns().contains(5) && document.ignored_cells().contains({2, 4}),
          "column insertion moves column/cell masks together");
    document.remove_row(2); save();
    check(document.matrix().rows() == 3 && document.matrix().cols() == 6 &&
          document.matrix().at({2, 5}) == mfg::Cell::One &&
          document.ignored_cells().size() == 1 && document.ignored_cells().contains({2, 1}),
          "non-square row deletion drops deleted masks and shifts remaining cells");
    document.remove_column(1); save();
    check(document.matrix().rows() == 3 && document.matrix().cols() == 5 &&
          document.matrix().at({2, 4}) == mfg::Cell::One &&
          document.ignored_cells().empty() && document.ignored_columns().contains(4),
          "non-square column deletion correctly transforms every mask");
    document.set_row_ignored(2, false); save();
    document.set_column_ignored(4, false); save();
    check(!document.ignored({2, 4}), "mask removals are visible");
    for (std::size_t count = states.size() - 1; count > 0; --count) {
        check(document.undo() && document.serialize() == states[count - 1],
              "undo restores whole state at history index " + std::to_string(count));
    }
    for (std::size_t index = 1; index < states.size(); ++index) {
        check(document.redo() && document.serialize() == states[index],
              "redo restores whole state at history index " + std::to_string(index));
    }
    document.set_cell_ignored({0, 0}, true);
    document.clear();
    check(document.matrix().rows() == 6 && document.matrix().cols() == 6 &&
          document.ignored_cells().empty(), "clear resets dimensions, values, and masks");
    check(document.undo() && document.matrix().rows() == 3 &&
          document.ignored_cells().contains({0, 0}), "clear remains fully undoable");
}

void test_bounds_and_large_coordinates() {
    mfg::Document document(1, 1);
    const auto original = document.serialize();
    check(!document.remove_row(0) && !document.remove_column(0), "cannot delete final row or column");
    check(!document.remove_row(-1) && !document.remove_column(1) &&
          !document.insert_row(-1) && !document.insert_column(2), "structural bounds checked");
    check(!document.set_cell({-1, 0}, mfg::Cell::One) && !document.toggle_cell({0, 1}) &&
          !document.set_cell({0, 0}, static_cast<mfg::Cell>(42)), "invalid cells rejected");
    check(!document.set_row_ignored(1, true) && !document.set_column_ignored(-1, true) &&
          !document.set_cell_ignored({1, 0}, true) && !document.ignored({1, 0}), "invalid masks rejected");
    check(!document.clear(0, 5) && document.serialize() == original && !document.can_undo(),
          "invalid operations preserve state and history");
    rejects([&] { document.load("2147483647;2147483647;;;;"); }, "huge document rejected before allocation");
    rejects([] { const mfg::Document enormous(1001, 1000); }, "constructor enforces document allocation bound");
    rejects([&] { document.clear(1001, 1000); }, "clear enforces document allocation bound");
    check(document.serialize() == original && !document.can_undo(), "allocation-bound rejections atomic");

    mfg::Document large(130, 132);
    large.set_cell({129, 130}, mfg::Cell::One);
    large.set_cell({129, 130}, mfg::Cell::Zero);
    check(large.undo() && large.matrix().at({129, 130}) == mfg::Cell::One,
          "coordinates beyond Java Integer cache use numeric equality");
    large.set_cell_ignored({129, 130}, true);
    large.remove_column(128);
    check(large.matrix().at({129, 129}) == mfg::Cell::One && large.ignored({129, 129}),
          "large-index column deletion shifts actual coordinates");
}

mfg::Matrix brute_maximal(const mfg::Matrix& matrix) {
    mfg::Matrix result = matrix;
    for (const auto position : matrix.blank_positions()) {
        result.at(position) = mfg::Cell::Zero;
    }
    for (int top = 0; top < matrix.rows(); ++top) {
        for (int bottom = top + 1; bottom < matrix.rows(); ++bottom) {
            for (int left = 0; left < matrix.cols(); ++left) {
                for (int right = left + 1; right < matrix.cols(); ++right) {
                    if (matrix.at({top, left}) == mfg::Cell::One &&
                        matrix.at({bottom, right}) == mfg::Cell::One &&
                        matrix.at({top, right}) == mfg::Cell::Blank &&
                        matrix.at({bottom, left}) == mfg::Cell::Blank) {
                        result.at({top, right}) = mfg::Cell::Blank;
                        result.at({bottom, left}) = mfg::Cell::Blank;
                    }
                }
            }
        }
    }
    return result;
}

void test_maximal_configuration() {
    for (const auto shape : std::vector<mfg::Coord>{{2, 3}, {3, 2}, {1, 6}, {6, 1}}) {
        for (int code = 0; code < 729; ++code) {
            mfg::Matrix matrix(shape.row, shape.col);
            int remaining = code;
            for (int row = 0; row < matrix.rows(); ++row) {
                for (int col = 0; col < matrix.cols(); ++col) {
                    matrix.at({row, col}) = static_cast<mfg::Cell>(remaining % 3);
                    remaining /= 3;
                }
            }
            const mfg::Document document(matrix);
            const auto maximal = document.maximal_configuration();
            check(maximal.rows() == matrix.rows() && maximal.cols() == matrix.cols() &&
                  maximal.to_string() == brute_maximal(matrix).to_string(),
                  "maximal configuration matches rectangle definition for code " + std::to_string(code));
        }
    }
    mfg::Document diagonal(4, 5);
    diagonal.set_cell({0, 0}, mfg::Cell::One);
    diagonal.set_cell({3, 4}, mfg::Cell::One);
    check(diagonal.maximal_configuration().at({0, 4}) == mfg::Cell::Blank,
          "non-adjacent rectangle leaves off-diagonal blank");
    diagonal.set_cell({3, 0}, mfg::Cell::Zero);
    check(diagonal.maximal_configuration().at({0, 4}) == mfg::Cell::Zero,
          "explicit opposite zero prevents a surviving maximal blank");
    check(diagonal.matrix().at({0, 4}) == mfg::Cell::Blank, "display normalization never mutates raw data");
    diagonal.toggle_cell({0, 4});
    check(diagonal.matrix().at({0, 4}) == mfg::Cell::One, "click displayed zero creates one");
    diagonal.toggle_cell({0, 4});
    check(diagonal.matrix().at({0, 4}) == mfg::Cell::Blank, "click one restores raw blank");
}

void test_serialization() {
    mfg::Document document(3, 7);
    document.set_cell({2, 6}, mfg::Cell::One);
    document.set_cell({0, 4}, mfg::Cell::Zero);
    document.set_row_ignored(1, true);
    document.set_column_ignored(5, true);
    document.set_cell_ignored({2, 3}, true);
    const auto saved = document.serialize();
    mfg::Document restored;
    const auto previous = restored.serialize();
    check(restored.load(saved) && restored.serialize() == saved,
          "serialized non-square dimensions, fixed zeros, ones, blanks and masks round-trip");
    check(restored.undo() && restored.serialize() == previous, "file load is atomic and undoable");
    check(restored.redo() && restored.serialize() == saved, "loaded file can be redone");
    check(!restored.load(saved), "loading identical document is a no-op");
    for (const auto& malformed : std::vector<std::string>{
             "", "0;2;;;;", "2;2;0,0,5/;;;", "2;2;0,0,1/0,0,0/;;;",
             "2;2;2,0,1/;;;", "2;2;;2,0/;;", "2;2;;;-1/;", "2;2;;;;2/",
             "2;2;0'0'1/;;;", "0,0,1/;;;", "2;2;;;;;", "01\n1", "0x1\n1b0"}) {
        const auto revision = restored.revision();
        rejects([&] { restored.load(malformed); }, "malformed document rejected: " + malformed);
        check(restored.serialize() == saved && restored.revision() == revision,
              "failed parse leaves document and revision unchanged");
    }
    check(restored.load(" 0,1,b\r\n B . ?\n 212 \n"), "plain matrix import accepts blank spellings");
    check(restored.matrix().rows() == 3 && restored.matrix().cols() == 3 &&
          restored.matrix().at({0, 0}) == mfg::Cell::Zero &&
          restored.matrix().at({0, 1}) == mfg::Cell::One &&
          restored.matrix().at({2, 2}) == mfg::Cell::Blank &&
          restored.ignored_rows().empty(), "plain import preserves dimensions and clears masks");
    mfg::Document empty(1, 2);
    check(empty.serialize() == "1;2;;;;", "all-empty fields retain six-field format");
    check(restored.load(empty.serialize()) && restored.serialize() == empty.serialize(),
          "empty sparse matrix round-trip preserves trailing fields");
}

} // namespace

int main() {
    test_value_history();
    test_structural_masks_and_history();
    test_bounds_and_large_coordinates();
    test_maximal_configuration();
    test_serialization();
    if (failures != 0) {
        std::cerr << failures << " document test(s) failed\n";
        return 1;
    }
    std::cout << "All document regression tests passed\n";
    return 0;
}
