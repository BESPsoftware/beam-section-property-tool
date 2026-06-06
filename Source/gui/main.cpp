#include "calculation/SectionCalculator.h"
#include "geometry/GeometryUtils.h"
#include "geometry/SectionBuilder.h"
#include "mesh/MeshEngine.h"
#include "stress/StressPointEngine.h"

#include <QtCore/QLineF>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStringList>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
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
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace {

QString unitForProperty(const QString& name) {
    if (name == "Area" || name == "Az" || name == "Ay") {
        return "mm2";
    }
    if (name == "Cw") {
        return "mm6";
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

spt::PointYZ fromQt(const QPointF& point) {
    return {point.x(), -point.y()};
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

enum class CanvasTool {
    Select,
    DrawPlate
};

class CanvasGraphicsView : public ZoomableGraphicsView {
public:
    explicit CanvasGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr)
        : ZoomableGraphicsView(scene, parent) {
        setTool(CanvasTool::Select);
    }

    void setTool(CanvasTool tool) {
        tool_ = tool;
        cancelDrawing();
        setDragMode(tool_ == CanvasTool::DrawPlate ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
        viewport()->setCursor(tool_ == CanvasTool::DrawPlate ? Qt::CrossCursor : Qt::ArrowCursor);
    }

    void setPlateDrawnHandler(std::function<void(spt::PointYZ, spt::PointYZ)> handler) {
        plateDrawnHandler_ = std::move(handler);
    }

    void setPlateSelectedHandler(std::function<void(int)> handler) {
        plateSelectedHandler_ = std::move(handler);
    }

    void setEndpointProvider(std::function<std::vector<std::pair<int, spt::PlateSegment>>()> provider) {
        endpointProvider_ = std::move(provider);
    }

    void setEndpointMovedHandler(std::function<void(int, bool, spt::PointYZ, bool)> handler) {
        endpointMovedHandler_ = std::move(handler);
    }

    void setShowGrid(bool show) {
        showGrid_ = show;
        viewport()->update();
    }

    void setSnapToGrid(bool snap) {
        snapToGrid_ = snap;
    }

    void cancelDrawing() {
        drawing_ = false;
        draggingEndpoint_ = false;
        removePreviewLine();
    }

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override {
        QGraphicsView::drawBackground(painter, rect);
        if (!showGrid_) {
            return;
        }

        double step = gridSpacing_;
        while ((rect.width() / step) > 240.0 || (rect.height() / step) > 240.0) {
            step *= 2.0;
        }

        std::vector<QLineF> lines;
        for (double x = std::floor(rect.left() / step) * step; x <= rect.right(); x += step) {
            lines.push_back(QLineF(x, rect.top(), x, rect.bottom()));
        }
        for (double y = std::floor(rect.top() / step) * step; y <= rect.bottom(); y += step) {
            lines.push_back(QLineF(rect.left(), y, rect.right(), y));
        }

        painter->save();
        painter->setPen(cosmeticPen(QColor(210, 215, 225), 0.0));
        if (!lines.empty()) {
            painter->drawLines(lines.data(), static_cast<int>(lines.size()));
        }
        painter->restore();
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (tool_ == CanvasTool::DrawPlate && event->button() == Qt::LeftButton) {
            const QPointF point = snapPoint(mapToScene(event->pos()));
            if (!drawing_) {
                drawing_ = true;
                drawStart_ = point;
                previewLine_ = scene()->addLine(QLineF(drawStart_, drawStart_), cosmeticPen(QColor(25, 118, 210), 1.8, Qt::DashLine));
                previewLine_->setZValue(10.0);
            } else {
                const spt::PointYZ start = fromQt(drawStart_);
                const spt::PointYZ end = fromQt(point);
                cancelDrawing();
                if (plateDrawnHandler_) {
                    plateDrawnHandler_(start, end);
                }
            }
            event->accept();
            return;
        }

        if (tool_ == CanvasTool::Select && event->button() == Qt::LeftButton) {
            const QPointF scenePoint = mapToScene(event->pos());
            if (endpointProvider_ && endpointMovedHandler_) {
                const double radiusScene = 8.0 / std::max(1.0e-9, std::abs(transform().m11()));
                const double radius2 = radiusScene * radiusScene;
                int bestRow = -1;
                bool bestIsStart = true;
                double bestDistance2 = radius2;
                for (const auto& item : endpointProvider_()) {
                    const QPointF start = toQt(item.second.start);
                    const QPointF end = toQt(item.second.end);
                    const double ds = QLineF(scenePoint, start).length();
                    const double de = QLineF(scenePoint, end).length();
                    if (ds * ds <= bestDistance2) {
                        bestDistance2 = ds * ds;
                        bestRow = item.first;
                        bestIsStart = true;
                    }
                    if (de * de <= bestDistance2) {
                        bestDistance2 = de * de;
                        bestRow = item.first;
                        bestIsStart = false;
                    }
                }
                if (bestRow >= 0) {
                    draggingEndpoint_ = true;
                    dragRow_ = bestRow;
                    dragStartEndpoint_ = bestIsStart;
                    if (plateSelectedHandler_) {
                        plateSelectedHandler_(bestRow);
                    }
                    endpointMovedHandler_(dragRow_, dragStartEndpoint_, fromQt(snapPoint(scenePoint)), false);
                    event->accept();
                    return;
                }
            }

            QGraphicsItem* item = itemAt(event->pos());
            while (item && !item->data(0).isValid()) {
                item = item->parentItem();
            }
            bool ok = false;
            const int row = item ? item->data(0).toInt(&ok) : -1;
            if (ok && plateSelectedHandler_) {
                plateSelectedHandler_(row);
                event->accept();
                return;
            }
        }

        ZoomableGraphicsView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (tool_ == CanvasTool::Select && draggingEndpoint_ && endpointMovedHandler_) {
            endpointMovedHandler_(dragRow_, dragStartEndpoint_, fromQt(snapPoint(mapToScene(event->pos()))), false);
            event->accept();
            return;
        }
        if (tool_ == CanvasTool::DrawPlate && drawing_ && previewLine_) {
            previewLine_->setLine(QLineF(drawStart_, snapPoint(mapToScene(event->pos()))));
            event->accept();
            return;
        }
        ZoomableGraphicsView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (tool_ == CanvasTool::Select && draggingEndpoint_ && event->button() == Qt::LeftButton) {
            if (endpointMovedHandler_) {
                endpointMovedHandler_(dragRow_, dragStartEndpoint_, fromQt(snapPoint(mapToScene(event->pos()))), true);
            }
            draggingEndpoint_ = false;
            dragRow_ = -1;
            event->accept();
            return;
        }
        ZoomableGraphicsView::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Escape && drawing_) {
            cancelDrawing();
            event->accept();
            return;
        }
        ZoomableGraphicsView::keyPressEvent(event);
    }

private:
    QPointF snapPoint(const QPointF& point) const {
        if (!snapToGrid_) {
            return point;
        }
        return QPointF(std::round(point.x() / gridSpacing_) * gridSpacing_,
                       std::round(point.y() / gridSpacing_) * gridSpacing_);
    }

    void removePreviewLine() {
        if (!previewLine_) {
            return;
        }
        if (scene()) {
            scene()->removeItem(previewLine_);
        }
        delete previewLine_;
        previewLine_ = nullptr;
    }

    CanvasTool tool_ = CanvasTool::Select;
    bool drawing_ = false;
    bool draggingEndpoint_ = false;
    bool showGrid_ = true;
    bool snapToGrid_ = false;
    double gridSpacing_ = 25.0;
    QPointF drawStart_;
    int dragRow_ = -1;
    bool dragStartEndpoint_ = true;
    QGraphicsLineItem* previewLine_ = nullptr;
    std::function<void(spt::PointYZ, spt::PointYZ)> plateDrawnHandler_;
    std::function<void(int)> plateSelectedHandler_;
    std::function<std::vector<std::pair<int, spt::PlateSegment>>()> endpointProvider_;
    std::function<void(int, bool, spt::PointYZ, bool)> endpointMovedHandler_;
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

    QComboBox* sectionTypeComboForTest() const { return typeCombo_; }
    QTableWidget* parameterTableForTest() const { return parameterTable_; }
    QTableWidget* propertyTableForTest() const { return propertyTable_; }
    QTableWidget* stressTableForTest() const { return stressTable_; }
    QTableWidget* canvasTableForTest() const { return canvasTable_; }
    QGraphicsScene* meshSceneForTest() const { return meshScene_; }
    void clearCanvasForTest() { clearCanvasPlates(); }
    void addCanvasPlateForTest(const spt::PlateSegment& plate) { insertCanvasPlate(plate, false); }
    void buildCanvasSectionForTest() { buildCanvasSection(); }

private:
    QTabWidget* tabs_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QTableWidget* parameterTable_ = nullptr;
    QTableWidget* propertyTable_ = nullptr;
    QTableWidget* stressTable_ = nullptr;
    QTableWidget* canvasTable_ = nullptr;
    QDoubleSpinBox* meshRefinement_ = nullptr;
    QDoubleSpinBox* canvasThickness_ = nullptr;
    QLineEdit* canvasIdEdit_ = nullptr;
    QLabel* canvasStatus_ = nullptr;
    QGraphicsScene* generalScene_ = nullptr;
    QGraphicsScene* stressScene_ = nullptr;
    QGraphicsScene* meshScene_ = nullptr;
    QGraphicsScene* canvasScene_ = nullptr;
    ZoomableGraphicsView* generalView_ = nullptr;
    ZoomableGraphicsView* stressView_ = nullptr;
    ZoomableGraphicsView* meshView_ = nullptr;
    CanvasGraphicsView* canvasView_ = nullptr;
    spt::SectionModel model_;
    spt::CalculationResult result_;
    bool updatingCanvasTable_ = false;
    int selectedCanvasRow_ = -1;
    int nextPlateNumber_ = 1;

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
        parameterTable_->setObjectName("parameterTable");
        parameterTable_->setColumnCount(3);
        parameterTable_->setHorizontalHeaderLabels({"Name", "Value", "Unit"});
        parameterTable_->horizontalHeader()->setStretchLastSection(true);
        connect(parameterTable_, &QTableWidget::cellChanged, this, [this](int, int column) {
            if (column == 1) {
                recalculate();
            }
        });

        propertyTable_ = new QTableWidget;
        propertyTable_->setObjectName("propertyTable");
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
        stressTable_->setObjectName("stressTable");
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
        auto* splitter = new QSplitter(Qt::Horizontal);

        auto* left = new QWidget;
        auto* leftLayout = new QVBoxLayout(left);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        canvasTable_ = new QTableWidget;
        canvasTable_->setObjectName("canvasTable");
        canvasTable_->setColumnCount(6);
        canvasTable_->setHorizontalHeaderLabels({"y1", "z1", "y2", "z2", "t", "id"});
        canvasTable_->horizontalHeader()->setStretchLastSection(true);
        canvasTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        canvasTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        connect(canvasTable_, &QTableWidget::cellChanged, this, [this](int row, int) {
            if (!updatingCanvasTable_) {
                selectedCanvasRow_ = row;
                redrawCanvasScene();
            }
        });
        connect(canvasTable_, &QTableWidget::currentCellChanged, this, [this](int row, int, int, int) {
            if (!updatingCanvasTable_) {
                selectCanvasRow(row);
            }
        });

        auto* inputLayout = new QGridLayout;
        canvasThickness_ = new QDoubleSpinBox;
        canvasThickness_->setRange(0.001, 1.0e9);
        canvasThickness_->setDecimals(3);
        canvasThickness_->setValue(10.0);
        canvasThickness_->setSingleStep(1.0);
        canvasIdEdit_ = new QLineEdit;
        canvasIdEdit_->setPlaceholderText("auto");
        inputLayout->addWidget(new QLabel("New plate thickness"), 0, 0);
        inputLayout->addWidget(canvasThickness_, 0, 1);
        inputLayout->addWidget(new QLabel("New plate id"), 1, 0);
        inputLayout->addWidget(canvasIdEdit_, 1, 1);

        auto* addPlate = new QPushButton("Add Plate");
        auto* deletePlate = new QPushButton("Delete Selected Plate");
        auto* clearAll = new QPushButton("Clear All");
        auto* buildSection = new QPushButton("Build Canvas Section");
        connect(addPlate, &QPushButton::clicked, this, [this] { addDefaultCanvasPlate(); });
        connect(deletePlate, &QPushButton::clicked, this, [this] { deleteSelectedCanvasPlate(); });
        connect(clearAll, &QPushButton::clicked, this, [this] { clearCanvasPlates(); });
        connect(buildSection, &QPushButton::clicked, this, [this] { buildCanvasSection(); });

        auto* leftButtons = new QGridLayout;
        leftButtons->addWidget(addPlate, 0, 0);
        leftButtons->addWidget(deletePlate, 0, 1);
        leftButtons->addWidget(clearAll, 1, 0);
        leftButtons->addWidget(buildSection, 1, 1);

        canvasStatus_ = new QLabel("Canvas ready");
        leftLayout->addLayout(inputLayout);
        leftLayout->addWidget(canvasTable_, 1);
        leftLayout->addLayout(leftButtons);
        leftLayout->addWidget(canvasStatus_);

        auto* right = new QWidget;
        auto* rightLayout = new QVBoxLayout(right);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        canvasScene_ = new QGraphicsScene(this);
        canvasView_ = new CanvasGraphicsView(canvasScene_);
        canvasView_->setPlateDrawnHandler([this](spt::PointYZ start, spt::PointYZ end) {
            addDrawnCanvasPlate(start, end);
        });
        canvasView_->setPlateSelectedHandler([this](int row) {
            selectCanvasRow(row);
        });
        canvasView_->setEndpointProvider([this] {
            return canvasEndpointPlates();
        });
        canvasView_->setEndpointMovedHandler([this](int row, bool startEndpoint, spt::PointYZ point, bool commit) {
            moveCanvasEndpoint(row, startEndpoint, point, commit);
        });

        auto* selectMode = new QPushButton("Select/Edit");
        auto* drawMode = new QPushButton("Draw Plate");
        selectMode->setCheckable(true);
        drawMode->setCheckable(true);
        selectMode->setChecked(true);
        auto setTool = [this, selectMode, drawMode](CanvasTool tool) {
            selectMode->setChecked(tool == CanvasTool::Select);
            drawMode->setChecked(tool == CanvasTool::DrawPlate);
            canvasView_->setTool(tool);
        };
        connect(selectMode, &QPushButton::clicked, this, [setTool] { setTool(CanvasTool::Select); });
        connect(drawMode, &QPushButton::clicked, this, [setTool] { setTool(CanvasTool::DrawPlate); });

        auto* canvasDelete = new QPushButton("Delete Selected");
        auto* canvasZoomIn = new QPushButton("Zoom +");
        auto* canvasZoomOut = new QPushButton("Zoom -");
        auto* canvasFit = new QPushButton("Fit");
        auto* canvasReset = new QPushButton("Reset");
        auto* showGrid = new QCheckBox("Show Grid");
        auto* snapGrid = new QCheckBox("Snap to Grid");
        showGrid->setChecked(true);
        connect(canvasDelete, &QPushButton::clicked, this, [this] { deleteSelectedCanvasPlate(); });
        connect(canvasZoomIn, &QPushButton::clicked, canvasView_, [this] { canvasView_->zoomIn(); });
        connect(canvasZoomOut, &QPushButton::clicked, canvasView_, [this] { canvasView_->zoomOut(); });
        connect(canvasFit, &QPushButton::clicked, canvasView_, [this] { canvasView_->fitToScene(); });
        connect(canvasReset, &QPushButton::clicked, canvasView_, [this] { canvasView_->resetView(); });
        connect(showGrid, &QCheckBox::toggled, canvasView_, [this](bool checked) { canvasView_->setShowGrid(checked); });
        connect(snapGrid, &QCheckBox::toggled, canvasView_, [this](bool checked) { canvasView_->setSnapToGrid(checked); });

        auto* canvasTools = new QHBoxLayout;
        canvasTools->setContentsMargins(0, 0, 0, 0);
        canvasTools->addWidget(selectMode);
        canvasTools->addWidget(drawMode);
        canvasTools->addWidget(canvasDelete);
        canvasTools->addSpacing(12);
        canvasTools->addWidget(canvasZoomIn);
        canvasTools->addWidget(canvasZoomOut);
        canvasTools->addWidget(canvasFit);
        canvasTools->addWidget(canvasReset);
        canvasTools->addSpacing(12);
        canvasTools->addWidget(showGrid);
        canvasTools->addWidget(snapGrid);
        canvasTools->addStretch(1);

        rightLayout->addLayout(canvasTools);
        rightLayout->addWidget(canvasView_, 1);

        splitter->addWidget(left);
        splitter->addWidget(right);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
        layout->addWidget(splitter);
        tabs_->addTab(page, "Canvas");
        redrawCanvasScene();
    }

    QString formatCanvasNumber(double value) const {
        return QString::number(value, 'g', 12);
    }

    QString canvasCellText(int row, int column) const {
        const auto* item = canvasTable_->item(row, column);
        return item ? item->text().trimmed() : QString();
    }

    void setCanvasCell(int row, int column, const QString& value) {
        auto* item = canvasTable_->item(row, column);
        if (!item) {
            item = new QTableWidgetItem;
            canvasTable_->setItem(row, column, item);
        }
        item->setText(value);
    }

    bool readCanvasDouble(int row, int column, const QString& name, double& value, QStringList* errors) const {
        bool ok = false;
        value = canvasCellText(row, column).toDouble(&ok);
        if (!ok || !std::isfinite(value)) {
            if (errors) {
                errors->push_back(QString("Row %1: %2 must be numeric.").arg(row + 1).arg(name));
            }
            return false;
        }
        return true;
    }

    std::set<QString> existingCanvasIds(int skipRow = -1) const {
        std::set<QString> ids;
        for (int row = 0; row < canvasTable_->rowCount(); ++row) {
            if (row == skipRow) {
                continue;
            }
            const QString id = canvasCellText(row, 5);
            if (!id.isEmpty()) {
                ids.insert(id);
            }
        }
        return ids;
    }

    QString nextCanvasId() {
        return nextCanvasId({});
    }

    QString nextCanvasId(const std::set<QString>& reserved) {
        std::set<QString> ids = existingCanvasIds();
        ids.insert(reserved.begin(), reserved.end());
        QString id;
        do {
            id = QString("plate_%1").arg(nextPlateNumber_++);
        } while (ids.count(id) != 0);
        return id;
    }

    QString uniqueCanvasId(const QString& requested) {
        const std::set<QString> ids = existingCanvasIds();
        QString base = requested.trimmed();
        if (base.isEmpty()) {
            return nextCanvasId();
        }
        if (ids.count(base) == 0) {
            return base;
        }
        int suffix = 2;
        QString candidate;
        do {
            candidate = QString("%1_%2").arg(base).arg(suffix++);
        } while (ids.count(candidate) != 0);
        return candidate;
    }

    bool readCanvasPlateForDisplay(int row, spt::PlateSegment& plate) const {
        QStringList ignored;
        if (!readCanvasDouble(row, 0, "y1", plate.start.y, &ignored) ||
            !readCanvasDouble(row, 1, "z1", plate.start.z, &ignored) ||
            !readCanvasDouble(row, 2, "y2", plate.end.y, &ignored) ||
            !readCanvasDouble(row, 3, "z2", plate.end.z, &ignored) ||
            !readCanvasDouble(row, 4, "t", plate.thickness, &ignored)) {
            return false;
        }
        if (plate.thickness <= 0.0 || spt::distance(plate.start, plate.end) <= 1.0e-9) {
            return false;
        }
        const QString id = canvasCellText(row, 5);
        plate.id = id.isEmpty() ? QString("row_%1").arg(row + 1).toStdString() : id.toStdString();
        return true;
    }

    std::vector<std::pair<int, spt::PlateSegment>> canvasEndpointPlates() const {
        std::vector<std::pair<int, spt::PlateSegment>> plates;
        for (int row = 0; row < canvasTable_->rowCount(); ++row) {
            spt::PlateSegment plate;
            if (readCanvasPlateForDisplay(row, plate)) {
                plates.push_back({row, plate});
            }
        }
        return plates;
    }

    void moveCanvasEndpoint(int row, bool startEndpoint, spt::PointYZ point, bool commit) {
        if (row < 0 || row >= canvasTable_->rowCount()) {
            return;
        }
        selectedCanvasRow_ = row;
        if (!commit) {
            const QSignalBlocker blocker(canvasTable_);
            setCanvasCell(row, startEndpoint ? 0 : 2, formatCanvasNumber(point.y));
            setCanvasCell(row, startEndpoint ? 1 : 3, formatCanvasNumber(point.z));
            redrawCanvasScene();
            return;
        }
        setCanvasCell(row, startEndpoint ? 0 : 2, formatCanvasNumber(point.y));
        setCanvasCell(row, startEndpoint ? 1 : 3, formatCanvasNumber(point.z));
        redrawCanvasScene();
    }

    std::vector<spt::PlateSegment> collectCanvasPlatesForBuild(QStringList& errors) {
        std::map<QString, int> explicitIdCounts;
        for (int row = 0; row < canvasTable_->rowCount(); ++row) {
            const QString id = canvasCellText(row, 5);
            if (!id.isEmpty()) {
                ++explicitIdCounts[id];
            }
        }

        std::set<QString> usedIds;
        std::vector<spt::PlateSegment> plates;
        for (int row = 0; row < canvasTable_->rowCount(); ++row) {
            spt::PlateSegment plate;
            bool rowOk = true;
            rowOk &= readCanvasDouble(row, 0, "y1", plate.start.y, &errors);
            rowOk &= readCanvasDouble(row, 1, "z1", plate.start.z, &errors);
            rowOk &= readCanvasDouble(row, 2, "y2", plate.end.y, &errors);
            rowOk &= readCanvasDouble(row, 3, "z2", plate.end.z, &errors);
            rowOk &= readCanvasDouble(row, 4, "t", plate.thickness, &errors);

            QString id = canvasCellText(row, 5);
            if (id.isEmpty()) {
                id = nextCanvasId(usedIds);
                const QSignalBlocker blocker(canvasTable_);
                setCanvasCell(row, 5, id);
            } else if (explicitIdCounts[id] > 1 || usedIds.count(id) != 0) {
                errors.push_back(QString("Row %1: id '%2' must be unique.").arg(row + 1).arg(id));
                rowOk = false;
            }
            usedIds.insert(id);

            if (rowOk && plate.thickness <= 0.0) {
                errors.push_back(QString("Row %1: thickness must be positive.").arg(row + 1));
                rowOk = false;
            }
            if (rowOk && spt::distance(plate.start, plate.end) <= 1.0e-9) {
                errors.push_back(QString("Row %1: plate length must be greater than zero.").arg(row + 1));
                rowOk = false;
            }
            if (!rowOk) {
                continue;
            }
            plate.id = id.toStdString();
            plates.push_back(plate);
        }

        if (plates.empty() && errors.empty()) {
            errors.push_back("Add at least one valid plate before building the Canvas section.");
        }
        return plates;
    }

    void addDefaultCanvasPlate() {
        spt::PlateSegment plate;
        plate.start = {0.0, 0.0};
        plate.end = {100.0, 0.0};
        plate.thickness = canvasThickness_ ? canvasThickness_->value() : 10.0;
        plate.id = uniqueCanvasId(canvasIdEdit_ ? canvasIdEdit_->text() : QString()).toStdString();
        insertCanvasPlate(plate, true);
    }

    void addDrawnCanvasPlate(spt::PointYZ start, spt::PointYZ end) {
        if (spt::distance(start, end) <= 1.0e-9) {
            QMessageBox::warning(this, "Invalid Plate", "A Canvas plate must have positive length.");
            return;
        }
        spt::PlateSegment plate;
        plate.start = start;
        plate.end = end;
        plate.thickness = canvasThickness_ ? canvasThickness_->value() : 10.0;
        plate.id = uniqueCanvasId(canvasIdEdit_ ? canvasIdEdit_->text() : QString()).toStdString();
        insertCanvasPlate(plate, true);
    }

    void insertCanvasPlate(const spt::PlateSegment& plate, bool fit) {
        updatingCanvasTable_ = true;
        const QSignalBlocker blocker(canvasTable_);
        const int row = canvasTable_->rowCount();
        canvasTable_->insertRow(row);
        setCanvasCell(row, 0, formatCanvasNumber(plate.start.y));
        setCanvasCell(row, 1, formatCanvasNumber(plate.start.z));
        setCanvasCell(row, 2, formatCanvasNumber(plate.end.y));
        setCanvasCell(row, 3, formatCanvasNumber(plate.end.z));
        setCanvasCell(row, 4, formatCanvasNumber(plate.thickness));
        setCanvasCell(row, 5, QString::fromStdString(plate.id));
        updatingCanvasTable_ = false;
        selectCanvasRow(row);
        redrawCanvasScene();
        if (fit && canvasView_) {
            canvasView_->fitToScene();
        }
    }

    void deleteSelectedCanvasPlate() {
        const int row = canvasTable_->currentRow() >= 0 ? canvasTable_->currentRow() : selectedCanvasRow_;
        if (row < 0 || row >= canvasTable_->rowCount()) {
            return;
        }
        updatingCanvasTable_ = true;
        const QSignalBlocker blocker(canvasTable_);
        canvasTable_->removeRow(row);
        updatingCanvasTable_ = false;
        const int nextRow = std::min(row, canvasTable_->rowCount() - 1);
        selectCanvasRow(nextRow);
        redrawCanvasScene();
    }

    void clearCanvasPlates() {
        updatingCanvasTable_ = true;
        const QSignalBlocker blocker(canvasTable_);
        canvasTable_->setRowCount(0);
        updatingCanvasTable_ = false;
        selectedCanvasRow_ = -1;
        redrawCanvasScene();
        if (canvasStatus_) {
            canvasStatus_->setText("Canvas cleared");
        }
    }

    void selectCanvasRow(int row) {
        if (row < 0 || row >= canvasTable_->rowCount()) {
            selectedCanvasRow_ = -1;
            canvasTable_->clearSelection();
            redrawCanvasScene();
            return;
        }
        selectedCanvasRow_ = row;
        const QSignalBlocker blocker(canvasTable_);
        canvasTable_->selectRow(row);
        canvasTable_->setCurrentCell(row, 0);
        redrawCanvasScene();
    }

    void drawCanvasEndpoint(const QPointF& point, int row, const QColor& color) {
        auto* endpoint = canvasScene_->addEllipse(-3.5, -3.5, 7.0, 7.0, cosmeticPen(color, 1.0), QBrush(color));
        endpoint->setPos(point);
        endpoint->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        endpoint->setData(0, row);
        endpoint->setZValue(4.0);
    }

    void drawCanvasPlate(int row, const spt::PlateSegment& plate, bool selected) {
        QPolygonF polygon;
        for (const auto& point : spt::plateToPolygon(plate)) {
            polygon << toQt(point);
        }

        const QColor outlineColor = selected ? QColor(230, 126, 34) : QColor(36, 78, 112);
        const QColor fillColor = selected ? QColor(255, 193, 7, 115) : QColor(64, 145, 190, 80);
        const QColor lineColor = selected ? QColor(194, 92, 23) : QColor(20, 83, 122);
        if (!polygon.isEmpty()) {
            auto* item = canvasScene_->addPolygon(polygon, cosmeticPen(outlineColor, selected ? 2.0 : 1.2), QBrush(fillColor));
            item->setData(0, row);
            item->setZValue(selected ? 2.0 : 1.0);
        }

        const QPointF start = toQt(plate.start);
        const QPointF end = toQt(plate.end);
        auto* centerline = canvasScene_->addLine(QLineF(start, end), cosmeticPen(lineColor, selected ? 2.2 : 1.4));
        centerline->setData(0, row);
        centerline->setZValue(3.0);
        drawCanvasEndpoint(start, row, lineColor);
        drawCanvasEndpoint(end, row, lineColor);

        auto* label = canvasScene_->addText(QString("%1  t=%2")
                                                .arg(QString::fromStdString(plate.id))
                                                .arg(formatCanvasNumber(plate.thickness)));
        label->setDefaultTextColor(lineColor);
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setData(0, row);
        label->setPos(end + QPointF(6.0, -18.0));
        label->setZValue(5.0);
    }

    void redrawCanvasScene() {
        if (!canvasScene_ || !canvasView_) {
            return;
        }
        canvasView_->cancelDrawing();
        canvasScene_->clear();
        for (int row = 0; row < canvasTable_->rowCount(); ++row) {
            spt::PlateSegment plate;
            if (readCanvasPlateForDisplay(row, plate)) {
                drawCanvasPlate(row, plate, row == selectedCanvasRow_);
            }
        }
        updateSceneRect(canvasScene_);
        canvasView_->viewport()->update();
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
        QStringList errors;
        const std::vector<spt::PlateSegment> plates = collectCanvasPlatesForBuild(errors);
        if (!errors.empty()) {
            if (canvasStatus_) {
                canvasStatus_->setText("Canvas build failed; current results were not changed.");
            }
            QMessageBox::warning(this, "Invalid Canvas Section", errors.join("\n"));
            redrawCanvasScene();
            return;
        }

        auto built = spt::SectionBuilder::buildFromCanvasLines(plates);
        if (!built.ok()) {
            QStringList diagnostics;
            for (const auto& diagnostic : built.diagnostics) {
                if (diagnostic.severity == spt::ErrorSeverity::Error) {
                    diagnostics.push_back(QString::fromStdString(diagnostic.message));
                }
            }
            if (diagnostics.empty()) {
                diagnostics.push_back("Canvas section could not be built.");
            }
            if (canvasStatus_) {
                canvasStatus_->setText("Canvas build failed; current results were not changed.");
            }
            QMessageBox::warning(this, "Invalid Canvas Section", diagnostics.join("\n"));
            return;
        }

        model_ = built.model;
        result_ = spt::SectionCalculator::calculate(model_);
        updateProperties();
        updateStressTable();
        drawSection(generalScene_, true, generalView_, true);
        drawSection(stressScene_, true, stressView_, true);
        updateMesh(true);
        redrawCanvasScene();
        if (canvasView_) {
            canvasView_->fitToScene();
        }
        if (canvasStatus_) {
            canvasStatus_->setText(QString("Canvas section built from %1 plate(s).").arg(static_cast<int>(plates.size())));
        }
    }

    void updateProperties() {
        propertyTable_->blockSignals(true);
        propertyTable_->setRowCount(0);
        const auto add = [this](const QString& name, double value) {
            const int row = propertyTable_->rowCount();
            propertyTable_->insertRow(row);
            propertyTable_->setItem(row, 0, new QTableWidgetItem(name));
            const int decimals = (name.startsWith("J") || name == "Cw") ? 4 : 2;
            propertyTable_->setItem(row, 1, new QTableWidgetItem(QString::number(value, 'f', decimals)));
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
        add("Cw", p.warpingConstant);
        add("ys", p.shearCenterY);
        add("zs", p.shearCenterZ);
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

#ifndef SPT_GUI_NO_MAIN
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    CrossSectionWindow window;
    window.show();
    return app.exec();
}
#endif
