QT       += core gui serialbus serialport dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

#CONFIG += opengles2

INCLUDEPATH += "/usr/include/vpi3"
INCLUDEPATH += "/opt/nvidia/vpi3/include"
INCLUDEPATH += /usr/include/SDL2
LIBS += -L/opt/nvidia/vpi3/lib/x86_64-linux-gnu -lnvvpi
LIBS += -lSDL2


# Jetson-specific configurations

unix {
    contains(QMAKE_HOST.arch, "x86_64") {
        # PC-specific configurations
        INCLUDEPATH += "/usr/local/cuda-12.2/targets/x86_64-linux/include"
        INCLUDEPATH += "/opt/nvidia/deepstream/deepstream-6.4/sources/includes"
        #LIBS += -L/usr/local/cuda-12.2/lib64 -lcudart
        #LIBS += -L/opt/nvidia/deepstream/deepstream-6.4/lib -lnvdsgst_meta -lnvds_meta
        LIBS += -L/usr/lib/x86_64-linux-gnu/gstreamer-1.0 -lgstxvimagesink
    } else:contains(QMAKE_HOST.arch, "aarch64") {
        # Jetson-specific configurations
        INCLUDEPATH +="/usr/local/cuda-12.6/targets/aarch64-linux/include"
        INCLUDEPATH +="/opt/nvidia/deepstream/deepstream/sources/includes"
        #LIBS += -L/usr/local/cuda-12.6/lib64 -lcudart
        #LIBS += -L/opt/nvidia/deepstream/deepstream/lib #-lnvdsgst_meta -lnvds_meta
        LIBS += -L/usr/lib/aarch64-linux-gnu/tegra -lnvbufsurface -lnvbufsurftransform
        LIBS+=-L"/usr/lib/aarch64-linux-gnu/gstreamer-1.0" -lgstxvimagesink -L"/usr/lib/aarch64-linux-gnu" -lgstbase-1.0 -lgstreamer-1.0 -lglib-2.0 -lgobject-2.0

    }
}

# Common configurations
#INCLUDEPATH += "/usr/include/opencv4"
INCLUDEPATH += "/usr/local/include/opencv4"
INCLUDEPATH += "/usr/include/eigen3"
INCLUDEPATH += "/usr/include/glib-2.0"
INCLUDEPATH += "/usr/include/gstreamer-1.0"

CONFIG += link_pkgconfig
PKGCONFIG += gstreamer-1.0
PKGCONFIG += gstreamer-video-1.0

#INCLUDEPATH += /usr/include/freetype2
#LIBS += -lfreetype
#LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio
LIBS += -lgstreamer-1.0 -lgstapp-1.0 -lgstbase-1.0 -lgobject-2.0 -lglib-2.0
LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc
LIBS += -L/usr/local/lib -lopencv_core   -lopencv_dnn -lopencv_videoio
PKGCONFIG += gstreamer-gl-1.0


SOURCES += \
    controllers/cameracontroller.cpp \
    controllers/gimbalcontroller.cpp \
    controllers/joystickcontroller.cpp \
    controllers/motion_modes/autosectorscanmotionmode.cpp \
    controllers/motion_modes/gimbalmotionmodebase.cpp \
    controllers/motion_modes/manualmotionmode.cpp \
    controllers/motion_modes/radarslewmotionmode.cpp \
    controllers/motion_modes/trackingmotionmode.cpp \
    controllers/motion_modes/trpscanmotionmode.cpp \
    controllers/weaponcontroller.cpp \
    core/systemcontroller.cpp \
    devices/baseserialdevice.cpp \
    devices/imudevice.cpp \
    devices/modbusdevicebase.cpp \
    devices/osdrenderer.cpp \
    devices/outlinedtextitem.cpp \
    devices/radardevice.cpp \
    devices/cameravideostreamdevice.cpp \
    ui/areazoneparameterpanel.cpp \
    ui/basestyledwidget.cpp \
    ui/radartargetlistwidget.cpp \
    ui/sectorscanparameterpanel.cpp \
    ui/trpparameterpanel.cpp \
    ui/videodisplaywidget.cpp \
    main.cpp \
    ui/mainwindow.cpp \
    ui/custommenudialog.cpp \
    ui/systemstatuswidget.cpp \
    devices/servoactuatordevice.cpp \
    devices/plc42device.cpp \
    devices/plc21device.cpp \
    devices/nightcameracontroldevice.cpp \
    devices/daycameracontroldevice.cpp \
    devices/lrfdevice.cpp \
    devices/joystickdevice.cpp \
    devices/lensdevice.cpp \
    devices/servodriverdevice.cpp \
    models/joystickdatamodel.cpp \
    models/systemstatemodel.cpp \
    ui/zonedefinitionwidget.cpp \
    ui/zeroingwidget.cpp \
    ui/windagewidget.cpp \
    ui/zonemapwidget.cpp \
    utils/ballisticsprocessor.cpp \
    ui/cameracontainerwidget.cpp \
    utils/colorutils.cpp \
    utils/inference.cpp \
    utils/reticleaimpointcalculator.cpp

HEADERS += \
    controllers/cameracontroller.h \
    controllers/gimbalcontroller.h \
    controllers/joystickcontroller.h \
    controllers/motion_modes/autosectorscanmotionmode.h \
    controllers/motion_modes/gimbalmotionmodebase.h \
    controllers/motion_modes/manualmotionmode.h \
    controllers/motion_modes/pidcontroller.h \
    controllers/motion_modes/radarslewmotionmode.h \
    controllers/motion_modes/trackingmotionmode.h \
    controllers/motion_modes/trpscanmotionmode.h \
    controllers/weaponcontroller.h \
    core/systemcontroller.h \
    devices/baseserialdevice.h \
    devices/imudevice.h \
    devices/modbusdevicebase.h \
    devices/osdrenderer.h \
    devices/outlinedtextitem.h \
    devices/radardevice.h \
    devices/cameravideostreamdevice.h \
    devices/vpi_helpers.h \
    models/radardatamodel.h \
    ui/areazoneparameterpanel.h \
    ui/basestyledwidget.h \
    ui/radartargetlistwidget.h \
    ui/sectorscanparameterpanel.h \
    ui/trpparameterpanel.h \
    ui/videodisplaywidget.h \
    models/gyrodatamodel.h \
    models/lensdatamodel.h \
    models/nightcameradatamodel.h \
    ui/mainwindow.h \
    ui/custommenudialog.h \
    ui/systemstatuswidget.h \
    devices/servoactuatordevice.h \
    devices/plc42device.h \
    devices/plc21device.h \
    devices/nightcameracontroldevice.h \
    devices/daycameracontroldevice.h \
    devices/lrfdevice.h \
    devices/joystickdevice.h \
    devices/lensdevice.h \
    devices/servodriverdevice.h \
    models/daycameradatamodel.h \
    models/joystickdatamodel.h \
    models/lrfdatamodel.h \
    models/plc21datamodel.h \
    models/plc42datamodel.h \
    models/servoactuatordatamodel.h \
    models/servodriverdatamodel.h \
    models/systemstatedata.h \
    models/systemstatemodel.h \
    ui/zonedefinitionwidget.h \
    ui/zeroingwidget.h \
    ui/windagewidget.h \
    ui/zonemapwidget.h \
    utils/ballisticsprocessor.h \
    ui/cameracontainerwidget.h \
    utils/colorutils.h \
    utils/millenious.h \
    utils/inference.h \
    utils/reticleaimpointcalculator.h \
    utils/targetstate.h

FORMS += \
    ui/mainwindow.ui

#RESOURCES += resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

