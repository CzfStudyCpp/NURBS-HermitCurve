


/**
 * @file nurbseditor.cpp
 * @brief NURBSEditor 类实现文件，提供 NURBS 曲线编辑器的交互、绘制与计算逻辑
 *
 * 实现内容包括：
 * - 控制点管理（添加、删除、拖动）
 * - 切线手柄控制与权重计算
 * - NURBS 曲线评估与绘制
 * - 事件处理（鼠标、键盘）
 */
#include "NurbsEditor.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QtMath>
#include <algorithm>

const double NURBSEditor::HANDLE_RADIUS = 60.0;

NURBSEditor::NURBSEditor(QWidget *parent)
    : QWidget(parent), selectedPoint(nullptr), activeSlopeHandle(nullptr)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setBackgroundRole(QPalette::Base);
}

NURBSEditor::~NURBSEditor()
{
    qDeleteAll(controlPoints);
    controlPoints.clear();
}



//----------------------------------------
// 绘图函数
//----------------------------------------

/**
 * @brief 主绘图入口
 */
void NURBSEditor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(250, 250, 250));

     if (showControlPoints)drawConnectionLines(painter);
    drawNURBSCurve(painter);
     //drawHermiteCurve(painter);  // 替代 drawNURBSCurve

     if (showControlPoints)drawSlopeHandles(painter);
    if (showControlPoints)drawControlPoints(painter);

    painter.setPen(Qt::black);
    painter.drawText(10, 20, "左键点击: 添加/移动点");
    painter.drawText(10, 40, "拖动绿色手柄: 调整权重");
    painter.drawText(10, 60, "上下箭头: 微调权重");
    painter.drawText(10, 80, "Delete: 删除选中点");
    painter.drawText(10, 100, "C: 清除所有点");
    painter.drawText(10, 120, QString("当前阶数(数字1-5): %1").arg(degree));
    painter.drawText(10, 140, QString("曲线采样(+ / -): %1").arg(sampleResolution));
    painter.drawText(10, 160, QString("显示控制点: %1 (按 V 切换)").arg(showControlPoints ? "是" : "否"));

    if (selectedPoint) {
        painter.drawText(10, 160, QString("权重: %1").arg(selectedPoint->weight, 0, 'f', 2));
    }

    QVector<double> knots = generateKnots();
    QString knotStr = "Knots: ";
    for (double k : knots)
        knotStr += QString::number(k, 'f', 2) + ", ";
    painter.drawText(10, height() - 20, knotStr);
}


//----------------------------------------
// 鼠标交互处理
//----------------------------------------

//鼠标单机，右键删除点，左键选中点
void NURBSEditor::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        deleteControlPoint(event->pos());
    }else if (event->button() == Qt::LeftButton) {
        bool pointHit = false;

        if (!trySelectSlopeHandle(event->pos())) {
            for (auto *cp : controlPoints) {
                if (QLineF(event->pos(), cp->position).length() < SNAP_DISTANCE) {
                    selectedPoint = cp;
                    isDraggingPoint = true;
                    pointHit = true;
                    break;
                }
            }
        }

        if (!pointHit && !activeSlopeHandle) {
            // 点击空白区域：取消选中点 & 不显示手柄
            selectedPoint = nullptr;
        }
    }

    update();
}

//鼠标拖动，分为拖动手柄或者控制顶点
void NURBSEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (activeSlopeHandle && selectedPoint) {
        updateSlopeHandles(event->pos());
    } else if (selectedPoint&&isDraggingPoint) {
        QPointF newPos = event->pos();
        newPos.setX(qBound(20.0, newPos.x(), width() - 20.0));
        newPos.setY(qBound(20.0, newPos.y(), height() - 20.0));
        selectedPoint->position = newPos;
        updateSlopeHandlePositions(*selectedPoint);
    }
    update();
}
//双击放置顶点
void NURBSEditor::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        createNewControlPoint(event->pos());
        update();
    }
}

//释放之后，需要重新计算手柄位置
void NURBSEditor::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    activeSlopeHandle = nullptr;

       if (isDraggingPoint && selectedPoint) {
           // 拖动结束后，显示并更新切线手柄
           //initSlopeHandles(*selectedPoint);
           updateSlopeHandlePositions(*selectedPoint);
       }

       isDraggingPoint = false;
       update();
}

//----------------------------------------
// 键盘交互
//----------------------------------------

void NURBSEditor::keyPressEvent(QKeyEvent *event)
{
    if (selectedPoint) {
        switch (event->key()) {
        case Qt::Key_Delete: {
            auto it = std::find(controlPoints.begin(), controlPoints.end(), selectedPoint);
            if (it != controlPoints.end()) {
                delete *it;
                controlPoints.erase(it);
                selectedPoint = nullptr;
            }
            break;
        }
        case Qt::Key_C:
            qDeleteAll(controlPoints);
            controlPoints.clear();
            selectedPoint = nullptr;
            break;
        case Qt::Key_Up:
            selectedPoint->weight += 0.01;
            updateSlopeHandlePositions(*selectedPoint);
            break;
        case Qt::Key_Down:
            selectedPoint->weight = qMax(selectedPoint->weight - 0.01, 0.1);
            updateSlopeHandlePositions(*selectedPoint);
            break;
        case Qt::Key_V:
               showControlPoints = !showControlPoints;
               break;
        }
    }
    //调整采样精度
    switch (event->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        sampleResolution = qMin(sampleResolution + 10, 1000);
        break;
    case Qt::Key_Minus:
        sampleResolution = qMax(sampleResolution - 10, 10);
        break;
    case Qt::Key_1: case Qt::Key_2: case Qt::Key_3:
    case Qt::Key_4: case Qt::Key_5:
        degree = event->key() - Qt::Key_0;
        break;
    }

    update();
}

//----------------------------------------
// 控制点与手柄管理
//----------------------------------------
void NURBSEditor::deleteControlPoint(const QPointF &p)
{
    for (auto it = controlPoints.begin(); it != controlPoints.end();) {
        if (QLineF(p, (*it)->position).length() < SNAP_DISTANCE) {
            if (*it == selectedPoint) selectedPoint = nullptr;
            delete *it;
            it = controlPoints.erase(it);
        } else {
            ++it;
        }
    }
}

bool NURBSEditor::trySelectSlopeHandle(const QPointF &p)
{
    if (!selectedPoint) return false;

    for (auto &handle : selectedPoint->slopeHandles) {
        if (QLineF(p, handle).length() < HANDLE_SIZE) {
            activeSlopeHandle = &handle;
            return true;
        }
    }
    return false;
}

void NURBSEditor::selectOrCreateControlPoint(const QPointF &p)
{
    for (auto *cp : controlPoints) {
        if (QLineF(p, cp->position).length() < SNAP_DISTANCE) {
            selectedPoint = cp;
            if (cp->slopeHandles.isEmpty()) {
                initSlopeHandles(*cp);
            }
            return;
        }
    }
    createNewControlPoint(p);
}

void NURBSEditor::createNewControlPoint(const QPointF &p)
{
    ControlPoint* cp = new ControlPoint(p);
    controlPoints.append(cp);
    selectedPoint = cp;
    initSlopeHandles(*cp);
}

void NURBSEditor::updateSlopeHandles(const QPointF &p)
{
    QPointF center = selectedPoint->position;
        double dx = p.x() - center.x();
        double dy = p.y() - center.y();
        double distance = qSqrt(dx * dx + dy * dy);

        // 更新角度和权重
        selectedPoint->slopeAngle = qAtan2(dy, dx);
        // 修改为不限制权重版本
        selectedPoint->weight = qMax(0.1, distance / HANDLE_RADIUS);

        updateSlopeHandlePositions(*selectedPoint);

}

void NURBSEditor::initSlopeHandles(ControlPoint &cp)
{
    cp.slopeHandles.clear();
    cp.slopeHandles.append(QPointF(cp.position.x() + HANDLE_RADIUS, cp.position.y()));
    cp.slopeHandles.append(QPointF(cp.position.x() - HANDLE_RADIUS, cp.position.y()));
    cp.weight = 1.0;
}

void NURBSEditor::updateSlopeHandlePositions(ControlPoint &cp)
{
    double angle = cp.slopeAngle;
   // double length = cp.weight * HANDLE_RADIUS;
    double visualLength = qMin(cp.weight * HANDLE_RADIUS, 600.0); // 可视手柄不超过300像素

    cp.slopeHandles[0] = QPointF(cp.position.x() + visualLength * qCos(angle),
                                 cp.position.y() + visualLength * qSin(angle));
    cp.slopeHandles[1] = QPointF(cp.position.x() - visualLength * qCos(angle),
                                 cp.position.y() - visualLength * qSin(angle));
}


double NURBSEditor::getCurrentAngle(const ControlPoint &cp)
{
    if (cp.slopeHandles.isEmpty()) return 0;
    return qAtan2(cp.slopeHandles[0].y() - cp.position.y(),
                  cp.slopeHandles[0].x() - cp.position.x());
}


//----------------------------------------
// 曲线绘制与评估
//----------------------------------------

QPointF* NURBSEditor::getOppositeHandle(ControlPoint &cp)
{
    if (&cp.slopeHandles[0] == activeSlopeHandle) {
        return &cp.slopeHandles[1];
    }
    return &cp.slopeHandles[0];
}

void NURBSEditor::drawConnectionLines(QPainter &painter)
{
    painter.setPen(QPen(QColor(200, 200, 200, 150), 2));
    for (int i = 1; i < controlPoints.size(); ++i) {
        painter.drawLine(controlPoints[i - 1]->position, controlPoints[i]->position);
    }
}

void NURBSEditor::drawControlPoints(QPainter &painter)
{
    int idx = 0;
    for (const auto *cp : controlPoints) {
        bool isSelected = (cp == selectedPoint);

        painter.setBrush(QColor(0, 0, 0, 30));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(cp->position, POINT_SIZE / 2 + 3, POINT_SIZE / 2 + 3);

        QRadialGradient gradient(cp->position, POINT_SIZE / 2);
        gradient.setColorAt(0, isSelected ? QColor(255, 90, 90) : QColor(80, 140, 220));
        gradient.setColorAt(1, isSelected ? QColor(220, 70, 70) : QColor(60, 120, 200));

        painter.setBrush(gradient);
        painter.setPen(QPen(QColor(255, 255, 255, 150), 1.5));
        painter.drawEllipse(cp->position, POINT_SIZE / 2, POINT_SIZE / 2);

        painter.setPen(Qt::darkGray);
        painter.drawText(cp->position + QPointF(-10, 20), QString::number(idx++));

        if (isSelected) {
            painter.setPen(Qt::black);
            painter.drawText(cp->position + QPointF(15, -5),
                             QString::number(cp->weight, 'f', 2));
        }
    }
}

void NURBSEditor::drawSlopeHandles(QPainter &painter)
{
    if (!selectedPoint) return;
    painter.setPen(QPen(QColor(150, 200, 150, 150), 1.5));

    for (const auto &handle : selectedPoint->slopeHandles) {
        painter.drawLine(selectedPoint->position, handle);
        painter.setBrush(QColor(150, 200, 150, 150));
        painter.drawEllipse(handle, HANDLE_SIZE / 2, HANDLE_SIZE / 2);
    }
}

void NURBSEditor::drawNURBSCurve(QPainter &painter)
{
    if (controlPoints.size() < 2) return;

    QPainterPath path;
    path.moveTo(evaluateNURBS(0));

    for (int t = 1; t <= sampleResolution; ++t) {
        path.lineTo(evaluateNURBS(static_cast<double>(t) / sampleResolution));
    }

    painter.setPen(QPen(QColor(220, 80, 80), 3.5));
    painter.drawPath(path);
}


//----------------------------------------
// NURBS 数学计算
//----------------------------------------
QPointF NURBSEditor::evaluateNURBS(double t) const
{
    if (controlPoints.size() < 2) return QPointF();
    if (t <= 0) return controlPoints.first()->position;
    if (t >= 1) return controlPoints.last()->position;

    t = qBound(0.0, t, 1.0);
    QVector<double> knots = generateKnots();
    int n = controlPoints.size() - 1;

    double x = 0.0, y = 0.0, denominator = 0.0;
    for (int i = 0; i <= n; ++i) {
        double basis = basisFunction(degree, i, knots, t) * controlPoints[i]->weight;
        x += controlPoints[i]->position.x() * basis;
        y += controlPoints[i]->position.y() * basis;
        denominator += basis;
    }

    return denominator != 0.0 ? QPointF(x / denominator, y / denominator) : QPointF();
}

double NURBSEditor::basisFunction(int p, int i, const QVector<double> &knots, double t) const
{
    if (p == 0) {
        return (knots[i] <= t && t < knots[i + 1]) ? 1.0 : 0.0;
    }

    double term1 = 0.0, term2 = 0.0;
    double denom1 = knots[i + p] - knots[i];
    if (!qFuzzyIsNull(denom1))
        term1 = (t - knots[i]) / denom1 * basisFunction(p - 1, i, knots, t);

    double denom2 = knots[i + p + 1] - knots[i + 1];
    if (!qFuzzyIsNull(denom2))
        term2 = (knots[i + p + 1] - t) / denom2 * basisFunction(p - 1, i + 1, knots, t);

    return term1 + term2;
}

QVector<double> NURBSEditor::generateKnots() const
{
    int n = controlPoints.size() - 1;
    int m = n + degree + 1;
    QVector<double> knots(m + 1);

    for (int i = 0; i <= degree; ++i) knots[i] = 0.0;
    for (int i = m - degree; i <= m; ++i) knots[i] = 1.0;
    for (int i = degree + 1; i < m - degree; ++i)
        knots[i] = static_cast<double>(i - degree) / (n - degree + 1);

    return knots;
}


