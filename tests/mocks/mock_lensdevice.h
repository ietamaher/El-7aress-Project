#ifndef MOCK_LENSDEVICE_H
#define MOCK_LENSDEVICE_H

#include "../../src/devices/lensdevice.h"

class MockLensDevice : public LensDevice
{
    Q_OBJECT

public:
    MockLensDevice() {}

    // Add call counters and state tracking here if needed
};

#endif // MOCK_LENSDEVICE_H
