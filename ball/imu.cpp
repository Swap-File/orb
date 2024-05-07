#include <Arduino.h>
#include <Adafruit_SPIFlash.h>

#include "LSM6DS3.h"
#include "leds.h"

extern bool megan_enable;
extern bool jo_enable;

#define TAP_TIMEOUT 400
#define TAP_SPACING 50

Adafruit_FlashTransport_QSPI flashTransport;

static LSM6DS3 myIMU(I2C_MODE, 0x6A);

volatile static uint32_t tap_time_previous = 0;
volatile static uint8_t tap_interrupt_counter = 0;

void QSPIF_sleep(void) {
  flashTransport.begin();
  flashTransport.runCommand(0xB9);
  flashTransport.end();
}


void setupWakeUpInterrupt() {
  // Start with LSM6DS3 in disabled to save power

  detachInterrupt(digitalPinToInterrupt(PIN_LSM6DS3TR_C_INT1));

  myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, 0b00100000);  //* Acc =26 Hz (low power)// Turn on the accelerometer
                                                               // ODR_XL = 26 Hz, FS_XL = 2g
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL6_G, 0x10);         // High-performance operating mode disabled for accelerometer
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_WAKE_UP_THS, 0x80);     // Single and double tap
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, 0x08);         // Double-tap interrupt driven to INT1 pin

  // Set up the sense mechanism to generate the DETECT signal to wake from system_off
  // No need to attach a handler, if just waking with the GPIO input.
  pinMode(PIN_LSM6DS3TR_C_INT1, INPUT_PULLDOWN_SENSE);

  return;
}

void enter_sleep(bool blink) {
  // Flash red before system_off sleep
  if (blink) {
    leds_red();
    delay(20);
  }

  leds_clear();
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_RED, HIGH);

  // Setup up double tap interrupt to wake back up
  setupWakeUpInterrupt();

  // Execution should not go beyond this
  NRF_POWER->SYSTEMOFF = 1;
}

static void tap_isr() {

  uint32_t tap_time = millis();
  if (tap_time - tap_time_previous < TAP_SPACING) {
    //add vibe check
    return;
  }
  tap_interrupt_counter++;
  tap_time_previous = tap_time;
}

int imu_update() {

  if (tap_interrupt_counter > 0 && millis() - tap_time_previous > TAP_TIMEOUT) {
    Serial.print("Taps: ");
    Serial.println(tap_interrupt_counter);
    //handle tap here
    if (tap_interrupt_counter == 4) {
      enter_sleep(false);
    }
    int temp  = tap_interrupt_counter;
    tap_interrupt_counter = 0;
    return temp;
  }
  return 0;
}

void imu_init(void) {

  QSPIF_sleep();


  if (myIMU.begin() != 0) {
    Serial.println("myIMU error");

  } else {
    Serial.println("myIMU OK!");
  }

  pinMode(PIN_LSM6DS3TR_C_INT1, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_LSM6DS3TR_C_INT1), tap_isr, RISING);


  myIMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, 0x60);     //* Acc = 416Hz (High-Performance mode)// Turn on the accelerometer
                                                            // ODR_XL = 416 Hz, FS_XL = 2g
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x8E);     // INTERRUPTS_ENABLE, SLOPE_FDS// Enable interrupts and tap detection on X, Y, Z-axis
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_THS_6D, 0x85);   // Set tap threshold 8C
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_INT_DUR2, 0x7F);     // Set Duration, Quiet and Shock time windows 7F
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_WAKE_UP_THS, 0x00);  // Single tap only
  myIMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, 0b01000000);
  //single tap route to int
}
