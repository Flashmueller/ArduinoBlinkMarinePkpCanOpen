/*
 * PKP Library for controlling Blink Marine KeyPads
 *
 * Initially Created by MBMatthews, December 2021 for PKP-2600
 * Ported for useage with PKP-3500-SI-MT by Flashmueller, 2024
 *
 * Functions and algorithms for status handling implemented with reference to
 * https://github.com/designer2k2/EMUcan
 *
 * spell-checker: enableCompoundWords
 */

#ifndef BLINK_MARINE_CAN_OPEN_KEYPAD
#define BLINK_MARINE_CAN_OPEN_KEYPAD

#include <Arduino.h>
#include <can.h>

//Inline functions as alternative to pre-processor macros
inline bool inLimits(int32_t value, int32_t low, int32_t high) {
    return value >= low && value <= high;
}

inline bool checkBit(int32_t value, uint8_t pos) {
    return (value & (1 << pos)) != 0;
}

constexpr size_t PKP_MAX_KEY_AMOUNT            = 15;
constexpr size_t PKP_MAX_WIRED_IN_AMOUNT       = 4;
constexpr size_t PKP_MAX_ROTARY_ENCODER_AMOUNT = 2;

//Definition for message transmitting callback function(pointer)
typedef uint8_t (*CanMsgTxCallback)(const struct can_frame& txMsg);

//Class definitions
class Pkp {
  public:
    // ------ Public Type Definitions ------
    enum keypadCanStatus_e : int8_t {
        KPS_FRESH                    = -1,
        KPS_RX_WITHIN_LAST_SECOND    = 0,
        KPS_NO_RX_WITHIN_LAST_SECOND = 1
    };

    enum keyMode_e : uint8_t {
        KEY_MODE_MOMENTARY = 0,
        KEY_MODE_TOGGLE    = 1,
        KEY_MODE_CYCLE3    = 2,
        KEY_MODE_CYCLE4    = 3
    };

    enum keyColor_e : uint8_t { /*0bRGB bitfield*/
        KEY_COLOR_BLANK  = 0b000,
        KEY_COLOR_RED    = 0b100,
        KEY_COLOR_GREEN  = 0b010,
        KEY_COLOR_BLUE   = 0b001,
        KEY_COLOR_AMBER  = 0b110,
        KEY_COLOR_CYAN   = 0b011,
        KEY_COLOR_VIOLET = 0b101,
        KEY_COLOR_WHITE  = 0b111
    };

    enum keyBacklight_e : uint8_t {
        BACKLIGHT_DEFAULT     = 0x00,
        BACKLIGHT_RED         = 0x01,
        BACKLIGHT_GREEN       = 0x02,
        BACKLIGHT_BLUE        = 0x03,
        BACKLIGHT_YELLOW      = 0x04,
        BACKLIGHT_CYAN        = 0x05,
        BACKLIGHT_VIOLET      = 0x06,
        BACKLIGHT_WHITE       = 0x07,
        BACKLIGHT_AMBER       = 0x08,
        BACKLIGHT_YELLOWGREEN = 0x09
    };

    enum keyIndex_e : uint8_t {
        KEY_1  = 0,
        KEY_2  = 1,
        KEY_3  = 2,
        KEY_4  = 3,
        KEY_5  = 4,
        KEY_6  = 5,
        KEY_7  = 6,
        KEY_8  = 7,
        KEY_9  = 8,
        KEY_10 = 9,
        KEY_11 = 10,
        KEY_12 = 11,
        KEY_13 = 12,
        KEY_14 = 13,
        KEY_15 = 14
    };

    enum class encoderIndex_e : uint8_t {
        ENCODER_1 = 0,
        ENCODER_2 = 1
    };

    enum returnState_e {
        RS_SUCCESS,
        RS_KEYPAD_NOT_INITIALIZED,
        RS_INVALID_KEY_INDEX,
        RS_INVALID_ENCODER_INDEX,
        RS_INVALID_KEY_STATE,
        RS_INVALID_KEY_MODE,
        RS_INVALID_COLOR,
        RS_CAN_TX_ERROR,
        RS_NULLPOINTER
    };

    enum updateType_e {
        UT_KEY_LEDS     = 0b01,
        UT_ENCODER_LEDS = 0b10,
        UT_ALL          = UT_KEY_LEDS | UT_ENCODER_LEDS
    };

    // ------ Public Functions ------
    Pkp(uint8_t canId, CanMsgTxCallback callback, uint16_t heartBeatInterval = 500);
    returnState_e     applyDefaultKeyStates();
    returnState_e     begin();
    uint16_t          getEncoderPosition(uint8_t encoderIndex);
    bool              getKeyPress(uint8_t keyIndex);
    uint8_t           getKeyState(uint8_t keyIndex);
    uint8_t           getWiredInput(uint8_t inputIndex);
    int16_t           getRelativeEncoderTicks(uint8_t encoderIndex);
    keypadCanStatus_e getStatus();
    returnState_e     initializeEncoder(uint8_t encoderIndex, uint8_t topValue, uint16_t actValue);
    returnState_e     presetDefaultKeyStates(const int8_t defaultStates[PKP_MAX_KEY_AMOUNT]);
    bool              process(const struct can_frame& rxMsg);
    returnState_e     setBacklight(int8_t color, int8_t brightness);
    returnState_e     setEncoderLeds(int32_t ledsEncoder[PKP_MAX_ROTARY_ENCODER_AMOUNT]);
    returnState_e     setKeyBrightness(uint8_t brightness);
    returnState_e     setKeyColor(uint8_t keyIndex, const uint8_t colors[4], const uint8_t blinkColors[4]);
    returnState_e     setKeyMode(uint8_t keyIndex, uint8_t keyMode);
    returnState_e     setKeyStateOverride(uint8_t keyIndex, int8_t _keyState);


  private:
    // ------ Private Type Definitions  ------
    enum colorMode_e : uint8_t {
        CM_SOLID = 0,
        CM_BLINK = 1
    };

    enum keypadStatusUpdate_e : uint8_t {
        MSG_RECEIVED_VALID   = 0,
        MSG_RECEIVED_NOTHING = 1
    };

    // ------ Private Constants ------
    static constexpr uint16_t CAN_RX_BASE_ID_ENCODER_1     = 0x280;
    static constexpr uint16_t CAN_RX_BASE_ID_ENCODER_2     = 0x380;
    static constexpr uint16_t CAN_RX_BASE_ID_HEARTBEAT     = 0x700;
    static constexpr uint16_t CAN_RX_BASE_ID_KEYS          = 0x180;
    static constexpr uint16_t CAN_RX_BASE_ID_WIRED_IN      = 0x480;
    static constexpr uint16_t CAN_TX_BASE_ID_ENCODER_LED   = 0x400;
    static constexpr uint16_t CAN_TX_BASE_ID_KEY_BACKLIGHT = 0x500;
    static constexpr uint16_t CAN_TX_BASE_ID_KEY_BLINK     = 0x300;
    static constexpr uint16_t CAN_TX_BASE_ID_KEY_COLOR     = 0x200;
    static constexpr uint16_t CAN_TX_BASE_ID_SDO           = 0x600;

    // ------ Private Variables ------
    uint16_t          _canNodeHeartbeatInterval                              = 0;
    uint16_t          _canNodeReconnectInterval                              = 2000;
    uint16_t          _canNodeWatchdogTime                                   = 1200;
    uint8_t           _backlightBrightness                                   = 10;
    uint8_t           _backlightColor                                        = BACKLIGHT_AMBER;
    uint8_t           _canId                                                 = 0x15;
    uint16_t          _currentEncoderBlinkLed[PKP_MAX_ROTARY_ENCODER_AMOUNT] = {0};
    uint16_t          _currentEncoderLed[PKP_MAX_ROTARY_ENCODER_AMOUNT]      = {0};
    uint8_t           _defaultKeyState[PKP_MAX_KEY_AMOUNT]                   = {0};
    uint16_t          _encoderInitValue[PKP_MAX_ROTARY_ENCODER_AMOUNT]       = {0};
    uint16_t          _encoderPosition[PKP_MAX_ROTARY_ENCODER_AMOUNT]        = {0};
    uint8_t           _encoderTopValue[PKP_MAX_ROTARY_ENCODER_AMOUNT]        = {0};
    bool              _initialized                                           = false;
    uint8_t           _keyBlinkColor[4][PKP_MAX_KEY_AMOUNT]                  = {0};
    uint8_t           _keyBrightness                                         = 50;
    uint8_t           _keyColor[4][PKP_MAX_KEY_AMOUNT]                       = {0};
    bool              _keyPressed[PKP_MAX_KEY_AMOUNT]                        = {0};
    uint8_t           _keyState[PKP_MAX_KEY_AMOUNT]                          = {0};
    keypadCanStatus_e _keypadCanStatus                                       = KPS_FRESH;
    uint8_t           _keyMode[PKP_MAX_KEY_AMOUNT]                           = {0};
    bool              _lastKeyPressed[PKP_MAX_KEY_AMOUNT]                    = {0};
    uint32_t          _lastCanFrameTimestamp                                 = 0;
    uint32_t          _lastReconnectTry                                      = 0;
    int8_t            _overrideKeyState[PKP_MAX_KEY_AMOUNT]                  = {0};
    int8_t            _relativeEncoderTicks[PKP_MAX_ROTARY_ENCODER_AMOUNT]   = {0};
    uint8_t           _wiredInputValue[PKP_MAX_WIRED_IN_AMOUNT]              = {0};
    CanMsgTxCallback  _transmitMessage;


    // ------ Private Functions ------
    returnState_e     _decodeKeyStates(const uint8_t data[8]);
    returnState_e     _decodeRotaryEncoder(const uint8_t data[8], uint8_t encoderIndex);
    returnState_e     _decodeWiredInputs(const uint8_t data[8]);
    returnState_e     _initializeKeypad();
    keypadCanStatus_e _keypadStatusWatchdog(const keypadStatusUpdate_e action);
    returnState_e     _transmit(const struct can_frame& txMsg, bool initMsg = false);
    returnState_e     _writeEncoderLeds();
    returnState_e     _writeKeyLeds(bool mode);
    returnState_e     _update(updateType_e updateType = UT_ALL);
};

#endif // BLINK_MARINE_CAN_OPEN_KEYPAD
