#pragma once

#include "mfg/automaton.hpp"

#include <QByteArray>
#include <QDialog>

class QCloseEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QTabWidget;
class QTimer;

namespace mfg::gui {

// The matrix application and this viewer share the same DFA implementation.
// Graphviz is optional: DOT source remains usable without the dot executable.
class AutomatonDialog final : public QDialog {
public:
    explicit AutomatonDialog(QWidget* parent = nullptr);
    ~AutomatonDialog() override;

    [[nodiscard]] const Dfa& automaton() const noexcept { return automaton_; }
    bool loadText(const QString& text, QString* error = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void showDemo();
    void showAutomaton(Dfa automaton, const QString& name);
    void openAutomaton(bool intersection);
    void saveAutomaton(bool dot);
    void evaluateWord();
    void renderGraph();
    void stopRenderer();
    void renderFailed(const QString& message);

    Dfa automaton_;
    QLabel* summary_ = nullptr;
    QLabel* graph_ = nullptr;
    QLabel* status_ = nullptr;
    QLineEdit* word_ = nullptr;
    QLabel* word_result_ = nullptr;
    QPlainTextEdit* dot_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    QProcess* renderer_ = nullptr;
    QTimer* render_timer_ = nullptr;
    QByteArray rendered_png_;
    QByteArray render_error_;
};

} // namespace mfg::gui
