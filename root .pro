# project.pro  (root)
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    src \
    tests

RESOURCES += resources/qml.qrc

src.depends =
tests.depends = src
