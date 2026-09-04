#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <Arduino.h>
#include <BleKeyboard.h>
#include <Preferences.h>

#define ROWS 3
#define COLS 3
#define SC_LEN_MAX 5
#define TOTAL_COMBINATIONS (1 << (ROWS * COLS))
#define SC_HOLD_TIME_MILLI 100

// External global Bluetooth keyboard object declared in main.cpp
extern BleKeyboard bleKeyboard;

class Shortcut_Manager {
private:
    uint16_t isKeyModifierMask = 0;
    uint8_t shortcuts[TOTAL_COMBINATIONS][SC_LEN_MAX] = {};

    struct KeyOption {
        const char *name;
        uint8_t code;
    };

    static const KeyOption keyOptions[56];
    static const int keyOptionCount;

    Shortcut_Manager();
    ~Shortcut_Manager() {}

    void sendShortcut(uint16_t combinationIndex);
    int parseKeycodeNameToValue(String& keyName);
    void loadShortcuts();
    void saveShortcuts();
    void resetShortcuts();
    void printShortcuts();
    void setModifier(uint8_t rawKey, bool set);
    void setShortcut(String& button, String& key);
    void printHelp();

public:
    static Shortcut_Manager& getInstance();

    void parseCommand(String& line);
    void update(uint16_t currentButtonState, uint16_t lastButtonState);

    // Prevent copying
    Shortcut_Manager(const Shortcut_Manager&) = delete;
    void operator=(const Shortcut_Manager&) = delete;
};

#endif // SHORTCUT_MANAGER_H