#ifndef NURBSEDITOR_H
#define NURBSEDITOR_H

#include <QWidget>
#include <QVector>
#include <QPointF>
/*
 * @class NURBSEditor
 * @brief 一个可交互的 NURBS 曲线编辑器控件
 *  NURBSEditor 是一个基于 Qt 的 QWidget 子类，允许用户通过鼠标和键盘操作交互式地创建、编辑和可视化 NURBS 曲线。
 *  支持控制点权重调整、阶数设置、曲线采样精度调整以及切线控制手柄的显示与编辑。
 *功能特性包括：
 * 鼠标左键点击选择控制点，拖动移动点或调整手柄，手柄用于调整权重
 * 双击增加控制点
 * 鼠标右键点击删除控制点
 * 支持调整控制点的权重、曲线阶数和采样精度
 *  支持控制点与切线手柄可视化
 *
 */
class NURBSEditor : public QWidget
{
    Q_OBJECT

public:
    explicit NURBSEditor(QWidget *parent = nullptr);
    ~NURBSEditor();

protected:
    // QWidget 重载函数：处理绘图与交互事件
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
     /* 控制点数据结构，包含位置、权重、切线角度与切线手柄 */
    struct ControlPoint {
        QPointF position;
        QVector<QPointF> slopeHandles;
        double slopeAngle;
        double weight;

        ControlPoint(const QPointF &p) : position(p), slopeAngle(0.0), weight(1.0) {}
    };

    // Constants
    static const int POINT_SIZE = 14;       ///< 控制点显示大小
    static const int HANDLE_SIZE = 10;      ///< 手柄显示大小
    static const int SNAP_DISTANCE = 30;    ///< 鼠标点击判定距离
    static const double HANDLE_RADIUS;      ///< 手柄默认长度（非可视）

    // State
    bool isDraggingPoint = false;                 ///< 是否处于点拖动状态
    QVector<ControlPoint*> controlPoints;        ///< 控制点列表
    ControlPoint* selectedPoint = nullptr;      ///< 当前选中的控制点
    QPointF* activeSlopeHandle = nullptr;       ///< 当前选中的切线手柄
    bool showControlPoints = true;                ///< 是否显示控制点及其多边形


    // Parameters
    int degree = 3;                        ///< NURBS 曲线阶数（默认 3 次）
    int sampleResolution = 100;            ///< 曲线采样密度（影响绘制精度）


    // 控制点操作及手柄操作
    void deleteControlPoint(const QPointF &p);
    bool trySelectSlopeHandle(const QPointF &p);
    void selectOrCreateControlPoint(const QPointF &p);
    void createNewControlPoint(const QPointF &p);
    void updateSlopeHandles(const QPointF &p);
    void initSlopeHandles(ControlPoint &cp);
    void updateSlopeHandlePositions(ControlPoint &cp);
    double getCurrentAngle(const ControlPoint &cp);
    QPointF* getOppositeHandle(ControlPoint &cp);

    // 曲线绘制辅助函数
    void drawConnectionLines(QPainter &painter);
    void drawControlPoints(QPainter &painter);
    void drawSlopeHandles(QPainter &painter);
    void drawNURBSCurve(QPainter &painter);
    void drawHermiteCurve(QPainter &painter);

    // NURBS 计算相关核心
    QPointF evaluateNURBS(double t) const;
    double basisFunction(int p, int i, const QVector<double> &knots, double t) const;
    QVector<double> generateKnots() const;
};

#endif // NURBSEDITOR_H
