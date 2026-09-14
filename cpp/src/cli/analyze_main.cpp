#include "mfg/analysis.hpp"
#include "mfg/document.hpp"

#include <charconv>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
volatile std::sig_atomic_t interrupted = 0;
void interrupt_handler(int) { interrupted = 1; }

std::uint64_t number(std::string_view text, const std::string& option) {
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::invalid_argument(option + " requires a nonnegative integer");
    return value;
}

const char* status(mfg::AnalysisStatus value) {
    switch (value) {
    case mfg::AnalysisStatus::Complete: return "complete";
    case mfg::AnalysisStatus::Cancelled: return "cancelled";
    case mfg::AnalysisStatus::LimitReached: return "limit reached";
    case mfg::AnalysisStatus::NotRequested: return "not requested";
    }
    return "unknown";
}

void estimate(const char* label, const mfg::CompletionEstimate& result) {
    std::cout << label << ": ";
    if (result.status == mfg::AnalysisStatus::Complete)
        std::cout << result.valid_ratio << " (standard error " << result.standard_error
                  << ", samples " << result.samples << ')';
    else
        std::cout << status(result.status);
    if (!result.note.empty()) std::cout << " — " << result.note;
    std::cout << '\n';
}
} // namespace

int main(int argc, char** argv) {
    try {
        mfg::AnalysisOptions options;
        bool raw = false;
        bool paired = false;
        std::string filename;
        for (int i = 1; i < argc; ++i) {
            const std::string argument(argv[i]);
            if (argument == "--help") {
                std::cout << "Usage: mfg_analyze FILE [options]\n"
                          << "FILE is a saved .mfg document or rectangular 0/1/b text; - reads stdin.\n"
                          << "  --raw              analyze exactly the supplied cells\n"
                          << "  --no-exact         skip exhaustive completion counting\n"
                          << "  --no-sampling      skip both sampling estimates\n"
                          << "  --samples N        samples per estimator (default 100000)\n"
                          << "  --seed N           reproducible sampling seed (default 1)\n"
                          << "  --exact-limit N    maximum constrained variables (default 24, at most 62)\n"
                          << "  --paired-greedy    also print the paired-zero greedy candidate\n"
                          << "Default view is the maximal configuration, as in the desktop editor.\n";
                return 0;
            } else if (argument == "--raw") raw = true;
            else if (argument == "--paired-greedy") paired = true;
            else if (argument == "--no-exact") options.compute_exact = false;
            else if (argument == "--no-sampling") {
                options.compute_estimate = false;
                options.compute_importance = false;
            } else if (argument == "--samples" || argument == "--seed" || argument == "--exact-limit") {
                if (++i == argc) throw std::invalid_argument(argument + " requires a value");
                const auto value = number(argv[i], argument);
                if (argument == "--samples") {
                    if (value == 0) throw std::invalid_argument("--samples must be positive");
                    options.sample_count = value;
                } else if (argument == "--seed") options.seed = value;
                else {
                    if (value > 62) throw std::invalid_argument("--exact-limit must be at most 62");
                    options.max_exact_variables = static_cast<std::size_t>(value);
                }
            } else if (argument == "-" || !argument.starts_with('-')) {
                if (!filename.empty()) throw std::invalid_argument("supply exactly one input file");
                filename = argument;
            } else throw std::invalid_argument("unknown option: " + argument);
        }
        if (filename.empty()) throw std::invalid_argument("supply a matrix file; use --help for usage");
        std::ifstream file;
        std::istream* input = &std::cin;
        if (filename != "-") {
            file.open(filename);
            if (!file) throw std::runtime_error("cannot open " + filename);
            input = &file;
        }
        const std::string text{std::istreambuf_iterator<char>(*input), std::istreambuf_iterator<char>()};
        if (input->bad()) throw std::runtime_error("failed reading matrix");
        mfg::Document document;
        document.load(text);
        const auto matrix = raw ? document.matrix() : document.maximal_configuration();
        mfg::IgnoreMask mask;
        for (int row = 0; row < matrix.rows(); ++row)
            for (int col = 0; col < matrix.cols(); ++col)
                mask.push_back(document.ignored({row, col}));
        std::signal(SIGINT, interrupt_handler);
        options.cancelled = [] { return interrupted != 0; };
        const auto result = mfg::analyze_matrix(matrix, mask, options);
        std::cout << std::setprecision(12)
                  << (raw ? "Raw" : "Maximal") << " matrix (" << matrix.rows() << " x " << matrix.cols() << ")\n"
                  << matrix.to_string() << "\n\nAnalysis: " << status(result.status) << '\n';
        if (!result.note.empty()) std::cout << result.note << '\n';
        std::cout << "Valid completions: ";
        if (result.exact.status == mfg::AnalysisStatus::Complete)
            std::cout << result.exact.valid_count << " / " << result.exact.total_count
                      << "\nExact ratio: " << result.exact.valid_ratio;
        else std::cout << status(result.exact.status) << " " << result.exact.note;
        std::cout << '\n';
        estimate("Uniform estimate", result.uniform);
        estimate("Importance estimate", result.importance);
        if (result.blanks.status == mfg::AnalysisStatus::Complete)
            std::cout << "BlankStat B0/B: " << result.blanks.b0 << '/' << result.blanks.blanks << '\n';
        if (result.greedy) {
            const auto& greedy = *result.greedy;
            std::cout << "Greedy hypothesis: "
                      << (greedy.status != mfg::AnalysisStatus::Complete ? status(greedy.status)
                          : greedy.valid ? "Works (verified completion)" : "Fails (greedy candidate)")
                      << "\n" << greedy.matrix.to_string() << '\n';
        }
        if (result.violation) {
            const auto& rect = *result.violation;
            std::cout << "Fixed 1001 witness (zero-based): rows " << rect.top << ',' << rect.bottom
                      << "; columns " << rect.left << ',' << rect.right << '\n';
        }
        if (result.cnf) std::cout << "CNF: " << result.cnf->to_string() << '\n';
        else if (!result.cnf_note.empty()) std::cout << "CNF: " << result.cnf_note << '\n';
        if (paired && !interrupted) {
            const auto greedy = mfg::greedy_fill(matrix, mfg::GreedyMethod::PairedZeros, mask, options);
            std::cout << "Paired-zero greedy: " << (greedy.valid ? "valid" : "invalid or incomplete")
                      << '\n' << greedy.matrix.to_string() << '\n';
        }
        if (interrupted || result.status == mfg::AnalysisStatus::Cancelled) return 130;
        return result.status == mfg::AnalysisStatus::LimitReached ? 2 : 0;
    } catch (const mfg::AnalysisCancelled&) {
        std::cerr << "cancelled\n";
        return 130;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
