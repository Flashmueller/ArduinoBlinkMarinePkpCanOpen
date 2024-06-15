/**
 * @brief   Example for PKP-3500-SI-MT on Adafruit Feather M4 Can
 * @author  Brandon Matthews, Stefan Hirschenberger
 */

#define KEYPAD_BASE_ID   0x15
#define CAN_BUS_BAUDRATE 125000

#include <Adafruit_NeoPixel.h>
#include <BlinkMarinePkpCanOpen.h>
#include <CANSAME5x.h>

//Prototype for hardware specific callback function
uint8_t transmittMessageCallBack(const struct can_frame& txMsg);

CANSAME5x         can;
Pkp               keypad(KEYPAD_BASE_ID, transmittMessageCallBack);
Adafruit_NeoPixel pixel(1, 8, NEO_GRB + NEO_KHZ800);

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial) {
        digitalWrite(LED_BUILTIN, (millis() % 500 > 250));
        delay(10);
        if (millis() > 5000) {
            break;
        }
    }
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(F("PKP-3500-SI-MT Example Sketch started."));

    // start the CAN bus at 250 kbps
    if (!can.begin(CAN_BUS_BAUDRATE)) {
        Serial.println("Starting CAN failed!");
        while (1) {
            delay(10);
        }
    }
    Serial.println("Starting CAN!");

    // register the receive callback
    can.onReceive(onReceive);

    // Set Key color and blink states
    uint8_t colors1[4] = {Pkp::KEY_COLOR_BLANK, Pkp::KEY_COLOR_GREEN, Pkp::KEY_COLOR_BLANK, Pkp::KEY_COLOR_RED};
    uint8_t blinks1[4] = {Pkp::KEY_COLOR_BLANK, Pkp::KEY_COLOR_BLANK, Pkp::KEY_COLOR_GREEN, Pkp::KEY_COLOR_BLANK};

    keypad.setKeyColor(Pkp::KEY_2, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_3, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_4, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_5, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_6, colors1, blinks1);
    colors1[1] = Pkp::KEY_COLOR_GREEN;
    keypad.setKeyColor(Pkp::KEY_7, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_8, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_9, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_10, colors1, blinks1);
    colors1[1] = Pkp::KEY_COLOR_RED;
    colors1[2] = Pkp::KEY_COLOR_GREEN;
    colors1[3] = Pkp::KEY_COLOR_BLUE;
    keypad.setKeyColor(Pkp::KEY_12, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_13, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_14, colors1, blinks1);
    keypad.setKeyColor(Pkp::KEY_15, colors1, blinks1);

    keypad.setKeyMode(Pkp::KEY_1, Pkp::KEY_MODE_TOGGLE);
    keypad.setKeyMode(Pkp::KEY_2, Pkp::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(Pkp::KEY_3, Pkp::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(Pkp::KEY_4, Pkp::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(Pkp::KEY_5, Pkp::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(Pkp::KEY_6, Pkp::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(Pkp::KEY_7, Pkp::KEY_MODE_TOGGLE);
    keypad.setKeyMode(Pkp::KEY_8, Pkp::KEY_MODE_TOGGLE);
    keypad.setKeyMode(Pkp::KEY_9, Pkp::KEY_MODE_TOGGLE);
    keypad.setKeyMode(Pkp::KEY_10, Pkp::KEY_MODE_TOGGLE);
    keypad.setKeyMode(Pkp::KEY_11, Pkp::KEY_MODE_MOMENTARY);
    keypad.setKeyMode(Pkp::KEY_12, Pkp::KEY_MODE_CYCLE4);
    keypad.setKeyMode(Pkp::KEY_13, Pkp::KEY_MODE_TOGGLE);
    keypad.setKeyMode(Pkp::KEY_14, Pkp::KEY_MODE_TOGGLE);
    keypad.setKeyMode(Pkp::KEY_15, Pkp::KEY_MODE_CYCLE4);


    int8_t defaultStates[PKP_MAX_KEY_AMOUNT] = {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0};
    keypad.presetDefaultKeyStates(defaultStates);
    keypad.applyDefaultKeyStates();
    keypad.setKeyBrightness(70);
    keypad.setBacklight(Pkp::BACKLIGHT_BLUE, 50);
    keypad.initializeEncoder(0, 16, 5);
    keypad.initializeEncoder(1, 16, 10);

    keypad.begin();
}

void loop() {

    static uint32_t key9OnTime    = 0;
    static bool     lastKey9State = 0;
    static uint32_t lastIncrement = 0;
    uint32_t        currentMillis = millis();

    if (keypad.getKeyState(Pkp::KEY_1) == 1) {
        // do stuff
    }

    // if key 9 is release, blink for two seconds and turn off afterwards
    uint8_t key9State = keypad.getKeyState(Pkp::KEY_9);
    if (key9State == 0 && lastKey9State == 1) {
        key9OnTime = currentMillis;
        keypad.setKeyStateOverride(Pkp::KEY_9, 2);
    }
    if (key9OnTime + 2000 < currentMillis) {
        keypad.setKeyStateOverride(Pkp::KEY_9, -1);
    }
    lastKey9State = key9State;


    bool    newData = false;
    int32_t leds[2] = {-1, -1};
    for (int i = 0; i < 2; i++) {
        int16_t encoder = keypad.getEncoderPosition(i);
        for (int j = 0; j < encoder; j++) {
            leds[i] |= 1 << j;
        }
    }
    keypad.setEncoderLeds(leds);

    if (keypad.getStatus() != Pkp::KPS_RX_WITHIN_LAST_SECOND) {
        pixel.setPixelColor(0, pixel.Color(255, 0, 0));
        pixel.show();
        delay(100);
        // Turn off the pixel
        pixel.setPixelColor(0, pixel.Color(0, 0, 0));
        pixel.show();
        delay(300);
    }
}

uint8_t transmittMessageCallBack(const struct can_frame& txMsg) {
    can.beginPacket(txMsg.can_id, txMsg.can_dlc);
    for (int i = 0; i < txMsg.can_dlc; i++) {
        can.write(txMsg.data[i]);
    }
    can.endPacket();
    return 0;
}

void onReceive(int packetSize) {
    struct can_frame rxMsg;
    rxMsg.can_id  = can.packetId();
    rxMsg.can_dlc = can.packetDlc();
    packetSize    = min(packetSize, sizeof(rxMsg.data) / sizeof(rxMsg.data[0]));
    for (int i = 0; i < packetSize; i++) {
        if (can.peek() == -1) {
            break;
        }
        rxMsg.data[i] = (char)can.read();
    }
    keypad.process(rxMsg);
}
