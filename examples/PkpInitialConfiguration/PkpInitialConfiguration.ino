/**
 * @brief   This example lets you configure various parameters of your Blink Marine Keypad
 * @author  Stefan Hirschenberger
 */

enum BaudRate_e : uint8_t {
    PKP_BAUDRATE_0020K = 0x07,
    PKP_BAUDRATE_0050K = 0x06,
    PKP_BAUDRATE_0125K = 0x04,
    PKP_BAUDRATE_0250K = 0x03,
    PKP_BAUDRATE_0500K = 0x02,
    PKP_BAUDRATE_1000K = 0x00
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

///////////////////
// PRECONDITIONS //
///////////////////
constexpr uint8_t ACT_CAN_ID   = 0x15;   //Actual node ID of the keypad
constexpr uint8_t ACT_BAUDRATE = 125000; //Actual can bus baud rate of the keypad

#define MCP2515 // Comment out the can interface your are not using
//#define FEATHER_M4_CAN

//Includes and defines for different CAN interfaces
#ifdef MCP2515
#include <Adafruit_MCP2515.h>
#define MCP_CS_PIN      10
#define MCP_CLOCK_SPEED 8e6 //16e6 for 16MHz
Adafruit_MCP2515 can(MCP_CS_PIN);
#elif defined(FEATHER_M4_CAN)
#include <CANSAME5x.h>
CANSAME5x         can;
#endif

////////////////////////////
// CONFIGURE THESE VALUES //
////////////////////////////
#define NEW_CAN_ID                    0x16
#define NEW_BAUD_RATE                 PKP_BAUDRATE_1000K
#define DEFAULT_BACKGROUND_COLOR      BACKLIGHT_WHITE
#define DEFAULT_BACKGROUND_BRIGHTNESS 10
#define DEFAULT_KEY_LED_BRIGHTNESS    70
#define ACTIVE_AT_STARTUP             1 //1: enable, 0: disable
#define STARTUP_LED_SHOW              1 //1: complete show (default), 0: disable, 2: fast flash
#define EVENTBASED_KEY_TRANSMISSION   1
#define PERIODIC_KEY_INTERVAL         0 //0: eventbased pdo transmission, otherwise pdo interval [ms]
#define PERIODIC_ANALOG_IN_INTERVAL   0
#define BOOTUP_SERVICE_MESSAGE        1
#define PROD_HEARTBEAT_INTERVAL       500 // 10 to 65279ms (0 to disable heartbeat production)
// #define DEMO_MODE                       1
// #define RESTORE_DEFAULTS                1

// Macro to transmit a message with an optional destination ID
void sendMessage(const uint8_t* data, uint8_t length, uint8_t destination_id = ACT_CAN_ID);
#define TRANSMIT_MSG(DATA, ...) sendMessage(DATA, sizeof(DATA), ##__VA_ARGS__)

/////////////////////////////////////////////////
// CAN FRAME PAYLOADS - NO CHANGES NEEDED HERE //
/////////////////////////////////////////////////
constexpr uint8_t setCanProtocol[] = {0x04, 0x1B, 0x80, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

#ifdef NEW_BAUD_RATE
constexpr uint8_t setBaudRate[] = {0x2F, 0x10, 0x20, 0x00, 0x02};
#endif

#ifdef NEW_CAN_ID
constexpr uint8_t setNodeId[] = {0x2F, 0x13, 0x20, 0x00, NEW_CAN_ID};
#endif

#ifdef STARTUP_LED_SHOW
constexpr uint8_t startUpShow[] = {0x2F, 0x14, 0x20, 0x00, STARTUP_LED_SHOW};
#endif

#ifdef EVENTBASED_KEY_TRANSMISSION
constexpr uint8_t EventBasedKeyTx[] = {0x2F, 0x00, 0x18, 0x02, 0xFE};
#elif defined(PERIODIC_KEY_INTERVAL)
constexpr uint8_t PeriodicKeyTx[]     = {0x2F, 0x00, 0x18, 0x05, 0x01};
constexpr uint8_t PeriodicKeyTxTime[] = {
    0x2B, 0x00, 0x18, 0x05, static_cast<uint8_t>(PERIODIC_KEY_INTERVAL & 0xFF), static_cast<uint8_t>(PERIODIC_KEY_INTERVAL >> 8)};
#endif

#ifdef PERIODIC_ANALOG_IN_INTERVAL
constexpr uint8_t PeriodicAnalogInTxTime[] = {0x2F, 0x06, 0x20, 0x00, constrain(PERIODIC_ANALOG_IN_INTERVAL / 10, 0x08, 0xC8)};
#endif

#ifdef ACTIVE_AT_STARTUP
constexpr uint8_t ActiveAtStartUp[] = {0x2F, 0x12, 0x20, 0x00, ACTIVE_AT_STARTUP};
#endif

#ifdef DEFAULT_BACKGROUND_BRIGHTNESS
constexpr uint8_t defaultBackBrightness[] = {0x2F, 0x03, 0x20, 0x06, DEFAULT_BACKGROUND_BRIGHTNESS};
#endif

#ifdef DEFAULT_BACKGROUND_COLOR
constexpr uint8_t defaultBackColor[] = {0x2F, 0x03, 0x20, 0x04, DEFAULT_BACKGROUND_COLOR};
#endif

#ifdef DEFAULT_KEY_LED_BRIGHTNESS
constexpr uint8_t defaultKeyBrightness[] = {0x2F, 0x03, 0x20, 0x05, DEFAULT_KEY_LED_BRIGHTNESS};
#endif

#ifdef BOOTUP_SERVICE_MESSAGE
constexpr uint8_t setBootUpService[] = {0x2F, 0x11, 0x20, 0x00, BOOTUP_SERVICE_MESSAGE};
#endif

#ifdef PROD_HEARTBEAT_INTERVAL
constexpr uint8_t setProducerHeartbeat[] = {0x2B, 0x17, 0x10, 0x00, PROD_HEARTBEAT_INTERVAL & 0xFF, PROD_HEARTBEAT_INTERVAL >> 8};
#endif

#ifdef DEMO_MODE
constexpr uint8_t activateDemoMode[] = {0x2F, 0x00, 0x21, 0x00, DEMO_MODE};
#endif

#ifdef RESTORE_DEFAULTS
constexpr uint8_t restoreDefaults[] = {0x23, 0x11, 0x10, 0x01, 0x6C 0x6F 0x61, 0x64};
#endif

///////////////////////////////////////
// FUNCTIONS TO CONFIGURE THE KEYPAD //
///////////////////////////////////////
void setup() {
    Serial.begin(115200);

    //Waiting for a serial connection for five seconds
    pinMode(LED_BUILTIN, OUTPUT);
    while (!Serial) {
        delay(10);
        digitalWrite(LED_BUILTIN, (millis() % 500 > 250));
        if (millis() > 5000) {
            break;
        }
    }
    digitalWrite(LED_BUILTIN, 0);

    Serial.println(F("Starting CAN interface with actual baud rate of the keypad."));

    if (!can.begin(ACT_BAUDRATE)) {
        Serial.println(F("Error initializing can interface."));
        while (1) {
            delay(10);
        }
    }
#ifdef MCP2515
    can.setClockFrequency(MCP_CLOCK_SPEED);
#endif
    Serial.println(F("Initialisation done"));

    //Send configuration messages
    configureKeypad();
}

void configureKeypad() {

    Serial.println(F("Starting configuration procedure."));
    Serial.println(F(" - Switching Keypad to CANopen in case the keypad is set to J1939"));
    TRANSMIT_MSG(setCanProtocol);
    delay(1000);

#ifdef RESTORE_DEFAULTS
    Serial.println(F(" - Restoring default settings")
    TRANSMIT_MSG(restoreDefaults);
    Serial.println(F("Stopping here..."));
    while(1){
        delay(10);}
#endif

#ifdef STARTUP_LED_SHOW
    Serial.println(F(" - Configure startup LED show"));
    TRANSMIT_MSG(startUpShow);
    delay(100);
#endif

#ifdef EVENTBASED_KEY_TRANSMISSION
    Serial.println(F(" - Configure Event-Based Key Transmission"));
    TRANSMIT_MSG(EventBasedKeyTx);
    delay(100);
#elif defined PERIODIC_KEY_INTERVAL
    Serial.println(F(" - Configure Periodic Key Transmission"));
    TRANSMIT_MSG(PeriodicKeyTx);
    delay(100);
    Serial.println(F(" - Configure Periodic Key Transmission Time"));
    TRANSMIT_MSG(PeriodicKeyTxTime);
    delay(100);
#endif

#ifdef PERIODIC_ANALOG_IN_INTERVAL
    Serial.println(F(" - Configure Periodic Analog Input Interval"));
    TRANSMIT_MSG(PeriodicAnalogInTxTime);
    delay(100);
#endif

#ifdef ACTIVE_AT_STARTUP
    Serial.println(F(" - Set Active at Startup"));
    TRANSMIT_MSG(ActiveAtStartUp);
    delay(100);
#endif

#ifdef DEFAULT_BACKGROUND_BRIGHTNESS
    Serial.println(F(" - Set Default Background Brightness"));
    TRANSMIT_MSG(defaultBackBrightness);
    delay(100);
#endif

#ifdef DEFAULT_BACKGROUND_COLOR
    Serial.println(F(" - Set Default Background Color"));
    TRANSMIT_MSG(defaultBackColor);
    delay(100);
#endif

#ifdef DEFAULT_KEY_LED_BRIGHTNESS
    Serial.println(F(" - Set Default Key Brightness"));
    TRANSMIT_MSG(defaultKeyBrightness);
    delay(100);
#endif

#ifdef BOOTUP_SERVICE_MESSAGE
    Serial.println(F(" - Set Boot Up Service Message"));
    TRANSMIT_MSG(setBootUpService);
    delay(100);
#endif

#ifdef PROD_HEARTBEAT_INTERVAL
    Serial.println(F(" - Set Producer Heartbeat Interval"));
    TRANSMIT_MSG(setProducerHeartbeat);
    delay(100);
#endif

#ifdef DEMO_MODE
    Serial.println(F(" - Activate Demo Mode"));
    TRANSMIT_MSG(activateDemoMode);
    delay(100);
#endif

#ifdef NEW_CAN_ID
    Serial.println(F(" - Set New CAN ID"));
    TRANSMIT_MSG(setNodeId);
    delay(100);
#endif

#ifdef NEW_BAUD_RATE
#ifdef NEW_CAN_ID
      uint8_t id = NEW_CAN_ID;
#else
      uint8_t id = ACT_CAN_ID;
#endif
    Serial.println(F(" - Set New Baud Rate"));
    TRANSMIT_MSG(setBaudRate, id);
    delay(100);
#endif
}

void sendMessage(const uint8_t* data, uint8_t length, uint8_t destination_id) {

    can.beginPacket(destination_id);
    for (int i = 0; i < length; i++) {
        can.write(data[i]);
    }
    can.endPacket();
}

void loop() {
    //No cyclic operation in this sketch.
}
