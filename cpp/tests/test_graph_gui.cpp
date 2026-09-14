#include "automaton_dialog.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QtTest>

#include <sstream>

class GraphGuiTest final : public QObject {
    Q_OBJECT
private slots:
    void loadEvaluateAndRejectMalformed() {
        mfg::gui::AutomatonDialog dialog;
        dialog.show();
        QString error;
        QVERIFY(dialog.loadText("2 2 0\n0 1 0\n1 0 1\n", &error));
        QCOMPARE(dialog.automaton().state_count(), std::size_t{2});
        auto* word = dialog.findChild<QLineEdit*>("automatonWord");
        auto* evaluate = dialog.findChild<QPushButton*>("automatonEvaluate");
        auto* result = dialog.findChild<QLabel*>("automatonWordResult");
        QVERIFY(word && evaluate && result);
        word->setText("0");
        QTest::mouseClick(evaluate, Qt::LeftButton);
        QVERIFY(result->text().contains("accepted", Qt::CaseInsensitive));
        word->setText("0 0");
        QTest::mouseClick(evaluate, Qt::LeftButton);
        QVERIFY(result->text().contains("rejected", Qt::CaseInsensitive));
        QVERIFY(!dialog.loadText("2 2 0\n1 999 0\n", &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(dialog.automaton().evaluate({0}));
        QVERIFY(!dialog.automaton().evaluate({0, 0}));
        auto* dot = dialog.findChild<QPlainTextEdit*>("automatonDot");
        QVERIFY(dot && dot->toPlainText().contains("doublecircle"));
        auto* minimize = dialog.findChild<QPushButton*>("automatonMinimize");
        QVERIFY(minimize);
        QTest::mouseClick(minimize, Qt::LeftButton);
        QCOMPARE(dialog.automaton().state_count(), std::size_t{2});
    }

    void asynchronousRenderingAndClose() {
        mfg::gui::AutomatonDialog dialog;
        dialog.show();
        auto* status = dialog.findChild<QLabel*>("automatonStatus");
        QVERIFY(status);
        if (!QStandardPaths::findExecutable("dot").isEmpty()) {
            QTRY_VERIFY_WITH_TIMEOUT(!status->text().contains("Rendering", Qt::CaseInsensitive), 20000);
            QVERIFY2(!status->text().contains("failed", Qt::CaseInsensitive), qPrintable(status->text()));
        } else {
            QTRY_VERIFY_WITH_TIMEOUT(status->text().contains("Graphviz", Qt::CaseInsensitive), 2000);
        }
        const QString screenshot = qEnvironmentVariable("MFG_GRAPH_SCREENSHOT");
        if (!screenshot.isEmpty()) QVERIFY(dialog.grab().save(screenshot));
        // Replacing a graph cancels its renderer; closing immediately must
        // safely release that process without a late callback to a dead UI.
        QVERIFY(dialog.loadText("2 2 0\n0 1 0\n1 0 1\n"));
        dialog.close();
    }
};

QTEST_MAIN(GraphGuiTest)
#include "test_graph_gui.moc"
