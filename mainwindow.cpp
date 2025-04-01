#include "mainwindow.h"

#include <QAudioDeviceInfo>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <condition_variable>
#include <mutex>

#include "areaselector.h"
#include "getaudiodevices.h"
#include "screenrecorder.h"
#include "ui_mainwindow.h"

#include <mutex>

std::unique_ptr<ScreenRecorder> screenRecorder;
RecordingRegionSettings rrs;
VideoSettings vs;
std::string outFilePath;
std::string deviceName;

// 用于同步录制结束和按钮启用的互斥锁和条件变量
mutex m;
condition_variable cv;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    screen = QGuiApplication::primaryScreen(); // 获取主屏幕

    ui->setupUi(this); // 初始化用户界面

    // 设置默认输出路径
    ui->lineEditPath->setText(QString::fromStdString(forge_outpath(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation).toStdString())));

    areaSelector = new AreaSelector(); // 创建区域选择器

    setWindowTitle("EasyRecorder"); // 设置窗口标题

    errorDialog.setFixedSize(500, 200); // 设置错误对话框大小

    // 设置默认音频选项
    ui->radioButtonYes->setChecked(true);

    // 设置选项卡默认显示第一个
    ui->tabWidget->setCurrentIndex(0);

    // 设置按钮属性
    ui->pushButtonFullscreen->setCheckable(true);
    ui->pushButtonFullscreen->setChecked(true);
    ui->pushButtonSelectArea->setCheckable(true);

    ui->pushButtonFullscreen->setIcon(QIcon(":/icons/fullscreen.png"));
    ui->pushButtonSelectArea->setIcon(QIcon(":/icons/area.png"));

    // 设置60fps单选按钮提示
    ui->radioButton60->setMouseTracking(true);
    ui->radioButton60->setToolTip("需要高性能支持");

    // 设置滑块属性
    ui->horizontalSlider->setTracking(true);
    ui->horizontalSlider->setMinimum(1);
    ui->horizontalSlider->setMaximum(5);

// 添加音频设备到下拉框
#if defined _WIN32
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos)
        ui->comboBox->addItem(tr(deviceInfo.deviceName().toStdString().c_str()));
#endif
#if defined __linux__
    for (auto &device : getAudioDevices()) {
        ui->comboBox->addItem(tr(device.c_str()));
    }
#endif

    // 设置屏幕录制对象的默认值
    alignValues();

    // 连接信号和槽
    connect(this, SIGNAL(signal_close()), areaSelector, SLOT(close()));
    connect(this, SIGNAL(signal_show(bool)), areaSelector, SLOT(setVisible(bool)));
    connect(this, SIGNAL(signal_recording(bool)), areaSelector, SLOT(slot_recordMode(bool)));
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        createActions(); // 创建系统托盘动作
        createTrayIcon(); // 创建系统托盘图标
    }

    // 设置窗口元素的默认属性
    setGeneralDefaultProperties();
}

MainWindow::~MainWindow() {
    delete ui; // 释放UI资源
}


void MainWindow::alignValues() {
    /// rrs 值设置
    if (ui->pushButtonFullscreen->isChecked()) {
        rrs.width = screen->size().width();   // 设置宽度为屏幕宽度
        rrs.height = screen->size().height(); // 设置高度为屏幕高度
        rrs.offset_x = 0;                     // X偏移量为0
        rrs.offset_y = 0;                     // Y偏移量为0

        rrs.screen_number = 0;                // 默认屏幕编号为0
    } else {
        rrs.height = areaSelector->getHeight(); // 获取选择区域高度
        rrs.width = areaSelector->getWidth();   // 获取选择区域宽度
        rrs.offset_x = areaSelector->getX();    // 获取选择区域X偏移
        rrs.offset_y = areaSelector->getY();    // 获取选择区域Y偏移
    }

    /// vs 值设置
    if (ui->radioButton30->isChecked())
        vs.fps = 30;    // 设置帧率为30
    else if (ui->radioButton24->isChecked())
        vs.fps = 24;    // 设置帧率为24
    else if (ui->radioButton60->isChecked())
        vs.fps = 60;    // 设置帧率为60

    setQualityANDCompression(ui->horizontalSlider->value()); // 设置质量和压缩

    vs.audioOn = ui->radioButtonYes->isChecked(); // 是否启用音频
    outFilePath = ui->lineEditPath->text().toStdString(); // 输出文件路径
    deviceName = ui->comboBox->currentText().toStdString(); // 音频设备名称
    minimizeInSysTray = ui->checkBoxMinimize->isChecked(); // 是否最小化到系统托盘
}

void MainWindow::setQualityANDCompression(int position) {
    // 根据滑块位置设置视频质量和压缩级别
    if (position == 1) {
        vs.quality = 0.3;  // 低质量
        vs.compression = 9; // 最高压缩
    } else if (position == 2) {
        vs.quality = 0.5;  // 较低质量
        vs.compression = 7; // 高压缩
    } else if (position == 3) {
        vs.quality = 0.7;  // 中等质量
        vs.compression = 5; // 中等压缩
    } else if (position == 4) {
        vs.quality = 0.85; // 较高质量
        vs.compression = 3; // 低压缩
    } else if (position == 5) {
        vs.quality = 1.0;  // 最高质量
        vs.compression = 1; // 最低压缩
    }
}

void MainWindow::defaultButtonProperties() {
    // 设置按钮默认属性
    ui->pushButtonStart->setEnabled(true);    // 启用开始按钮
    ui->pushButtonPause->setDisabled(true);   // 禁用暂停按钮
    ui->pushButtonResume->setDisabled(true);  // 禁用恢复按钮
    ui->pushButtonStop->setDisabled(true);    // 禁用停止按钮
    startAction->setDisabled(false);
    pauseAction->setDisabled(true);
    resumeAction->setDisabled(true);
    stopAction->setDisabled(true);
    quitAction->setEnabled(true);
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png")); // 设置系统托盘图标
}

void MainWindow::setGeneralDefaultProperties() {
    // 启用或禁用选项卡
    enable_or_disable_tabs(true);

    // 设置按钮属性
    defaultButtonProperties();

    // 设置默认路径文本
    // ui->lineEditPath->setText(QDir::homePath());

    // 设置默认单选按钮
    ui->radioButton30->setChecked(true);

    // 设置滑块默认值
    ui->horizontalSlider->setValue(5);
}

void MainWindow::showOrHideWindow(bool recording) {
    // 根据录制状态显示或隐藏窗口
    if (!recording) {
        if (minimizeInSysTray)
            show(); // 显示窗口
        trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png")); // 正常状态图标
    } else {
        if (minimizeInSysTray)
            hide(); // 隐藏窗口
        trayIcon->setIcon(QIcon(":/icons/trayicon_recording.png")); // 录制状态图标
    }
}



void MainWindow::createActions() {
    // 创建显示/隐藏动作
    showhideAction = new QAction(tr("显示/隐藏"), this);
    connect(showhideAction, &QAction::triggered, this, [&]() {
        if (this->isHidden()) {
            this->show();
        } else {
            this->hide();
        }
    });

    // 创建开始动作
    startAction = new QAction(tr("开始"), this);
    connect(startAction, &QAction::triggered, this, [&]() { on_pushButtonStart_clicked(); });

    // 创建恢复动作
    resumeAction = new QAction(tr("恢复"), this);
    connect(resumeAction, &QAction::triggered, this, [&]() { on_pushButtonResume_clicked(); });
    resumeAction->setEnabled(false);

    // 创建暂停动作
    pauseAction = new QAction(tr("暂停"), this);
    connect(pauseAction, &QAction::triggered, this, [&]() { on_pushButtonPause_clicked(); });
    pauseAction->setEnabled(false);

    // 创建停止动作
    stopAction = new QAction(tr("停止"), this);
    connect(stopAction, &QAction::triggered, this, [&]() { on_pushButtonStop_clicked(); });
    stopAction->setEnabled(false);

    // 创建退出动作
    quitAction = new QAction(tr("退出"), this);
    connect(quitAction, &QAction::triggered, qApp, [&]() {
        check_stopped_and_exec([&]() {
            emit signal_close();
            QCoreApplication::quit();
        }, nullptr);
    });
}

void MainWindow::createTrayIcon() {
    // 创建系统托盘菜单
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(showhideAction);
    trayIconMenu->addAction(startAction);
    trayIconMenu->addAction(resumeAction);
    trayIconMenu->addAction(pauseAction);
    trayIconMenu->addAction(stopAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));
    trayIcon->setVisible(true);
}

string MainWindow::forge_outpath(string outFilePath) {
    // 生成输出文件路径
    std::string ending = ".mp4";
    if (!std::equal(ending.rbegin(), ending.rend(), outFilePath.rbegin())) {
        ending = "/";
        if (std::equal(ending.rbegin(), ending.rend(), outFilePath.rbegin()))
            outFilePath += "out.mp4";
        else {
            outFilePath += "/out.mp4";
        }
    }
    return outFilePath;
}

void MainWindow::check_stopped_and_exec(function<void(void)> f, QCloseEvent *event) {
    // 检查录制是否停止并执行操作
    if (screenRecorder && screenRecorder.get()->getStatus() != RecordingStatus::stopped) {
        QMessageBox::information(this, tr("操作被禁止"),
                                 tr("应用程序仍在录制中。\n请先停止视频录制再关闭应用程序"));
        if (event != nullptr) event->ignore();
    } else {
        f();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // 处理窗口关闭事件
#ifdef Q_OS_MACOS
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    check_stopped_and_exec(
        [this, event]() {
            if (trayIcon->isVisible()) {
                QMessageBox::information(this, tr("系统托盘"),
                                         tr("程序将继续在系统托盘中运行。\n要终止程序，请在系统托盘条目的上下文菜单中选择退出。"));
                hide();
                event->ignore();
            } else {
                emit signal_close();
                QCoreApplication::quit();
            }
        },
        event);
}

void MainWindow::enable_or_disable_tabs(bool val) {
    // 启用或禁用选项卡中的控件
    QList<QWidget *> list = ui->tab_1->findChildren<QWidget *>();
    QList<QWidget *> list2 = ui->tab_2->findChildren<QWidget *>();
    list.append(list2);
    foreach (QWidget *w, list) { w->setEnabled(val); }
}

void MainWindow::on_pushButtonSelectArea_clicked() {
    // 处理选择区域按钮点击
    static bool first_call = true;
    bool state = ui->pushButtonSelectArea->isChecked();

    qDebug() << "rrs.fullscreen = false"; // 调试信息：取消全屏模式

    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    if (state) {
        if (first_call) {
            first_call = false;
            areaSelector->slot_init(); // 初始化区域选择器
        }
        emit signal_show(true); // 显示区域选择器
    }
}

void MainWindow::on_pushButtonFullscreen_clicked() {
    // 处理全屏按钮点击
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);

    rrs.width = screen->size().width();   // 设置全屏宽度
    rrs.height = screen->size().height(); // 设置全屏高度
    rrs.offset_x = 0;                     // X偏移为0
    rrs.offset_y = 0;                     // Y偏移为0

    qDebug() << "全屏: " << rrs.width << "x" << rrs.height; // 调试信息

    emit signal_show(false); // 隐藏区域选择器
}

void MainWindow::on_toolButton_clicked() {
    // 处理选择文件保存路径
    QString path = QFileDialog::getSaveFileName(this, tr("选择保存文件"), QString(QDir::homePath()), tr("音频 (*.mp4)"));
    path = path.endsWith(".mp4") ? path : path + ".mp4";
    ui->lineEditPath->setText(path);

    outFilePath = path.toStdString();
    qDebug() << "输出文件路径: " << QString::fromStdString(outFilePath); // 调试信息
}

//////主要动作//////
void MainWindow::on_pushButtonStart_clicked() {
    // 处理开始按钮点击
    if (ui->lineEditPath->text().isEmpty()) {
        QMessageBox::information(this, tr("无效路径"),
                                 tr("保存路径的文本框不能为空"));
        return;
    }

    ui->pushButtonStop->setEnabled(true);
    stopAction->setEnabled(true);
    ui->pushButtonPause->setEnabled(true);
    pauseAction->setEnabled(true);
    ui->pushButtonStart->setDisabled(true);
    startAction->setDisabled(true);
    quitAction->setDisabled(true);
    enable_or_disable_tabs(false);

    ui->lineEditPath->setText(QString(outFilePath.c_str()));

    showOrHideWindow(true);

    if (ui->pushButtonSelectArea->isChecked())
        emit signal_recording(true); // 更改区域选择器边框颜色

    alignValues();

    // 调试信息：输出录制参数
    qDebug() << "rrs值: \n 宽x高: " << rrs.width << " x " << rrs.height << " \n偏移: " << rrs.offset_x << ", " << rrs.offset_y
             << "\n 屏幕: " << rrs.screen_number << "\n 全屏: " << ui->pushButtonFullscreen->isChecked() << "\n";
    qDebug() << "vs值:"
             << "\n 帧率: " << vs.fps << "\n 质量: " << vs.quality << "\n 压缩: " << vs.compression << "\n 音频: " << QString::number(vs.audioOn) << "\n";
    qDebug() << "目录: " << QString::fromStdString(outFilePath);
    qDebug() << "设备名称: " << QString::fromStdString(deviceName);
    qDebug() << "最小化: " << minimizeInSysTray;
    try {
        screenRecorder = make_unique<ScreenRecorder>(rrs, vs, outFilePath, deviceName);
        std::cout << "创建ScreenRecorder对象" << std::endl;
        auto record_thread = std::thread{[&]() {
            try {
                std::cout << "-> 开始录制..." << std::endl;
                screenRecorder->record();
            } catch (const std::exception &e) {
                setGeneralDefaultProperties();
                alignValues();
                std::string message = e.what();
                errorDialog.critical(0, "错误", QString::fromStdString(message));
            }
            screenRecorder.reset();
            cv.notify_one();
        }};
        record_thread.detach();
    } catch (const std::exception &e) {
        setGeneralDefaultProperties();
        alignValues();
        std::string message = e.what();
        errorDialog.critical(0, "错误", QString::fromStdString(message));
    }
}

void MainWindow::on_pushButtonPause_clicked() {
    // 处理暂停按钮点击
    screenRecorder->pauseRecording();
    ui->pushButtonResume->setEnabled(true);
    resumeAction->setEnabled(true);
    ui->pushButtonPause->setEnabled(false);
    pauseAction->setEnabled(false);
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));
}

void MainWindow::on_pushButtonResume_clicked() {
    // 处理恢复按钮点击
    screenRecorder->resumeRecording();
    ui->pushButtonPause->setEnabled(true);
    pauseAction->setEnabled(true);
    ui->pushButtonResume->setEnabled(false);
    resumeAction->setEnabled(false);
    showOrHideWindow(true);
}

void MainWindow::on_pushButtonStop_clicked() {
    // 处理停止按钮点击
    if (screenRecorder) screenRecorder.get()->stopRecording();
    if (ui->pushButtonSelectArea->isChecked()) {
        emit signal_recording(false);
    }
    pauseAction->setDisabled(true);
    stopAction->setDisabled(true);
    ui->pushButtonPause->setEnabled(false);
    ui->pushButtonStop->setEnabled(false);

    auto waiting_thread = std::thread{[&]() {
        unique_lock<mutex> ul{m};
        cv.wait(ul, []() { return !screenRecorder; });
        defaultButtonProperties();
        showOrHideWindow(false);
        enable_or_disable_tabs(true);
    }};
    waiting_thread.detach();
}

////设置
void MainWindow::on_checkBoxMinimize_toggled(bool checked) {
    // 处理最小化到托盘选项
    minimizeInSysTray = checked;
}

void MainWindow::on_radioButtonYes_clicked() {
    // 处理启用音频选项
    vs.audioOn = true;
    ui->comboBox->setDisabled(false);
}

void MainWindow::on_radioButtonNo_clicked() {
    // 处理禁用音频选项
    vs.audioOn = false;
    ui->comboBox->setDisabled(true);
}

void MainWindow::on_radioButton24_clicked() {
    // 设置24帧率
    vs.fps = 24;
}

void MainWindow::on_radioButton30_clicked() {
    // 设置30帧率
    vs.fps = 30;
}

void MainWindow::on_radioButton60_clicked() {
    // 设置60帧率
    vs.fps = 60;
}

void MainWindow::on_horizontalSlider_sliderMoved(int position) {
    // 处理滑块移动
    setQualityANDCompression(position);
}

void MainWindow::on_lineEditPath_textEdited(const QString &arg1) {
    // 处理路径文本编辑
    outFilePath = forge_outpath(arg1.toStdString());
}

void MainWindow::on_comboBox_activated(const QString &arg1) {
    // 处理音频设备选择
    deviceName = arg1.toStdString();
}
