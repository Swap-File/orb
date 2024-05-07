#include <Arduino.h>
#include <Metro.h>
#include <Adafruit_TinyUSB.h>
#include "leds.h"
#include "imu.h"

#define NUM_EFFECTS 2

bool megan_enable = false;
bool jo_enable = true;
bool auto_change_enabled = true;


Metro fps_timer = Metro(1000);

float battery_voltage = 3.8;
bool tethered = true;
void setup() {


  //battery meter adc settings
  analogReference(AR_INTERNAL_2_4);  //Vref=2.4V
  analogReadResolution(12);          //12bits

  //enable battery measuring
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);

  //set the pin to read the battery voltage
  pinMode(PIN_VBAT, INPUT);

  //set battery charge speed to 100mA
  pinMode(22, OUTPUT);
  digitalWrite(22, LOW);

  //debug
  Serial.begin(115200);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  leds_init();
  imu_init();

  if (megan_enable && jo_enable){
    Serial.println("Don't enable both");
    delay(1000);
  }
}


void loop() {
  static int effect = 0;
  static uint32_t last_change_time = millis();

  leds_update(effect);
  int result = imu_update();
  if (result == 3)
    effect = 2;

  //prevent autochange in effect 2
  if (effect == 2)
    last_change_time = millis();

  //detect auto change timer running out
  bool auto_change = false;
  if ((millis() - last_change_time > 20000) && auto_change_enabled) {
    auto_change = true;
  }

  if (result == 1 || result == 2 || (effect != 2 && auto_change)) {
    effect++;
    last_change_time = millis();
    if (effect >= NUM_EFFECTS)
      effect = 0;
  }


  static int main_fps = 0;
  main_fps++;

if (analogRead(A0) < 2048) tethered = false;
else tethered = true;
  //battery empty indicator
  if (battery_voltage < 3.6) {  // 3.6v
    if (tethered == false) {
      //if unplugged and battery is low go to sleep to save battery
      //but you still need to flip the switch off, this only buys some time!

      enter_sleep(true);
    }
    digitalWrite(LED_RED, !(millis() >> 8 & 0x01));
  } else {
    digitalWrite(LED_RED, HIGH);
  }

  if (Serial.available()) {
    Serial.read();
    effect++;
    if (effect > NUM_EFFECTS)
      effect = 0;
    while (Serial.available())
      Serial.read();
  }


  //blink green if battery full indicator and connected to charger
  if (tethered == true && battery_voltage > 4.0) {
    digitalWrite(LED_GREEN, !(millis() >> 8 & 0x01));
  } else {
    digitalWrite(LED_GREEN, HIGH);
  }



  //read voltage measure 0.03 v low (experimentally)
  double vBat;

  vBat = ((((float)analogRead(PIN_VBAT)) * 2.4) / 4096.0) * 1510.0 / 510.0 * 1.027;  // Voltage divider from Vbat to ADC

  battery_voltage = battery_voltage * 0.95 + 0.05 * vBat;


  if (fps_timer.check()) {
    Serial.print(effect);
    Serial.print("\t");
    Serial.print(battery_voltage);
    Serial.print("\t");
    Serial.println(main_fps);
    main_fps = 0;
  }
}