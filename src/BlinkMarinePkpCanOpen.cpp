
#include "BlinkMarinePkpCanOpen.h"

//********** CONSTRUCTOR **********

/**
 * @brief Constructs a Pkp object with a specific CAN ID and transmission callback.
 *
 * This constructor initializes a keypad object, setting up the CAN ID and the message transmission callback.
 * Default states for override key states and encoder top values are also initialized.
 *
 * @param canId The CAN ID to be used by the keypad.
 * @param callback The function to call for transmitting messages over the CAN bus.
 */
Pkp::Pkp(uint8_t canId, CanMsgTxCallback callback, uint16_t heartBeatInterval)
    : _canId(canId), _transmitMessage(callback), _canNodeHeartbeatInterval(heartBeatInterval) {
    for (int i = 0; i < PKP_MAX_KEY_AMOUNT; i++) {
        _overrideKeyState[i] = -1;
    }
    for (int i = 0; i < PKP_MAX_ROTARY_ENCODER_AMOUNT; i++) {
        _encoderTopValue[i] = 16;
    }
}

//********** PUBLIC METHODS **********

/**
 * @brief Applies the default key states set via presetDefaultKeyStates() to physical keys.
 *
 * This function copies the internally stored default key states to the _keyState array and
 * triggers an update to refresh the physical state of the LEDs on the keypad.
 *
 * @return A status code indicating success or the type of error encountered (e.g., communication errors).
 */
Pkp::returnState_e Pkp::applyDefaultKeyStates() {
    memcpy(_keyState, _defaultKeyState, sizeof(_keyState));
    return _update(UT_KEY_LEDS);
}

/**
 * @brief Initializes the keypad system.
 *
 * This function is responsible for performing all the initialization required for the keypad's hardware and software.
 * It must be called before the keypad can be used.
 *
 * @return A status code indicating success or the type of error encountered (e.g., not initialized).
 */
Pkp::returnState_e Pkp::begin() {
    return _initializeKeypad();
}

/**
 * @brief Retrieves the position of the specified encoder.
 *
 * This function returns the current position of the encoder specified by the encoderIndex parameter.
 * If the encoderIndex is out of range (greater than PKP_MAX_ROTARY_ENCODER_AMOUNT - 1), the function
 * returns 0.
 *
 * @param encoderIndex The index of the encoder (0 to PKP_MAX_ROTARY_ENCODER_AMOUNT - 1).
 * @return The position of the specified encoder, or 0 if the index is out of range.
 */
uint16_t Pkp::getEncoderPosition(uint8_t encoderIndex) {
    if (encoderIndex > PKP_MAX_ROTARY_ENCODER_AMOUNT - 1) {
        return 0;
    }
    return _encoderPosition[encoderIndex];
}

/**
 * @brief Checks if a key is pressed.
 *
 * This function checks if the key specified by the keyIndex parameter is currently pressed.
 * If the keyIndex is out of range (greater than PKP_MAX_KEY_AMOUNT - 1), the function
 * returns false.
 *
 * @param keyIndex The index of the key (0 to PKP_MAX_KEY_AMOUNT - 1).
 * @return true if the specified key is pressed, false otherwise or if the index is out of range.
 */
bool Pkp::getKeyPress(uint8_t keyIndex) {
    if (keyIndex > PKP_MAX_KEY_AMOUNT - 1) {
        return false;
    }
    return _keyPressed[keyIndex];
}

/**
 * @brief Retrieves the state of the specified key.
 *
 * This function returns the current state of the key specified by the keyIndex parameter.
 * If the keyIndex is out of range (greater than PKP_MAX_KEY_AMOUNT - 1), the function
 * returns 0.
 *
 * @param keyIndex The index of the key (0 to PKP_MAX_KEY_AMOUNT - 1).
 * @return The state of the specified key, or 0 if the index is out of range.
 */
uint8_t Pkp::getKeyState(uint8_t keyIndex) {
    if (keyIndex > PKP_MAX_KEY_AMOUNT - 1) {
        return 0;
    }
    return _keyState[keyIndex];
}

/**
 * @brief Retrieves the state of the specified wired input.
 *
 * This function returns the state of the wired input specified by the inputIndex parameter.
 * If the inputIndex is out of range (greater than PKP_MAX_WIRED_IN_AMOUNT - 1), the function
 * returns 0.
 *
 * @param inputIndex The index of the wired input (0 to PKP_MAX_WIRED_IN_AMOUNT - 1).
 * @return The state of the specified wired input, or 0 if the index is out of range.
 */
uint8_t Pkp::getWiredInput(uint8_t inputIndex) {
    if (inputIndex > PKP_MAX_WIRED_IN_AMOUNT - 1) {
        return 0;
    }
    return _wiredInputValue[inputIndex];
}

/**
 * @brief Checks and returns the communication status of the keypad.
 *
 * This function assesses the status of the keypad in terms of receiving messages over CAN. It is used to monitor
 * the communication health between the keypad and CAN network.
 *
 * @return The current communication status as an enum, indicating if messages have been received recently or not.
 */
Pkp::keypadCanStatus_e Pkp::getStatus() {
    return _keypadStatusWatchdog(MSG_RECEIVED_NOTHING);
}

/**
 * @brief Retrieves and resets the count of encoder ticks for a specified encoder since the last check.
 *
 * This function returns the number of ticks that have occurred for a particular encoder and then resets the tick count.
 * This is useful for applications needing to measure increments or decrements in encoder position since the last poll.
 *
 * @param encoderIndex The index of the encoder whose ticks are to be retrieved.
 * @return The number of ticks since the last retrieval. Returns 0 if an invalid encoder index is provided.
 */
int16_t Pkp::getRelativeEncoderTicks(uint8_t encoderIndex) {
    if (encoderIndex > PKP_MAX_ROTARY_ENCODER_AMOUNT - 1) {
        return 0;
    }
    int16_t ticks                       = _relativeEncoderTicks[encoderIndex];
    _relativeEncoderTicks[encoderIndex] = 0;
    return ticks;
}

/**
 * @brief Initializes a rotary encoder with specified limits and initial value.
 *
 * Configures an encoder's maximum value and its initial position. If the maximum value is set to zero, it is treated as the maximum possible value (0xFFFF).
 * This method also sends CAN frames to set the encoder's maximum and initial values.
 *
 * @param index The index of the encoder to initialize.
 * @param topValue The maximum value the encoder can count to before wrapping around.
 * @param actValue The starting position of the encoder.
 * @return A status code indicating the success of the initialization or the reason for failure (e.g., keypad not initialized).
 */
Pkp::returnState_e Pkp::initializeEncoder(uint8_t index, uint8_t topValue, uint16_t actValue) {
    index    = constrain(index, 0, PKP_MAX_ROTARY_ENCODER_AMOUNT - 1);
    topValue = constrain(topValue, 0, 0x10);

    uint16_t realTopValue    = (topValue == 0) ? 0xFFFF : topValue;
    _encoderTopValue[index]  = realTopValue;
    _encoderInitValue[index] = min(actValue, realTopValue);

    if (!_initialized) {
        return RS_KEYPAD_NOT_INITIALIZED;
    }

    // Initialize can frame for encoder top level (0 equals to 0xFFFF)
    struct can_frame txMsg = {0};
    txMsg.can_id           = CAN_TX_BASE_ID_SDO + _canId;
    txMsg.can_dlc          = 5;
    txMsg.data[0]          = 0x2F;
    txMsg.data[1]          = 0x00;
    txMsg.data[2]          = 0x20;
    txMsg.data[3]          = 0x06 + index;
    txMsg.data[4]          = _encoderTopValue[index];

    returnState_e returnValue = _transmit(txMsg);
    if (returnValue != RS_SUCCESS) {
        return returnValue;
    }

    // Initialize can frame for encoder start value
    memset(&txMsg, 0, sizeof(txMsg));
    txMsg.can_id  = CAN_TX_BASE_ID_SDO + _canId;
    txMsg.can_dlc = 6;
    txMsg.data[0] = 0x2B;
    txMsg.data[1] = 0x00;
    txMsg.data[2] = 0x20;
    txMsg.data[3] = (0 == index) ? 0x03 : 0x05;
    txMsg.data[4] = _encoderInitValue[index] & 0xFF;
    txMsg.data[5] = _encoderInitValue[index] >> 8;

    return _transmit(txMsg);
}

/**
 * @brief Sets the default states for all keys on the keypad.
 *
 * This function sets default states for keys, which are used when resetting key states to defaults.
 * It validates each state to ensure it is within allowable ranges.
 * Providing -1 for a key leafs its default state as it is.
 *
 * @param defaultStates Array of default states for each key.
 * @return A status code indicating whether the default states were set successfully or if there was an invalid state.
 */
Pkp::returnState_e Pkp::presetDefaultKeyStates(const int8_t defaultStates[PKP_MAX_KEY_AMOUNT]) {
    bool invalidKeyState = false;
    for (int i = 0; i < PKP_MAX_KEY_AMOUNT; i++) {
        if (!inLimits(defaultStates[i], -1, 3)) {
            invalidKeyState = true;
            continue;
        }
        if (defaultStates[i] == -1) {
            continue;
        }
        _defaultKeyState[i] = defaultStates[i];
    }
    if (invalidKeyState) {
        return RS_INVALID_KEY_STATE;
    }
    return RS_SUCCESS;
}

/**
 * @brief Processes incoming CAN frames and updates the keypad state accordingly.
 *
 * This function decodes the CAN messages directed to keys, encoders, and wired inputs.
 * It ensures that messages are from expected CAN IDs and updates internal states.
 *
 * @param can_id The CAN ID of the received message.
 * @param can_dlc The data length code of the CAN frame.
 * @param data The data payload of the CAN frame.
 * @return True if the message was relevant and processed, false if keypay has not been the transmitter.
 */
bool Pkp::process(const struct can_frame& rxMsg) {

    if (rxMsg.can_id == CAN_RX_BASE_ID_KEYS + _canId) {
        _decodeKeyStates(rxMsg.data);

    } else if (rxMsg.can_id == CAN_RX_BASE_ID_ENCODER_1 + _canId) {
        _decodeRotaryEncoder(rxMsg.data, 0);

    } else if (rxMsg.can_id == CAN_RX_BASE_ID_ENCODER_2 + _canId) {
        _decodeRotaryEncoder(rxMsg.data, 1);

    } else if (rxMsg.can_id == CAN_RX_BASE_ID_WIRED_IN + _canId) {
        _decodeWiredInputs(rxMsg.data);

    } else if (rxMsg.can_id == CAN_RX_BASE_ID_HEARTBEAT + _canId) {
        //nothing to do here...
    } else {
        //control reachers else clause only in case the can frame did not come from the keypad
        _keypadStatusWatchdog(MSG_RECEIVED_NOTHING);
        return false;
    }

    _keypadStatusWatchdog(MSG_RECEIVED_VALID);
    return true;
}

/**
 * @brief Sets the backlight color and brightness for the keypad.
 *
 * This method adjusts the backlight settings for the entire keypad, affecting color and intensity based on provided parameters.
 * It ensures the color and brightness are within predefined limits.
 *
 * @param color The color setting for the backlight, constrained within defined limits.
 * @param brightness The brightness level from 0 (off) to 100 (full brightness), constrained within these limits.
 * @return A status code indicating the success of setting the backlight or the reason for failure (e.g., invalid color).
 */
Pkp::returnState_e Pkp::setBacklight(int8_t color, int8_t brightness) {

    if (!inLimits(color, BACKLIGHT_DEFAULT, BACKLIGHT_YELLOWGREEN)) {
        return RS_INVALID_COLOR;
    }

    _backlightBrightness = constrain(brightness, 0, 100);
    _backlightColor      = color;

    // Initialize can frame
    struct can_frame txMsg = {0};

    txMsg.can_id  = CAN_TX_BASE_ID_KEY_BACKLIGHT + _canId;
    txMsg.can_dlc = 2;
    txMsg.data[0] = 0x3F * _backlightBrightness / 100;
    txMsg.data[1] = _backlightColor;

    return _transmit(txMsg);
}

/**
 * @brief Sets the LED states for all encoders.
 *
 * Updates the LED indicators for the encoders if their new state differs from the current state. This function triggers an update only if a change is detected.
 *
 * @param ledsEncoder Array of new LED states for each encoder.
 * @return A status code indicating the success of the operation or if no update was needed.
 */
Pkp::returnState_e Pkp::setEncoderLeds(int32_t ledsEncoder[PKP_MAX_ROTARY_ENCODER_AMOUNT]) {
    bool writeEncoderLed = 0;
    for (int i = 0; i < PKP_MAX_ROTARY_ENCODER_AMOUNT; i++) {
        if (ledsEncoder[i] >= 0 && _currentEncoderLed[i] != (uint16_t)ledsEncoder[i]) {
            _currentEncoderLed[i] = ledsEncoder[i];
            writeEncoderLed       = true;
        }
    }
    if (writeEncoderLed) {
        return _update(UT_ENCODER_LEDS);
    }
    return RS_SUCCESS;
}

/**
 * @brief Sets the brightness level for all keys on the keypad.
 *
 * This function sets a uniform brightness for all keys, sending a CAN frame to apply the setting across the keypad.
 *
 * @param brightness Brightness level from 0 (off) to 100 (full brightness).
 * @return A status code indicating the success of setting the brightness.
 */
Pkp::returnState_e Pkp::setKeyBrightness(uint8_t brightness) {
    _keyBrightness = constrain(brightness, 0, 100);

    // Initialize can frame
    struct can_frame txMsg = {0};
    txMsg.can_id           = CAN_TX_BASE_ID_SDO + _canId;
    txMsg.can_dlc          = 5;
    txMsg.data[0]          = 0x2F;
    txMsg.data[1]          = 0x03;
    txMsg.data[2]          = 0x20;
    txMsg.data[3]          = 0x01;
    txMsg.data[4]          = 0x3F * _keyBrightness / 100;

    return _transmit(txMsg);
}

/**
 * @brief Sets the color and blink color for a specific key.
 *
 * Updates the solid and blink colors for a specified key. It checks for valid key indices and color ranges before applying the updates.
 *
 * @param keyIndex Index of the key to modify.
 * @param colors Array representing the new color values for solid state.
 * @param blinkColors Array representing the new color values for blinking state.
 * @return A status code that could be success, invalid key index, or invalid color if checks fail.
 */
Pkp::returnState_e Pkp::setKeyColor(uint8_t keyIndex, const uint8_t colors[4], const uint8_t blinkColors[4]) {

    bool invalidColor = false;

    if (!inLimits(keyIndex, 0, PKP_MAX_KEY_AMOUNT - 1)) {
        return RS_INVALID_KEY_INDEX;
    }
    for (int i = 0; i < 4; i++) {
        if (!inLimits(keyIndex, 0, PKP_MAX_KEY_AMOUNT - 1)) {
            invalidColor = true;
            continue;
        }
        _keyColor[i][keyIndex]      = colors[i];
        _keyBlinkColor[i][keyIndex] = blinkColors[i];
    }

    returnState_e returnValue = RS_SUCCESS;
    if (invalidColor) {
        returnValue = RS_INVALID_COLOR;
    }

    returnValue = max(returnValue, _update(UT_KEY_LEDS));
    return returnValue;
}

/**
 * @brief Sets the operating mode for a specified key.
 *
 * This function configures how a key behaves when pressed. Supported modes include momentary, toggle, and cycle through several states.
 *
 * @param keyIndex Index of the key to configure.
 * @param keyMode Mode to set for the key, defined by keyMode_e.
 * @return A status code indicating the success of the operation or reasons for failure such as invalid key index or mode.
 */
Pkp::returnState_e Pkp::setKeyMode(uint8_t keyIndex, uint8_t keyMode) {
    if (0 == inLimits(keyIndex, 0, PKP_MAX_KEY_AMOUNT - 1)) {
        return RS_INVALID_KEY_INDEX;
    }
    if (0 == inLimits(keyMode, KEY_MODE_MOMENTARY, KEY_MODE_CYCLE4)) {
        return RS_INVALID_KEY_MODE;
    }
    _keyMode[keyIndex] = keyMode;
    return RS_SUCCESS;
}

/**
 * @brief Overrides the state of a specific key.
 *
 * Allows setting a specific state for a key regardless of its actual physical state, which can be useful for software control of key states.
 *
 * @param keyIndex Index of the key whose state is to be overridden.
 * @param overrideKeyState The state to enforce on the key; -1 to disable override.
 * @return A status code indicating the success of the operation or reasons for failure such as invalid key index or state.
 */
Pkp::returnState_e Pkp::setKeyStateOverride(uint8_t keyIndex, int8_t overrideKeyState) {
    if (keyIndex >= PKP_MAX_KEY_AMOUNT) {
        return RS_INVALID_KEY_INDEX;
    }

    if (!inLimits(overrideKeyState, -1, KEY_MODE_CYCLE4)) {
        return RS_INVALID_KEY_STATE;
    }

    _overrideKeyState[keyIndex] = overrideKeyState;
    if (overrideKeyState >= 0) {
        _keyState[keyIndex] = overrideKeyState;
    }
    return _update(UT_KEY_LEDS);
}

//********** PRIVATE METHODS **********
Pkp::returnState_e Pkp::_decodeKeyStates(const uint8_t data[8]) {

    for (int i = 0; i < PKP_MAX_KEY_AMOUNT; i++) {
        _keyPressed[i] = checkBit(data[i > 7 ? 1 : 0], i % 8);
        if (_lastKeyPressed[i] != _keyPressed[i]) {
            if (_keyMode[i] == KEY_MODE_MOMENTARY) {
                _keyState[i] = _keyPressed[i];
            } else if (_keyPressed[i] == true) {
                switch (_keyMode[i]) {
                    case KEY_MODE_TOGGLE:
                        _keyState[i] = !_keyState[i];
                        break;
                    case KEY_MODE_CYCLE3:
                        if (_keyState[i] < 2) {
                            _keyState[i]++;
                        } else {
                            _keyState[i] = 0;
                        }
                        break;
                    case KEY_MODE_CYCLE4:
                        if (_keyState[i] < 3) {
                            _keyState[i]++;
                        } else {
                            _keyState[i] = 0;
                        }
                        break;
                }
            }
            _lastKeyPressed[i] = _keyPressed[i];
        }
        if (_overrideKeyState[i] >= 0) {
            _keyState[i] = (uint8_t)_overrideKeyState[i];
        }
    }

    return _update(UT_KEY_LEDS);
}

Pkp::returnState_e Pkp::_decodeRotaryEncoder(const uint8_t data[8], uint8_t encoderIndex) {
    if (!inLimits(encoderIndex, 0, PKP_MAX_ROTARY_ENCODER_AMOUNT - 1)) {
        return RS_INVALID_ENCODER_INDEX;
    }

    int8_t ticks                        = data[0] & 0x7F;
    bool   ccWise                       = checkBit(data[0], 7);
    _relativeEncoderTicks[encoderIndex] += ccWise ? -ticks : ticks;

    _encoderPosition[encoderIndex] = data[1] | data[2] << 8;

    return RS_SUCCESS;
}

Pkp::returnState_e Pkp::_decodeWiredInputs(const uint8_t data[8]) {

    uint32_t value;
    for (int i = 0; i < PKP_MAX_WIRED_IN_AMOUNT; i++) {
        value               = data[i * 2] | (data[i * 2 + 1] << 8);
        value               = min(value, 500);
        _wiredInputValue[i] = (uint8_t)(value * 255uL / 500uL);
    }

    return RS_SUCCESS;
}

Pkp::returnState_e Pkp::_initializeKeypad() {

    // Send startup message to keypad
    struct can_frame txMsg = {0};
    txMsg.can_id           = 0x00;
    txMsg.can_dlc          = 2;
    txMsg.data[0]          = 0x01;
    txMsg.data[1]          = _canId;

    returnState_e returnValue = _transmit(txMsg, true);
    if (returnValue != RS_SUCCESS) {
        return returnValue;
    }

    if (_canNodeHeartbeatInterval > 0) {
        // Activate heartbeat production
        memset(&txMsg, 0, sizeof(txMsg));
        txMsg.can_id  = CAN_TX_BASE_ID_SDO + _canId;
        txMsg.can_dlc = 6;
        txMsg.data[0] = 0x2B;
        txMsg.data[1] = 0x17;
        txMsg.data[2] = 0x10;
        txMsg.data[3] = 0x00;
        txMsg.data[4] = _canNodeHeartbeatInterval & 0xFF;
        txMsg.data[5] = _canNodeHeartbeatInterval >> 8;
        returnValue   = _transmit(txMsg, true);
        if (returnValue != RS_SUCCESS) {
            return returnValue;
        }
    }

    _initialized = true;

    returnValue = setBacklight(_backlightColor, _backlightBrightness);
    if (returnValue != RS_SUCCESS) {
        return returnValue;
    }
    returnValue = setKeyBrightness(_keyBrightness);
    if (returnValue != RS_SUCCESS) {
        return returnValue;
    }
    for (int i = 0; i < PKP_MAX_ROTARY_ENCODER_AMOUNT; i++) {
        returnValue = initializeEncoder(i, _encoderTopValue[i], _encoderInitValue[i]);
        if (returnValue != RS_SUCCESS) {
            return returnValue;
        }
    }
    return _update(UT_ALL);
}

Pkp::keypadCanStatus_e Pkp::_keypadStatusWatchdog(const keypadStatusUpdate_e action) {
    unsigned long currentMillis = millis();

    switch (action) {
        case MSG_RECEIVED_VALID:
            _lastCanFrameTimestamp = currentMillis;
            _keypadCanStatus       = KPS_RX_WITHIN_LAST_SECOND;
            break;
        case MSG_RECEIVED_NOTHING:
        default:
            if ((currentMillis - _lastCanFrameTimestamp) >= _canNodeWatchdogTime) {
                _keypadCanStatus = KPS_NO_RX_WITHIN_LAST_SECOND;

                // set all key states back to default key states as a safety feature
                for (int i = 0; i < PKP_MAX_KEY_AMOUNT; i++) {
                    _keyState[i] = _defaultKeyState[i];
                }

                if (currentMillis - _lastReconnectTry > _canNodeReconnectInterval) {

                    _initializeKeypad();
                    _lastReconnectTry = currentMillis;
                }
            }
            break;
    }
    return _keypadCanStatus;
}

Pkp::returnState_e Pkp::_transmit(const struct can_frame& txMsg, bool initMsg) {

    if (!_initialized && !initMsg) {
        return RS_KEYPAD_NOT_INITIALIZED;
    }

    if (_transmitMessage == nullptr) {
        return RS_NULLPOINTER;
    }

    if (0 != _transmitMessage(txMsg)) {
        return RS_CAN_TX_ERROR;
    }
    return RS_SUCCESS;
}

Pkp::returnState_e Pkp::_writeEncoderLeds() {
    bool writeBlinking = false;

    for (int i = 0; i < PKP_MAX_ROTARY_ENCODER_AMOUNT; i++) {
        if (_currentEncoderBlinkLed[i] > 0) {
            writeBlinking         = true;
            _currentEncoderLed[i] = 0;
        }
    }

    struct can_frame txMsg = {0};

    if (writeBlinking) {
        txMsg.can_id  = CAN_TX_BASE_ID_SDO + _canId;
        txMsg.can_dlc = 8;
        txMsg.data[0] = 0x23;
        txMsg.data[1] = 0x02;
        txMsg.data[2] = 0x20;
        txMsg.data[3] = 0x04;
        txMsg.data[4] = _currentEncoderBlinkLed[0] & 0xFF;
        txMsg.data[5] = _currentEncoderBlinkLed[0] >> 8;
        txMsg.data[6] = _currentEncoderBlinkLed[1] & 0xFF;
        txMsg.data[7] = _currentEncoderBlinkLed[1] >> 8;
    } else {
        txMsg.can_id  = CAN_TX_BASE_ID_ENCODER_LED + _canId;
        txMsg.can_dlc = 4;
        txMsg.data[0] = _currentEncoderLed[0] & 0xFF;
        txMsg.data[1] = _currentEncoderLed[0] >> 8;
        txMsg.data[2] = _currentEncoderLed[1] & 0xFF;
        txMsg.data[3] = _currentEncoderLed[1] >> 8;
    }

    return _transmit(txMsg);
}

Pkp::returnState_e Pkp::_writeKeyLeds(bool mode) {

    mode = constrain(mode, CM_SOLID, CM_BLINK);

    // Initialize can frame
    struct can_frame txMsg = {0};
    txMsg.can_id           = mode ? CAN_TX_BASE_ID_KEY_BLINK : CAN_TX_BASE_ID_KEY_COLOR;
    txMsg.can_id           += _canId;
    txMsg.can_dlc          = 6;

    bool pColArray[PKP_MAX_KEY_AMOUNT][3] = {0};

    // setting up current colors array
    for (int m = 0; m < 4; m++) {
        for (int k = 0; k < PKP_MAX_KEY_AMOUNT; k++) {
            if (_keyState[k] == m) {
                if (mode == CM_SOLID || (_keyColor[m][k] != 0 && _keyBlinkColor[m][k] != 0)) {
                    pColArray[k][0] = (_keyColor[m][k] >> 2) & 0b1; // RED bit solid
                    pColArray[k][1] = (_keyColor[m][k] >> 1) & 0b1; // GREEN bit solid
                    pColArray[k][2] = (_keyColor[m][k]) & 0b1;      // BLUE bit solid
                    if (mode == CM_SOLID) {
                        continue;
                    }
                }
                pColArray[k][0] |= (_keyBlinkColor[m][k] >> 2) & 0b1; // RED bit blinking
                pColArray[k][1] |= (_keyBlinkColor[m][k] >> 1) & 0b1; // GREEN bit blinking
                pColArray[k][2] |= (_keyBlinkColor[m][k]) & 0b1;      // BLUE bit blinking
            }
        }
    }

    // Reorganizing colors array into proper byte format for the keypad can message
    // BYTE 0 (R8 R7 R6 R5 - R4 R3 R2 R1)
    for (int i = 0; i < PKP_MAX_KEY_AMOUNT; i++) {
        uint8_t redByte       = 0 + (i > 7);
        uint8_t greenByte     = 2 + (i > 7);
        uint8_t blueByte      = 4 + (i > 7);
        uint8_t bitIdx        = i % 8;
        txMsg.data[redByte]   |= pColArray[i][0] << bitIdx;
        txMsg.data[greenByte] |= pColArray[i][1] << bitIdx;
        txMsg.data[blueByte]  |= pColArray[i][2] << bitIdx;
    }

    return _transmit(txMsg);
}

Pkp::returnState_e Pkp::_update(updateType_e updateType) {
    returnState_e returnValue = RS_SUCCESS;

    if (updateType & UT_KEY_LEDS) {
        returnValue = max(returnValue, _writeKeyLeds(CM_SOLID));
        returnValue = max(returnValue, _writeKeyLeds(CM_BLINK));
    }

    if (updateType & UT_ENCODER_LEDS) {
        returnValue = max(returnValue, _writeEncoderLeds());
    }

    return returnValue;
}
