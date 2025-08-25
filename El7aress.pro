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
INCLUDEPATH += "/usr/include/opencv4"
#INCLUDEPATH += "/usr/local/include/opencv4"
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
    src/controllers/cameracontroller.cpp \
    src/controllers/gimbalcontroller.cpp \
    src/controllers/joystickcontroller.cpp \
    src/controllers/motion_modes/autosectorscanmotionmode.cpp \
    src/controllers/motion_modes/gimbalmotionmodebase.cpp \
    src/controllers/motion_modes/manualmotionmode.cpp \
    src/controllers/motion_modes/radarslewmotionmode.cpp \
    src/controllers/motion_modes/trackingmotionmode.cpp \
    src/controllers/motion_modes/trpscanmotionmode.cpp \
    src/controllers/weaponcontroller.cpp \
    src/core/systemcontroller.cpp \
    src/devices/baseserialdevice.cpp \
    src/devices/imudevice.cpp \
    src/devices/modbusdevicebase.cpp \
    src/devices/osdrenderer.cpp \
    src/devices/outlinedtextitem.cpp \
    src/devices/radardevice.cpp \
    src/devices/cameravideostreamdevice.cpp \
    src/ui/areazoneparameterpanel.cpp \
    src/ui/basestyledwidget.cpp \
    src/ui/radartargetlistwidget.cpp \
    src/ui/sectorscanparameterpanel.cpp \
    src/ui/trpparameterpanel.cpp \
    src/ui/videodisplaywidget.cpp \
    src/main.cpp \
    src/ui/mainwindow.cpp \
    src/ui/custommenudialog.cpp \
    src/ui/systemstatuswidget.cpp \
    src/devices/servoactuatordevice.cpp \
    src/devices/plc42device.cpp \
    src/devices/plc21device.cpp \
    src/devices/nightcameracontroldevice.cpp \
    src/devices/daycameracontroldevice.cpp \
    src/devices/lrfdevice.cpp \
    src/devices/joystickdevice.cpp \
    src/devices/lensdevice.cpp \
    src/devices/servodriverdevice.cpp \
    src/models/joystickdatamodel.cpp \
    src/models/systemstatemodel.cpp \
    src/ui/zonedefinitionwidget.cpp \
    src/ui/zeroingwidget.cpp \
    src/ui/windagewidget.cpp \
    src/ui/zonemapwidget.cpp \
    src/utils/ballisticsprocessor.cpp \
    src/ui/cameracontainerwidget.cpp \
    src/utils/colorutils.cpp \
    src/utils/inference.cpp \
    src/utils/reticleaimpointcalculator.cpp

HEADERS += \
    src/controllers/cameracontroller.h \
    src/controllers/gimbalcontroller.h \
    src/controllers/joystickcontroller.h \
    src/controllers/motion_modes/autosectorscanmotionmode.h \
    src/controllers/motion_modes/gimbalmotionmodebase.h \
    src/controllers/motion_modes/manualmotionmode.h \
    src/controllers/motion_modes/pidcontroller.h \
    src/controllers/motion_modes/radarslewmotionmode.h \
    src/controllers/motion_modes/trackingmotionmode.h \
    src/controllers/motion_modes/trpscanmotionmode.h \
    src/controllers/weaponcontroller.h \
    src/core/systemcontroller.h \
    src/devices/baseserialdevice.h \
    src/devices/imudevice.h \
    src/devices/modbusdevicebase.h \
    src/devices/osdrenderer.h \
    src/devices/outlinedtextitem.h \
    src/devices/radardevice.h \
    src/devices/cameravideostreamdevice.h \
    src/devices/vpi_helpers.h \
    src/models/radardatamodel.h \
    src/ui/areazoneparameterpanel.h \
    src/ui/basestyledwidget.h \
    src/ui/radartargetlistwidget.h \
    src/ui/sectorscanparameterpanel.h \
    src/ui/trpparameterpanel.h \
    src/ui/videodisplaywidget.h \
    src/models/gyrodatamodel.h \
    src/models/lensdatamodel.h \
    src/models/nightcameradatamodel.h \
    src/ui/mainwindow.h \
    src/ui/custommenudialog.h \
    src/ui/systemstatuswidget.h \
    src/devices/servoactuatordevice.h \
    src/devices/plc42device.h \
    src/devices/plc21device.h \
    src/devices/nightcameracontroldevice.h \
    src/devices/daycameracontroldevice.h \
    src/devices/lrfdevice.h \
    src/devices/joystickdevice.h \
    src/devices/lensdevice.h \
    src/devices/servodriverdevice.h \
    src/models/daycameradatamodel.h \
    src/models/joystickdatamodel.h \
    src/models/lrfdatamodel.h \
    src/models/plc21datamodel.h \
    src/models/plc42datamodel.h \
    src/models/servoactuatordatamodel.h \
    src/models/servodriverdatamodel.h \
    src/models/systemstatedata.h \
    src/models/systemstatemodel.h \
    src/ui/zonedefinitionwidget.h \
    src/ui/zeroingwidget.h \
    src/ui/windagewidget.h \
    src/ui/zonemapwidget.h \
    src/utils/ballisticsprocessor.h \
    src/ui/cameracontainerwidget.h \
    src/utils/colorutils.h \
    src/utils/millenious.h \
    src/utils/inference.h \
    src/utils/reticleaimpointcalculator.h \
    src/utils/targetstate.h

FORMS += \
    src/ui/mainwindow.ui

#RESOURCES += src/resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
