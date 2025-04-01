# 项目基本配置
QT       += core gui widgets multimedia  # 使用Qt核心、GUI、窗口部件和多媒体模块
CONFIG   += c++17                        # 使用C++17标准

# 定义，启用Qt的弃用警告
DEFINES  += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    areaselector.cpp \
    areaselectorbuttons.cpp \
    getaudiodevices.cpp \
    main.cpp \
    mainwindow.cpp \
    memorychecklinux.cpp \
    screenrecorder.cpp

HEADERS += \
    areaselector.h \
    areaselectorbuttons.h \
    getaudiodevices.h \
    mainwindow.h \
    memorychecklinux.h \
    screenrecorder.h

FORMS += \
    mainwindow.ui

# 资源文件（用于图标等）
RESOURCES += \
    resource.qrc

# 平台特定设置
win32 {
    # Windows下链接FFmpeg库
    # 头文件路径
    INCLUDEPATH += "C:/msys64/mingw64/include/ffmpeg4.4"

    # window库
    LIBS += -lpthread -lole32 -loleaut32

    # 链接 FFmpeg 库
    LIBS += -L"C:/msys64/mingw64/lib/ffmpeg4.4" \
            -lavcodec \
            -lavformat \
            -lavutil \
            -lswscale \
            -lavdevice \
            -lswresample \
            -lavfilter \
            -lpostproc
}

unix {
    # Linux下链接FFmpeg库
    LIBS += -lavformat -lavcodec -lavutil -lavdevice -lswscale -lX11 -lpthread -lswresample -lasound
}
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

icon.files=./icons/icon.svg
