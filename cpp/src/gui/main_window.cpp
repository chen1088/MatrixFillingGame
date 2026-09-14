#include "main_window.hpp"
#include "automaton_dialog.hpp"
#include "mfg/analysis.hpp"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <thread>
#include <utility>

namespace mfg::gui {
namespace {
QString htmlFormula(const Cnf& formula) {
    if (formula.is_false())
        return QStringLiteral("<p style='color:#ba283d'><b>FALSE</b> — a forbidden rectangle is already fixed.</p>");
    if (formula.clauses().empty())
        return QStringLiteral("<p style='color:#247052'><b>TRUE</b> — no remaining rectangle constraints.</p>");
    QString html = QStringLiteral("<p style='font-size:12pt;line-height:1.8'>");
    bool first_clause = true;
    std::size_t displayed_clauses = 0;
    for (const auto& clause : formula.clauses()) {
        if (displayed_clauses++ >= 1000) break;
        if (!first_clause) html += QStringLiteral(" ∧ <br>");
        first_clause = false;
        html += '(';
        bool first_literal = true;
        for (const auto& literal : clause) {
            if (!first_literal) html += QStringLiteral(" ∨ ");
            first_literal = false;
            html += QStringLiteral("<span style='color:%1'>%2x<sub>%3,%4</sub></span>")
                .arg(literal.positive ? "#b62942" : "#2464af",
                     literal.positive ? "" : "¬")
                .arg(literal.variable.row + 1).arg(literal.variable.col + 1);
        }
        html += ')';
    }
    html += "</p>";
    if (formula.clauses().size() > 1000)
        html += QString("<p><b>Showing the first 1,000 of %1 clauses.</b> Use Analysis → Export CNF for the full formula.</p>").arg(formula.clauses().size());
    return html;
}
QString analysisStatus(AnalysisStatus status, const std::string& note = {}) {
    if (status == AnalysisStatus::Cancelled) return QStringLiteral("Cancelled");
    if (status == AnalysisStatus::LimitReached)
        return note.empty() ? QStringLiteral("Limit reached") : QString::fromStdString(note);
    return QStringLiteral("Not computed");
}
QPushButton* button(const QString& text, const char* name, QWidget* parent = nullptr) {
    auto* result = new QPushButton(text, parent);
    result->setObjectName(QString::fromLatin1(name));
    return result;
}
}

MatrixCanvas::MatrixCanvas(QWidget* parent) : QWidget(parent) {
    setObjectName("matrixCanvas");
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName("Editable matrix");
    setAccessibleDescription("Click toggles a one. Arrow keys move; 0, 1 and B set a cell. Right-click for cell options.");
    setToolTip("Click: 1 ↔ blank / 0 → 1\nArrow keys: select cell · 0 / 1 / B: set cell\nRight-click: cell options");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    resize(sizeHint());
}
void MatrixCanvas::setDocument(const Document& document, bool maximal) {
    displayed_ = maximal ? document.maximal_configuration() : document.matrix();
    ignored_.clear();
    for (int r = 0; r < displayed_.rows(); ++r)
        for (int c = 0; c < displayed_.cols(); ++c)
            if (document.ignored({r, c})) ignored_.insert({r, c});
    selected_.row = std::clamp(selected_.row, 0, displayed_.rows() - 1);
    selected_.col = std::clamp(selected_.col, 0, displayed_.cols() - 1);
    resize(sizeHint());
    updateGeometry();
    update();
}
QSize MatrixCanvas::sizeHint() const {
    return {margin_ + displayed_.cols() * cell_size_ + 12,
            margin_ + displayed_.rows() * cell_size_ + 12};
}
QRect MatrixCanvas::cellRect(Coord p) const {
    return {margin_ + p.col * cell_size_, margin_ + p.row * cell_size_, cell_size_, cell_size_};
}
void MatrixCanvas::setCellSize(int size) {
    cell_size_ = std::clamp(size, 22, 80);
    resize(sizeHint());
    updateGeometry();
    update();
}
void MatrixCanvas::setSelection(Coord p) {
    p.row = std::clamp(p.row, 0, displayed_.rows() - 1);
    p.col = std::clamp(p.col, 0, displayed_.cols() - 1);
    if (p == selected_) return;
    selected_ = p;
    update();
    emit selectionChanged(p.row, p.col);
}
std::optional<Coord> MatrixCanvas::hitTest(QPoint point) const {
    if (point.x() < margin_ || point.y() < margin_) return std::nullopt;
    Coord p{(point.y() - margin_) / cell_size_, (point.x() - margin_) / cell_size_};
    if (p.row >= displayed_.rows() || p.col >= displayed_.cols()) return std::nullopt;
    return p;
}
void MatrixCanvas::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#f7f9fc"));
    QFont heading = font();
    heading.setPointSize(10);
    painter.setFont(heading);
    painter.setPen(QColor("#65748b"));
    const int first_row = std::max(0, (event->rect().top() - margin_) / cell_size_);
    const int last_row = std::min(displayed_.rows() - 1, (event->rect().bottom() - margin_) / cell_size_);
    const int first_col = std::max(0, (event->rect().left() - margin_) / cell_size_);
    const int last_col = std::min(displayed_.cols() - 1, (event->rect().right() - margin_) / cell_size_);
    for (int r = first_row; r <= last_row; ++r)
        painter.drawText(QRect(0, margin_ + r * cell_size_, margin_ - 8, cell_size_), Qt::AlignRight | Qt::AlignVCenter, QString::number(r + 1));
    for (int c = first_col; c <= last_col; ++c)
        painter.drawText(QRect(margin_ + c * cell_size_, 0, cell_size_, margin_ - 4), Qt::AlignCenter, QString::number(c + 1));
    QFont cell_font = font();
    cell_font.setPointSize(std::max(10, cell_size_ / 3));
    cell_font.setWeight(QFont::DemiBold);
    painter.setFont(cell_font);
    for (int r = first_row; r <= last_row; ++r) {
        for (int c = first_col; c <= last_col; ++c) {
            const Coord p{r, c};
            const auto value = displayed_.at(p);
            const QRect box = cellRect(p).adjusted(2, 2, -2, -2);
            painter.setPen(Qt::NoPen);
            painter.setBrush(value == Cell::One ? QColor("#fde8ec") : value == Cell::Zero ? QColor("#e6effc") : QColor("#ffffff"));
            painter.drawRoundedRect(box, 5, 5);
            painter.setPen(value == Cell::One ? QColor("#b92b45") : value == Cell::Zero ? QColor("#2d64ab") : QColor("#9cabc0"));
            painter.drawText(box, Qt::AlignCenter, value == Cell::One ? "1" : value == Cell::Zero ? "0" : QString::fromUtf8("·"));
            if (ignored_.contains(p)) {
                painter.setPen(QPen(QColor("#758197"), 1.4, Qt::DashLine));
                painter.drawLine(box.topLeft() + QPoint(4, 4), box.bottomRight() - QPoint(4, 4));
            }
        }
    }
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor("#304fb7"), hasFocus() ? 2.5 : 1.5));
    painter.drawRoundedRect(cellRect(selected_).adjusted(1, 1, -1, -1), 6, 6);
}
void MatrixCanvas::mousePressEvent(QMouseEvent* event) {
    const auto p = hitTest(event->position().toPoint());
    if (!p) return;
    setFocus(Qt::MouseFocusReason);
    setSelection(*p);
    if (event->button() == Qt::LeftButton) emit cellActivated(p->row, p->col);
}
void MatrixCanvas::keyPressEvent(QKeyEvent* event) {
    Coord next = selected_;
    switch (event->key()) {
    case Qt::Key_Up: --next.row; break;
    case Qt::Key_Down: ++next.row; break;
    case Qt::Key_Left: --next.col; break;
    case Qt::Key_Right: ++next.col; break;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter: emit cellActivated(next.row, next.col); return;
    case Qt::Key_0: emit cellEdited(next.row, next.col, static_cast<int>(Cell::Zero)); return;
    case Qt::Key_1: emit cellEdited(next.row, next.col, static_cast<int>(Cell::One)); return;
    case Qt::Key_B:
    case Qt::Key_Delete:
    case Qt::Key_Backspace: emit cellEdited(next.row, next.col, static_cast<int>(Cell::Blank)); return;
    default: QWidget::keyPressEvent(event); return;
    }
    setSelection(next);
    event->accept();
}
void MatrixCanvas::contextMenuEvent(QContextMenuEvent* event) {
    const auto p = hitTest(event->pos());
    if (p) setSelection(*p);
    QMenu menu(this);
    auto* one = menu.addAction("Set 1");
    auto* zero = menu.addAction("Set 0");
    auto* blank = menu.addAction("Set blank");
    menu.addSeparator();
    auto* ignore = menu.addAction(ignored_.contains(selected_) ? "Toggle cell exclusion" : "Exclude cell from constraints");
    auto* chosen = menu.exec(event->globalPos());
    if (chosen == ignore) emit cellIgnoreRequested(selected_.row, selected_.col);
    else if (chosen) emit cellEdited(selected_.row, selected_.col,
        static_cast<int>(chosen == one ? Cell::One : chosen == zero ? Cell::Zero : Cell::Blank));
}

struct MainWindow::Request {
    Matrix matrix;
    mfg::IgnoreMask mask;
    AnalysisOptions options;
    std::uint64_t serial;
    std::uint64_t revision;
    bool statistics;
};
struct MainWindow::Worker {
    std::atomic<bool> cancelled{false};
    std::atomic<bool> done{false};
    std::mutex mutex;
    QString stage;
    int progress = -1;
    std::optional<AnalysisResult> result;
    QString error;
    std::thread thread;
    std::uint64_t serial = 0;
    std::uint64_t revision = 0;
    bool statistics = false;
    ~Worker() {
        cancelled.store(true);
        if (thread.joinable()) thread.join();
    }
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName("mainWindow");
    resize(1180, 790);
    setMinimumSize(840, 640);
    saved_state_ = document_.serialize();
    buildUi();
    buildMenus();
    worker_timer_ = new QTimer(this);
    worker_timer_->setInterval(40);
    connect(worker_timer_, &QTimer::timeout, this, &MainWindow::pollWorker);
    qApp->installEventFilter(this);
    documentChanged();
}
MainWindow::~MainWindow() {
    qApp->removeEventFilter(this);
    closing_ = true;
    pending_.reset();
    if (worker_) worker_->cancelled.store(true);
    worker_.reset();
}
bool MainWindow::computationRunning() const noexcept { return worker_ || pending_; }

void MainWindow::buildUi() {
    setStyleSheet(QStringLiteral(
        "QMainWindow{background:#f7f9fc;} QWidget{color:#26344c;}"
        "QGroupBox{font-weight:600;border:1px solid #dce3ee;border-radius:8px;margin-top:12px;padding-top:10px;}"
        "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 5px;}"
        "QPushButton{padding:6px 9px;border:1px solid #ccd6e5;border-radius:5px;background:#ffffff;}"
        "QPushButton:hover{background:#edf2fc;border-color:#849ac5;}"
        "QPushButton:pressed{background:#dfe8fc;} QPushButton:disabled{color:#94a0b3;background:#f1f4f8;}"
        "QPushButton#computeButton{background:#3556b9;color:white;border-color:#3556b9;font-weight:600;}"
        "QPushButton#computeButton:hover{background:#2a469d;}"
        "QSpinBox{padding:4px;border:1px solid #ccd6e5;border-radius:4px;background:white;}"
        "QTextBrowser{background:white;border:1px solid #dce3ee;border-radius:6px;padding:6px;}"
        "QToolBar{background:white;border-bottom:1px solid #dce3ee;spacing:5px;padding:4px;}"
        "QProgressBar{border:1px solid #dce3ee;border-radius:4px;text-align:center;background:#edf1f8;}"
        "QProgressBar::chunk{background:#879dde;border-radius:3px;}"));
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(18, 12, 18, 8);
    auto* title_row = new QHBoxLayout;
    auto* title = new QLabel("Matrix Filling Game");
    QFont title_font = title->font(); title_font.setPointSize(20); title_font.setWeight(QFont::DemiBold); title->setFont(title_font);
    title_row->addWidget(title);
    title_row->addStretch();
    dimensions_ = new QLabel;
    dimensions_->setObjectName("dimensionsLabel");
    title_row->addWidget(dimensions_);
    layout->addLayout(title_row);
    auto* subtitle = new QLabel("Edit a sparse support and study completions that avoid ordered 1001 rectangles.");
    subtitle->setStyleSheet("color:#64748b;");
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);
    auto* splitter = new QSplitter(Qt::Horizontal);
    auto* left = new QWidget;
    auto* left_layout = new QVBoxLayout(left); left_layout->setContentsMargins(0, 0, 8, 0);
    auto* view_controls = new QHBoxLayout;
    maximal_ = new QCheckBox("Maximal configuration"); maximal_->setChecked(true); maximal_->setObjectName("maximalViewCheck");
    maximal_->setToolTip("Non-1 cells outside every supported diagonal rectangle are displayed as 0. Turn off to view and analyze raw editable cells. Computations use the current view.");
    view_controls->addWidget(maximal_);
    view_controls->addStretch();
    view_controls->addWidget(new QLabel("Zoom"));
    auto* zoom = new QSlider(Qt::Horizontal); zoom->setObjectName("zoomSlider"); zoom->setRange(22, 80); zoom->setValue(44); zoom->setMaximumWidth(115);
    view_controls->addWidget(zoom);
    left_layout->addLayout(view_controls);
    auto* scroll = new QScrollArea; scroll->setObjectName("matrixScrollArea"); scroll->setFrameShape(QFrame::NoFrame); scroll->setAlignment(Qt::AlignCenter); scroll->setMinimumHeight(230);
    canvas_ = new MatrixCanvas; scroll->setWidget(canvas_);
    left_layout->addWidget(scroll, 1);
    selection_ = new QLabel;
    selection_->setObjectName("selectionLabel");
    left_layout->addWidget(selection_);
    auto* legend = new QLabel("<span style='color:#b92b45'>■ 1</span> &nbsp; <span style='color:#2d64ab'>■ 0</span> &nbsp; · blank &nbsp; / excluded &nbsp;&nbsp; Click to toggle; 0 / 1 / B to set.");
    legend->setWordWrap(true); legend->setStyleSheet("color:#64748b;font-size:11px;");
    left_layout->addWidget(legend);
    auto* edit_box = new QGroupBox("Rows and columns");
    auto* edits = new QGridLayout(edit_box);
    row_ = new QSpinBox; row_->setObjectName("rowSelector"); row_->setPrefix("Row ");
    column_ = new QSpinBox; column_->setObjectName("columnSelector"); column_->setPrefix("Column ");
    edits->addWidget(row_, 0, 0); edits->addWidget(column_, 1, 0);
    auto* row_before = button("Insert before", "insertRowBeforeButton");
    auto* row_after = button("Insert after", "insertRowAfterButton");
    delete_row_ = button("Delete row", "deleteRowButton");
    auto* col_before = button("Insert before", "insertColumnBeforeButton");
    auto* col_after = button("Insert after", "insertColumnAfterButton");
    delete_column_ = button("Delete column", "deleteColumnButton");
    edits->addWidget(row_before, 0, 1); edits->addWidget(row_after, 0, 2); edits->addWidget(delete_row_, 0, 3);
    edits->addWidget(col_before, 1, 1); edits->addWidget(col_after, 1, 2); edits->addWidget(delete_column_, 1, 3);
    left_layout->addWidget(edit_box);
    auto* cell_box = new QGroupBox("Selected cell and exclusions");
    auto* cell_layout = new QGridLayout(cell_box);
    auto* zero = button("Set 0", "setZeroButton"); auto* one = button("Set 1", "setOneButton"); auto* blank = button("Set blank", "setBlankButton");
    cell_layout->addWidget(zero, 0, 0); cell_layout->addWidget(one, 0, 1); cell_layout->addWidget(blank, 0, 2);
    ignore_cell_ = new QCheckBox("Exclude cell"); ignore_cell_->setObjectName("ignoreCellCheck");
    ignore_row_ = new QCheckBox("Exclude row"); ignore_row_->setObjectName("ignoreRowCheck");
    ignore_column_ = new QCheckBox("Exclude column"); ignore_column_->setObjectName("ignoreColumnCheck");
    cell_layout->addWidget(ignore_cell_, 1, 0); cell_layout->addWidget(ignore_row_, 1, 1); cell_layout->addWidget(ignore_column_, 1, 2);
    auto* mask_help = new QLabel("Excluded cells are omitted from the completion universe; rectangles touching them are skipped.");
    mask_help->setWordWrap(true); mask_help->setStyleSheet("color:#64748b;font-size:11px;"); cell_layout->addWidget(mask_help, 2, 0, 1, 3);
    left_layout->addWidget(cell_box);

    auto* right = new QWidget; right->setMinimumWidth(315);
    auto* right_layout = new QVBoxLayout(right); right_layout->setContentsMargins(8, 0, 0, 0);
    auto* analysis_box = new QGroupBox("Computations");
    auto* analysis_layout = new QVBoxLayout(analysis_box);
    auto* run_controls = new QHBoxLayout;
    compute_ = button("Compute", "computeButton"); cancel_ = button("Cancel", "cancelButton"); cancel_->setEnabled(false);
    automatic_ = new QCheckBox("Auto-compute"); automatic_->setObjectName("automaticCheck"); automatic_->setChecked(true);
    run_controls->addWidget(compute_); run_controls->addWidget(cancel_); run_controls->addWidget(automatic_); run_controls->addStretch();
    analysis_layout->addLayout(run_controls);
    const QStringList names{"CountOfM", "RatioOfM", "APRIS", "Hypothesis", "BlankStat"};
    const QStringList descriptions{
        "Exact number of valid completions (including one completion when there are no blanks).",
        "Exact valid-completion count divided by 2 to the number of active blanks.",
        "Estimated valid-completion ratio with Monte Carlo standard error; an importance-sampling estimate is shown in the tooltip.",
        "Forced-implication greedy fill. Works requires a complete matrix verified to avoid all 1001 rectangles.",
        "B0 / B: diagonally paired blanks with zero opposite corners / all active blanks."};
    auto* statistics = new QGridLayout;
    for (int i = 0; i < 5; ++i) {
        enabled_[i] = new QCheckBox(names[i]); enabled_[i]->setObjectName("enable" + names[i]); enabled_[i]->setChecked(true); enabled_[i]->setToolTip(descriptions[i]);
        results_[i] = new QLabel("Not computed"); results_[i]->setObjectName("result" + names[i]); results_[i]->setWordWrap(true); results_[i]->setTextInteractionFlags(Qt::TextSelectableByMouse);
        results_[i]->setToolTip(descriptions[i]);
        statistics->addWidget(enabled_[i], i, 0); statistics->addWidget(results_[i], i, 1);
        connect(enabled_[i], &QCheckBox::toggled, this, [this] { queueComputation(automatic_->isChecked()); });
    }
    statistics->setColumnStretch(1, 1);
    analysis_layout->addLayout(statistics);
    auto* limits = new QHBoxLayout;
    samples_ = new QSpinBox; samples_->setObjectName("sampleCountSpin"); samples_->setRange(100, 10000000); samples_->setSingleStep(10000); samples_->setValue(100000); samples_->setToolTip("Monte Carlo samples; estimates use a fixed seed for reproducibility.");
    exact_limit_ = new QSpinBox; exact_limit_->setObjectName("exactLimitSpin"); exact_limit_->setRange(0, 40); exact_limit_->setValue(24); exact_limit_->setToolTip("Maximum constrained blank variables for exact enumeration. Free variables are counted without enumeration.");
    limits->addWidget(new QLabel("Samples")); limits->addWidget(samples_, 1); limits->addWidget(new QLabel("Exact limit")); limits->addWidget(exact_limit_);
    analysis_layout->addLayout(limits);
    connect(samples_, &QSpinBox::valueChanged, this, [this] { queueComputation(automatic_->isChecked()); });
    connect(exact_limit_, &QSpinBox::valueChanged, this, [this] { queueComputation(automatic_->isChecked()); });
    progress_ = new QProgressBar; progress_->setObjectName("computationProgress"); progress_->setRange(0, 100); progress_->setValue(0); progress_->setMaximumHeight(15); progress_->setTextVisible(false);
    analysis_layout->addWidget(progress_);
    computation_status_ = new QLabel("Ready"); computation_status_->setObjectName("computationStatus"); computation_status_->setWordWrap(true); computation_status_->setStyleSheet("color:#64748b;font-size:11px;");
    analysis_layout->addWidget(computation_status_);
    right_layout->addWidget(analysis_box);
    auto* formula_box = new QGroupBox("Rectangle CNF");
    auto* formula_layout = new QVBoxLayout(formula_box);
    auto* formula_help = new QLabel("Uses the current matrix view. Coordinates start at 1; all ordered row pairs and column pairs are considered.");
    formula_help->setWordWrap(true); formula_help->setStyleSheet("color:#64748b;font-size:11px;"); formula_layout->addWidget(formula_help);
    formula_ = new QTextBrowser; formula_->setObjectName("formulaBrowser"); formula_->setMinimumHeight(130); formula_layout->addWidget(formula_, 1);
    right_layout->addWidget(formula_box, 1);
    splitter->addWidget(left); splitter->addWidget(right); splitter->setStretchFactor(0, 3); splitter->setStretchFactor(1, 2); splitter->setSizes({620, 460});
    layout->addWidget(splitter, 1);
    setCentralWidget(central);
    statusBar()->showMessage("Ready");

    connect(zoom, &QSlider::valueChanged, canvas_, &MatrixCanvas::setCellSize);
    connect(maximal_, &QCheckBox::toggled, this, [this] { canvas_->setDocument(document_, maximal_->isChecked()); updateSelection(); queueComputation(automatic_->isChecked()); });
    connect(canvas_, &MatrixCanvas::cellActivated, this, [this](int r, int c) { if (document_.toggle_cell({r, c})) documentChanged(); });
    connect(canvas_, &MatrixCanvas::cellEdited, this, [this](int r, int c, int v) { setCell({r, c}, static_cast<Cell>(v)); });
    connect(canvas_, &MatrixCanvas::selectionChanged, this, [this, scroll](int r, int c) { updateSelection(); const auto rect = canvas_->cellRect({r, c}); scroll->ensureVisible(rect.center().x(), rect.center().y(), rect.width(), rect.height()); });
    connect(canvas_, &MatrixCanvas::cellIgnoreRequested, this, [this](int r, int c) { if (document_.set_cell_ignored({r, c}, !document_.ignored_cells().contains({r, c}))) documentChanged(); });
    connect(row_, &QSpinBox::valueChanged, this, [this](int r) { canvas_->setSelection({r - 1, canvas_->selection().col}); });
    connect(column_, &QSpinBox::valueChanged, this, [this](int c) { canvas_->setSelection({canvas_->selection().row, c - 1}); });
    connect(row_before, &QPushButton::clicked, this, &MainWindow::insertRowBefore);
    connect(row_after, &QPushButton::clicked, this, &MainWindow::insertRowAfter);
    connect(delete_row_, &QPushButton::clicked, this, &MainWindow::deleteRow);
    connect(col_before, &QPushButton::clicked, this, &MainWindow::insertColumnBefore);
    connect(col_after, &QPushButton::clicked, this, &MainWindow::insertColumnAfter);
    connect(delete_column_, &QPushButton::clicked, this, &MainWindow::deleteColumn);
    connect(zero, &QPushButton::clicked, this, [this] { setSelectedValue(Cell::Zero); });
    connect(one, &QPushButton::clicked, this, [this] { setSelectedValue(Cell::One); });
    connect(blank, &QPushButton::clicked, this, [this] { setSelectedValue(Cell::Blank); });
    connect(ignore_cell_, &QCheckBox::toggled, this, [this](bool checked) { if (document_.set_cell_ignored(canvas_->selection(), checked)) documentChanged(); });
    connect(ignore_row_, &QCheckBox::toggled, this, [this](bool checked) { if (document_.set_row_ignored(canvas_->selection().row, checked)) documentChanged(); });
    connect(ignore_column_, &QCheckBox::toggled, this, [this](bool checked) { if (document_.set_column_ignored(canvas_->selection().col, checked)) documentChanged(); });
    connect(compute_, &QPushButton::clicked, this, &MainWindow::compute);
    connect(cancel_, &QPushButton::clicked, this, &MainWindow::cancelComputation);
    connect(automatic_, &QCheckBox::toggled, this, [this](bool checked) { queueComputation(checked); });
}

void MainWindow::buildMenus() {
    auto* file = menuBar()->addMenu("&File");
    auto* new_action = file->addAction("&New…", QKeySequence::New, this, &MainWindow::newDocument); new_action->setObjectName("newAction");
    auto* open_action = file->addAction("&Open…", QKeySequence::Open, this, &MainWindow::openFile); open_action->setObjectName("openAction");
    auto* save_action = file->addAction("&Save", QKeySequence::Save, this, [this] { saveFile(false); }); save_action->setObjectName("saveAction");
    file->addAction("Save &as…", QKeySequence::SaveAs, this, [this] { saveFile(true); });
    file->addSeparator();
    file->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);
    auto* edit = menuBar()->addMenu("&Edit");
    undo_action_ = edit->addAction("&Undo", QKeySequence::Undo, this, &MainWindow::undo); undo_action_->setObjectName("undoAction");
    redo_action_ = edit->addAction("&Redo", QKeySequence::Redo, this, &MainWindow::redo); redo_action_->setObjectName("redoAction");
    edit->addSeparator();
    auto* copy = edit->addAction("Copy matrix data", this, &MainWindow::copyDocument); copy->setShortcut(QKeySequence("Ctrl+Shift+C")); copy->setObjectName("copyMatrixAction");
    auto* paste = edit->addAction("Paste matrix data", this, &MainWindow::pasteDocument); paste->setShortcut(QKeySequence("Ctrl+Shift+V")); paste->setObjectName("pasteMatrixAction");
    edit->addSeparator();
    edit->addAction("Insert row before selection", this, &MainWindow::insertRowBefore);
    edit->addAction("Insert row after selection", this, &MainWindow::insertRowAfter);
    edit->addAction("Delete selected row", this, &MainWindow::deleteRow);
    edit->addAction("Insert column before selection", this, &MainWindow::insertColumnBefore);
    edit->addAction("Insert column after selection", this, &MainWindow::insertColumnAfter);
    edit->addAction("Delete selected column", this, &MainWindow::deleteColumn);
    auto* analysis = menuBar()->addMenu("&Analysis");
    analysis->addAction("&Compute selected statistics", QKeySequence("F5"), this, &MainWindow::compute);
    export_formula_ = analysis->addAction("Export CNF…", this, &MainWindow::exportFormula);
    export_formula_->setObjectName("exportCnfAction");
    export_formula_->setEnabled(false);
    analysis->addAction("Cancel computation", QKeySequence("Escape"), this, &MainWindow::cancelComputation);
    auto* tools = menuBar()->addMenu("&Tools");
    auto* automata = tools->addAction("Automata…", this, [this] {
        auto* dialog = new AutomatonDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });
    automata->setObjectName("automataAction");
    auto* help = menuBar()->addMenu("&Help");
    help->addAction("Controls and conventions", this, [this] {
        QMessageBox::information(this, "Matrix Filling Game",
            "Click a cell to toggle its support: 1 becomes blank; 0 or blank becomes 1.\n"
            "Arrow keys select a cell. Press 0, 1 or B to set its raw value.\n\n"
            "Maximal configuration shows the normalization used by the Java application. "
            "Computations use the current view, including exclusions. Turn off the view checkbox to inspect and analyze raw cells.\n\n"
            "Rows and columns are numbered from 1 in the interface. Select any row/column and insert before or after it; "
            "the last row and column cannot be deleted. All edits, loads and exclusions support Undo and Redo.\n\n"
            "CNF refreshes on every edit. Compute (F5) runs the selected statistics. Auto-compute runs them after every change. "
            "APRIS is an estimate, with its standard error shown. Exact counting may reach the chosen variable limit.\n\n"
            "Open accepts six-field semicolon records and rectangular text using 0, 1 and b / . / ? for blanks. "
            "Saved .mfg files also preserve exclusion masks. The desktop editor supports up to 256 × 256 cells.");
    });
    auto* toolbar = addToolBar("Document"); toolbar->setObjectName("documentToolbar"); toolbar->setMovable(false);
    toolbar->addAction(new_action); toolbar->addAction(open_action); toolbar->addAction(save_action); toolbar->addSeparator(); toolbar->addAction(undo_action_); toolbar->addAction(redo_action_);
}

bool MainWindow::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::ShortcutOverride && event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(object, event);
    auto* widget = qobject_cast<QWidget*>(object);
    if (!widget || widget->window() != this) return QMainWindow::eventFilter(object, event);
    auto* key = static_cast<QKeyEvent*>(event);
    const bool redo_key = key->matches(QKeySequence::Redo) ||
        (key->key() == Qt::Key_Z && (key->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) && (key->modifiers() & Qt::ShiftModifier));
    const bool undo_key = !redo_key && key->matches(QKeySequence::Undo);
    if (!undo_key && !redo_key) return QMainWindow::eventFilter(object, event);
    // Claim the shortcut even when a text widget/spin box has focus. A plain Z
    // is never an undo command. ShortcutOverride prevents competing actions.
    key->accept();
    if (event->type() == QEvent::KeyPress) {
        if (redo_key) redo(); else undo();
    }
    return true;
}
void MainWindow::updateTitle() {
    const bool dirty = document_.serialize() != saved_state_;
    setWindowModified(dirty);
    const QString name = file_path_.isEmpty() ? "Untitled" : QFileInfo(file_path_).fileName();
    setWindowTitle(name + "[*] — Matrix Filling Game");
}
void MainWindow::documentChanged() {
    canvas_->setDocument(document_, maximal_->isChecked());
    undo_action_->setEnabled(document_.can_undo()); redo_action_->setEnabled(document_.can_redo());
    delete_row_->setEnabled(document_.matrix().rows() > 1); delete_column_->setEnabled(document_.matrix().cols() > 1);
    dimensions_->setText(QString("%1 rows × %2 columns").arg(document_.matrix().rows()).arg(document_.matrix().cols()));
    updateSelection(); updateTitle();
    queueComputation(automatic_->isChecked());
}
void MainWindow::updateSelection() {
    const QSignalBlocker row_block(row_), col_block(column_);
    row_->setRange(1, document_.matrix().rows()); column_->setRange(1, document_.matrix().cols());
    const Coord selected = canvas_->selection(); row_->setValue(selected.row + 1); column_->setValue(selected.col + 1);
    const Cell raw = document_.matrix().at(selected);
    const QString raw_value = raw == Cell::One ? "1" : raw == Cell::Zero ? "0" : "blank";
    selection_->setText(QString("Selected (%1, %2)  ·  raw value: %3").arg(selected.row + 1).arg(selected.col + 1).arg(raw_value));
    updateMasks();
}
void MainWindow::updateMasks() {
    const Coord selected = canvas_->selection();
    const QSignalBlocker cell_block(ignore_cell_), row_block(ignore_row_), col_block(ignore_column_);
    ignore_cell_->setChecked(document_.ignored_cells().contains(selected));
    ignore_row_->setChecked(document_.ignored_rows().contains(selected.row));
    ignore_column_->setChecked(document_.ignored_columns().contains(selected.col));
}
bool MainWindow::setCell(Coord p, Cell value) { if (!document_.set_cell(p, value)) return false; documentChanged(); return true; }
void MainWindow::setSelectedValue(Cell value) { setCell(canvas_->selection(), value); }
void MainWindow::undo() { if (document_.undo()) documentChanged(); }
void MainWindow::redo() { if (document_.redo()) documentChanged(); }
void MainWindow::insertRowBefore() {
    if (document_.matrix().rows() >= 256) { statusBar()->showMessage("The desktop editor supports up to 256 rows.", 5000); return; }
    if (document_.insert_row(canvas_->selection().row)) documentChanged();
}
void MainWindow::insertRowAfter() {
    if (document_.matrix().rows() >= 256) { statusBar()->showMessage("The desktop editor supports up to 256 rows.", 5000); return; }
    const Coord selected = canvas_->selection();
    if (document_.insert_row(selected.row + 1)) { documentChanged(); canvas_->setSelection({selected.row + 1, selected.col}); }
}
void MainWindow::deleteRow() { if (document_.remove_row(canvas_->selection().row)) documentChanged(); }
void MainWindow::insertColumnBefore() {
    if (document_.matrix().cols() >= 256) { statusBar()->showMessage("The desktop editor supports up to 256 columns.", 5000); return; }
    if (document_.insert_column(canvas_->selection().col)) documentChanged();
}
void MainWindow::insertColumnAfter() {
    if (document_.matrix().cols() >= 256) { statusBar()->showMessage("The desktop editor supports up to 256 columns.", 5000); return; }
    const Coord selected = canvas_->selection();
    if (document_.insert_column(selected.col + 1)) { documentChanged(); canvas_->setSelection({selected.row, selected.col + 1}); }
}
void MainWindow::deleteColumn() { if (document_.remove_column(canvas_->selection().col)) documentChanged(); }

void MainWindow::clearResults(bool statistics) {
    displayed_revision_.reset();
    current_formula_.reset();
    export_formula_->setEnabled(false);
    formula_->setPlainText("Updating constraints…");
    for (int i = 0; i < 5; ++i) {
        results_[i]->setText(!enabled_[i]->isChecked() ? "Disabled" : statistics ? "Queued…" : "Not computed");
        results_[i]->setToolTip(enabled_[i]->toolTip());
    }
}
void MainWindow::queueComputation(bool statistics) {
    if (closing_) return;
    ++request_serial_;
    clearResults(statistics);
    AnalysisOptions options;
    options.compute_exact = statistics && (enabled_[0]->isChecked() || enabled_[1]->isChecked());
    options.compute_estimate = statistics && enabled_[2]->isChecked();
    options.compute_importance = options.compute_estimate;
    options.compute_greedy = statistics && enabled_[3]->isChecked();
    options.sample_count = static_cast<std::uint64_t>(samples_->value());
    options.max_exact_variables = static_cast<std::size_t>(exact_limit_->value());
    mfg::IgnoreMask mask(static_cast<std::size_t>(document_.matrix().rows()) * document_.matrix().cols(), false);
    bool any_ignored = false;
    for (int r = 0; r < document_.matrix().rows(); ++r)
        for (int c = 0; c < document_.matrix().cols(); ++c) {
            const bool ignored = document_.ignored({r, c});
            mask[static_cast<std::size_t>(r) * document_.matrix().cols() + c] = ignored;
            any_ignored = any_ignored || ignored;
        }
    if (!any_ignored) mask.clear();
    // Snapshots cannot change underneath a running computation. At most one
    // active worker and one replacement request exist, regardless of edit rate.
    pending_ = std::make_unique<Request>(Request{maximal_->isChecked() ? document_.maximal_configuration() : document_.matrix(), std::move(mask), options,
        request_serial_, document_.revision(), statistics});
    if (worker_) worker_->cancelled.store(true);
    computation_status_->setText(worker_ ? "Cancelling earlier computation; latest matrix is queued…" : "Computing…");
    cancel_->setEnabled(true);
    if (!worker_) startPending();
}
void MainWindow::startPending() {
    if (closing_ || worker_ || !pending_) return;
    Request request = std::move(*pending_); pending_.reset();
    worker_ = std::make_unique<Worker>();
    Worker* job = worker_.get();
    job->serial = request.serial; job->revision = request.revision; job->statistics = request.statistics;
    progress_->setRange(0, 0);
    computation_status_->setText(request.statistics ? "Computing selected statistics…" : "Updating rectangle constraints…");
    job->thread = std::thread([job, request = std::move(request)]() mutable {
        request.options.cancelled = [job] { return job->cancelled.load(); };
        request.options.progress = [job](const std::string& stage, std::uint64_t done, std::uint64_t total) {
            std::lock_guard lock(job->mutex);
            job->stage = QString::fromStdString(stage);
            job->progress = total == 0 ? -1 : static_cast<int>(std::min(100.0, 100.0 * static_cast<double>(done) / static_cast<double>(total)));
        };
        try { job->result = analyze_matrix(request.matrix, request.mask, request.options); }
        catch (const AnalysisCancelled&) { job->error = "Cancelled"; }
        catch (const std::exception& error) { job->error = QString::fromUtf8(error.what()); }
        catch (...) { job->error = "An unexpected computation error occurred."; }
        job->done.store(true, std::memory_order_release);
    });
    worker_timer_->start();
}
void MainWindow::pollWorker() {
    if (!worker_) { worker_timer_->stop(); return; }
    if (!worker_->done.load(std::memory_order_acquire)) {
        if (!pending_) {
            std::lock_guard lock(worker_->mutex);
            if (!worker_->stage.isEmpty()) computation_status_->setText(worker_->stage);
            if (worker_->progress < 0) progress_->setRange(0, 0);
            else { progress_->setRange(0, 100); progress_->setValue(worker_->progress); }
        }
        return;
    }
    const bool current = worker_->serial == request_serial_ && worker_->revision == document_.revision() && !worker_->cancelled.load();
    if (current) {
        if (!worker_->error.isEmpty()) {
            computation_status_->setText("Computation failed: " + worker_->error);
            formula_->setPlainText("Computation failed: " + worker_->error);
            for (int i = 0; i < 5; ++i) if (enabled_[i]->isChecked()) results_[i]->setText("Unavailable");
        } else if (worker_->result) {
            const auto& result = *worker_->result;
            displayed_revision_ = worker_->revision;
            current_formula_ = result.cnf;
            export_formula_->setEnabled(current_formula_.has_value());
            formula_->setHtml(result.cnf ? htmlFormula(*result.cnf) : QString("<p>%1</p>").arg(QString::fromStdString(result.cnf_note).toHtmlEscaped()));
            if (worker_->statistics) {
                if (enabled_[0]->isChecked()) results_[0]->setText(result.exact.status == AnalysisStatus::Complete ? QString::fromStdString(result.exact.valid_count) : analysisStatus(result.exact.status, result.exact.note));
                if (enabled_[1]->isChecked()) results_[1]->setText(result.exact.status == AnalysisStatus::Complete ? QString::number(result.exact.valid_ratio, 'g', 12) : analysisStatus(result.exact.status, result.exact.note));
                if (enabled_[2]->isChecked()) {
                    results_[2]->setText(result.uniform.status == AnalysisStatus::Complete
                        ? QString("%1 ± %2 SE").arg(result.uniform.valid_ratio, 0, 'g', 8).arg(result.uniform.standard_error, 0, 'g', 3)
                        : analysisStatus(result.uniform.status, result.uniform.note));
                    if (result.uniform.status == AnalysisStatus::Complete && result.uniform.samples > 0 &&
                        (result.uniform.valid_ratio == 0.0 || result.uniform.valid_ratio == 1.0))
                        results_[2]->setText(QString("%1 / %2 samples valid")
                            .arg(result.uniform.valid_ratio == 0.0 ? 0 : result.uniform.samples).arg(result.uniform.samples));
                    if (result.importance.status == AnalysisStatus::Complete)
                        results_[2]->setToolTip(QString("Uniform estimate from %1 samples. A sample proportion of 0 or 1 does not prove that the true proportion is 0 or 1. Importance estimate: %2 ± %3 SE (%4 samples).")
                            .arg(result.uniform.samples).arg(result.importance.valid_ratio, 0, 'g', 10).arg(result.importance.standard_error, 0, 'g', 4).arg(result.importance.samples));
                }
                if (enabled_[3]->isChecked()) {
                    if (result.greedy) {
                        const auto& greedy = *result.greedy;
                        results_[3]->setText(greedy.status == AnalysisStatus::Complete ? (greedy.completed && greedy.valid ? "Works" : "Fails") : analysisStatus(greedy.status));
                        if (greedy.violation) { const auto rect = *greedy.violation; results_[3]->setToolTip(QString("Forbidden rectangle: rows %1, %2; columns %3, %4.").arg(rect.top + 1).arg(rect.bottom + 1).arg(rect.left + 1).arg(rect.right + 1)); }
                    } else results_[3]->setText("Unavailable");
                }
                if (enabled_[4]->isChecked()) results_[4]->setText(result.blanks.status == AnalysisStatus::Complete ? QString("%1 / %2").arg(result.blanks.b0).arg(result.blanks.blanks) : analysisStatus(result.blanks.status));
            }
            QString status = worker_->statistics ? "Up to date" : "Constraints up to date · Press Compute for statistics";
            if (result.status == AnalysisStatus::LimitReached) status = "Computation limit reached";
            if (result.status == AnalysisStatus::Cancelled) status = "Cancelled";
            if (!result.note.empty()) status += " · " + QString::fromStdString(result.note);
            if (!result.cnf_note.empty()) status += " · " + QString::fromStdString(result.cnf_note);
            computation_status_->setText(status);
        }
    }
    worker_.reset();
    if (pending_) startPending();
    else { worker_timer_->stop(); cancel_->setEnabled(false); progress_->setRange(0, 100); progress_->setValue(current ? 100 : 0); }
}
void MainWindow::compute() { queueComputation(true); }
void MainWindow::cancelComputation() {
    ++request_serial_;
    pending_.reset();
    if (worker_) worker_->cancelled.store(true);
    displayed_revision_.reset();
    current_formula_.reset();
    export_formula_->setEnabled(false);
    for (int i = 0; i < 5; ++i) if (enabled_[i]->isChecked()) results_[i]->setText("Cancelled");
    formula_->setPlainText("Computation cancelled. Press Compute to refresh.");
    computation_status_->setText("Cancelled");
    cancel_->setEnabled(false);
}

bool MainWindow::loadText(const QString& text, QString* error) {
    try {
        if (text.size() > 16 * 1024 * 1024) throw std::invalid_argument("Matrix input exceeds the desktop editor's 16 MB limit.");
        Document candidate;
        candidate.load(text.toStdString());
        if (candidate.matrix().rows() > 256 || candidate.matrix().cols() > 256)
            throw std::invalid_argument("The desktop editor supports matrices up to 256 rows and 256 columns.");
        if (document_.load(text.toStdString())) documentChanged();
        return true;
    } catch (const std::exception& exception) {
        if (error) *error = QString::fromUtf8(exception.what());
        return false;
    }
}
bool MainWindow::openPath(const QString& path, QString* error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { if (error) *error = file.errorString(); return false; }
    if (file.size() > 16 * 1024 * 1024) { if (error) *error = "Matrix input exceeds the desktop editor's 16 MB limit."; return false; }
    if (!loadText(QString::fromUtf8(file.readAll()), error)) return false;
    file_path_ = path; saved_state_ = document_.serialize(); updateTitle();
    statusBar()->showMessage("Opened " + path, 5000);
    return true;
}
void MainWindow::openFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Open matrix", file_path_, "Matrix files (*.mfg *.txt);;All files (*)");
    if (path.isEmpty() || !confirmDiscard()) return;
    QString error;
    if (!openPath(path, &error)) QMessageBox::warning(this, "Cannot open matrix", error);
}
void MainWindow::saveFile(bool saveAs) {
    QString path = file_path_;
    if (path.isEmpty() || saveAs) {
        path = QFileDialog::getSaveFileName(this, "Save matrix", path.isEmpty() ? "matrix.mfg" : path, "Matrix files (*.mfg);;All files (*)");
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) path += ".mfg";
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { QMessageBox::warning(this, "Cannot save matrix", file.errorString()); return; }
    const auto contents = QByteArray::fromStdString(document_.serialize()) + '\n';
    if (file.write(contents) != contents.size() || !file.commit()) { QMessageBox::warning(this, "Cannot save matrix", file.errorString()); return; }
    file_path_ = path; saved_state_ = document_.serialize(); updateTitle();
    statusBar()->showMessage("Saved " + path, 5000);
}
bool MainWindow::confirmDiscard() {
    if (document_.serialize() == saved_state_) return true;
    const auto answer = QMessageBox::question(this, "Save changes?", "Save changes to the current matrix?", QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Save) { saveFile(false); return document_.serialize() == saved_state_; }
    return true;
}
void MainWindow::newDocument() {
    QDialog dialog(this); dialog.setWindowTitle("New matrix");
    auto* layout = new QFormLayout(&dialog);
    QSpinBox rows, columns; rows.setRange(1, 256); columns.setRange(1, 256); rows.setValue(6); columns.setValue(6);
    layout->addRow("Rows", &rows); layout->addRow("Columns", &columns);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted || !confirmDiscard()) return;
    document_.clear(rows.value(), columns.value());
    file_path_.clear(); saved_state_ = document_.serialize(); canvas_->setSelection({0, 0}); documentChanged();
}
void MainWindow::exportFormula() {
    if (!current_formula_) return;
    const QString path = QFileDialog::getSaveFileName(this, "Export rectangle CNF", "matrix-cnf.txt", "Text files (*.txt);;All files (*)");
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { QMessageBox::warning(this, "Cannot export CNF", file.errorString()); return; }
    const auto contents = QByteArray("# Matrix Filling Game CNF. Variables xROW_COLUMN use zero-based coordinates.\n") + QByteArray::fromStdString(current_formula_->to_string()) + '\n';
    if (file.write(contents) != contents.size() || !file.commit()) { QMessageBox::warning(this, "Cannot export CNF", file.errorString()); return; }
    statusBar()->showMessage("Exported full CNF to " + path, 5000);
}
void MainWindow::copyDocument() {
    QApplication::clipboard()->setText(QString::fromStdString(document_.serialize()));
    statusBar()->showMessage("Matrix data copied, including exclusions.", 3500);
}
void MainWindow::pasteDocument() {
    QString error;
    if (!loadText(QApplication::clipboard()->text(), &error)) QMessageBox::warning(this, "Invalid matrix data", error);
    else statusBar()->showMessage("Matrix data pasted. Undo restores the previous matrix.", 3500);
}
void MainWindow::closeEvent(QCloseEvent* event) {
    if (!confirmDiscard()) { event->ignore(); return; }
    closing_ = true; pending_.reset();
    if (worker_) worker_->cancelled.store(true);
    worker_timer_->stop();
    // Worker holds no QWidget/QObject references. Destruction joins it after
    // cooperative cancellation, so neither queued updates nor dangling UI
    // callbacks can outlive the window.
    event->accept();
}

} // namespace mfg::gui
