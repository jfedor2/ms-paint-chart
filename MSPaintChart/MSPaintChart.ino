// Jacek Fedorynski <jfedor@jfedor.org>
// http://www.jfedor.org/

#include <Adafruit_LSM6DS33.h>
#include <bluefruit.h>

#define MOUSE_REPORT_ID 1
#define KEYBOARD_REPORT_ID 2

#define SAMPLES_PER_SCREEN 256

Adafruit_LSM6DS33 lsm6ds;
Adafruit_Sensor *accelerometer;

BLEDis bledis;
BLEHidGeneric blehid = BLEHidGeneric(2, 1, 0);

long sample_no = 0;

uint8_t const hid_report_descriptor[] = {
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
  HID_USAGE(HID_USAGE_DESKTOP_MOUSE),
  HID_COLLECTION(HID_COLLECTION_APPLICATION),
  HID_REPORT_ID(MOUSE_REPORT_ID)
  HID_USAGE(HID_USAGE_DESKTOP_POINTER),
  HID_COLLECTION(HID_COLLECTION_PHYSICAL),
  HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),
  HID_USAGE_MIN(1),
  HID_USAGE_MAX(3),
  HID_LOGICAL_MIN(0),
  HID_LOGICAL_MAX(1),
  /* buttons */
  HID_REPORT_COUNT(3),
  HID_REPORT_SIZE(1),
  HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
  /* padding */
  HID_REPORT_COUNT(1),
  HID_REPORT_SIZE(5),
  HID_INPUT(HID_CONSTANT),
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
  /* X, Y position [0, 32767] */
  HID_USAGE(HID_USAGE_DESKTOP_X),
  HID_USAGE(HID_USAGE_DESKTOP_Y),
  HID_LOGICAL_MIN_N(0x0000, 2),
  HID_LOGICAL_MAX_N(0x7fff, 2),
  HID_REPORT_COUNT(2),
  HID_REPORT_SIZE(16),
  HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
  HID_COLLECTION_END,
  HID_COLLECTION_END,

  TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(KEYBOARD_REPORT_ID) ),
};

void setup() {
  lsm6ds.begin_I2C();
  accelerometer = lsm6ds.getAccelerometerSensor();
  lsm6ds.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
  lsm6ds.setAccelDataRate(LSM6DS_RATE_208_HZ);

  Bluefruit.configPrphBandwidth(BANDWIDTH_HIGH);
  Bluefruit.begin();
  Bluefruit.setName("Datalogging mouse");

  // Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Feather Sense");
  bledis.begin();

  blehid.setReportMap(hid_report_descriptor, sizeof(hid_report_descriptor));
  uint16_t input_len[] = { 5, sizeof(hid_keyboard_report_t) };
  uint16_t output_len[] = { 1 };
  blehid.setReportLen(input_len, output_len, NULL);
  blehid.begin();

  Bluefruit.Periph.setConnInterval(6, 6);

  startAdv();

  Wire.setClock(400000);
}

void startAdv(void)
{
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.Advertising.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void mouse_position(int16_t x, int16_t y, uint8_t buttons) {
  uint8_t report[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
  report[0] = buttons;
  report[1] = x & 0xff;
  report[2] = (x >> 8) & 0xff;
  report[3] = y & 0xff;
  report[4] = (y >> 8) & 0xff;
  blehid.inputReport(MOUSE_REPORT_ID, report, sizeof(report));
}

void keystroke(uint8_t modifier, uint8_t keycode) {
  hid_keyboard_report_t report;
  memset(&report, 0, sizeof(report));
  report.modifier = modifier;
  report.keycode[0] = keycode;
  blehid.inputReport(KEYBOARD_REPORT_ID, &report, sizeof(report));
  memset(&report, 0, sizeof(report));
  blehid.inputReport(KEYBOARD_REPORT_ID, &report, sizeof(report));
}

void plot(float val, int chart_no) {
  int x = 0.1 * 32767 + 0.8 * 32767 * sample_no / SAMPLES_PER_SCREEN;
  x = max(0, min(32767, x));
  int y = (chart_no + 1) * 0.25 * 32767 + val * 256;
  y = max(0, min(32767, y));

  mouse_position(x, y, 0);
  mouse_position(x, y, 1);
  mouse_position(x, y, 0);
}

void loop() {
  sensors_event_t accel;
  accelerometer->getEvent(&accel);

  for (int i = 0; i < 3; i++) {
    plot(accel.acceleration.v[i], i);
  }

  sample_no++;
  if (sample_no == SAMPLES_PER_SCREEN) {
    // new document
    keystroke(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_N);
    delay(50);
    // don't save
    keystroke(0, HID_KEY_N);
    delay(50);
    sample_no = 0;
  }
}
