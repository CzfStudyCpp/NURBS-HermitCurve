/**
 * @class HermiteEditor
 * @brief 二维交互式 Hermite 曲线编辑器（基于 Qt）
 *
 * HermiteEditor 提供基于 QWidget 的 Hermite 三次插值曲线绘制与控制点交互功能。
 * 用户可通过鼠标点击创建插值点、拖动控制切线方向，按键调整精度或清空画布。
 *
 * 功能特点：
 * - 鼠标左键点击空白区域添加点，右键点击删除点
 * - 拖动插值点位置或切线手柄动态影响曲线形状
 * - Hermite 三次曲线基于点与切线构建
 * - 可调采样精度，可隐藏控制点
 *
 * 适用于图形教学、几何建模、曲线可视化等交互式应用场景。
 */


// hermiteeditor.h
#ifndef HERMITEEDITOR_H
#define HERMITEEDITOR_H

#include <QWidget>
#include <QVector>
#include <QPointF>

class HermiteEditor : public QWidget
{
    Q_OBJECT

public:
    explicit HermiteEditor(QWidget *parent = nullptr);
    ~HermiteEditor();

protected:
     // QWidget 事件处理函数
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event)override;

private:

    /**
      * @struct InterpPoint
      * @brief 表示插值点及其切线数据
      */
    struct InterpPoint {
        QPointF position;  ///< 插值点位置
        QPointF tangent;    ///< 切线向量（右切线）
        bool hasTangent = false;   ///< 是否启用用户自定义切线

        InterpPoint(const QPointF &pos) : position(pos), tangent(50, 0) {}
    };

    QVector<InterpPoint*> points;               ///< 插值点列表
    InterpPoint* selectedPoint = nullptr;       ///< 当前选中的插值点
    QPointF* activeTangent = nullptr;           ///< 当前活动的切线句柄

    static const int POINT_RADIUS = 8;           ///< 插值点显示半径
    static const int HANDLE_RADIUS = 6;          ///< 切线手柄显示半径
    static const int SNAP_DISTANCE = 20;         ///< 鼠标命中判定半径
    int sampleResolution = 100;                  ///< 曲线采样精度

    bool draggingPoint=false;
    bool showPoints = true;
    bool isEditingNegativeHandle = false;



    void drawPoints(QPainter &painter);
    void drawTangents(QPainter &painter);              ///< 绘制当前点切线
    void drawHermiteCurve(QPainter &painter);          ///< 绘制 Hermite 曲线

    void updateTangent(const QPointF &pos);             ///< 更新切线向量
    void deletePointAt(const QPointF &pos);

};

#endif // HERMITEEDITOR_H
