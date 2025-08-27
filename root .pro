# project.pro  (root)
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    src \
    tests


src.depends =
tests.depends = src
