#pragma once

#ifndef Q_MOC_RUN
#include "mfg/document.hpp"
#include "mfg/cnf.hpp"
#endif

#include <QMainWindow>
#include <QWidget>

#include <cstdint>
#include <memory>
#include <optional>

class QAction;
class QCheckBox;
class QCloseEvent;
class QLabel;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTextBrowser;
class QTimer;

namespace mfg::gui {

class MatrixCanvas final : public QWidget {
    Q_OBJECT
public:
    explicit MatrixCanvas(QWidget* parent = nullptr);
    void setDocument(const Document& document, bool maximal);
    void setSelection(Coord position);
    [[nodiscard]] Coord selection() const noexcept { return selected_; }
    [[nodiscard]] QRect cellRect(Coord position) const;
    [[nodiscard]] QSize sizeHint() const override;
    void setCellSize(int size);

signals:
    void cellActivated(int row, int column);
    void cellEdited(int row, int column, int value);
    void selectionChanged(int row, int column);
    void cellIgnoreRequested(int row, int column);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;

private:
    std::optional<Coord> hitTest(QPoint point) const;
    Matrix displayed_{6, 6};
    std::set<Coord> ignored_;
    Coord selected_{};
    int cell_size_ = 44;
    static constexpr int margin_ = 38;
};

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    [[nodiscard]] const Document& document() const noexcept { return document_; }
    [[nodiscard]] MatrixCanvas* canvas() const noexcept { return canvas_; }
    [[nodiscard]] bool computationRunning() const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> displayedRevision() const noexcept {
        return displayed_revision_;
    }
    // File and clipboard input share the same atomic parser. Errors do not
    // change the current document or its undo history.
    bool loadText(const QString& text, QString* error = nullptr);
    bool openPath(const QString& path, QString* error = nullptr);
    bool setCell(Coord position, Cell value);

public slots:
    void undo();
    void redo();
    void insertRowBefore();
    void insertRowAfter();
    void deleteRow();
    void insertColumnBefore();
    void insertColumnAfter();
    void deleteColumn();
    void compute();
    void cancelComputation();

protected:
    bool eventFilter(QObject*, QEvent*) override;
    void closeEvent(QCloseEvent*) override;

private:
    struct Request;
    struct Worker;
    void buildUi();
    void buildMenus();
    void documentChanged();
    void updateSelection();
    void queueComputation(bool statistics);
    void startPending();
    void pollWorker();
    void clearResults(bool statistics);
    void openFile();
    void saveFile(bool saveAs);
    void newDocument();
    bool confirmDiscard();
    void exportFormula();
    void copyDocument();
    void pasteDocument();
    void updateTitle();
    void updateMasks();
    void setSelectedValue(Cell value);

    Document document_;
    MatrixCanvas* canvas_ = nullptr;
    QCheckBox* maximal_ = nullptr;
    QCheckBox* automatic_ = nullptr;
    QCheckBox* enabled_[5]{};
    QLabel* results_[5]{};
    QCheckBox* ignore_cell_ = nullptr;
    QCheckBox* ignore_row_ = nullptr;
    QCheckBox* ignore_column_ = nullptr;
    QSpinBox* row_ = nullptr;
    QSpinBox* column_ = nullptr;
    QSpinBox* samples_ = nullptr;
    QSpinBox* exact_limit_ = nullptr;
    QLabel* dimensions_ = nullptr;
    QLabel* selection_ = nullptr;
    QLabel* computation_status_ = nullptr;
    QTextBrowser* formula_ = nullptr;
    QProgressBar* progress_ = nullptr;
    QPushButton* compute_ = nullptr;
    QPushButton* cancel_ = nullptr;
    QPushButton* delete_row_ = nullptr;
    QPushButton* delete_column_ = nullptr;
    QAction* undo_action_ = nullptr;
    QAction* redo_action_ = nullptr;
    QAction* export_formula_ = nullptr;
    std::optional<Cnf> current_formula_;
    QTimer* worker_timer_ = nullptr;
    std::unique_ptr<Request> pending_;
    std::unique_ptr<Worker> worker_;
    std::uint64_t request_serial_ = 0;
    std::optional<std::uint64_t> displayed_revision_;
    QString file_path_;
    std::string saved_state_;
    bool closing_ = false;
};

} // namespace mfg::gui
