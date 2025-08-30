# System State Analysis Report

This report summarizes the analysis of the `SystemStateData` struct and the `SystemStateModel` class.

## 1. Missing Variables in `SystemStateData`

This section lists the variables that are present in the device-specific data structures but are missing from the central `SystemStateData` struct in `src/models/systemstatedata.h`.

### RADAR Device
*   No missing variables.

### LRF Device (`LrfData`)
*   `isConnected`
*   `isLastRangingValid`
*   `pulseCount`
*   `isFault`
*   `noEcho`
*   `laserNotOut`
*   `isTempValid`
*   `temperature`
*   `laserCount`

### IMU Device (`ImuData`)
*   `isConnected`

### PLC21 Device (`Plc21PanelData`)
*   `isConnected`
*   `switchCameraSW`

### PLC42 Device (`Plc42Data`)
*   `isConnected`
*   `solenoidActive`

### ServomotorDriver Device (`ServoData`)
*   `isConnected`
*   `rpm`
*   `torque`
*   `fault`

### ServoActuatorDevice (`ServoActuatorData`)
*   `isConnected`
*   `velocity_mm_s`
*   `temperature_c`
*   `busVoltage_v`
*   `torque_percent`
*   `status` (and its sub-fields)

### NightCameraControlDevice (`NightCameraData`)
*   `ffcInProgress`
*   `digitalZoomEnabled`
*   `digitalZoomLevel`
*   `videoMode`

---

## 2. Unhandled Variables in `SystemStateModel` Methods

This section lists the variables from device data structures that are not being used to update `SystemStateData` within the corresponding `on...DataChanged` methods in `SystemStateModel`.

### `onPlc21DataChanged(const Plc21PanelData &pData)`
*   `isConnected`
*   `panelTemperature`

### `onPlc42DataChanged(const Plc42Data &pData)`
*   `isConnected`
*   `solenoidActive`

### `onServoAzDataChanged(const ServoData &azData)` & `onServoElDataChanged(const ServoData &elData)`
*   `isConnected`
*   `rpm`
*   `torque`
*   `fault`

### `onServoActuatorDataChanged(const ServoActuatorData &actuatorData)`
*   `isConnected`
*   `velocity_mm_s`
*   `temperature_c`
*   `busVoltage_v`
*   `torque_percent`
*   `status`

### `onLrfDataChanged(const LrfData &lrfData)`
*   `isConnected`
*   `isLastRangingValid`
*   `pulseCount`
*   `rawStatusByte` (Note: `lrfSystemStatus` is being set from the boolean `isFault` instead of the full status byte, which is a loss of information)
*   `noEcho`
*   `laserNotOut`
*   `isTempValid`
*   `temperature`
*   `laserCount`

### `onDayCameraDataChanged(const DayCameraData &dayData)`
*   `zoomMovingIn`
*   `zoomMovingOut`
*   `autofocusEnabled`
*   `focusPosition`

### `onGyroDataChanged(const ImuData &gyroData)`
*   `isConnected`

### `onLensDataChanged(const LensData &lensData)`
*   This method is empty. **None** of the fields from `lensData` are used.

### `onNightCameraDataChanged(const NightCameraData &nightData)`
*   `ffcInProgress`
*   `digitalZoomEnabled`
*   `videoMode`

### Joystick Methods
*   **`onJoystickButtonChanged(int button, bool pressed)`**: This method is empty and does not handle any button presses.
