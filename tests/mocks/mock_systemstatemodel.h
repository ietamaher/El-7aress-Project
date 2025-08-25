#ifndef MOCK_SYSTEMSTATEMODEL_H
#define MOCK_SYSTEMSTATEMODEL_H

#include "../../src/models/systemstatemodel.h"

class MockSystemStateModel : public SystemStateModel
{
    Q_OBJECT

public:
    MockSystemStateModel() {
        setTrackingStartedCalled = 0;
        lastTrackingStartedState = false;
    }

    // Call counters and state
    int setTrackingStartedCalled;
    bool lastTrackingStartedState;

    // Override methods to track calls
    void setTrackingStarted(bool start) override {
        setTrackingStartedCalled++;
        lastTrackingStartedState = start;
        // In a real scenario, this would also change the internal state
        // and emit dataChanged. For the mock, we can control that manually.
    }

    // Method to manually set the data for the data() method to return
    void setMockedData(const SystemStateData& data) {
        m_mockedData = data;
    }

    // Override data() to return the mocked data
    SystemStateData data() const override {
        return m_mockedData;
    }

    // Method to manually emit the dataChanged signal for testing
    void emitDataChanged(const SystemStateData& data) {
        emit SystemStateModel::dataChanged(data);
    }

private:
    SystemStateData m_mockedData;
};

#endif // MOCK_SYSTEMSTATEMODEL_H
