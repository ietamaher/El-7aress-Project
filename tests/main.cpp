#include <QtTest>
#include "test_cameracontroller.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    int status = 0;

    TestCameraController tc;
    status |= QTest::qExec(&tc, argc, argv);

    // Add other test classes here

    return status;
}
