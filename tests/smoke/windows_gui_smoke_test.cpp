#include <QtTest/QtTest>

#include <algorithm>
#include <functional>

#define SPT_GUI_NO_MAIN
#include "../../Source/gui/main.cpp"

class SectionPropertySmokeTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void windowConstructsAndSectionsCalculate() {
        CrossSectionWindow window;
        QVERIFY(window.propertyTableForTest());
        QVERIFY(window.propertyTableForTest()->rowCount() > 0);

        QVector<int> rowCounts;
        for (int i = 0; i < window.sectionTypeComboForTest()->count(); ++i) {
            window.sectionTypeComboForTest()->setCurrentIndex(i);
            QApplication::processEvents();
            rowCounts.push_back(window.parameterTableForTest()->rowCount());
            QVERIFY(propertyValue(&window, "Area") > 0.0);
            QCOMPARE(window.stressTableForTest()->rowCount(), 4);
        }
        QVERIFY(std::adjacent_find(rowCounts.begin(), rowCounts.end(), std::not_equal_to<int>()) != rowCounts.end());
    }

    void meshSceneHasItems() {
        CrossSectionWindow window;
        QVERIFY(window.meshSceneForTest());
        QVERIFY(!window.meshSceneForTest()->items().isEmpty());
    }

    void canvasUShapeBuilds() {
        CrossSectionWindow window;
        window.clearCanvasForTest();
        window.addCanvasPlateForTest({{0.0, 0.0}, {100.0, 0.0}, 10.0, 0, "bottom"});
        window.addCanvasPlateForTest({{50.0, 0.0}, {50.0, 120.0}, 8.0, 0, "web"});
        window.addCanvasPlateForTest({{0.0, 120.0}, {100.0, 120.0}, 10.0, 0, "top"});
        window.buildCanvasSectionForTest();
        QCOMPARE(window.canvasTableForTest()->rowCount(), 3);
        QVERIFY(std::abs(propertyValue(&window, "Area") - 2960.0) < 1.0e-6);
    }

private:
    double propertyValue(CrossSectionWindow* window, const QString& name) const {
        auto* table = window->propertyTableForTest();
        for (int row = 0; row < table->rowCount(); ++row) {
            if (table->item(row, 0) && table->item(row, 0)->text() == name) {
                return table->item(row, 1)->text().toDouble();
            }
        }
        return 0.0;
    }
};

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    QApplication app(argc, argv);
    SectionPropertySmokeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "windows_gui_smoke_test.moc"
