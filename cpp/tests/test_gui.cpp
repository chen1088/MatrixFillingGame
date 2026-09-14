#include "main_window.hpp"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextBrowser>
#include <QTemporaryDir>
#include <QFile>
#include <QtTest>

using mfg::Cell;
using mfg::Coord;
using mfg::gui::MainWindow;

class GuiTests final : public QObject {
    Q_OBJECT
private:
    static QString diagonal(int size) {
        QString text;
        for (int r = 0; r < size; ++r) {
            for (int c = 0; c < size; ++c) text += r == c ? '1' : 'b';
            text += '\n';
        }
        return text;
    }
    static void expose(MainWindow& window) {
        window.show();
        window.activateWindow();
        QApplication::processEvents();
    }
private slots:
    void undoWithEveryInputFocus() {
        MainWindow window;
        expose(window);
        const QList<QWidget*> targets{
            window.canvas(),
            window.findChild<QPushButton*>("deleteRowButton"),
            window.findChild<QSpinBox*>("rowSelector")->findChild<QLineEdit*>(),
            window.findChild<QTextBrowser*>("formulaBrowser")};
        for (auto* target : targets) {
            QVERIFY(target);
            QVERIFY(window.setCell({0, 0}, Cell::One));
            target->setFocus();
            QApplication::processEvents();
            QTest::keyClick(target, Qt::Key_Z, Qt::ControlModifier);
            QCOMPARE(window.document().matrix().at({0, 0}), Cell::Blank);
            QTest::keyClick(target, Qt::Key_Z, Qt::ControlModifier | Qt::ShiftModifier);
            QCOMPARE(window.document().matrix().at({0, 0}), Cell::One);
            window.undo();
        }
        QVERIFY(window.setCell({0, 0}, Cell::One));
        window.canvas()->setFocus();
        QTest::keyClick(window.canvas(), Qt::Key_Z);
        QCOMPARE(window.document().matrix().at({0, 0}), Cell::One);
    }
    void clickKeyboardAndMasks() {
        MainWindow window;
        expose(window);
        auto* canvas = window.canvas();
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, canvas->cellRect({2, 3}).center());
        QCOMPARE(canvas->selection(), (Coord{2, 3}));
        QCOMPARE(window.document().matrix().at({2, 3}), Cell::One);
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, canvas->cellRect({2, 3}).center());
        QCOMPARE(window.document().matrix().at({2, 3}), Cell::Blank);
        QTest::keyClick(canvas, Qt::Key_0);
        QCOMPARE(window.document().matrix().at({2, 3}), Cell::Zero);
        auto* excluded = window.findChild<QCheckBox*>("ignoreCellCheck");
        excluded->click();
        QVERIFY(window.document().ignored_cells().contains({2, 3}));
        window.undo();
        QVERIFY(!window.document().ignored_cells().contains({2, 3}));
        QVERIFY(!excluded->isChecked());
        QTest::keyClick(canvas, Qt::Key_Right);
        QCOMPARE(canvas->selection(), (Coord{2, 4}));
        window.findChild<QPushButton*>("setOneButton")->click();
        QCOMPARE(window.document().matrix().at({2, 4}), Cell::One);
    }
    void deletionRestoresCoordinatesAndBoundaries() {
        MainWindow window;
        expose(window);
        QVERIFY(window.loadText("1001\n0100\n0010"));
        const auto original = window.document().serialize();
        window.canvas()->setSelection({2, 3});
        auto* delete_row = window.findChild<QPushButton*>("deleteRowButton");
        QTest::mouseClick(delete_row, Qt::LeftButton);
        QCOMPARE(window.document().matrix().rows(), 2);
        QCOMPARE(window.canvas()->selection(), (Coord{1, 3}));
        QTest::keyClick(delete_row, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(window.document().serialize(), original);
        auto* delete_col = window.findChild<QPushButton*>("deleteColumnButton");
        QTest::mouseClick(delete_col, Qt::LeftButton);
        QCOMPARE(window.document().matrix().cols(), 3);
        QTest::keyClick(delete_col, Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(window.document().serialize(), original);
        while (window.document().matrix().rows() > 1) delete_row->click();
        while (window.document().matrix().cols() > 1) delete_col->click();
        QVERIFY(!delete_row->isEnabled());
        QVERIFY(!delete_col->isEnabled());
        const auto single = window.document().serialize();
        window.deleteRow(); window.deleteColumn();
        QCOMPARE(window.document().serialize(), single);
        window.findChild<QPushButton*>("insertRowBeforeButton")->click();
        window.findChild<QPushButton*>("insertColumnBeforeButton")->click();
        QCOMPARE(window.document().matrix().rows(), 2);
        QCOMPARE(window.document().matrix().cols(), 2);
        window.canvas()->setSelection({1, 1});
        window.findChild<QPushButton*>("insertRowAfterButton")->click();
        window.findChild<QPushButton*>("insertColumnAfterButton")->click();
        QCOMPARE(window.document().matrix().rows(), 3);
        QCOMPARE(window.document().matrix().cols(), 3);
        QCOMPARE(window.canvas()->selection(), (Coord{2, 2}));
    }
    void malformedLoadIsAtomic() {
        MainWindow window;
        QVERIFY(window.loadText("10\nb1"));
        const auto original = window.document().serialize();
        const auto revision = window.document().revision();
        QString error;
        QVERIFY(!window.loadText("10\n1", &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(window.document().serialize(), original);
        QCOMPARE(window.document().revision(), revision);
        QVERIFY(!window.loadText("257;1;;;;", &error));
        QCOMPARE(window.document().serialize(), original);
    }
    void openedFileHasPathAndCleanState() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("example.mfg");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("1b\nb1\n"), qint64(6));
        file.close();
        MainWindow window;
        QString error;
        QVERIFY2(window.openPath(path, &error), qPrintable(error));
        QVERIFY(window.windowTitle().startsWith("example.mfg"));
        QVERIFY(!window.isWindowModified());
        QVERIFY(window.setCell({0, 1}, Cell::Zero));
        QVERIFY(window.isWindowModified());
        window.undo();
        QVERIFY(!window.isWindowModified());
    }
    void latestSnapshotWinsAndResultsClearImmediately() {
        MainWindow window;
        expose(window);
        window.findChild<QSpinBox*>("sampleCountSpin")->setValue(1000000);
        window.findChild<QCheckBox*>("automaticCheck")->setChecked(true);
        QVERIFY(window.loadText(diagonal(9)));
        window.compute();
        QVERIFY(window.computationRunning());
        for (int i = 0; i < 8; ++i) window.setCell({0, 1}, i % 2 ? Cell::One : Cell::Blank);
        QVERIFY(window.loadText("0"));
        QCOMPARE(window.findChild<QLabel*>("resultCountOfM")->text(), QString("Queued…"));
        QVERIFY(!window.displayedRevision());
        QTRY_VERIFY_WITH_TIMEOUT(!window.computationRunning(), 15000);
        QVERIFY(window.displayedRevision());
        QCOMPARE(*window.displayedRevision(), window.document().revision());
        QCOMPARE(window.findChild<QLabel*>("resultCountOfM")->text(), QString("1"));
        QCOMPARE(window.findChild<QLabel*>("resultBlankStat")->text(), QString("0 / 0"));
        QVERIFY(window.findChild<QTextBrowser*>("formulaBrowser")->toPlainText().contains("TRUE"));
    }
    void viewChangeSelectsAnalysisMatrix() {
        MainWindow window;
        expose(window);
        window.findChild<QSpinBox*>("sampleCountSpin")->setValue(100);
        window.findChild<QCheckBox*>("automaticCheck")->setChecked(true);
        QVERIFY(window.loadText("b"));
        QTRY_VERIFY_WITH_TIMEOUT(!window.computationRunning(), 5000);
        QCOMPARE(window.findChild<QLabel*>("resultCountOfM")->text(), QString("1"));
        window.findChild<QCheckBox*>("maximalViewCheck")->setChecked(false);
        QVERIFY(!window.displayedRevision());
        QTRY_VERIFY_WITH_TIMEOUT(!window.computationRunning(), 5000);
        QCOMPARE(window.findChild<QLabel*>("resultCountOfM")->text(), QString("2"));
        QCOMPARE(window.findChild<QLabel*>("resultBlankStat")->text(), QString("0 / 1"));
    }
    void cancellationAndDestructionWhileComputing() {
        auto window = std::make_unique<MainWindow>();
        window->findChild<QSpinBox*>("sampleCountSpin")->setValue(10000000);
        QVERIFY(window->loadText(diagonal(12)));
        window->compute();
        QVERIFY(window->computationRunning());
        window->cancelComputation();
        QTRY_VERIFY_WITH_TIMEOUT(!window->computationRunning(), 5000);
        QCOMPARE(window->findChild<QLabel*>("resultCountOfM")->text(), QString("Cancelled"));
        QVERIFY(!window->displayedRevision());
        window->compute();
        QElapsedTimer timer; timer.start();
        window.reset();
        QVERIFY2(timer.elapsed() < 5000, "Window destruction must cancel and join its worker promptly.");
    }
};

QTEST_MAIN(GuiTests)
#include "test_gui.moc"
