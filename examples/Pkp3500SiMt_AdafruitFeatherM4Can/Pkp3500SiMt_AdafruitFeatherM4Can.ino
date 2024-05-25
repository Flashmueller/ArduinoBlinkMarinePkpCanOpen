/*  Brandon Matthews
 *  Developed for Arduino UNO and MCP2515
 *  Modified for PKP-3500-SI-MT
 */

// This library depends on and requires TimerOne library, SPI library, and autowp mcp2515 library v1.03
#include <BlinkMarinePkpCanOpen.h>
#include <mcp2515.h>

#define CS_PIN          10
#define INTERRUPT_PIN   3
#define KEYPAD_BASE_ID  0x15
#define ENABLE_PASSCODE false

MCP2515   mcp2515(CS_PIN);
PkpKeypad keypad(mcp2515, INTERRUPT_PIN, KEYPAD_BASE_ID, ENABLE_PASSCODE);

unsigned long currentMillis;
unsigned long key9OnTime = 0;
bool          lastKey9State;

void setup() {
    Serial.begin(115200);
    keypad.setSerial(&Serial); // Required for the keypad library to print things out to serial

    uint8_t keypadPasscode[4] = {1, 2, 3, 4};
    keypad.setKeypadPassword(keypadPasscode);
    keypad.setKeyBrightness(70);
    keypad.setBacklight(BACKLIGHT_AMBER, 10);

    // Set Key color and blink states
    uint8_t colors1[4] = {PKP_COLOR_BLANK, PKP_COLOR_YELLOW, PKP_COLOR_BLANK,
                          PKP_COLOR_YELLOW}; // array for the 4 possible key states' respective colors
    uint8_t blinks1[4] = {PKP_COLOR_BLANK, PKP_COLOR_BLANK, PKP_COLOR_CYAN, PKP_COLOR_BLANK};

    keypad.setKeyColor(PKP_KEY_2, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_3, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_4, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_5, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_6, colors1, blinks1);
    colors1[1] = PKP_COLOR_GREEN;
    keypad.setKeyColor(PKP_KEY_7, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_8, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_9, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_10, colors1, blinks1);
    colors1[1] = PKP_COLOR_RED;
    colors1[2] = PKP_COLOR_GREEN;
    colors1[3] = PKP_COLOR_BLUE;
    keypad.setKeyColor(PKP_KEY_12, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_13, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_14, colors1, blinks1);
    keypad.setKeyColor(PKP_KEY_15, colors1, blinks1);

    keypad.setKeyMode(PKP_KEY_1, PkpKeypad::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(PKP_KEY_2, PkpKeypad::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(PKP_KEY_3, PkpKeypad::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(PKP_KEY_4, PkpKeypad::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(PKP_KEY_5, PkpKeypad::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(PKP_KEY_6, PkpKeypad::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(PKP_KEY_7, PkpKeypad::KEY_MODE_TOGGLE);
    keypad.setKeyMode(PKP_KEY_8, PkpKeypad::KEY_MODE_TOGGLE);
    keypad.setKeyMode(PKP_KEY_9, PkpKeypad::KEY_MODE_TOGGLE);
    keypad.setKeyMode(PKP_KEY_10, PkpKeypad::KEY_MODE_TOGGLE);
    keypad.setKeyMode(PKP_KEY_11, PkpKeypad::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(PKP_KEY_12, PkpKeypad::KEY_MODE_CYCLE4);
    keypad.setKeyMode(PKP_KEY_13, PkpKeypad::KEY_MODE_TOGGLE);
    keypad.setKeyMode(PKP_KEY_14, PkpKeypad::KEY_MODE_TOGGLE);
    keypad.setKeyMode(PKP_KEY_15, PkpKeypad::KEY_MODE_CYCLE4);


    uint8_t defaultStates[PKP_MAX_KEY_AMOUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    keypad.presetDefaultKeyStates(defaultStates);

    keypad.initializeEncoder(0, 16, 5);
    keypad.initializeEncoder(1, 16, 10);

    keypad.begin(CAN_1000KBPS, MCP_8MHZ); // These are MCP settings to be passed
}

//----------------------------------------------------------------------------

uint8_t  brightness    = 0;
uint32_t lastIncrement = 0;
int16_t  encoder       = 0;

void loop() {
    currentMillis = millis();

    keypad.process(); // must have this in main loop.

    if (keypad._keyState[PKP_KEY_1] == 1) {
        // do stuff
    }

    // if key 9 is pressed, turn off after 2 seconds
    if (keypad._keyState[PKP_KEY_9] == 1 && lastKey9State == 0) {
        key9OnTime = currentMillis;
    }
    lastKey9State = keypad._keyState[PKP_KEY_9];
    if (lastKey9State == 1 && (currentMillis - key9OnTime) > 2000) {
        keypad._keyState[PKP_KEY_9] = 0;
        keypad.update();
    }

    int      encoderOld = encoder;
    bool     newData    = false;
    uint16_t leds       = 0;
    encoder             += keypad.getRelativeEncoderTicks(0);
    encoder             = constrain(encoder, 0, 16);

    if (encoder != encoderOld) {
        for (int i = 0; i < encoder; i++) {
            leds |= 1 << i;
        }
        newData = 1;
    }

    if (newData && lastIncrement + 50 < currentMillis) {
        keypad.setEncoderLed(0, leds);
        lastIncrement = currentMillis;
    }
}
