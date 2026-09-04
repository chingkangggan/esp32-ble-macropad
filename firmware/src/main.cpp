#include <Arduino.h>
#include <BleKeyboard.h>
#include <ESP32Encoder.h>
#include "shortcutManager.h"

const uint8_t rowPins[ROWS] = {19, 16, 17}; // OUTPUT
const uint8_t colPins[COLS] = {13, 4, 18};  // INPUT

const uint8_t rotaryCLK1 = 27;
const uint8_t rotaryDT1 = 26;
const uint8_t rotarySW1 = 14;
const uint8_t rotaryCLK2 = 32;
const uint8_t rotaryDT2 = 25;
const uint8_t rotarySW2 = 33;

ESP32Encoder encoder1;
ESP32Encoder encoder2;

uint16_t buttonState = 0;
uint16_t lastButtonState = 0;

uint8_t rotaryButtonState1 = 0;
uint8_t rotaryButtonState2 = 0;
uint8_t lastRotaryButtonState1 = 255;
uint8_t lastRotaryButtonState2 = 255;

int32_t rotaryCount1 = 0;
int32_t lastRotaryCount1 = -1;
int32_t rotaryCount2 = 0;
int32_t lastRotaryCount2 = -1;
bool isMuted = false;

BleKeyboard bleKeyboard("ESP32 Macropad");

void readRotaryButton() {
    rotaryButtonState1 = !digitalRead(rotarySW1);
    rotaryButtonState2 = !digitalRead(rotarySW2);
}

void readMatrix() {
    uint16_t currentScanState = 0;
    for (int i = 0; i < ROWS; i++) {
        pinMode(rowPins[i], OUTPUT);
        digitalWrite(rowPins[i], LOW);
        
        for (int j = 0; j < COLS; j++) {
            if (!digitalRead(colPins[j])) {
                currentScanState |= (1 << (j + i * ROWS));
            }
        }
        digitalWrite(rowPins[i], HIGH);
        pinMode(rowPins[i], INPUT);
    }
    buttonState = currentScanState;
}

void decodeKeypressed() {
    Serial.print("[MATRIX] Pressed Keys: ");

    bool anyKeyPressed = false;
    for (int k = 0; k < (ROWS * COLS); k++) {
        if ((buttonState & (1 << k)) != 0) {
            Serial.print("Key ");
            Serial.print(k + 1);
            Serial.print(" ");
            anyKeyPressed = true;
        }
    }

    if (!anyKeyPressed) {
        Serial.print("None");
    }

    Serial.println();
}

void handleSerial() {
    static String inputBuffer = ""; 

    while (Serial.available() > 0) {
        char inChar = (char)Serial.read();

        if (inChar == '\n') {
            inputBuffer.trim();
            if (inputBuffer.length() > 0) {
                Shortcut_Manager::getInstance().parseCommand(inputBuffer);
            }
            inputBuffer = "";
        } 
        else {
            inputBuffer += inChar;
        }
    }
}

void setup() {
    Serial.begin(115200);

    for (int i = 0; i < ROWS; i++) {
        pinMode(rowPins[i], INPUT);
    }
    for (int i = 0; i < COLS; i++) {
        pinMode(colPins[i], INPUT_PULLUP);
    }

    pinMode(rotarySW1, INPUT_PULLUP);
    pinMode(rotarySW2, INPUT_PULLUP);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;

    encoder1.attachHalfQuad(rotaryDT1, rotaryCLK1);
    encoder2.attachHalfQuad(rotaryDT2, rotaryCLK2);

    encoder1.setFilter(1023);
    encoder2.setFilter(1023);

    encoder1.setCount(0);
    encoder2.setCount(0);

    bleKeyboard.begin();
    Shortcut_Manager::getInstance(); // Initialize
}

void loop() {
    handleSerial();
    readMatrix();
    readRotaryButton();

    if (buttonState != lastButtonState) {
        decodeKeypressed();
        Shortcut_Manager::getInstance().update(buttonState, lastButtonState);
        lastButtonState = buttonState;
    }

    rotaryCount1 = encoder1.getCount() / 2;
    rotaryCount2 = encoder2.getCount() / 2;

    if (rotaryCount1 != lastRotaryCount1 || rotaryButtonState1 != lastRotaryButtonState1) {

        if (rotaryCount1 > lastRotaryCount1) {
            bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        } else if (rotaryCount1 < lastRotaryCount1) {
            bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        }

        if (rotaryButtonState1 && rotaryButtonState1 != lastRotaryButtonState1) {
            bleKeyboard.write(KEY_MEDIA_MUTE);
            isMuted = !isMuted;
        }

        Serial.print("[ROTARY] Count: ");
        Serial.print(rotaryCount1);
        Serial.print(" | SW1: ");
        Serial.print(rotaryButtonState1 ? "PRESSED" : "OPEN");
        Serial.print(" | Mute State: ");
        Serial.println(isMuted ? "MUTED" : "UNMUTED");

        lastRotaryCount1 = rotaryCount1;
        lastRotaryButtonState1 = rotaryButtonState1;
    }
    if (rotaryCount2 != lastRotaryCount2 || rotaryButtonState2 != lastRotaryButtonState2) {

        Serial.print("[ROTARY] ");
        Serial.print("R2 Count: ");
        Serial.print(rotaryCount2);
        Serial.print(" | SW2: ");
        Serial.println(rotaryButtonState2 ? "PRESSED" : "OPEN");

        lastRotaryCount2 = rotaryCount2;
        lastRotaryButtonState2 = rotaryButtonState2;
    }

    delay(10);
}