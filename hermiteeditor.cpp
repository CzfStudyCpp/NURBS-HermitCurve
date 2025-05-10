
/**
 * @file hermiteeditor.cpp
 * @brief HermiteEditor 类实现文件
 *
 * 提供 Hermite 曲线的绘制与交互功能，包括插值点添加、移动、删除，切线拖动等。
 * 曲线基于 Hermite 三次插值实现，支持自动或手动切线，支持实时调节采样精度与点可视性。
 */


// hermiteeditor.cpp
#include "HermiteEditor.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QtMath>
#include <QString>

HermiteEditor::HermiteEditor(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

HermiteEditor::~HermiteEditor()
{
    qDeleteAll(points);
    points.clear();
}

//----------------------------------------
// 绘图主函数
//----------------------------------------

/**
 * @brief 重绘窗口
 *        包括曲线、控制点、切线与说明文字
 */
void HermiteEditor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);

    drawHermiteCurve(painter);
    drawTangents(painter);
    drawPoints(painter);

    painter.setPen(Qt::black);
    QStringList helpText = {
        "左键添加点，右键删除点",
        "拖动蓝点调整位置",
        "拖动绿色手柄调整切线",
        "C 清空, +/- 改变曲线精度",
        "V 显示/隐藏插值点",
        QString("当前精度: %1").arg(sampleResolution)
    };

    int y = 20;
    for (const QString &line : helpText) {
        painter.drawText(10, y, line);
        y += 16;
    }
}


//----------------------------------------
// 鼠标事件处理
//----------------------------------------

/**
 * @brief 处理鼠标按下事件
 *        支持：点选中、拖动、选中切线手柄、删除点
 */
void HermiteEditor::mousePressEvent(QMouseEvent *event) {
    selectedPoint = nullptr;
    if (event->button() == Qt::RightButton) {
        deletePointAt(event->pos());
        update();
        return;
    }

    for (auto *pt : points) {
        QPointF h0 = pt->position + pt->tangent;
        QPointF h1 = pt->position - pt->tangent;
        if (QLineF(event->pos(), h0).length() < SNAP_DISTANCE) {
            selectedPoint = pt;
            activeTangent = &pt->tangent;
            return;
        } else if (QLineF(event->pos(), h1).length() < SNAP_DISTANCE) {
            selectedPoint = pt;
            activeTangent = &pt->tangent;
            pt->tangent = pt->position - event->pos();
            return;
        }
    }

    for (auto *pt : points) {
        if (QLineF(event->pos(), pt->position).length() < SNAP_DISTANCE) {
            selectedPoint = pt;
            draggingPoint = true;
            return;
        }
    }

    // 鼠标单击空白区域，仅取消选中，不创建新点
    update();
}

void HermiteEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (selectedPoint && activeTangent) {
        selectedPoint->tangent = event->pos() - selectedPoint->position;
        selectedPoint->hasTangent = true;
        update();
    } else if (selectedPoint && draggingPoint) {
        selectedPoint->position = event->pos();
        update();
    }
}

void HermiteEditor::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        auto *newPt = new InterpPoint(event->pos());
        if (points.size() >= 1) {
            QPointF dir = newPt->position - points.last()->position;
            if (!dir.isNull())
                newPt->tangent = 0.5 * dir;
        }
        points.append(newPt);
        selectedPoint = newPt;
        update();
    }
}

void HermiteEditor::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    activeTangent = nullptr;
    draggingPoint = false;
}
//----------------------------------------
// 键盘事件
//----------------------------------------

/**
 * @brief 处理快捷键操作
 * C 清空, +/- 调节精度, V 显示/隐藏插值点
 */
void HermiteEditor::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_C:
        qDeleteAll(points);
        points.clear();
        selectedPoint = nullptr;
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        sampleResolution = qMin(sampleResolution + 10, 1000);
        break;
    case Qt::Key_Minus:
        sampleResolution = qMax(sampleResolution - 10, 10);
        break;
    case Qt::Key_V:
        showPoints = !showPoints;
        break;

    }
    update();
}


//----------------------------------------
// 曲线、点、切线绘制
//----------------------------------------

/**
 * @brief 绘制所有插值点（蓝点）及编号
 */
void HermiteEditor::drawPoints(QPainter &painter)
{
    if (!showPoints) return;

    painter.setPen(Qt::darkBlue);
    painter.setBrush(Qt::darkBlue);

    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    for (int i = 0; i < points.size(); ++i) {
        auto *pt = points[i];
        painter.drawEllipse(pt->position, 5, 5);

        // 显示编号
        painter.setPen(Qt::black);
        painter.drawText(pt->position + QPointF(6, -6), QString::number(i));
        painter.setPen(Qt::darkBlue);
    }
}


/**
 * @brief 绘制当前选中点的切线及手柄
 */
void HermiteEditor::drawTangents(QPainter &painter)
{
    if (!selectedPoint) return;

    painter.setPen(QPen(Qt::darkGreen, 1.5));
    painter.setBrush(Qt::green);

    QPointF p = selectedPoint->position;
    QPointF h0 = p + selectedPoint->tangent;
    QPointF h1 = p - selectedPoint->tangent;

    if (!selectedPoint->tangent.isNull()) {
        painter.drawLine(p, h0);
        painter.drawLine(p, h1);
    }

    painter.drawEllipse(h0, HANDLE_RADIUS, HANDLE_RADIUS);
    painter.drawEllipse(h1, HANDLE_RADIUS, HANDLE_RADIUS);
}


/**
 * @brief Hermite 三次曲线插值绘制函数
 *        支持手动或自动切线估算
 */
void HermiteEditor::drawHermiteCurve(QPainter &painter)
{
    if (points.size() < 2) return;
    QPainterPath path;

    // Estimate tangents automatically if not user-defined
    QVector<QPointF> autoTangents(points.size());
    for (int i = 0; i < points.size(); ++i) {
        if (i == 0)
            autoTangents[i] = 0.5 * (points[i + 1]->position - points[i]->position);
        else if (i == points.size() - 1)
            autoTangents[i] = 0.5 * (points[i]->position - points[i - 1]->position);
        else
            autoTangents[i] = 0.5 * (points[i + 1]->position - points[i - 1]->position);
    }

    for (int i = 0; i < points.size() - 1; ++i) {
        auto *p0 = points[i];
        auto *p1 = points[i + 1];

        QPointF t0 = p0->hasTangent ? p0->tangent : autoTangents[i];
        QPointF t1 = p1->hasTangent ? p1->tangent : autoTangents[i + 1];

        for (int j = 0; j <= sampleResolution; ++j) {
            double t = static_cast<double>(j) / sampleResolution;

            double h00 = 2 * t * t * t - 3 * t * t + 1;
            double h10 = t * t * t - 2 * t * t + t;
            double h01 = -2 * t * t * t + 3 * t * t;
            double h11 = t * t * t - t * t;

            QPointF pt = h00 * p0->position + h10 * t0 + h01 * p1->position + h11 * t1;

            if (j == 0)
                path.moveTo(pt);
            else
                path.lineTo(pt);
        }
    }

    painter.setPen(QPen(Qt::red, 2));
    painter.drawPath(path);


}

/**
 * @brief 删除指定位置附近的插值点
 * @param pos 鼠标点击位置
 */
void HermiteEditor::deletePointAt(const QPointF &pos)
{
    for (int i = 0; i < points.size(); ++i) {
        if (QLineF(pos, points[i]->position).length() < SNAP_DISTANCE) {
            delete points[i];
            points.remove(i);
            selectedPoint = nullptr;
            return;
        }
    }
}
