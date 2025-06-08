//using Adafruit ESP32 Feather board
//Junior Design - CPEG398 - UDElectric

#include <Adafruit_DRV2605.h>
#include "Arduino.h"
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "AdafruitIO_WiFi.h"
#include "AdafruitIO.h"
#include "Wifi.h"
#include <Bounce2.h>

#define EMMISIVITY 0.95
#define TA_SHIFT 8 



// Siren and Button Control
//was 12 before and 15 before

#define SIREN_PIN 12
#define BUTTON_PIN 15

//debouncing library
Bounce debouncer = Bounce();


bool currentState = LOW;             // Siren on/off toggle
unsigned long lastBeepTime = 0;
const unsigned long beepInterval = 50;  // 50ms ON/OFF beep cycle
bool sirenState = LOW;                 // Actual pin state (blinking)



unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000;



paramsMLX90640 mlx90640;
const byte MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640
static float tempValues[32 * 24];

Adafruit_DRV2605 drv; 
uint8_t effect = 1;

//play through specfic motor effects
const uint8_t effects[] = {1,14,47,70, 93, 112, 88};
uint8_t currentEffectIndex = 0;

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *temperatureFeed = io.feed("CPEG398");


void setup() {
  //Serial.begin(115200);
  
  //changed baud rate so info can be printed out quicker on the serial monitor
  Serial.begin(921600);
  //connect to adafruit io
  Serial.print("Connecting to adafruit io");
  io.connect();

  while(io.status() < AIO_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.println();
  Serial.println("Adafruit IO connected");

  Wire.begin();
  //Wire.setClock(400000); //400kHz
  Wire.setClock(1000000); // 1MHz
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0) {
    Serial.println("MLX90640 not detected at default I2C address. Starting scan the device addr...");
    Device_Scan();
//    while(1);
  }
  else {
    Serial.println("MLX90640 online!");
  }


  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0) Serial.println("Failed to load system parameters");
  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0) Serial.println("Parameter extraction failed");
  MLX90640_SetRefreshRate(MLX90640_address, 0x05); 
  Wire.setClock(800000);

  //added haptic motor initialization
  if (!drv.begin()) {
    Serial.println("Could not find DRV2605");
    while (1) delay(10);
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  //siren/button setup
  pinMode(SIREN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(25);  // debounce interval in milliseconds
  digitalWrite(SIREN_PIN, LOW);
}

void loop(void) {
  io.run();
  handleSirenButton();

  if (millis() - lastReadTime >= readInterval) {
    readTempValues();
    lastReadTime = millis();
  }
}



/*void loop(void) {
  io.run();
  handleSirenButton();
  readTempValues();
  delay(2000);
}*/

void readTempValues() {
  io.run();
  for (byte x = 0 ; x < 2 ; x++) 
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0)
    {
      Serial.print("GetFrame Error: ");
      Serial.println(status);
    }

    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

    float tr = Ta - TA_SHIFT; 

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, EMMISIVITY, tr, tempValues);
  }
  Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera===============================");
  for (int i = 0; i < 768; i++) {
    if (((i % 32) == 0) && (i != 0)) {
      Serial.println(" ");
    }
    //Serial.print((int)tempValues[i]);
    //Serial.print(" ");

    //this prints it line by line instead of a matrix so python script can read it
    Serial.println((int)tempValues[i]);
  }
  Serial.println("\r\n===========================WaveShare MLX90640 Thermal Camera===============================");

  //added threshold for the haptic motor to interact with camera

  //refers to the center temp of the camera
  float alertThreshold = 35.0;
  bool thresholdExceeded = false;

  for (int i = 0; i < 768; i++){
    if (tempValues[i] > alertThreshold) {
      thresholdExceeded = true;
      break;
    }
  }

  if (thresholdExceeded) {
    Serial.println("Threshold has been exceeded! Vibrating Motor.");
    triggerVibration();
  }

  float centerTemp = tempValues[(32 * 12) + 16];
  Serial.print("Sending center temp to Adafruit IO: ");
  Serial.println(centerTemp);

  temperatureFeed->save(centerTemp);

  
}



void triggerVibration() {

  uint8_t effect = effects[currentEffectIndex];

  Serial.print("Triggering Effect #"); Serial.println(effect);
  Serial.println(effect);

   // set the effect to play
  drv.setWaveform(0, effect);  // play effect 
  drv.setWaveform(1, 0);       // end waveform
    // play the effect!
  drv.go();

  //move to next effect
  // will buzz faster
  delay(100);

  currentEffectIndex++;
  if (currentEffectIndex >= sizeof(effects)) {
    currentEffectIndex = 0;
  }
}

void handleSirenButton() {

  debouncer.update();  // Update button state

  if (debouncer.fell()) {  // Button was just pressed (went from HIGH to LOW)
    currentState = !currentState;
    Serial.println(currentState ? "Siren ON" : "Siren OFF");
  }

  // Siren beeping logic
  if (currentState == HIGH) {
    if (millis() - lastBeepTime >= beepInterval) {
      sirenState = !sirenState;
      digitalWrite(SIREN_PIN, sirenState);
      lastBeepTime = millis();
    }
  } else {
    digitalWrite(SIREN_PIN, LOW);
  }
}


void Device_Scan() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found");
  else
    Serial.println("done");
}
