#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include "areaselector.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);  // 创建应用程序对象
    // AreaSelector selector;
    // selector.init();
    // return app.exec();
    try {
        MainWindow w;              // 创建主窗口对象
        w.show();                  // 显示主窗口
        return app.exec();         // 进入事件循环
    } catch (const std::exception& e) {
        // 如果发生异常，显示错误信息
        QMessageBox::critical(nullptr, "错误", QString::fromStdString(e.what()) + "\n请重启应用程序。");
        return 1;                  // 返回错误码
    }
}
