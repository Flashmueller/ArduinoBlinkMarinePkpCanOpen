// This sketch provides a basic configuration for your keyboard without using the actual classes from the library

#define USE_MCP2515 // Comment this line to use Arduino CAN (Feather M4 CAN)

#ifdef USE_MCP2515
#include <SPI.h>
#include <mcp2515.h>
#define CS_PIN_MCP      10
#define MCP_CLOCK_SPEED MCP_8MHZ
MCP2515 mcp2515(CS_PIN_MCP, MCP_CLOCK_SPEED);
#else
#include <Adafruit_CAN.h>
Adafruit_CAN can;
#endif

enum BaudRate_e : uint8_t {
    PKP_BAUDRATE_0020k = 0x07,
    PKP_BAUDRATE_0050k = 0x06,
    PKP_BAUDRATE_0125k = 0x04,
    PKP_BAUDRATE_0250k = 0x03,
    PKP_BAUDRATE_0500k = 0x02,
    PKP_BAUDRATE_1000k = 0x00
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

////////////////////////////
// CONFIGURE THESE VALUES //
////////////////////////////
constexpr uint8_t  ACT_CAN_ID                    = 0x15;
constexpr uint8_t  NEW_CAN_ID                    = 0x16;
constexpr uint8_t  NEW_BAUD_RATE                 = PKP_BAUDRATE_1000k;
constexpr uint8_t  ACTIVE_AT_STARTUP             = 1; //1: enable, 0: disable
constexpr uint8_t  STARTUP_LED_SHOW              = 1; //1: complete show (default), 0: disable, 2: fast flash
constexpr uint8_t  PERIODIC_KEY_INTERVAL         = 0; //0: eventbased pdo transmission, otherwise pdo interval [ms]
constexpr uint8_t  PERIODIC_AI_INTERVAL          = 0;
constexpr uint8_t  DEFAULT_BACKGROUND_COLOR      = BACKLIGHT_WHITE;
constexpr uint8_t  DEFAULT_BACKGROUND_BRIGHTNESS = 10;
constexpr uint8_t  DEFAULT_KEY_LED_BRIGHTNESS    = 70;
constexpr uint8_t  BOOTUP_SERVICE_MESAGE         = 1;
constexpr uint16_t PROD_HEARTBEAT_INTERVAL       = 500; // 10 to 65279ms (0 to disable heartbeat production)
constexpr uint8_t  DEMO_MODE                     = 0;
constexpr uint8_t  RESTORE_DEFAULTS              = 0;

/////////////////////////////////////////////////
// CAN FRAME PAYLOADS - NO CHANGES NEEDED HERE //
/////////////////////////////////////////////////
constexpr uint8_t setCanProtocol[]         = {0x04, 0x1B, 0x80, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr uint8_t setBaudRate[]            = {0x2F, 0x10, 0x20, 0x00, 0x02};
constexpr uint8_t setNodeId[]              = {0x2F, 0x13, 0x20, 0x00, NEW_CAN_ID};
constexpr uint8_t startUpShow[]            = {0x2F, 0x14, 0x20, 0x00, STARTUP_LED_SHOW};
constexpr uint8_t EventBasedKeyTx[]        = {0x2F, 0x00, 0x18, 0x02, 0xFE};
constexpr uint8_t PeriodicKeyTx[]          = {0x2F, 0x00, 0x18, 0x05, 0x01};
constexpr uint8_t PeriodicKeyTxTime[]      = {0x2B, 0x00, 0x18, 0x05, PERIODIC_TX_INTERVAL & 0xFF, PERIODIC_TX_INTERVAL >> 8};
constexpr uint8_t PeriodicAnalogInTxTime[] = {0x2F, 0x06, 0x20, 0x00, constrain(PERIODIC_AI_INTERVAL / 10, 0x08, 0xC8)};
constexpr uint8_t ActiveAtStartUp[]        = {0x2F, 0x12, 0x20, 0x00, ACTIVE_AT_STARTUP};
constexpr uint8_t defaultBackBrightness[]  = {0x2F, 0x03, 0x20, 0x06, DEFAULT_BACKGROUND_BRIGHTNESS};
constexpr uint8_t defaultBackColor[]       = {0x2F, 0x03, 0x20, 0x04, DEFAULT_BACKGROUND_COLOR};
constexpr uint8_t defaultKeyBrightness[]   = {0x2F, 0x03, 0x20, 0x05, DEFAULT_KEY_LED_BRIGHTNESS};
constexpr uint8_t setBootUpService[]       = {0x2F, 0x11, 0x20, 0x00, BOOTUP_SERVICE_MESAGE};
constexpr uint8_t setProducerHeartbeat[]   = {0x2B, 0x17, 0x10, 0x00, PROD_HEARTBEAT_INTERVAL & 0xFF, PROD_HEARTBEAT_INTERVAL >> 8};
constexpr uint8_t activateDemoMode[]       = {0x2F, 0x00, 0x21, 0x00, DEMO_MODE};
constexpr uint8_t restoreDefaults[]        = {0x23, 0x11, 0x10, 0x01, 0x6C 0x6F 0x61, 0x64};
constexpr uint8_t resetKeypad[]            = {0x81, NEW_CAN_ID}

///////////////////////////////////////
// FUNCTIONS TO CONFIGURE THE KEYPAD //
///////////////////////////////////////
void setup() {
    Serial.begin(115200);
#ifdef USE_MCP2515
    setupMCP2515();
#else
    setupArduinoCAN();
#endif


    void configureKeypad() {
        TRANSMIT_MSG(setCanProtocol);
        TRANSMIT_MSG(setBaudRate);
        TRANSMIT_MSG(setNodeId);
        TRANSMIT_MSG(startUpShow);
        TRANSMIT_MSG(EventBasedKeyTx);
        TRANSMIT_MSG(PeriodicKeyTx);
        TRANSMIT_MSG(PeriodicKeyTxTime);
        TRANSMIT_MSG(PeriodicAnalogInTxTime);
        TRANSMIT_MSG(ActiveAtStartUp);
        TRANSMIT_MSG(defaultBackBrightness);
        TRANSMIT_MSG(defaultBackColor);
        TRANSMIT_MSG(defaultKeyBrightness);
        TRANSMIT_MSG(setBootUpService);
        TRANSMIT_MSG(setProducerHeartbeat);
        TRANSMIT_MSG(activateDemoMode);
        TRANSMIT_MSG(restoreDefaults);
        TRANSMIT_MSG(resetKeypad);
    }
}

void loop() {
    // put your main code here, to run repeatedly:
}

#ifdef USE_MCP2515
void setupMCP2515() {
    while (mcp2515.reset() != MCP2515::ERROR_OK) {
        Serial.println("MCP reset failed. Retrying...");
        delay(100);
    }
    Serial.println("MCP OK");
    mcp2515.setBitrate(mcpBaudrate, mcpClockSpeed);
    mcp2515.setNormalMode();
}

#else
void setupArduinoCAN() {
    if (!can.begin(CAN_1000KBPS)) {
        Serial.println("CAN initialization failed");
        while (1) {
            ;
        }
    }
    Serial.println("CAN initialized");
}
#endif


void sendMessage(const uint8_t* data, uint8_t length) {
#ifdef USE_MCP2515
    mcp2515.sendMessage(&msg);
#else
    can.send(msg);
#endif
}
