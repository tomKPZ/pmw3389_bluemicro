#include <bluefruit.h>

#include "PMW3389/src/PMW3389.h"
// This is a hack to workaround the seeming inability to add this file
// to the build.
#include "PMW3389/src/PMW3389.cpp"

enum { REPORT_ID_MOUSE = 1 };

// clang-format off
uint8_t const hid_report_descriptor[] = {
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
  HID_USAGE(HID_USAGE_DESKTOP_MOUSE),
  HID_COLLECTION(HID_COLLECTION_APPLICATION),
    /* Report ID if any */
    HID_REPORT_ID(REPORT_ID_MOUSE)
    HID_USAGE(HID_USAGE_DESKTOP_POINTER),
    HID_COLLECTION(HID_COLLECTION_PHYSICAL),
      HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),
        HID_USAGE_MIN(1),
        HID_USAGE_MAX(5),
        HID_LOGICAL_MIN(0),
        HID_LOGICAL_MAX(1),
        // Left, Right, Middle, Backward, Forward buttons
        HID_REPORT_COUNT(5),
        HID_REPORT_SIZE(1),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        // 3 bit padding
        HID_REPORT_COUNT(1),
        HID_REPORT_SIZE(3),
        HID_INPUT(HID_CONSTANT),
      HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
        // X, Y position
        HID_USAGE(HID_USAGE_DESKTOP_X),
        HID_USAGE(HID_USAGE_DESKTOP_Y),
        HID_LOGICAL_MIN_N(0x8000, 2),
        HID_LOGICAL_MAX_N(0x7fff, 2),
        HID_REPORT_COUNT(2),
        HID_REPORT_SIZE(16),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_RELATIVE),
        // Verital wheel scroll
        HID_USAGE(HID_USAGE_DESKTOP_WHEEL),
        HID_LOGICAL_MIN_N(0x8000, 2),
        HID_LOGICAL_MAX_N(0x7fff, 2),
        HID_REPORT_COUNT(1),
        HID_REPORT_SIZE(16),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_RELATIVE),
      HID_USAGE_PAGE(HID_USAGE_PAGE_CONSUMER),
        // Horizontal wheel scroll
        HID_USAGE_N(HID_USAGE_CONSUMER_AC_PAN, 2),
        HID_LOGICAL_MIN_N(0x8000, 2),
        HID_LOGICAL_MAX_N(0x7fff, 2),
        HID_REPORT_COUNT(1),
        HID_REPORT_SIZE(16),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_RELATIVE),
    HID_COLLECTION_END,
  HID_COLLECTION_END
};
// clang-format on

typedef struct TU_ATTR_PACKED {
  uint8_t
      buttons; /**< buttons mask for currently pressed buttons in the mouse. */
  int16_t x;   /**< Current delta x movement of the mouse. */
  int16_t y;   /**< Current delta y movement on the mouse. */
  int16_t wheel; /**< Current delta wheel movement on the mouse. */
  int16_t pan;   // using AC Pan
} MouseReport;

class BLEHidMouse : public BLEHidGeneric {
public:
  BLEHidMouse(void) : BLEHidGeneric(1, 1, 0) {}

  virtual err_t begin(void) {
    uint16_t input_len[] = {sizeof(MouseReport)};
    uint16_t output_len[] = {1};

    setReportLen(input_len, output_len, NULL);
    enableMouse(true);
    setReportMap(hid_report_descriptor, sizeof(hid_report_descriptor));

    VERIFY_STATUS(BLEHidGeneric::begin());

    return ERROR_NONE;
  }

  bool mouseReport(MouseReport *report) {
    if (isBootMode()) {
      return false;
    } else {
      return inputReport(BLE_CONN_HANDLE_INVALID, REPORT_ID_MOUSE, report,
                         sizeof(MouseReport));
    }
  }
};

BLEDis bledis;
BLEHidMouse blehid;

PMW3389 sensor1;
PMW3389 sensor2;

void setup() {
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  pinMode(28, OUTPUT);
  digitalWrite(28, HIGH);

  pinMode(26, INPUT_PULLUP);
  pinMode(29, INPUT_PULLUP);

  Serial.begin(115200);

  init_bluetooth();

  sensor1.begin(3, 16000);
  sensor2.begin(28, 16000);
}

void init_bluetooth() {
  // Increase the queue size so that we can achieve a reasonable mouse polling
  // interval.
  Bluefruit.configPrphConn(BLE_GATT_ATT_MTU_DEFAULT, BLE_GAP_EVENT_LENGTH_MIN,
                           256, 256);
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  // HID Device can have a min connection interval of 9*1.25 = 11.25 ms
  Bluefruit.Periph.setConnInterval(9, 12);
  // Maximum power.
  Bluefruit.setTxPower(8);

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();

  // BLE HID
  blehid.begin();

  // Set up and start advertising
  startAdv();
}

void startAdv() {
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);

  // Include BLE HID service
  Bluefruit.Advertising.addService(blehid);

  // There is enough room for 'Name' in the advertising packet
  Bluefruit.Advertising.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   *
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);   // number of seconds in fast mode
  Bluefruit.Advertising.start(0); // 0 = Don't stop advertising after n seconds
}

// 22.41mm / 2 * 16000/in
const float DELTA = 7058.3;

void loop() {
  PMW3389_DATA data1 = sensor1.readBurst();
  PMW3389_DATA data2 = sensor2.readBurst();

  bool l = !digitalRead(26);
  bool r = !digitalRead(29);

  if ((data1.isOnSurface && data1.isMotion) ||
      (data2.isOnSurface && data2.isMotion)) {
    float dx = (data1.dx + data2.dx) / 2.0;
    float dy = (data1.dy + data2.dy) / 2.0;
    float x = data1.dx - dx + DELTA;
    float y = data1.dy - dy;
    float a = atan2(y, x);

    MouseReport report = {
        .buttons = (l << 0) | (r << 1),
        .x = -dx / 20,
        .y = -dy / 20,
        .wheel = a * 4096,
        .pan = 0,
    };
    blehid.mouseReport(&report);
  }

  delay(4);
}
