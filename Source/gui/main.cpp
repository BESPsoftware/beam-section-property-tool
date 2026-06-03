#include "calculation/SectionCalculator.h"
#include "geometry/SectionBuilder.h"
#include "mesh/MeshEngine.h"
#include "stress/StressPointEngine.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsPolygonItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace {

QString unitForProperty(const QString& name) {
    if (name == "Area" || name == "Az" || name == "Ay") {
        return "mm2";
    }
    if (name.startsWith("J")) {
        return "mm4";
    }
    if (name == "theta") {
        return "rad";
    }
    return "mm";
}

QPointF toQt(spt::PointYZ point) {
    return QPointF(point.y, -point.z);
}

QRectF usableSceneRect(QRectF rect) {
    if (!rect.isValid() || rect.isNull()) {
        rect = QRectF(-50.0, -50.0, 100.0, 100.0);
    }
    const double maxDimension = std::max(rect.width(), rect.height());
    const double margin = std::max(20.0, maxDimension * 0.08);
    rect = rect.adjusted(-margin, -margin, margin, margin);

    constexpr double minimumSpan = 100.0;
    if (rect.width() < minimumSpan) {
        const double delta = 0.5 * (minimumSpan - rect.width());
        rect.adjust(-delta, 0.0, delta, 0.0);
    }
    if (rect.height() < minimumSpan) {
        const double delta = 0.5 * (minimumSpan - rect.height());
        rect.adjust(0.0, -delta, 0.0, delta);
    }
    return rect;
}

void updateSceneRect(QGraphicsScene* scene) {
    if (!scene) {
        return;
    }
    scene->setSceneRect(usableSceneRect(scene->itemsBoundingRect()));
}

QPen cosmeticPen(const QColor& color, double width = 1.0, Qt::PenStyle style = Qt::SolidLine) {
    QPen pen(color, width, style);
    pen.setCosmetic(true);
    return pen;
}

class ZoomableGraphicsView : public QGraphicsView {
public:
    explicit ZoomableGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr)
        : QGraphicsView(scene, parent) {
        setRenderHint(QPainter::Antialiasing);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorViewCenter);
        setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    }

    void zoomIn() {
        scaleView(1.25);
    }

    void zoomOut() {
        scaleView(0.8);
    }

    void fitToScene() {
        if (!scene()) {
            return;
        }
        const QRectF rect = usableSceneRect(scene()->itemsBoundingRect());
        scene()->setSceneRect(rect);
        resetTransform();
        fitInView(rect, Qt::KeepAspectRatio);
        if (std::max(std::abs(transform().m11()), std::abs(transform().m22())) > 8.0) {
            resetTransform();
            scale(8.0, 8.0);
            centerOn(rect.center());
        }
    }

    void resetView() {
        resetTransform();
        if (scene()) {
            centerOn(scene()->sceneRect().center());
        }
    }

protected:
    void wheelEvent(QWheelEvent* event) override {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else if (event->angleDelta().y() < 0) {
            zoomOut();
        }
        event->accept();
    }

private:
    void scaleView(double factor) {
        const double nextScale = std::abs(transform().m11() * factor);
        if (nextScale < 1.0e-6 || nextScale > 1.0e6) {
            return;
        }
        scale(factor, factor);
    }
};

QWidget* buildViewPanel(ZoomableGraphicsView* view) {
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* tools = new QHBoxLayout;
    tools->setContentsMargins(0, 0, 0, 0);

    auto* zoomIn = new QPushButton("Zoom +");
    auto* zoomOut = new QPushButton("Zoom -");
    auto* fit = new QPushButton("Fit");
    auto* reset = new QPushButton("Reset");

    QObject::connect(zoomIn, &QPushButton::clicked, view, [view] { view->zoomIn(); });
    QObject::connect(zoomOut, &QPushButton::clicked, view, [view] { view->zoomOut(); });
    QObject::connect(fit, &QPushButton::clicked, view, [view] { view->fitToScene(); });
    QObject::connect(reset, &QPushButton::clicked, view, [view] { view->resetView(); });

    tools->addWidget(zoomIn);
    tools->addWidget(zoomOut);
    tools->addWidget(fit);
    tools->addWidget(reset);
    tools->addStretch(1);
    layout->addLayout(tools);
    layout->addWidget(view, 1);
    return panel;
}

class CrossSectionWindow : public QMainWindow {
public:
    CrossSectionWindow() {
        setWindowTitle("Cross-Section");
        resize(980, 720);

        tabs_ = new QTabWidget(this);
        setCentralWidget(tabs_);
        buildGeneralTab();
        buildStressTab();
        buildMeshTab();
        buildCanvasTab();
        setDefaultSection(0);
        recalculate();
    }

private:
    QTabWidget* tabs_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QTableWidget* parameterTable_ = nullptr;
    QTableWidget* propertyTable_ = nullptr;
    QTableWidget* stressTable_ = nullptr;
    QTableWidget* canvasTable_ = nullptr;
    QDoubleSpinBox* meshRefinement_ = nullptr;
    QGraphicsScene* generalScene_ = nullptr;
    QGraphicsScene* stressScene_ = nullptr;
    QGraphicsScene* meshScene_ = nullptr;
    ZoomableGraphicsView* generalView_ = nullptr;
    ZoomableGraphicsView* stressView_ = nullptr;
    ZoomableGraphicsView* meshView_ = nullptr;
    spt::SectionModel model_;
    spt::CalculationResult result_;

    void buildGeneralTab() {
        auto* page = new QWidget;
        auto* layout = new QGridLayout(page);
        typeCombo_ = new QComboBox;
        typeCombo_->addItem("H Section", static_cast<int>(spt::SectionType::HSection));
        typeCombo_->addItem("Box Section", static_cast<int>(spt::SectionType::BoxSection));
        typeCombo_->addItem("Pipe Section", static_cast<int>(spt::SectionType::PipeSection));
        typeCombo_->addItem("Quayside Crane Girder", static_cast<int>(spt::SectionType::QuaysideCraneGirder));
        connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            setDefaultSection(index);
            recalculate();
        });

        parameterTable_ = new QTableWidget;
        parameterTable_->setColumnCount(3);
        parameterTable_->setHorizontalHeaderLabels({"Name", "Value", "Unit"});
        parameterTable_->horizontalHeader()->setStretchLastSection(true);
        connect(parameterTable_, &QTableWidget::cellChanged, this, [this](int, int column) {
            if (column == 1) {
                recalculate();
            }
        });

        propertyTable_ = new QTableWidget;
        propertyTable_->setColumnCount(3);
        propertyTable_->setHorizontalHeaderLabels({"Name", "Value", "Unit"});
        propertyTable_->horizontalHeader()->setStretchLastSection(true);

        generalScene_ = new QGraphicsScene(this);
        generalView_ = new ZoomableGraphicsView(generalScene_);

        auto* apply = new QPushButton("Apply");
        connect(apply, &QPushButton::clicked, this, [this] { recalculate(); });
        auto* ok = new QPushButton("OK");
        connect(ok, &QPushButton::clicked, this, [this] { close(); });
        auto* cancel = new QPushButton("Cancel");
        connect(cancel, &QPushButton::clicked, this, [this] { close(); });

        layout->addWidget(new QLabel("Category"), 0, 0);
        layout->addWidget(typeCombo_, 0, 1);
        layout->addWidget(new QLabel("Geometry Parameters"), 1, 0, 1, 2);
        layout->addWidget(parameterTable_, 2, 0, 1, 2);
        layout->addWidget(buildViewPanel(generalView_), 0, 2, 3, 1);
        layout->addWidget(new QLabel("Properties"), 3, 0, 1, 3);
        layout->addWidget(propertyTable_, 4, 0, 1, 3);
        layout->addWidget(ok, 5, 0);
        layout->addWidget(cancel, 5, 1);
        layout->addWidget(apply, 5, 2);
        tabs_->addTab(page, "General");
    }

    void buildStressTab() {
        auto* page = new QWidget;
        auto* layout = new QGridLayout(page);
        stressTable_ = new QTableWidget;
        stressTable_->setColumnCount(5);
        stressTable_->setHorizontalHeaderLabels({"ID", "y", "z", "y0", "z0"});
        stressTable_->horizontalHeader()->setStretchLastSection(true);
        connect(stressTable_, &QTableWidget::cellChanged, this, [this](int, int column) {
            if (column == 1 || column == 2) {
                applyStressEdits();
            }
        });
        stressScene_ = new QGraphicsScene(this);
        stressView_ = new ZoomableGraphicsView(stressScene_);
        auto* reset = new QPushButton("Reset Defaults");
        connect(reset, &QPushButton::clicked, this, [this] {
            recalculate();
        });
        layout->addWidget(stressTable_, 0, 0);
        layout->addWidget(buildViewPanel(stressView_), 0, 1);
        layout->addWidget(reset, 1, 0, 1, 2);
        tabs_->addTab(page, "Stress Points");
    }

    void buildMeshTab() {
        auto* page = new QWidget;
        auto* layout = new QGridLayout(page);
        meshRefinement_ = new QDoubleSpinBox;
        meshRefinement_->setRange(0.1, 10.0);
        meshRefinement_->setValue(1.0);
        meshRefinement_->setSingleStep(0.25);
        connect(meshRefinement_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            updateMesh();
        });
        meshScene_ = new QGraphicsScene(this);
        meshView_ = new ZoomableGraphicsView(meshScene_);
        layout->addWidget(new QLabel("Mesh refinement factor"), 0, 0);
        layout->addWidget(meshRefinement_, 0, 1);
        layout->addWidget(buildViewPanel(meshView_), 1, 0, 1, 2);
        tabs_->addTab(page, "FE Mesh");
    }

    void buildCanvasTab() {
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        canvasTable_ = new QTableWidget;
        canvasTable_->setColumnCount(6);
        canvasTable_->setHorizontalHeaderLabels({"y1", "z1", "y2", "z2", "t", "id"});
        canvasTable_->setRowCount(3);
        const double values[3][5] = {{0, 0, 100, 0, 10}, {50, 0, 50, 120, 8}, {0, 120, 100, 120, 10}};
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 5; ++c) {
                canvasTable_->setItem(r, c, new QTableWidgetItem(QString::number(values[r][c])));
            }
            canvasTable_->setItem(r, 5, new QTableWidgetItem(QString("plate_%1").arg(r + 1)));
        }
        auto* fromCanvas = new QPushButton("Build Canvas Section");
        connect(fromCanvas, &QPushButton::clicked, this, [this] {
            buildCanvasSection();
            recalculate();
            tabs_->setCurrentIndex(0);
        });
        layout->addWidget(canvasTable_);
        layout->addWidget(fromCanvas);
        tabs_->addTab(page, "Canvas");
    }

    void setDefaultSection(int index) {
        parameterTable_->blockSignals(true);
        parameterTable_->setRowCount(0);
        const auto type = static_cast<spt::SectionType>(typeCombo_->itemData(index).toInt());
        std::map<QString, double> values;
        switch (type) {
            case spt::SectionType::HSection:
                values = {{"A", 100}, {"H", 210}, {"e", 20}, {"f", 12}};
                break;
            case spt::SectionType::BoxSection:
                values = {{"A", 1320}, {"B", 1250}, {"H", 2600}, {"D", 40}, {"E", 16}, {"H1", 600}, {"D1", 22}, {"E1", 16}};
                break;
            case spt::SectionType::PipeSection:
                values = {{"Do", 1300}, {"t", 14}};
                break;
            case spt::SectionType::QuaysideCraneGirder:
                values = {{"A", 766}, {"B", 836}, {"G", 1090}, {"D", 1160}, {"e", 20}, {"f", 12}, {"H", 2000}, {"W", 934}, {"M", 350}, {"N", 518}, {"p", 20}, {"s", 12}, {"t", 10}, {"u", 10}, {"M1", 175}, {"k", 150}, {"k1", 12}, {"h", 138}, {"h1", 10}};
                break;
            case spt::SectionType::CanvasThinWalled:
                break;
        }
        for (const auto& item : values) {
            const int row = parameterTable_->rowCount();
            parameterTable_->insertRow(row);
            parameterTable_->setItem(row, 0, new QTableWidgetItem(item.first));
            parameterTable_->setItem(row, 1, new QTableWidgetItem(QString::number(item.second)));
            parameterTable_->setItem(row, 2, new QTableWidgetItem("mm"));
        }
        parameterTable_->blockSignals(false);
    }

    void recalculate() {
        spt::SectionParameters params;
        params.type = static_cast<spt::SectionType>(typeCombo_->currentData().toInt());
        for (int r = 0; r < parameterTable_->rowCount(); ++r) {
            params.values[parameterTable_->item(r, 0)->text().toStdString()] = parameterTable_->item(r, 1)->text().toDouble();
        }
        auto built = spt::SectionBuilder::build(params);
        if (!built.ok()) {
            QMessageBox::warning(this, "Invalid Section", QString::fromStdString(built.diagnostics.front().message));
            return;
        }
        model_ = built.model;
        result_ = spt::SectionCalculator::calculate(model_);
        updateProperties();
        updateStressTable();
        drawSection(generalScene_, true, generalView_, true);
        drawSection(stressScene_, true, stressView_, true);
        updateMesh(true);
    }

    void buildCanvasSection() {
        std::vector<spt::PlateSegment> plates;
        for (int r = 0; r < canvasTable_->rowCount(); ++r) {
            spt::PlateSegment plate;
            plate.start.y = canvasTable_->item(r, 0)->text().toDouble();
            plate.start.z = canvasTable_->item(r, 1)->text().toDouble();
            plate.end.y = canvasTable_->item(r, 2)->text().toDouble();
            plate.end.z = canvasTable_->item(r, 3)->text().toDouble();
            plate.thickness = canvasTable_->item(r, 4)->text().toDouble();
            plate.id = canvasTable_->item(r, 5)->text().toStdString();
            plates.push_back(plate);
        }
        auto built = spt::SectionBuilder::buildFromCanvasLines(plates);
        if (built.ok()) {
            model_ = built.model;
            result_ = spt::SectionCalculator::calculate(model_);
        }
    }

    void updateProperties() {
        propertyTable_->blockSignals(true);
        propertyTable_->setRowCount(0);
        const auto add = [this](const QString& name, double value) {
            const int row = propertyTable_->rowCount();
            propertyTable_->insertRow(row);
            propertyTable_->setItem(row, 0, new QTableWidgetItem(name));
            propertyTable_->setItem(row, 1, new QTableWidgetItem(QString::number(value, 'f', 2)));
            propertyTable_->setItem(row, 2, new QTableWidgetItem(unitForProperty(name)));
        };
        const auto& p = result_.properties;
        add("Area", p.area);
        add("Jz", p.Jz);
        add("Jy", p.Jy);
        add("Jyz", p.Jyz);
        add("Jzo", p.Jzo);
        add("Jyo", p.Jyo);
        add("Jx", p.Jx);
        add("Az", p.Az);
        add("Ay", p.Ay);
        add("cy", p.cy);
        add("cz", p.cz);
        add("theta", p.theta);
        propertyTable_->blockSignals(false);
    }

    void updateStressTable() {
        stressTable_->blockSignals(true);
        stressTable_->setRowCount(0);
        for (const auto& point : result_.stressPoints) {
            const int row = stressTable_->rowCount();
            stressTable_->insertRow(row);
            stressTable_->setItem(row, 0, new QTableWidgetItem(QString::number(point.id)));
            stressTable_->setItem(row, 1, new QTableWidgetItem(QString::number(point.global.y, 'f', 2)));
            stressTable_->setItem(row, 2, new QTableWidgetItem(QString::number(point.global.z, 'f', 2)));
            stressTable_->setItem(row, 3, new QTableWidgetItem(QString::number(point.principal.y, 'f', 2)));
            stressTable_->setItem(row, 4, new QTableWidgetItem(QString::number(point.principal.z, 'f', 2)));
        }
        stressTable_->blockSignals(false);
    }

    void applyStressEdits() {
        for (int r = 0; r < stressTable_->rowCount() && r < static_cast<int>(result_.stressPoints.size()); ++r) {
            result_.stressPoints[r].global.y = stressTable_->item(r, 1)->text().toDouble();
            result_.stressPoints[r].global.z = stressTable_->item(r, 2)->text().toDouble();
            result_.stressPoints[r].principal = spt::StressPointEngine::toPrincipal(result_.stressPoints[r].global, result_.properties);
        }
        drawSection(stressScene_, true, stressView_, false);
    }

    void drawSection(QGraphicsScene* scene, bool includeStress, ZoomableGraphicsView* view, bool fit) {
        scene->clear();
        const QPen outline = cosmeticPen(Qt::black, 1.4);
        const QBrush fill(QColor(85, 125, 255, 70));
        for (const auto& contour : model_.contours) {
            QPolygonF polygon;
            for (const auto& point : contour.outer) {
                polygon << toQt(point);
            }
            scene->addPolygon(polygon, outline, fill);
        }
        if (includeStress) {
            for (const auto& point : result_.stressPoints) {
                const QPointF p = toQt(point.global);
                auto* marker = scene->addEllipse(-4, -4, 8, 8, cosmeticPen(Qt::red), QBrush(Qt::red));
                marker->setPos(p);
                marker->setFlag(QGraphicsItem::ItemIgnoresTransformations);
                auto* label = scene->addText(QString::number(point.id));
                label->setPos(p + QPointF(4, -18));
                label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
            }
        }
        updateSceneRect(scene);
        if (fit && view) {
            view->fitToScene();
        }
    }

    void updateMesh(bool fit = false) {
        if (!meshScene_) {
            return;
        }
        meshScene_->clear();
        spt::MeshSettings settings;
        settings.refinementFactor = meshRefinement_ ? meshRefinement_->value() : 1.0;
        auto mesh = spt::MeshEngine::generate(model_, settings);
        result_.meshSummary = mesh;
        const QPen pen = cosmeticPen(QColor(120, 120, 120), 0.5);
        for (const auto& tri : mesh.triangles) {
            if (tri.n1 < 0 || tri.n2 < 0 || tri.n3 < 0) {
                continue;
            }
            QPolygonF polygon;
            polygon << toQt(mesh.nodes[tri.n1]) << toQt(mesh.nodes[tri.n2]) << toQt(mesh.nodes[tri.n3]);
            meshScene_->addPolygon(polygon, pen, QBrush(Qt::NoBrush));
        }
        updateSceneRect(meshScene_);
        if (fit && meshView_) {
            meshView_->fitToScene();
        }
    }
};

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    CrossSectionWindow window;
    window.show();
    return app.exec();
}
