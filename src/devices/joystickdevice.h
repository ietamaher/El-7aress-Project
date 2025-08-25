#ifndef JOYSTICKHANDLER_H
#define JOYSTICKHANDLER_H

#include <QObject>
#include <QTimer>
#include <SDL2/SDL.h>
#include "../TimestampLogger.h"

// A simple data structure to hold joystick states:
struct JoystickData {
    // Example: 2 axes (X = 0, Y = 1)
    float axisX = 0.0f;
    float axisY = 0.0f;

    // Example: 4 hats (0-3)
    // Each hat can have 4 directions: up, down, left, right
    // Represented as a bitmask: 0 = centered, 1 = up,
    // 2 = right, 3 = down, 4 = left
    // For simplicity, we can use an int to represent the state
    int hatState = 0; // 0 = centered, 1 = up,
                      // 2 = right, 3 = down, 4 = left      


    // Example: store up to 16 buttons
    static const int MAX_BUTTONS = 16;
    bool buttons[MAX_BUTTONS] = { false };

    bool operator==(const JoystickData &other) const {
        if (axisX != other.axisX || axisY != other.axisY)
            return false;
        for (int i=0; i<MAX_BUTTONS; i++) {
            if (buttons[i] != other.buttons[i])
                return false;
        }
        if (hatState != other.hatState)
            return false;
        return true;
    }

    bool operator!=(const JoystickData &other) const {
        return !(*this == other);
    }
};



class JoystickDevice : public QObject {
    Q_OBJECT
public:
    explicit JoystickDevice(QObject *parent = nullptr);
    ~JoystickDevice();
    
    void printJoystickGUIDs();
signals:
    void axisMoved(int axis, int value);
    void buttonPressed(int button, bool pressed);
    void hatMoved(int hat, int value); // For hat switches (if needed)

private slots:
    void pollJoystick(); // Replaces readData()

private:
    SDL_Joystick *m_joystick;
    QTimer *m_pollTimer;
};

#endif // JOYSTICKHANDLER_H
