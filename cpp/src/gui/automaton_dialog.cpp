#include "automaton_dialog.hpp"

#include <QBuffer>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <sstream>
#include <stdexcept>
#include <utility>

namespace mfg::gui {
namespace {

constexpr qint64 max_file_bytes = 8 * 1024 * 1024;
constexpr qsizetype max_png_bytes = 16 * 1024 * 1024;

Dfa parseAutomaton(const QString& text) {
    if (text.size() > max_file_bytes) {
        throw std::invalid_argument("The DFA viewer accepts text files up to 8 MiB.");
    }
    // Validate dimensions before the core parser can allocate a transition
    // table. These limits keep interactive operations and DOT text manageable.
    std::istringstream header(text.toStdString());
    std::string states_text, alphabet_text;
    header >> states_text >> alphabet_text;
    bool valid_states = false;
    bool valid_alphabet = false;
    const auto states = QString::fromStdString(states_text).toULongLong(&valid_states);
    const auto alphabet = QString::fromStdString(alphabet_text).toULongLong(&valid_alphabet);
    if (!valid_states || !valid_alphabet || states == 0 || alphabet == 0) {
        throw std::invalid_argument("The first line must contain positive state and alphabet counts, followed by the start state.");
    }
    if (states > 4096 || alphabet > 65536 || states > 65536 / alphabet) {
        throw std::invalid_argument("The interactive DFA viewer supports up to 4,096 states and 65,536 transitions. Use mfg_tools for larger automata.");
    }
    std::istringstream input(text.toStdString());
    return Dfa::read(input);
}

QString readAutomatonFile(const QString& path) {
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly)) throw std::runtime_error(input.errorString().toStdString());
    if (input.size() > max_file_bytes) throw std::invalid_argument("The DFA viewer accepts text files up to 8 MiB.");
    const QByteArray contents = input.read(max_file_bytes + 1);
    if (input.error() != QFileDevice::NoError) throw std::runtime_error(input.errorString().toStdString());
    if (contents.size() > max_file_bytes) throw std::invalid_argument("The DFA viewer accepts text files up to 8 MiB.");
    return QString::fromUtf8(contents);
}

} // namespace

AutomatonDialog::AutomatonDialog(QWidget* parent) : QDialog(parent) {
    setObjectName("automatonDialog");
    setWindowTitle(tr("DFA viewer"));
    resize(1040, 760);
    auto* layout = new QVBoxLayout(this);
    auto* operations = new QHBoxLayout;
    auto addButton = [this, operations](const QString& title, const char* name, auto action) {
        auto* button = new QPushButton(title, this);
        button->setAutoDefault(false);
        button->setObjectName(name);
        operations->addWidget(button);
        connect(button, &QPushButton::clicked, this, action);
    };
    addButton(tr("Open DFA…"), "automatonOpen", [this] { openAutomaton(false); });
    addButton(tr("Save DFA…"), "automatonSave", [this] { saveAutomaton(false); });
    addButton(tr("Show demo"), "automatonDemo", [this] { showDemo(); });
    addButton(tr("Intersect…"), "automatonIntersect", [this] { openAutomaton(true); });
    addButton(tr("Trim"), "automatonTrim", [this] {
        try { showAutomaton(automaton_.trim(), tr("Reachable states")); }
        catch (const std::exception& error) { QMessageBox::warning(this, tr("Trim DFA"), QString::fromUtf8(error.what())); }
    });
    addButton(tr("Minimize"), "automatonMinimize", [this] {
        try { showAutomaton(automaton_.minimize(), tr("Minimal DFA")); }
        catch (const std::exception& error) { QMessageBox::warning(this, tr("Minimize DFA"), QString::fromUtf8(error.what())); }
    });
    addButton(tr("Export DOT…"), "automatonExportDot", [this] { saveAutomaton(true); });
    layout->addLayout(operations);

    auto* format = new QLabel(tr("DFA text: first line = states alphabet start; each following state row = accepting (0 or 1), then destinations for symbols 0, 1, … . State indices start at 0."), this);
    format->setWordWrap(true);
    layout->addWidget(format);
    summary_ = new QLabel(this);
    summary_->setObjectName("automatonSummary");
    summary_->setTextFormat(Qt::PlainText);
    summary_->setWordWrap(true);
    layout->addWidget(summary_);

    tabs_ = new QTabWidget(this);
    auto* scroll = new QScrollArea(tabs_);
    scroll->setWidgetResizable(true);
    graph_ = new QLabel(scroll);
    graph_->setObjectName("automatonGraph");
    graph_->setAlignment(Qt::AlignCenter);
    graph_->setWordWrap(true);
    graph_->setTextFormat(Qt::PlainText);
    scroll->setWidget(graph_);
    tabs_->addTab(scroll, tr("Graph"));
    dot_ = new QPlainTextEdit(tabs_);
    dot_->setObjectName("automatonDot");
    dot_->setReadOnly(true);
    dot_->setLineWrapMode(QPlainTextEdit::NoWrap);
    tabs_->addTab(dot_, tr("DOT source"));
    layout->addWidget(tabs_, 1);

    status_ = new QLabel(this);
    status_->setObjectName("automatonStatus");
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    layout->addWidget(status_);
    auto* evaluate = new QHBoxLayout;
    evaluate->addWidget(new QLabel(tr("Word:"), this));
    word_ = new QLineEdit(this);
    word_->setObjectName("automatonWord");
    word_->setPlaceholderText(tr("Binary word such as 00101, or symbols separated by spaces; blank = empty word"));
    evaluate->addWidget(word_, 1);
    auto* evaluate_button = new QPushButton(tr("Evaluate"), this);
    evaluate_button->setAutoDefault(false);
    evaluate_button->setObjectName("automatonEvaluate");
    evaluate->addWidget(evaluate_button);
    word_result_ = new QLabel(this);
    word_result_->setObjectName("automatonWordResult");
    word_result_->setTextFormat(Qt::PlainText);
    evaluate->addWidget(word_result_);
    layout->addLayout(evaluate);
    connect(evaluate_button, &QPushButton::clicked, this, [this] { evaluateWord(); });
    connect(word_, &QLineEdit::returnPressed, this, [this] { evaluateWord(); });
    connect(word_, &QLineEdit::textChanged, word_result_, &QLabel::clear);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setAutoDefault(false);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);
    render_timer_ = new QTimer(this);
    render_timer_->setSingleShot(true);
    connect(render_timer_, &QTimer::timeout, this, [this] {
        renderFailed(tr("Graphviz rendering exceeded 15 seconds. The DFA is available in the DOT source tab and can be exported."));
    });
    showDemo();
}

AutomatonDialog::~AutomatonDialog() { stopRenderer(); }

void AutomatonDialog::closeEvent(QCloseEvent* event) {
    stopRenderer();
    QDialog::closeEvent(event);
}

bool AutomatonDialog::loadText(const QString& text, QString* error) {
    try {
        auto parsed = parseAutomaton(text);
        showAutomaton(std::move(parsed), tr("Loaded DFA"));
        if (error) error->clear();
        return true;
    } catch (const std::exception& exception) {
        if (error) *error = QString::fromUtf8(exception.what());
        return false;
    }
}

void AutomatonDialog::showDemo() {
    // The two six-state automata from Java TestGraphviz.getadfa().
    Dfa left({{1, 2}, {0, 3}, {4, 5}, {4, 5}, {4, 5}, {5, 5}},
             {false, false, true, true, true, false});
    Dfa right({{1, 2}, {0, 3}, {4, 5}, {5, 4}, {4, 5}, {5, 5}},
              {false, false, true, true, true, false});
    showAutomaton(left.intersect(right).minimize(), tr("Java demo: minimized intersection"));
}

void AutomatonDialog::showAutomaton(Dfa automaton, const QString& name) {
    const QString source = QString::fromStdString(automaton.to_dot());
    automaton_ = std::move(automaton);
    summary_->setText(tr("%1 — %2 states; %3 symbols; start state %4. Double circles mark accepting states.")
        .arg(name).arg(static_cast<qulonglong>(automaton_.state_count()))
        .arg(static_cast<qulonglong>(automaton_.alphabet_size()))
        .arg(static_cast<qulonglong>(automaton_.start_state())));
    word_result_->clear();
    dot_->setPlainText(source);
    renderGraph();
}

void AutomatonDialog::openAutomaton(bool intersection) {
    const QString path = QFileDialog::getOpenFileName(this,
        intersection ? tr("Intersect with DFA") : tr("Open DFA"), {}, tr("DFA text (*.dfa *.txt);;All files (*)"));
    if (path.isEmpty()) return;
    try {
        auto parsed = parseAutomaton(readAutomatonFile(path));
        if (intersection) {
            if (parsed.alphabet_size() != automaton_.alphabet_size()) {
                throw std::invalid_argument("DFA intersection requires equal alphabet sizes.");
            }
            if (automaton_.state_count() > 4096 / parsed.state_count() ||
                automaton_.state_count() * parsed.state_count() > 65536 / parsed.alphabet_size()) {
                throw std::invalid_argument("This product may exceed the viewer's 4,096-state or 65,536-transition limit. Compute it with mfg_tools, then open a smaller trimmed or minimized result.");
            }
            showAutomaton(automaton_.intersect(parsed), tr("Intersection with %1").arg(QFileInfo(path).fileName()));
        } else {
            showAutomaton(std::move(parsed), QFileInfo(path).fileName());
        }
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Open DFA"), QString::fromUtf8(error.what()));
    }
}

void AutomatonDialog::saveAutomaton(bool dot) {
    const QString path = QFileDialog::getSaveFileName(this,
        dot ? tr("Export DOT") : tr("Save DFA"), dot ? "automaton.dot" : "automaton.dfa",
        dot ? tr("Graphviz DOT (*.dot);;All files (*)") : tr("DFA text (*.dfa *.txt);;All files (*)"));
    if (path.isEmpty()) return;
    QByteArray contents;
    if (dot) {
        contents = dot_->toPlainText().toUtf8();
    } else {
        std::ostringstream output;
        automaton_.write(output);
        contents = QByteArray::fromStdString(output.str());
    }
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(contents) != contents.size() || !output.commit()) {
        QMessageBox::warning(this, tr("Save DFA"), output.errorString());
    }
}

void AutomatonDialog::evaluateWord() {
    const QString text = word_->text().trimmed();
    std::vector<std::size_t> symbols;
    if (automaton_.alphabet_size() == 2 && !text.isEmpty() &&
        !text.contains(QRegularExpression("\\s"))) {
        for (const QChar symbol : text) {
            if (symbol != QLatin1Char('0') && symbol != QLatin1Char('1')) {
                word_result_->setText(tr("Use symbols 0 and 1."));
                return;
            }
            symbols.push_back(symbol == QLatin1Char('1') ? 1U : 0U);
        }
    } else {
        for (const QString& token : text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)) {
            bool valid = false;
            const auto symbol = token.toULongLong(&valid);
            if (!valid || token.startsWith(QLatin1Char('-')) || symbol >= automaton_.alphabet_size()) {
                word_result_->setText(tr("Symbols must be between 0 and %1.")
                    .arg(static_cast<qulonglong>(automaton_.alphabet_size() - 1U)));
                return;
            }
            symbols.push_back(static_cast<std::size_t>(symbol));
        }
    }
    word_result_->setText(automaton_.evaluate(symbols) ? tr("Accepted") : tr("Rejected"));
}

void AutomatonDialog::stopRenderer() {
    if (render_timer_) render_timer_->stop();
    if (!renderer_) return;
    QProcess* old = std::exchange(renderer_, nullptr);
    old->disconnect(this);
    if (old->state() == QProcess::NotRunning) {
        old->deleteLater();
    } else {
        // Do not synchronously wait for a renderer on the GUI thread.
        connect(old, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), old, &QObject::deleteLater);
        old->kill();
    }
}

void AutomatonDialog::renderFailed(const QString& message) {
    stopRenderer();
    graph_->setText(message);
    graph_->setMinimumSize(0, 0);
    status_->setText(message);
    tabs_->setCurrentIndex(1);
}

void AutomatonDialog::renderGraph() {
    stopRenderer();
    graph_->clear();
    graph_->setMinimumSize(0, 0);
    const QString executable = QStandardPaths::findExecutable("dot");
    if (executable.isEmpty()) {
        renderFailed(tr("Graphviz was not found. Install Graphviz and add its bin folder to PATH, then reopen this viewer or click Show demo. DOT source and export are available now."));
        return;
    }
    if (automaton_.state_count() > 512 ||
        automaton_.state_count() * automaton_.alphabet_size() > 4096) {
        renderFailed(tr("This DFA is too large for a readable graph preview (over 512 states or 4,096 transitions). Use the DOT source tab or export DOT for external layout."));
        return;
    }
    rendered_png_.clear();
    render_error_.clear();
    graph_->setText(tr("Rendering graph…"));
    status_->setText(tr("Rendering with Graphviz…"));
    auto* process = new QProcess(this);
    renderer_ = process;
    process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process] {
        if (renderer_ != process) return;
        rendered_png_.append(process->readAllStandardOutput());
        if (rendered_png_.size() > max_png_bytes) {
            renderFailed(tr("Graph preview exceeds the 16 MiB image limit. Export the DOT source to render externally."));
        }
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process] {
        if (renderer_ != process) return;
        const QByteArray chunk = process->readAllStandardError();
        if (render_error_.size() < 4096) render_error_.append(chunk.left(4096 - render_error_.size()));
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (renderer_ != process) return;
        if (error == QProcess::FailedToStart) {
            renderFailed(tr("Could not start Graphviz: %1. Check that dot runs from PATH. The DOT source is available.").arg(process->errorString()));
        }
    });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this, process](int code, QProcess::ExitStatus exit_status) {
        if (renderer_ != process) return;
        rendered_png_.append(process->readAllStandardOutput());
        render_error_.append(process->readAllStandardError().left(4096 - render_error_.size()));
        stopRenderer();
        if (code != 0 || exit_status != QProcess::NormalExit) {
            renderFailed(tr("Graphviz could not render the graph. %1").arg(QString::fromUtf8(render_error_)));
            return;
        }
        if (rendered_png_.size() > max_png_bytes) {
            renderFailed(tr("Graph preview exceeds the 16 MiB image limit. Export the DOT source to render externally."));
            return;
        }
        QBuffer buffer(&rendered_png_);
        buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "PNG");
        const QSize size = reader.size();
        if (!size.isValid() || size.width() > 8192 || size.height() > 8192 ||
            static_cast<qint64>(size.width()) * size.height() > 32 * 1024 * 1024) {
            renderFailed(tr("Graphviz returned an image too large to preview. Export the DOT source to render externally."));
            return;
        }
        const QImage image = reader.read();
        if (image.isNull()) {
            renderFailed(tr("Could not read the Graphviz image: %1").arg(reader.errorString()));
            return;
        }
        const QPixmap pixmap = QPixmap::fromImage(image);
        graph_->setPixmap(pixmap);
        graph_->setMinimumSize(pixmap.size());
        status_->setText(tr("Graph rendered. DOT source and export are available in addition to the preview."));
    });
    const QByteArray source = dot_->toPlainText().toUtf8();
    connect(process, &QProcess::started, this, [this, process, source] {
        if (renderer_ != process) return;
        process->write(source);
        process->closeWriteChannel();
    });
    render_timer_->start(15000);
    process->start(executable, {"-Tpng", "-Gsize=18,12", "-Gdpi=100"});
}

} // namespace mfg::gui
