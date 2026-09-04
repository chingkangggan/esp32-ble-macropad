#include "shortcutManager.h"

const Shortcut_Manager::KeyOption Shortcut_Manager::keyOptions[56] = {
    { "NONE", 0 }, { "LCTRL", KEY_LEFT_CTRL }, { "LSHIFT", KEY_LEFT_SHIFT },
    { "LALT", KEY_LEFT_ALT }, { "LWIN", KEY_LEFT_GUI }, { "TAB", KEY_TAB },
    { "ENTER", KEY_RETURN }, { "RETURN", KEY_RETURN }, { "ESC", KEY_ESC },
    { "SPACE", ' ' }, { "UP", KEY_UP_ARROW }, { "DOWN", KEY_DOWN_ARROW },
    { "LEFT", KEY_LEFT_ARROW }, { "RIGHT", KEY_RIGHT_ARROW },
    { "A", 'a' }, { "B", 'b' }, { "C", 'c' }, { "D", 'd' }, { "E", 'e' },
    { "F", 'f' }, { "G", 'g' }, { "H", 'h' }, { "I", 'i' }, { "J", 'j' },
    { "K", 'k' }, { "L", 'l' }, { "M", 'm' }, { "N", 'n' }, { "O", 'o' },
    { "P", 'p' }, { "Q", 'q' }, { "R", 'r' }, { "S", 's' }, { "T", 't' },
    { "U", 'u' }, { "V", 'v' }, { "W", 'w' }, { "X", 'x' }, { "Y", 'y' },
    { "Z", 'z' }, { "0", '0' }, { "1", '1' }, { "2", '2' }, { "3", '3' },
    { "4", '4' }, { "5", '5' }, { "6", '6' }, { "7", '7' }, { "8", '8' },
    { "9", '9' }, { "SLASH", '/' }, { "DOT", '.' }, { "COMMA", ',' },
    { "MINUS", '-' }, { "EQUAL", '=' }, { "BACKTICK", '`' }
};

const int Shortcut_Manager::keyOptionCount = sizeof(Shortcut_Manager::keyOptions) / sizeof(Shortcut_Manager::keyOptions[0]);

Shortcut_Manager::Shortcut_Manager() {
    loadShortcuts();

    Serial.println();
    printHelp();
    printShortcuts();
}

Shortcut_Manager& Shortcut_Manager::getInstance() {
    static Shortcut_Manager instance;
    return instance;
}

void Shortcut_Manager::sendShortcut(uint16_t combinationIndex) {
    if (!bleKeyboard.isConnected()) {
        Serial.println("WARNING: Bluetooth not connected.");
        return;
    }

    bool pressedAny = false;
    for (int slot = 0; slot < SC_LEN_MAX; slot++) {
        uint8_t code = shortcuts[combinationIndex][slot];
        if (code != 0) {
            bleKeyboard.press(code);
            pressedAny = true;
        }
    }

    if (pressedAny) {
        delay(SC_HOLD_TIME_MILLI);
        bleKeyboard.releaseAll();
    }
}

int Shortcut_Manager::parseKeycodeNameToValue(String& keyName) {
    for (int i = 0; i < keyOptionCount; i++) {
        if (keyName == keyOptions[i].name) {
            return keyOptions[i].code;
        }
    }
    return 0;
}

void Shortcut_Manager::loadShortcuts() {
    Preferences preferences;
    preferences.begin("macropad", true);
    if (preferences.isKey("mod_mask")) {
        preferences.getBytes("mod_mask", &isKeyModifierMask, sizeof(isKeyModifierMask));
    } else {
        isKeyModifierMask = 0;
    }

    if (preferences.isKey("shortcuts")) {
        preferences.getBytes("shortcuts", shortcuts, sizeof(shortcuts));
    } else {
        memset(shortcuts, 0, sizeof(shortcuts));
    }

    preferences.end();
}

void Shortcut_Manager::saveShortcuts() {
    Preferences preferences;
    preferences.begin("macropad", false);

    preferences.putBytes("mod_mask", &isKeyModifierMask, sizeof(isKeyModifierMask));
    preferences.putBytes("shortcuts", shortcuts, sizeof(shortcuts));

    preferences.end();
    Serial.println("Configuration saved to NVS Preferences.");
}

void Shortcut_Manager::resetShortcuts() {
    isKeyModifierMask = 0;
    memset(shortcuts, 0, sizeof(shortcuts));

    Preferences preferences;
    preferences.begin("macropad", false);
    preferences.remove("mod_mask");
    preferences.remove("shortcuts");
    preferences.end();
    Serial.println("Flash memory defaults cleared.");
}

void Shortcut_Manager::printShortcuts() {
    Serial.println(F("\n======================================================================"));
    Serial.println(F("                    MACROPAD CONFIGURATION                            "));
    Serial.println(F("======================================================================"));

    Serial.println(F(" [ PHYSICAL KEY STATUS MAP ]"));
    Serial.println(F("  +---+---+---+"));
    
    for (int row = 0; row < ROWS; row++) {
        Serial.print("  | ");
        for (int col = 0; col < COLS; col++) {
            int pinId = (row * COLS) + col;
            bool isMod = (isKeyModifierMask >> pinId) & 1;
            
            Serial.printf("%d:%c | ", pinId + 1, isMod ? 'M' : 'A');
        }
        Serial.println();
        Serial.println(F("  +---+---+---+"));
    }
    Serial.println(F("  (Legend -> M: MODIFIER holding key  |  A: ACTION trigger key)\n"));

    Serial.println(F(" [ ACTIVE CUSTOM SHORTCUTS ]"));
    int activeCount = 0;

    for (int idx = 0; idx < TOTAL_COMBINATIONS; idx++) {
        if (shortcuts[idx][0] == 0) {
            continue; 
        }

        activeCount++;
        String buttonSequenceStr = "";
        
        for (int btn = 0; btn < (ROWS * COLS); btn++) {
            if ((idx >> btn) & 1) {
                if ((isKeyModifierMask >> btn) & 1) {
                    if (buttonSequenceStr.length() > 0) buttonSequenceStr += "+";
                    buttonSequenceStr += String(btn + 1);
                }
            }
        }

        for (int btn = 0; btn < (ROWS * COLS); btn++) {
            if ((idx >> btn) & 1) {
                if (!((isKeyModifierMask >> btn) & 1)) {
                    if (buttonSequenceStr.length() > 0) buttonSequenceStr += "+";
                    buttonSequenceStr += String(btn + 1);
                }
            }
        }

        String keySequenceStr = "";
        for (int k = 0; k < SC_LEN_MAX; k++) {
            uint8_t code = shortcuts[idx][k];
            if (code == 0) break;

            if (keySequenceStr.length() > 0) keySequenceStr += "+";

            bool foundName = false;
            for (int i = 0; i < keyOptionCount; i++) {
                if (keyOptions[i].code == code) {
                    keySequenceStr += keyOptions[i].name;
                    foundName = true;
                    break;
                }
            }

            if (!foundName) {
                if (code >= 32 && code <= 126) {
                    keySequenceStr += String((char)code);
                } else {
                    char hexBuf[8];
                    sprintf(hexBuf, "0x%02X", code);
                    keySequenceStr += String(hexBuf);
                }
            }
        }

        Serial.printf("  SET %-10s : %s\n", buttonSequenceStr.c_str(), keySequenceStr.c_str());
    }

    if (activeCount == 0) {
        Serial.println("  No custom shortcuts mapped.");
    }
    Serial.println(F("======================================================================\n"));
}

void Shortcut_Manager::setModifier(uint8_t rawKey, bool set) {
    if (rawKey < 1 || rawKey > (ROWS * COLS)) {
        Serial.printf("ERROR: Key %d is out of matrix bounds (1-%d).\n", rawKey, (ROWS * COLS));
        return;
    }
    uint8_t key = rawKey - 1;

    bool isCurrentlyModifier = (isKeyModifierMask >> key) & 1;
    if (isCurrentlyModifier == set) {
        Serial.printf("Key %d is already %s.\n", rawKey, set ? "a modifier" : "a normal key");
        return; 
    }

    int affectedShortcutsCount = 0;
    for (int i = 0; i < TOTAL_COMBINATIONS; i++) {
        if ((i >> key) & 1) {
            if (shortcuts[i][0] != 0) { 
                affectedShortcutsCount++;
            }
        }
    }

    if (affectedShortcutsCount > 0) {
        Serial.printf("WARNING: Changing key %d will delete %d shortcut(s) using it.\n", rawKey, affectedShortcutsCount);
        Serial.print("Proceed? (y/n): ");
        
        while (Serial.available()) { Serial.read(); }
            
        while (Serial.available() == 0) {
            yield();
        }
        char response = Serial.read();
        while (Serial.available() > 0) { Serial.read(); } 
        Serial.println(response); 

        if (response != 'y' && response != 'Y') {
            Serial.println("Operation cancelled.");
            return;
        }

        for (int i = 0; i < TOTAL_COMBINATIONS; i++) {
            if ((i >> key) & 1) {
                memset(shortcuts[i], 0, SC_LEN_MAX); 
            }
        }
        Serial.printf("Purged %d affected shortcuts.\n", affectedShortcutsCount);
    }

    if (set) {
        isKeyModifierMask |= (1 << key);   
    } else {
        isKeyModifierMask &= ~(1 << key);  
    }

    Serial.printf("Key %d successfully updated to: %s\n", rawKey, set ? "MODIFIER" : "ACTION");
    saveShortcuts();
}

void Shortcut_Manager::setShortcut(String& button, String& key) {
    uint16_t combinationIndex = 0;
    int buttonCountLocal = 0;
    
    String btnToken = "";
    int btnStart = 0;
    int btnEnd = button.indexOf('+');
    
    int tempButtons[ROWS * COLS]; 
    
    while (btnStart < button.length()) {
        if (btnEnd == -1) btnEnd = button.length();
        btnToken = button.substring(btnStart, btnEnd);
        btnToken.trim();
        
        if (btnToken.length() > 0) {
            int rawBtnId = btnToken.toInt();
            if (rawBtnId < 1 || rawBtnId > (ROWS * COLS)) {
                Serial.printf("ERROR: Invalid pad button name '%s'. Use 1-%d\n", btnToken.c_str(), (ROWS * COLS));
                return;
            }
            if (buttonCountLocal >= (ROWS * COLS)) {
                Serial.println("ERROR: Too many button inputs in sequence.");
                return;
            }
            tempButtons[buttonCountLocal++] = rawBtnId - 1;
        }
        
        btnStart = btnEnd + 1;
        btnEnd = button.indexOf('+', btnStart);
    }

    if (buttonCountLocal == 0) {
        Serial.println("ERROR: No buttons specified.");
        return;
    }

    bool buttonSeen[ROWS * COLS] = {false};

    for (int i = 0; i < buttonCountLocal; i++) {
        int currentId = tempButtons[i];

        if (buttonSeen[currentId]) {
            Serial.printf("ERROR: Duplicate button %d detected in the sequence. Each button can only be used once.\n", currentId + 1);
            return;
        }
        buttonSeen[currentId] = true;

        combinationIndex |= (1 << currentId); 

        bool isMod = (isKeyModifierMask >> currentId) & 1;

        if (i < buttonCountLocal - 1) {
            if (!isMod) {
                Serial.printf("ERROR: Button %d is not configured as a MODIFIER. Fix using Modifier first.\n", currentId + 1);
                return;
            }
        } else {
            if (isMod) {
                Serial.printf("ERROR: The final trigger button %d is a MODIFIER. It must be an ACTION key.\n", currentId + 1);
                return;
            }
        }
    }

    uint8_t targetSequence[SC_LEN_MAX] = {0};
    int keyCount = 0;
    
    int keyStart = 0;
    int keyEnd = key.indexOf('+');
    String keyToken = "";

    while (keyStart < key.length()) {
        if (keyEnd == -1) keyEnd = key.length();
        keyToken = key.substring(keyStart, keyEnd);
        keyToken.trim();

        if (keyToken.length() > 0) {
            if (keyCount >= SC_LEN_MAX) {
                Serial.printf("ERROR: Key sequence length exceeds maximum limit of %d.\n", SC_LEN_MAX);
                return;
            }
            uint8_t parsedVal = parseKeycodeNameToValue(keyToken);
            if (parsedVal == 0) {
                Serial.printf("ERROR: Unrecognized key definition '%s'\n", keyToken.c_str());
                return;
            }
            targetSequence[keyCount++] = parsedVal;
        }

        keyStart = keyEnd + 1;
        keyEnd = key.indexOf('+', keyStart);
    }

    if (keyCount == 0) {
        Serial.println("ERROR: Key sequence cannot be empty.");
        return;
    }

    bool hasPreviousValue = false;
    for (int i = 0; i < SC_LEN_MAX; i++) {
        if (shortcuts[combinationIndex][i] != 0) {
            hasPreviousValue = true;
            break;
        }
    }

    if (hasPreviousValue) {
        Serial.printf("WARNING: Combination index [%d] already has a shortcut assigned.\n", combinationIndex);
        Serial.print("Overwrite existing shortcut? (y/n): ");

        while (Serial.available() > 0) { Serial.read(); } 

        while (Serial.available() == 0) {
            yield(); 
        }

        char response = Serial.read();
        while (Serial.available() > 0) { Serial.read(); } 
        Serial.println(response);

        if (response != 'y' && response != 'Y') {
            Serial.println("Operation cancelled.");
            return;
        }
    }

    memset(shortcuts[combinationIndex], 0, SC_LEN_MAX); 
    for (int i = 0; i < keyCount; i++) {
        shortcuts[combinationIndex][i] = targetSequence[i];
    }

    Serial.printf("SUCCESS: Shortcut saved to combination index %d!\n", combinationIndex);
    saveShortcuts();
}

void Shortcut_Manager::printHelp() {
    Serial.println(F(
        "======================================================================\n"
        "                    MACROPAD SHORTCUT MANAGER (3x3)\n"
        "======================================================================\n"
        "1. SHOW\n"
        "   Lists all physical pad keys (1-9) and active custom shortcuts.\n\n"
        "2. MODIFIER <button>:<0 or 1>\n"
        "   Sets a key's functional assignment type. Available pins: 1 to 9.\n"
        "   * Example: MODIFIER 1:1  -> Sets key 1 as a MODIFIER holding-key.\n"
        "   * Example: MODIFIER 1:0  -> Sets key 1 back to a normal ACTION key.\n\n"
        "3. SET <button_sequence>:<keyboard_sequence>\n"
        "   Maps macro key combinations to standard keyboard outputs.\n"
        "   * Syntax:  mod_btn+mod_btn+...+action_btn : key+key+...\n"
        "   * Rule:    The sequence must match real-time press order.\n"
        "              All initial buttons must be declared MODIFIERS first.\n"
        "              The final token serves as your trigger ACTION key.\n\n"
        "   Examples:\n"
        "   * SET 3:LCTRL+C\n"
        "     Pressing Action key 3 fires 'Ctrl+C' (No modifiers held).\n\n"
        "   * SET 1+2:LCTRL+V\n"
        "     Holding Modifier 1 and tapping Action key 2 fires 'Ctrl+V'.\n"
        "     (Note: 'SET 2+1' will fail if key 2 is not a configured Modifier).\n\n"
        "4. RESET\n"
        "   Clears all custom shortcut bindings and restores default profiles.\n"
        "======================================================================"
    ));
}

void Shortcut_Manager::parseCommand(String& line) {
    line.trim();
    line.toUpperCase();

    if (line == "HELP") {
        printHelp();
        return;
    }
    if (line == "SHOW") {
        printShortcuts();
        return;
    }
    if (line == "RESET") {
        resetShortcuts();
        saveShortcuts();
        printShortcuts();
        return;
    }
    if (line.startsWith("MODIFIER ")) {
        String payload = line.substring(9);
        payload.trim();
        int colonIdx = payload.indexOf(':');
        if (colonIdx <= 0 || colonIdx >= payload.length() - 1) {
            Serial.println("ERROR: Invalid MODIFIER format. Use MODIFIER button:0 or 1");
            return;
        }
        int button = payload.substring(0, colonIdx).toInt();
        bool set = payload.substring(colonIdx + 1).toInt() == 1;
        setModifier(button, set);
        return;
    }

    if (line.startsWith("SET ")) {
        String payload = line.substring(4);
        payload.trim();
        int colonIdx = payload.indexOf(':');
        if (colonIdx <= 0 || colonIdx >= payload.length() - 1) {
            Serial.println("ERROR: Invalid SET format. Use SET button+combo:key+combo");
            return;
        }
        String buttonPart = payload.substring(0, colonIdx);
        String keyPart = payload.substring(colonIdx + 1);
        setShortcut(buttonPart, keyPart);
        return;
    }
}

void Shortcut_Manager::update(uint16_t currentButtonState, uint16_t lastButtonState) {
    for (int i = 0; i < (ROWS * COLS); i++) {
        bool isPressed = (currentButtonState & (1 << i)) != 0;
        bool wasPressed = (lastButtonState & (1 << i)) != 0;

        if (isPressed && !wasPressed) {
            sendShortcut(currentButtonState);
        }
    }
}