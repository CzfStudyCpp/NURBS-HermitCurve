//#include <QApplication>
//#include "nurbseditor.h"

//int main(int argc, char *argv[])
//{
//    QApplication a(argc, argv);

//    // 设置应用程序样式
//    QApplication::setStyle("fusion");

//    // 创建主窗口
//    NURBSEditor editor;
//    editor.setWindowTitle("NURBS曲线编辑器");
//    editor.resize(1280, 800);
//    editor.show();

//    return a.exec();
//}
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>

#include "NurbsEditor.h"
#include "HermiteEditor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle("fusion");

    QStringList options = {"NURBS 曲线", "Hermite 样条"};
    bool ok;
    QString choice = QInputDialog::getItem(
        nullptr,
        "选择曲线类型",
        "请选择要使用的曲线编辑器：",
        options,
        0,
        false,
        &ok
    );

    if (!ok) return 0;

    QWidget *editor = nullptr;

    if (choice == "NURBS 曲线") {
        editor = new NURBSEditor();
        editor->setWindowTitle("NURBS 曲线编辑器");
    } else {
        editor = new HermiteEditor();
        editor->setWindowTitle("Hermite 样条曲线编辑器");
    }

    editor->resize(1280, 800);
    editor->show();

    return app.exec();
}
