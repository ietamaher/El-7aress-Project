QT += testlib
QT -= gui

CONFIG += console
CONFIG -= app_bundle

# The TARGET variable specifies the name of the executable
TARGET = tests

# The INCLUDEPATH specifies the directories where to search for include files
INCLUDEPATH += $$PWD/../src

# The SOURCES variable specifies the source files to be compiled
SOURCES += \
    main.cpp \
    test_cameracontroller.cpp \
    # Add the application sources needed for the tests.
    # This avoids creating a library and keeps the build simple.
    $$PWD/../src/controllers/cameracontroller.cpp \
    $$PWD/../src/models/systemstatemodel.cpp \
    $$PWD/../src/devices/daycameracontroldevice.cpp \
    $$PWD/../src/devices/nightcameracontroldevice.cpp \
    $$PWD/../src/devices/cameravideostreamdevice.cpp \
    $$PWD/../src/devices/lensdevice.cpp \
    $$PWD/../src/devices/baseserialdevice.cpp \
    $$PWD/../src/devices/modbusdevicebase.cpp \
    $$PWD/../src/models/joystickdatamodel.cpp \
    $$PWD/../src/devices/osdrenderer.cpp \
    $$PWD/../src/utils/inference.cpp

# The HEADERS variable specifies the header files to be processed by moc
HEADERS += \
    test_cameracontroller.h \
    mocks/mock_daycameracontroldevice.h \
    mocks/mock_nightcameracontroldevice.h \
    mocks/mock_cameravideostreamdevice.h \
    mocks/mock_lensdevice.h \
    mocks/mock_systemstatemodel.h
