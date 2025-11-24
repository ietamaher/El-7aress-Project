QT += core gui  qml quick quickcontrols2 testlib serialbus serialport dbus widgets

# 2. Project Configuration
CONFIG += console testcase
CONFIG -= app_bundle



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


INCLUDEPATH += /usr/local/cuda/include
LIBS += -L/usr/local/cuda/lib64 -lcudart

# 3. Target Executable Name
TARGET = tests

# 4. Project Template
TEMPLATE = app

# 5. Include Paths
INCLUDEPATH += ../src \
               ../include

# 6. Source Files

# Use files() to find all test source files
SOURCES += $$files(./tst_*.cpp)

# --- CORRECTED SECTION ---
# Use the qmake files() function to correctly expand wildcards
# before passing them to the compiler.
APP_CONTROLLERS = $$files(../src/controllers/*.cpp)
APP_MODELS      = $$files(../src/models/*.cpp)
APP_DEVICES     = $$files(../src/devices/*.cpp)
APP_UTILS       = $$files(../src/utils/*.cpp)
APP_MOTION_MODES = $$files(../src/controllers/motion_modes/*.cpp)
# Add the application source files to the build
SOURCES += \
    $$APP_CONTROLLERS \
    $$APP_MODELS \
    $$APP_DEVICES \
    $$APP_UTILS  \
    $$APP_MOTION_MODES

# Headers are good practice for IDEs
HEADERS += \
    $$files(../src/controllers/motion_modes/*.h) \
    $$files(../src/controllers/*.h) \
    $$files(../src/models/*.h) \
    $$files(../src/devices/*.h) \
    $$files(../src/utils/*.h)
