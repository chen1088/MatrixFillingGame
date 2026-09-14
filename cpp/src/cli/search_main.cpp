#include "mfg/cnf.hpp"
#include "mfg/model.hpp"
#include "mfg/search.hpp"
#include "mfg/analysis.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Options {
    int max_sparsity = 5;
    bool compute_projections = true;
    std::optional<int> dump_sparsity;
    bool blank_ratios = false;
};

[[noreturn]] void usage(const char* program, const int exit_code) {
    std::ostream& output = exit_code == 0 ? std::cout : std::cerr;
    output << "Usage: " << program << " [options]\n\n"
           << "  --max-k N          enumerate normalized supports through sparsity N\n"
           << "  --no-projections   skip exact child-to-parent CNF projection\n"
           << "  --dump-k N         print every support, strong form, and CNF at level N\n"
           << "  --blank-ratios     run the legacy B0/B sparse-support scoring experiment\n"
           << "  --help             show this help\n";
    std::exit(exit_code);
}

int parse_positive_int(const std::string_view text, const char* option) {
    int value = 0;
    const auto [end, error] =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value <= 0) {
        throw std::invalid_argument(std::string(option) + " requires a positive integer");
    }
    return value;
}

Options parse_options(const int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--help") {
            usage(argv[0], 0);
        } else if (argument == "--no-projections") {
            options.compute_projections = false;
        } else if (argument == "--blank-ratios") {
            options.blank_ratios = true;
        } else if (argument == "--max-k" || argument == "--dump-k") {
            if (index + 1 >= argc) {
                throw std::invalid_argument(std::string(argument) + " requires a value");
            }
            const int value = parse_positive_int(argv[++index], argument.data());
            if (argument == "--max-k") {
                options.max_sparsity = value;
            } else {
                options.dump_sparsity = value;
                options.max_sparsity = std::max(options.max_sparsity, value);
            }
        } else {
            throw std::invalid_argument("unknown option: " + std::string(argument));
        }
    }
    return options;
}

void print_counterexample(const char* heading, const mfg::Counterexample& counterexample) {
    std::cout << '\n' << heading << '\n'
              << "support: " << counterexample.support.key() << '\n'
              << counterexample.matrix.to_string() << '\n'
              << "CNF: " << counterexample.formula.to_string() << '\n';
}

void dump_level(const int sparsity) {
    const auto levels = mfg::enumerate_support_levels(sparsity);
    const auto& supports = levels.back();
    std::cout << "\nDetailed level k=" << sparsity << " (" << supports.size()
              << " supports)\n";
    for (std::size_t index = 0; index < supports.size(); ++index) {
        const mfg::SparseSupport& support = supports[index];
        const mfg::Matrix matrix = mfg::strong_normal_form(support);
        const mfg::Cnf formula = mfg::rectangle_cnf(matrix);
        std::cout << "\n#" << (index + 1U) << ' ' << support.key() << '\n'
                  << matrix.to_string() << '\n'
                  << formula.to_string() << '\n';
    }
}

void score_blank_ratios(const int max_sparsity) {
    const auto levels = mfg::enumerate_support_levels(max_sparsity);
    std::cout << "Sparse-support B0/B experiment (zero-blank ratios are undefined)\n";
    for (std::size_t level = 0; level < levels.size(); ++level) {
        double maximum = -1;
        std::size_t zero_blanks = 0;
        std::cout << "\nk=" << (level + 1) << " supports=" << levels[level].size() << '\n';
        for (const auto& support : levels[level]) {
            const auto matrix = mfg::strong_normal_form(support);
            const auto stats = mfg::blank_statistics(matrix);
            if (stats.blanks == 0) { ++zero_blanks; continue; }
            const double ratio = static_cast<double>(stats.b0) / static_cast<double>(stats.blanks);
            if (ratio > maximum) {
                maximum = ratio;
                std::cout << "record B0/B=" << stats.b0 << '/' << stats.blanks
                          << " ratio=" << std::setprecision(12) << ratio << '\n'
                          << "support: " << support.key() << '\n' << matrix.to_string() << '\n';
            }
        }
        std::cout << "zero-blank supports=" << zero_blanks << '\n';
    }
}

} // namespace

int main(const int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        if (options.blank_ratios) {
            score_blank_ratios(options.max_sparsity);
            if (options.dump_sparsity) dump_level(*options.dump_sparsity);
            return 0;
        }
        const mfg::SearchReport report =
            mfg::run_bounded_search(options.max_sparsity, options.compute_projections);

        std::cout << "Bounded strong-counterexample search\n"
                  << "projection column uses Gamma=true (the base pullback)\n\n"
                  << std::left << std::setw(5) << "k"
                  << std::right << std::setw(12) << "supports"
                  << std::setw(12) << "formulas"
                  << std::setw(14) << "projections"
                  << std::setw(12) << "max blanks"
                  << std::setw(13) << "max clauses"
                  << std::setw(10) << "viable"
                  << std::setw(8) << "SAT" << '\n';

        for (const mfg::LevelSummary& level : report.levels) {
            std::cout << std::left << std::setw(5) << level.sparsity
                      << std::right << std::setw(12) << level.support_count
                      << std::setw(12) << level.distinct_formula_count;
            if (options.compute_projections) {
                std::cout << std::setw(14) << level.distinct_projection_count;
            } else {
                std::cout << std::setw(14) << "skipped";
            }
            std::cout << std::setw(12) << level.max_blank_count
                      << std::setw(13) << level.max_clause_count
                      << std::setw(10) << (level.all_strong_forms_viable ? "yes" : "NO")
                      << std::setw(8) << (level.all_formulas_satisfiable ? "yes" : "NO")
                      << '\n';
        }

        if (report.first_normalization_error.has_value()) {
            print_counterexample("NORMALIZATION INVARIANT FAILED:",
                                 *report.first_normalization_error);
        }
        if (report.first_counterexample.has_value()) {
            print_counterexample("STRONG COUNTEREXAMPLE FOUND:",
                                 *report.first_counterexample);
        } else {
            std::cout << "\nNo counterexample occurs through k="
                      << options.max_sparsity << ".\n";
        }

        if (options.dump_sparsity.has_value()) {
            dump_level(*options.dump_sparsity);
        }

        if (report.first_normalization_error.has_value()) {
            return 3;
        }
        return report.first_counterexample.has_value() ? 2 : 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
