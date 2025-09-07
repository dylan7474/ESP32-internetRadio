/*
iradio V6.17
  Need to add status messages for firmware update and better error trapping/recovery
  Need to work out how to display menu again when returning from something that has changed the screen
  All around line 1119

  Keeps failing update with "Error fetching version information" whcih is line 1173
  http.begin(firmware_ver, root_ca); - so failing to get the version file which is the first operation
  Check if the share string has changed!
  https://drive.google.com/file/d/1AOl_bVifVcUjrKCpxU8K8zBjT-1ZYhaX/view?usp=drive_link
  Seems to be valid!



iradio V6.16
  switch updates to google drive - ALL WORKING BUT TAKES A LONG TIME!
  start with version
  //https://drive.google.com/file/d/1OC_veUhF0Xkz2dCfr45-ktKgOrjJE1H-/view?usp=drive_link
  http.begin("https://drive.google.com/uc?export=download&id=1OC_veUhF0Xkz2dCfr45-ktKgOrjJE1H-", root_ca);
  

  version6.txt
  //https://drive.google.com/file/d/1AOl_bVifVcUjrKCpxU8K8zBjT-1ZYhaX/view?usp=sharing
  http.begin("https://drive.google.com/uc?export=download&id=1AOl_bVifVcUjrKCpxU8K8zBjT-1ZYhaX", root_ca);
  
  bin file location (testing only for v6_16)
  https://drive.google.com/file/d/1Ac__NNNlRxqGkfTzr1CDCMyB8uwZp0_k/view?usp=sharing
  https://drive.google.com/uc?export=download&id=1Ac__NNNlRxqGkfTzr1CDCMyB8uwZp0_k


  pointer.txt
  https://drive.google.com/file/d/1BfadzeNY0jjCLJAYWaT9UdPfxzt1CUFi/view?usp=sharing
  https://drive.google.com/uc?export=download&id=1BfadzeNY0jjCLJAYWaT9UdPfxzt1CUFi

  iradio_v6_15.ino.bin on google drive
  https://drive.google.com/file/d/1Ac__NNNlRxqGkfTzr1CDCMyB8uwZp0_k/view?usp=drive_link
  https://drive.google.com/uc?export=download&id=1Ac__NNNlRxqGkfTzr1CDCMyB8uwZp0_k

iradio V6.15
  This is a test version to see if the update works
  Next thing will be to change to google drive

iradio V6.14
  Adding code for updating firmware from website
  take from iradio V5.03
  const char* firmware_url = "http://192.168.50.3/firmware/iradio/iradio_v6.00.ino.bin"; // URL to the firmware binary
  Need to fix thge code that check for 123 in the EPROM - prob works but need to add the write once channels loaded
  Should have been written to EEPROM 0 , have i used this for something else?
line 595 - is a delay - maybe this is needed for the eeprom(0) read - test it!
Tho worked fine on the other radios

Next need to upload code to the website and test

Need to allow mutiple versions of the firmware - maybe make menu 4 a version selector

iradio V6.13
  Modified channel file to use short headings and use less EERPOM space
    Station -> S
    StationInfoEnable -> I
    URL ->

    code 123 loop!

*/

#include <Arduino.h>
#include <SimpleRotary.h>     //https://github.com/mprograms/SimpleRotary
#include <Adafruit_GFX.h>     //https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SH110X.h>  //https://github.com/adafruit/Adafruit_SSD1306
#include <WiFiManager.h>      //https://github.com/tzapu/WiFiManager
#include <WiFi.h>
#include <Audio.h>        //https://github.com/schreibfaul1/ESP32-audioI2S
#include <ArduinoJson.h>  //https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>   //
#include <StreamUtils.h>
#include <ArduinoOTA.h>
#include <time.h>

#define i2c_Address 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define OLED_RESET -1
#define WIRE Wire
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char *product = "iradio";
const char *vers = "6.17";
const char *updated = "01/01/25";
const char *firmware_ver = "https://drive.google.com/uc?export=download&id=1AOl_bVifVcUjrKCpxU8K8zBjT-1ZYhaX";
char *firmware_url;
const char *firmware_ptr = "https://drive.google.com/uc?export=download&id=1BfadzeNY0jjCLJAYWaT9UdPfxzt1CUFi";
String firmwareURL;
String currentVersion = "6.17";  // Replace with your current firmware version

// Pin A, Pin B, Button Pin
SimpleRotary VolumeSelector(33, 4, 23);   //volume - reversed pins for V3 board - no idea why changed! - And swapped back for case version Very Odd
SimpleRotary ChannelSelector(25, 32, 2);  //channel

// Define pins for DAC
#define MAX98357A_I2S_DOUT 27
#define MAX98357A_I2S_BCLK 17
#define MAX98357A_I2S_LRC 16

// Inialise audio instance
Audio audio;

//Initialsise sound control variables
int volume = 4;
int eqBass = 3;
int eqMid = 0;
int eqTreb = 3;

float voltage = 0;
float batteryVoltage = 3.7;
int prevReading = 0;
int smoothingFactor = 200;  // adjust this value for your application

String StationInfo = "Waiting for connection";

byte VolumeChange, VolumePush, VolumePushLong, ChannelChange, ChannelPush, ChannelPushLong;

unsigned long Timer;

int Daymode = 0;
String StationNames[40];
String StationInfoEnable[40];
String StationURLS[40];
int eepromstatus = 0;
int channel = 0;
int currentChannel = 0;
int channelCount = 0;
int blankerDelay;
int sleepDelay;
int channelTime;
boolean switchChannel = false;
int ChannelPlayTimer;
boolean ChannelPlayComplete = true;
int ChannelPlayRetryCount = 0;
int LowPowerScore = 0;  //use this to determine if it's really low power

boolean Core0PlayChannel = false;  //use this to instruct core0 to play a new channel
boolean Core0PokeChannel = false;  //use this to instruct core0 to restart a failed channel

unsigned long ScreenTime = millis();
bool ScreenBlank = false;

boolean enableaudio = true;

// Constants for debouncing and throttling
const unsigned long DEBOUNCE_DELAY = 20;   // Debounce delay in milliseconds
const unsigned long THROTTLE_DELAY = 500;  // Throttle delay in milliseconds

unsigned long lastDebounceTime = 0;
unsigned long lastThrottleTime = 0;

//WiFiManager wm;

int x = 0;  // Start position for scrolling (Core2)

const char *root_ca =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n"
  "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n"
  "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n"
  "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n"
  "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n"
  "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n"
  "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n"
  "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n"
  "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n"
  "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n"
  "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n"
  "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n"
  "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n"
  "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n"
  "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n"
  "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n"
  "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n"
  "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n"
  "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
  "-----END CERTIFICATE-----\n";

TaskHandle_t task1Handle;
TaskHandle_t task2Handle;

// Task 1 code
void task1(void *parameter) {
  while (enableaudio) {
    audio.loop();
    delay(1);  //It looks like having nothing here will cause the application to crash - remove if audio patchy - maybe try a smaller value too

    if (Core0PlayChannel == true) {
      Core0PlayChannel = false;
      audio.stopSong();
      delay(100);
      char buf[1 + StationURLS[channel].length()];
      StationURLS[channel].toCharArray(buf, (1 + StationURLS[channel].length()));
      Serial.print("Changing channel to : ");
      Serial.println(buf);
      Serial.println(StationNames[channel]);
      audio.connecttohost(buf);
      ChannelPlayTimer = millis();
      ChannelPlayComplete = false;
      ChannelPlayRetryCount = 0;
    }

    if ((ChannelPlayComplete == false) && (millis() > (ChannelPlayTimer + 3000))) {
      Serial.println("****Channel play has failed - Retrying****");
      StationInfo = "  Retrying";
      x = 0;      //should cause message scroller to reset
      Timer = 0;  //should cause message to be displayed immediately
      WiFi.disconnect();
      delay(1000);
      WiFi.begin();                // Reconnect using saved credentials
      ChannelPlayComplete = true;  //temporary cease until add retry count stuff
      Core0PlayChannel = true;
    }
  }
  while (!enableaudio) {
    Serial.println("Audio Paused");
    sleep(100);
  }
}

// Task 2 code
void task2(void *parameter) {

  String &text = StationInfo;

  if (StationInfoEnable[channel] == "yes") {
    text = StationInfo;
  } else {
    text = StationNames[channel];
  }

  int delayTime = 1;  // Adjustable scroll speed
  int displayWidth = display.width();

  // Calculate how many characters can fit on the display width
  int maxChars = displayWidth / 12;  // Assuming each character is ~6 pixels wide

  // Add spaces at the end to create a text break
  String scrollText = text + "     ";  // Adjust the number of spaces as needed

  Timer = millis();
  ScreenTime = millis();
  // intially x was declared here - moved it to make it global

  while (true) {  // Continuous loop

    VolumeChange = VolumeSelector.rotate();
    VolumePush = VolumeSelector.pushType(200);  //Short is 1 and long is 2
    ChannelChange = ChannelSelector.rotate();
    ChannelPush = ChannelSelector.pushType(200);  //Short is 1 and long is 2

    //Read voltage pin (IO34)
    int reading = analogRead(34);

    //A bit of messin to scale to a voltage reading and then round to one decimal place
    prevReading = (prevReading * (smoothingFactor - 1) + reading) / smoothingFactor;
    voltage = prevReading / 510.11;  // use the smoothed reading
    //voltage = int((voltage * 10)) / 10;
    batteryVoltage = round(voltage * 10) / 10.0;

    //Serial.println(batteryVoltage);

    //Check for inputs

    //Enter Menu mode when Channel Button long pressed
    if (ChannelPush == 2) {
      //Put the Menu into a seperate funtion but remember to include the power off routine
      MenuMode();
    }

    //Increment volume when volume selector rotated in radio mode
    if (VolumeChange == 1) {
      ScreenTime = millis();
      ScreenBlank = false;
      //Serial.println("Volume rotated clockwise");
      if (++volume > 30) {
        volume = 30;
      } else {
        Serial.print("Volume : ");
        Serial.println(volume);
        audio.setVolume(volume);
      }
    }

    //Decrement volume when volume selector rotated in radio mode
    //Be good to trap the try to go below 0 so that no activity if yuo try to below zero
    if (VolumeChange == 2) {
      ScreenTime = millis();
      ScreenBlank = false;
      //Serial.println("Volume rotated counterclockwise");
      if (--volume < 0) {
        volume = 0;
        //Serial.print("Voltage : ");
        //Serial.println(voltage);
        //StationInfo = "Battery : " + String(voltage);
        //x = 0;      //reset scroller text
        //Timer = 0;  //to force immediate view
      } else {
        Serial.print("Volume : ");
        Serial.println(volume);
        audio.setVolume(volume);
      }
    }

    //Cycle down through channels when channel selector rotated in radio mode  ** Add routine to change channel so no press needed **
    if (ChannelChange == 2) {
      ScreenTime = millis();
      ScreenBlank = false;
      //Serial.println("Channel rotated counterclockwise");
      if (--channel < 0) {
        channel = 0;
      } else {
        Serial.print("Channel : ");
        Serial.println(channel);
        StationInfo = StationNames[channel];
        x = 0;
        channelTime = millis();
        switchChannel = true;
        Timer = 0;  //This is to force the immediate screen update
      }
    }

    //Cycle up through channels when channel selector rotated in radio mode ** Add routine to change channel so no press needed **
    if (ChannelChange == 1) {
      ScreenTime = millis();
      ScreenBlank = false;
      //Serial.println("Channel rotated clockwise");
      if (++channel > (channelCount - 1)) {
        channel = channelCount - 1;
      } else {
        Serial.print("Channel : ");
        Serial.println(channel);
        StationInfo = StationNames[channel];
        x = 0;
        channelTime = millis();
        switchChannel = true;
        Timer = 0;  //This is to force the immediate screen update
      }
    }

    //Change channel after a delay - need to make sure user has settled on a station
    if ((switchChannel == true) && (millis() - channelTime) > 750) {
      Serial.println("PLAY NEW CHANNEL");
      switchChannel = false;
      Core0PlayChannel = true;  //This should trigger the core0 routing to change channel
    }

    //Power off the radio if the Volume button is pressed in radio mode
    if ((VolumePush == 2) && (ChannelPush == 0)) {
      Serial.println("Volume button pressed");
      Poweroff("Goodbye");
    }

    // Toggle day/night mode with short channel press
    if (ChannelPush == 1) {
      Serial.println("Short Press of Channel Button");
      // Increment Daymode
      Daymode = (Daymode + 1) % 3;

      switch (Daymode) {
        case 0:
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Day Mode");
          display.display();
          delay(500);
          ScreenTime = millis();
          Serial.println("Day Mode Set");
          blankerDelay = 120 * 60 * 1000;  //2 hours
          sleepDelay = 120 * 60 * 1000;    //2 hours
          Timer = 0;
          x = 0;
          ScreenBlank = false;
          break;
        case 1:
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Night Mode");
          display.display();
          delay(500);
          ScreenTime = millis();
          Serial.println("Night Mode Set");
          blankerDelay = 1 * 1000;      //1 second (1000)
          sleepDelay = 30 * 60 * 1000;  //30 minutes (1800000)
          Timer = 0;
          x = 0;
          ScreenBlank = false;
          break;
        case 2:
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Dark Mode");
          display.display();
          delay(500);
          ScreenTime = millis();
          Serial.println("Dark Mode Set");
          blankerDelay = 1 * 1000;          //1 second (1000)
          sleepDelay = 2 * 60 * 60 * 1000;  //2 hours
          Timer = 0;
          x = 0;
          ScreenBlank = false;
          break;
      }
    }

    //Add routine here to count the number of LOW battery instances and if > 200 then shutdown
    if (batteryVoltage < 3.4) {
      LowPowerScore++;
      Serial.print("Low Power Score : ");
      Serial.println(LowPowerScore);
    }

    if (LowPowerScore > 2000) {
      Serial.println("Shutdown due to low battery");
      StationInfo = "LOW POWER";
      x = 0;
      Timer = 0;
      //Poweroff("Battery");
      Poweroff("Battery : " + String(voltage));
    }

    //Power off after time based on DayMode
    if (millis() > (ScreenTime + sleepDelay)) {
      Poweroff("SleepTime");
    }

    if ((millis() > (ScreenTime + blankerDelay)) && (ScreenBlank == false)) {
      ScreenBlank = true;
      display.clearDisplay();
      display.display();
    }

    //Now need to have a enable/disabled flag to control
    if ((ScreenBlank == false) && (millis() > (Timer + 500))) {  //333 seems reasonable

      Timer = millis();

      //Serial.print("Timer = ");
      //Serial.println(Timer);

      display.clearDisplay();

      if (StationInfoEnable[channel] == "yes") {
        text = StationInfo;
      } else {
        text = StationNames[channel];
      }

      //String& test="This is a new message";
      String scrollText = text + "     ";

      // Draw rounded rectangle as main border
      display.drawRoundRect(0, 0, display.width(), display.height(), 8, SH110X_WHITE);

      // ** CAN TAKE ALL THIS STUFF OUTSIDE THE LOOP ** apart from the display.width one - as needs display defined

      // Battery bar parameters (top-right corner)
      float minVoltage = 3.2;
      float maxVoltage = 4.2;
      int batteryBarWidth = 24;                                 // Width of the battery bar
      int batteryBarHeight = 8;                                 // Height of the battery bar
      int batteryXPos = display.width() - batteryBarWidth - 6;  // Position in top-right corner inside border
      int batteryYPos = 6;                                      // Vertical position near the top

      // Map battery voltage to a value between 0 and batteryBarWidth
      int batteryFilledWidth = (int)((batteryVoltage - minVoltage) / (maxVoltage - minVoltage) * batteryBarWidth);
      batteryFilledWidth = constrain(batteryFilledWidth, 0, batteryBarWidth);  // Constrain within bar width

      // Draw battery bar outline with rounded corners
      display.drawRoundRect(batteryXPos, batteryYPos, batteryBarWidth, batteryBarHeight, 2, SH110X_WHITE);

      // Fill the battery bar based on the voltage
      if (batteryFilledWidth > 0) {
        display.fillRect(batteryXPos, batteryYPos, batteryFilledWidth, batteryBarHeight, SH110X_WHITE);
      }

      // Volume bar parameters (moved right to avoid overlap with the battery)
      int volumeBarWidth = 24;                                  // Width of the volume bar
      int volumeBarHeight = 8;                                  // Height of the volume bar
      int volumeXPos = displayWidth - 2 * volumeBarWidth - 50;  // Adjusted to the left
      int volumeYPos = 6;                                       // Vertical position near the top

      // Draw volume bar outline with rounded corners
      display.drawRoundRect(volumeXPos, volumeYPos, volumeBarWidth, volumeBarHeight, 2, SH110X_WHITE);

      // Map volume to a value between 0 and volumeBarWidth
      int volumeFilledWidth = map(volume, 0, 30, 0, volumeBarWidth);
      volumeFilledWidth = constrain(volumeFilledWidth, 0, volumeBarWidth);  // Constrain within bar width

      // Fill the volume bar based on the volume
      if (volumeFilledWidth > 0) {
        display.fillRect(volumeXPos, volumeYPos, volumeFilledWidth, volumeBarHeight, SH110X_WHITE);
      }

      // Labels for battery and volume indicators
      display.setTextSize(1);

      // Adjust the volume label position with the new volumeXPos
      display.setCursor(volumeXPos - 20, volumeYPos + (volumeBarHeight / 2) - 4);  // Move label with the bar
      display.print("Vol");

      // Position for the battery label
      display.setCursor(batteryXPos - 20, batteryYPos + (batteryBarHeight / 2) - 4);  // Adjust vertical position for 'bat'
      display.print("Bat");

      // Create the visible portion of text with wrapping effect
      String visibleText;
      if (x + maxChars <= scrollText.length()) {
        visibleText = scrollText.substring(x, x + maxChars);
      } else {
        visibleText = scrollText.substring(x) + scrollText.substring(0, (x + maxChars) % scrollText.length());
      }

      // Set cursor to display the scrolling text in the center
      display.setTextSize(2);
      display.setCursor(0, display.height() / 2 - 8);  // Center vertically
      display.print(visibleText);
      display.display();
      //delay(delayTime);

      // Increment x for scrolling, wrapping around to the start of the text
      x = (x + 1) % scrollText.length();
    }
  }
}

void setup() {

  //Set Pin 19 high to keep the FET power enabled
  pinMode(19, OUTPUT);
  digitalWrite(19, HIGH);  //Switch 19 high to enable power circuit

  //Configure Serial Port
  Serial.begin(115200);
  Serial.println(product);
  Serial.print("Version: ");
  Serial.println(vers);
  Serial.println(updated);

  //Think this is to reduce resolution of the battery read analog input to reduce noise
  analogSetAttenuation(ADC_11db);
  analogSetWidth(9);

  //Initialse display
  display.begin(i2c_Address, true);  // Address 0x3C default
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(product);
  display.print("v ");
  display.println(vers);
  display.display();

  //Wifi Manager
  WiFiManager wm;
  bool res;
  res = wm.autoConnect();  // auto generated AP name from chipid

  if (!res) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WIFI FAIL");
    Serial.println("WIFI FAIL");
    display.display();
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CONNECTED");
    Serial.println("CONNECTED");
    //display.drawBitmap(0, 0, image_data_Saraarray_INV, 128, 32, 1);
    display.display();
  }

  EEPROM.begin(3100);
  //delay(3000); // removed this in v6.07 - seems to not need!

  //Check if there is a read(0);
  eepromstatus = EEPROM.read(0);
  Serial.print("EEPROM Status read from EEPROM(0): ");
  Serial.println(eepromstatus);

  //Will need to add some initialisation values if first run detected - reorder so that the check is done first!

  /*
  if (eepromstatus != 123)  //Can use this to force eeprom refresh after a reboot - need to update code to write 123 so doesn't get stuck in loop
  {
    Serial.println("EEPROM is not configured - first run?");
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(" Updating  Channels");
    display.display();
    //delay(3000);
    UpdateEEPROM();
    EEPROM.write(0, 123);
    //Set initial values for channel, volume and eq here - BEst to replace with data from googledrive
    EEPROM.write(1, 0);
    EEPROM.write(2, 3);
    EEPROM.write(3, 10);
    EEPROM.write(4, 6);
    EEPROM.write(5, 10);
    EEPROM.commit();

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(" Channels  Updated");
    display.display();
    delay(1000);
  }
*/
  channel = EEPROM.read(1);  //Read previous  from channelEEPROM
  Serial.print("Channel read from EEPROM: ");
  Serial.println(channel);
  currentChannel = channel;

  audio.setVolumeSteps(40);  //changed from 21 - the default

  volume = EEPROM.read(2);  //Read previous volume from EEPROM
  Serial.print("Volume read from EEPROM: ");
  Serial.println(volume);

  eqBass = EEPROM.read(3) - 6;  //Read previous eqBass from EEPROM
  Serial.print("eqBass read from EEPROM: ");
  Serial.println(eqBass);

  eqMid = EEPROM.read(4) - 6;  //Read previous eqMid from EEPROM
  Serial.print("eqMid read from EEPROM: ");
  Serial.println(eqMid);

  eqTreb = EEPROM.read(5) - 6;  //Read previous eqTreb from EEPROM
  Serial.print("eqTreb read from EEPROM: ");
  Serial.println(eqTreb);

  //Set the number of volume steps
  audio.setVolumeSteps(30);  //Deafult is 21
  audio.setPinout(MAX98357A_I2S_BCLK, MAX98357A_I2S_LRC, MAX98357A_I2S_DOUT);
  audio.setVolume(volume);
  audio.setTone(eqBass, eqMid, eqTreb);  // // values can be between -40 ... +6 (dB)

  Daymode = EEPROM.read(6);
  Serial.print("Daymode from EEPROM");
  Serial.println(Daymode);

  switch (Daymode) {
    case 0:
      Serial.println("Day Mode Set");
      blankerDelay = 120 * 60 * 1000;  //2 hours
      sleepDelay = 120 * 60 * 1000;    //2 hours
      break;
    case 1:
      Serial.println("Night Mode Set");
      blankerDelay = 1 * 1000;      //1 second (1000)
      sleepDelay = 30 * 60 * 1000;  //30 minutes (1800000)
      break;
    case 2:
      Serial.println("Dark Mode Set");
      blankerDelay = 1 * 1000;          //1 second (1000)
      sleepDelay = 2 * 60 * 60 * 1000;  //2 hours
      break;
  }

  DynamicJsonDocument doc(3090);        //increase this for more channels - was 2664 - >2800
  EepromStream eepromStream(10, 3100);  //was 1,2664 -> 1,2674 -> 2810
  deserializeJson(doc, eepromStream);

  // load the json data into string arrays - make this into a seperate funtion
  for (JsonObject item : doc["Channels"].as<JsonArray>()) {
    String Station = item["S"];                 //have to go to string variable first - doesn't work direct!
    String StationInfoEnableState = item["I"];  //Station information enable status
    String URL = item["U"];

    Serial.println("****************************************");
    StationNames[channelCount] = Station;
    Serial.print("StationName: ");
    Serial.println(StationNames[channelCount]);
    StationInfoEnable[channelCount] = StationInfoEnableState;
    Serial.print("StationInfoEnable: ");
    Serial.println(StationInfoEnable[channelCount]);
    StationURLS[channelCount] = URL;
    Serial.print("StationURL: ");
    Serial.println(StationURLS[channelCount]);
    channelCount++;  //count the number of stations in the array
  }
  Serial.println("****************************************");
  Serial.print("NUMBER OF STATIONS : ");
  Serial.println(channelCount);
  Serial.println("****************************************");

  if (volume > 30) {
    Serial.println("EEPROM Volume setting is out of range, resetting");
    volume = 1;
    EEPROM.write(2, 1);
    EEPROM.commit();
  }

  if (channel > channelCount) {
    Serial.println("EEPROM Channel setting is out of range, resetting");
    channel = 0;
    EEPROM.write(1, 0);
    EEPROM.commit();
  }

  Core0PlayChannel = true;

  StationInfo = StationNames[channel];  //see if this works - tho will interfer in trapping failed streams

  // Create Task 1 on Core 0
  xTaskCreatePinnedToCore(
    task1,         // Task function
    "Task1",       // Task name
    10000,         // Stack size (words, not bytes)
    NULL,          // Task parameters
    1,             // Priority
    &task1Handle,  // Task handle
    0              // Core number (0 or 1)
  );

  // Create Task 2 on Core 1
  xTaskCreatePinnedToCore(
    task2,         // Task function
    "Task2",       // Task name
    10000,         // Stack size (words, not bytes)
    NULL,          // Task parameters
    1,             // Priority
    &task2Handle,  // Task handle
    1              // Core number (0 or 1)
  );
}

void loop() {
  //audio.loop();
}

void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle ");
  Serial.println(info);
  //display.clearDisplay();
  //scrollTextWithBorder(info,3.6,25);
  StationInfo = info;  //This variable will be used in the deplay section of the main control loop
  //display.setCursor(0, 0);
  //display.print(info);
  //display.display();
}

void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
  if (strstr(info, "stream ready") != nullptr) {
    Serial.println("**** The string contains 'success'.*****");
    ChannelPlayComplete = true;
  }
}
void audio_id3data(const char *info) {  //id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_mp3(const char *info) {  //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     ");
  Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     ");
  Serial.println(info);
}
void audio_commercial(const char *info) {  //duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info) {  //homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}
void audio_lasthost(const char *info) {  //stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  ");
  Serial.println(info);
}

void Poweroff(String powermessage) {
  audio.stopSong();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(powermessage);
  display.display();

  WriteValuesToEEPROM();

  delay(1000);
  digitalWrite(19, LOW);
}

void WriteValuesToEEPROM(void) {
  EEPROM.write(1, channel);
  EEPROM.write(2, volume);
  EEPROM.write(3, eqBass + 6);
  EEPROM.write(4, eqMid + 6);
  EEPROM.write(5, eqTreb + 6);
  EEPROM.write(6, Daymode);
  EEPROM.commit();
}


//At the moment this just updates the EEPROM, so a restart is needed to refresh the arrays with the updated info
//Alternatively could just load the values into the array like the code in setup (then replace the setup code with this function)
void UpdateEEPROM(void) {
  Serial.println("********************UPDATE FROM GOOGLE CODE ******************************************************************************************");
  //enableaudio = false; //think this is now duplicate so can comment out - tho may also be triggered at startup for a new ESP
  WiFi.disconnect();
  delay(1000);
  WiFi.begin();  // Reconnect using saved credentials
  delay(1000);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  // Move thuis definition to front of code with the Firmware settings
  http.begin("https://drive.google.com/uc?export=download&id=1OC_veUhF0Xkz2dCfr45-ktKgOrjJE1H-", root_ca);  //New channelsv6.json file

  long int httpCode = http.GET();  //This is where it seems to trigger Guru Meditation Error: Core 1 panic.
  Serial.print("HTTP CODE IS : ");
  Serial.println(httpCode);
  if (httpCode < 0) {
    Serial.println("WANKERED - can't read channel file!");
    delay(3000);
    //ESP.restart();
  }

  if (httpCode > 0) {  //Check for the returning code
    String payload = http.getString();

    if (httpCode == 200) Serial.println("Success");
    if (httpCode == 404) Serial.println("The requested page can't be found on the website server");
    if (httpCode == 301) Serial.println("The requested resource has been definitively moved to the URL given by the Location headers");
    if (httpCode == 303) {
      Serial.println("The redirects don't link to the requested resource itself, but to another page");
      payload = http.getString();
    }
    if (httpCode == 400) Serial.println("The server cannot or will not process the request due to something that is perceived to be a client error");

    DynamicJsonDocument doc(3090);  //was 2664
    deserializeJson(doc, payload);

    EepromStream eepromStream(10, 3100);  //was 1,2664 -> 10,2674 -> 10,2810
    serializeJson(doc, eepromStream);
    EEPROM.commit();
  } else {
    Serial.println("Error on HTTP request");  //Suspect
  }
  http.end();  //Free the resources
  WriteValuesToEEPROM();
  ESP.restart();
}
//menu driven updated doesn't seems to get locked on exit - Forgot what thius means!

void MenuMode(void) {
  Serial.println("MenuMode1");
  boolean MenuMode = false;
  //Serial.println("Entering Settings");

  while (MenuMode == false) {
    VolumeChange = VolumeSelector.rotate();
    VolumePush = VolumeSelector.pushType(200);  //Short is 1 and long is 2
    ChannelChange = ChannelSelector.rotate();
    ChannelPush = ChannelSelector.pushType(200);  //Short is 1 and long is 2

    //Read voltage pin (IO34)
    int reading = analogRead(34);

    //A bit of messin to scale to a voltage reading and then round to one decimal place
    prevReading = (prevReading * (smoothingFactor - 1) + reading) / smoothingFactor;
    voltage = prevReading / 510.11;  // use the smoothed reading
    //voltage = int((voltage * 10)) / 10;
    batteryVoltage = round(voltage * 10) / 10.0;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Settings");
    display.print("Vers: ");
    display.println(vers);
    display.print("Batt: ");
    display.println(batteryVoltage);
    display.setTextSize(1);
    display.println();
    display.println(WiFi.localIP());
    display.setTextSize(2);
    display.display();

    if (ChannelPush == 2) {
      display.clearDisplay();
      display.display();
      MenuMode = true;
    }

    if (ChannelPush == 1) {
      display.clearDisplay();
      display.display();
      MenuMode2();
    }

    //Power off the radio if the Volume button is pressed in radio mode
    if ((VolumePush == 2) && (ChannelPush == 0)) {
      Serial.println("Volume button pressed");
      Poweroff("Goodbye");
    }
  }
}

void MenuMode2(void) {
  Serial.println("MenuMode2");
  boolean MenuMode = false;
  int selectedItem = 0;  // 0 for Bass, 1 for Mid, 2 for Treb
  int lastSelectedItem = -1;
  int lastEqBass = -99, lastEqMid = -99, lastEqTreb = -99;  // Track previous values to detect changes
  //int eqBass = 0, eqMid = 0, eqTreb = 0;  // Initial values

  while (!MenuMode) {
    int VolumeChange = VolumeSelector.rotate();
    int VolumePush = VolumeSelector.pushType(200);  // Short is 1, long is 2
    int ChannelChange = ChannelSelector.rotate();
    int ChannelPush = ChannelSelector.pushType(200);  // Short is 1, long is 2

    // Adjust the selected value based on VolumeChange
    int volumeAdjustment = 0;
    if (VolumeChange == 1) {
      volumeAdjustment = 1;  // Increment
    } else if (VolumeChange == 2) {
      volumeAdjustment = -1;  // Decrement
    }

    // Adjust and update tone values if changed
    if (volumeAdjustment != 0) {
      if (selectedItem == 0) {
        eqBass = constrain(eqBass + volumeAdjustment, -6, 6);
      } else if (selectedItem == 1) {
        eqMid = constrain(eqMid + volumeAdjustment, -6, 6);
      } else if (selectedItem == 2) {
        eqTreb = constrain(eqTreb + volumeAdjustment, -6, 6);
      }
      audio.setTone(eqBass, eqMid, eqTreb);
    }

    // Update selection if changed
    if (ChannelChange != 0) {
      selectedItem = (selectedItem + ChannelChange + 3) % 3;
    }

    // Display updates only if changes detected
    if (selectedItem != lastSelectedItem || eqBass != lastEqBass || eqMid != lastEqMid || eqTreb != lastEqTreb) {
      display.clearDisplay();

      // Display Bass with highlighting if selected
      if (selectedItem == 0) {
        display.fillRect(0, 0, 60, 18, SH110X_WHITE);  // Highlight
        display.setTextColor(SH110X_BLACK);            // Black text on white
      } else {
        display.setTextColor(SH110X_WHITE);  // White text on black
      }
      display.setCursor(3, 0);
      display.print("Bass");
      display.setCursor(70, 0);
      display.setTextColor(SH110X_WHITE);  // Ensure value displays in white
      display.print(eqBass);

      // Display Mid with highlighting if selected
      if (selectedItem == 1) {
        display.fillRect(0, 20, 60, 18, SH110X_WHITE);  // Highlight
        display.setTextColor(SH110X_BLACK);             // Black text on white
      } else {
        display.setTextColor(SH110X_WHITE);  // White text on black
      }
      display.setCursor(3, 20);
      display.print("Mid");
      display.setCursor(70, 20);
      display.setTextColor(SH110X_WHITE);  // Ensure value displays in white
      display.print(eqMid);

      // Display Treb with highlighting if selected
      if (selectedItem == 2) {
        display.fillRect(0, 40, 60, 18, SH110X_WHITE);  // Highlight
        display.setTextColor(SH110X_BLACK);             // Black text on white
      } else {
        display.setTextColor(SH110X_WHITE);  // White text on black
      }
      display.setCursor(3, 40);
      display.print("Treb");
      display.setCursor(70, 40);
      display.setTextColor(SH110X_WHITE);  // Ensure value displays in white
      display.print(eqTreb);

      display.display();  // Refresh display only on changes

      // Update last known states
      lastSelectedItem = selectedItem;
      lastEqBass = eqBass;
      lastEqMid = eqMid;
      lastEqTreb = eqTreb;
    }

    // Exit or switch mode based on ChannelPush
    if (ChannelPush == 2) {  // Long press to exit
      display.clearDisplay();
      display.display();
      MenuMode = true;
    } else if (ChannelPush == 1) {  // Short press to switch mode
      display.clearDisplay();
      display.display();
      MenuMode3();
      lastEqBass = -99;  //trigger a fake update state to show the display when returning from menu3
    }

    // Power off the radio if Volume button long press
    if ((VolumePush == 2) && (ChannelPush == 0)) {
      Serial.println("Volume button pressed");
      Poweroff("Goodbye");
    }

    // Small delay for stability
    delay(10);
  }
}

void MenuMode3(void) {
  Serial.println("MenuMode3");
  boolean MenuMode = false;
  int selectedItem = 0;  // 0 for EEPROM Update, 1 for Channel Update, 2 for Factory Reset
  int lastSelectedItem = -1;

  while (!MenuMode) {
    int ChannelChange = ChannelSelector.rotate();
    int ChannelPush = ChannelSelector.pushType(200);  // Short is 1, long is 2
    int VolumePush = VolumeSelector.pushType(200);    // Short is 1, long is 2

    // Change selection with ChannelChange
    if (ChannelChange != 0) {
      selectedItem = (selectedItem + ChannelChange + 3) % 3;  // Cycle through 0, 1, 2
    }

    // Display updates only if the selection has changed
    if (selectedItem != lastSelectedItem) {
      display.clearDisplay();

      // Display EEPROM Update with highlighting if selected
      if (selectedItem == 0) {
        display.fillRect(0, 0, 100, 18, SH110X_WHITE);  // Highlight
        display.setTextColor(SH110X_BLACK);             // Black text on white
      } else {
        display.setTextColor(SH110X_WHITE);  // White text on black
      }
      display.setCursor(3, 0);
      display.print("Firmware");

      // Display Channel Update with highlighting if selected
      if (selectedItem == 1) {
        display.fillRect(0, 20, 100, 18, SH110X_WHITE);  // Highlight
        display.setTextColor(SH110X_BLACK);              // Black text on white
      } else {
        display.setTextColor(SH110X_WHITE);  // White text on black
      }
      display.setCursor(3, 20);
      display.print("Channels");

      // Display Factory Reset with highlighting if selected
      if (selectedItem == 2) {
        display.fillRect(0, 40, 100, 18, SH110X_WHITE);  // Highlight
        display.setTextColor(SH110X_BLACK);              // Black text on white
      } else {
        display.setTextColor(SH110X_WHITE);  // White text on black
      }
      display.setCursor(3, 40);
      display.print("Reset");

      display.display();  // Refresh display only on changes
      lastSelectedItem = selectedItem;
    }

    // Launch the selected function on short Volume button press
    if (VolumePush == 1) {
      switch (selectedItem) {
        case 0:
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println(" Checking  Firmware  Update");
          display.display();
          enableaudio = false;
          checkForUpdate();
          Serial.println("Finished the update check");
          WriteValuesToEEPROM();
          ESP.restart();
          break;
        case 1:
          enableaudio = false;
          UpdateEEPROM();
          Serial.println("Finished the EEPROM Update");  //think you will never see this as app restarts elsewhere - maybe move to here
          enableaudio = true;
          //ESP.restart();
          break;
        case 2:
          factoryreset();
          break;
      }
      Serial.println("Dropped out of menu - need to redisplay menu");
      MenuMode = true;  //this works and displays the previous menu
    }

    // Exit or return to another menu based on long Channel button press
    if (ChannelPush == 2) {  // Long press to exit
      display.clearDisplay();
      display.display();
      MenuMode = true;
    }

    delay(10);  // Small delay for stability
  }
}

void factoryreset() {
}

void checkForUpdate() {
  Serial.println("Checking for a new firmware version");

  //Might not need this bit but probably good for reliability

  //WiFi.disconnect();
  //delay(1000);
  //WiFi.begin();  // Reconnect using saved credentials
  //delay(1000);

  String latestVersion = getLatestVersionFromServer();  // Function to get latest version (implement this)

  if (latestVersion > currentVersion) {
    Serial.println("New version available, getting pointer...");
    getPointer();
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(" Updating  Firmware");
    display.display();
    downloadFirmware();
  } else {
    Serial.println("No Update Available");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(" No Update Available");
    display.display();
    delay(2000);
  }
}

String getLatestVersionFromServer() {
  HTTPClient http;
  http.begin(firmware_ver, root_ca);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  String latestVersion = "0";  // Default version if something goes wrong

  if (httpCode == HTTP_CODE_OK) {
    latestVersion = http.getString();  // Retrieve the latest version from the server
    Serial.println("Latest version available: " + latestVersion);
    //display.println("Up to Date");
    //display.display();
  } else {
    Serial.println("Error fetching version information");
    Serial.println(firmware_ver);
    Serial.println(httpCode);
    Serial.println(http.errorToString(httpCode));
    display.println("Error(vers)Fetching");
    display.display();
  }

  http.end();
  return latestVersion;
}

void getPointer() {
  display.clearDisplay();
  display.setCursor(0, 0);
  //display.println(" Updating  Firmware");
  display.println(" Getting   Pointer");
  display.display();

  HTTPClient http;
  http.begin(firmware_ptr, root_ca);  // Example of hosting version info on a text file
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();

  // Will need some error checking like in the getLatestVersionFromServer function
  //Probably best to make this a function

  String latestVersion = "0";  // Default version if something goes wrong

  if (httpCode == HTTP_CODE_OK) {
    firmwareURL = http.getString();  // Retrieve the latest version from the server
    Serial.println("Address of latest bin: " + firmwareURL);
  } else {
    Serial.println("Error fetching address of latest bin");
    display.println("Error(bin)Fetching");
    display.display();
  }

  // need to convert firmware_url(string) = firmwareURL(char*)
  firmware_url = strdup(firmwareURL.c_str());

  http.end();
}

void downloadFirmware() {

  WiFiClient client;
  HTTPClient http;

  http.begin(firmware_url, root_ca);  //this one for google drive based share
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Updating firmware");
    int len = http.getSize();
    if (len > 0) {
      WiFiClient *stream = http.getStreamPtr();
      Update.begin(len);                             // Begin OTA update with expected size
      size_t written = Update.writeStream(*stream);  // Write the data stream to flash

      if (written == len) {
        Serial.println("Update successfully written to flash memory!");
        if (Update.end()) {
          Serial.println("Firmware update complete, rebooting...");
          delay(3000);
          WriteValuesToEEPROM();
          ESP.restart();  // Reboot to load the new firmware
        } else {
          Serial.printf("Update failed: %s\n", Update.errorString());  // Corrected errorString() handling
        }
      } else {
        Serial.printf("Written only %d of %d bytes\n", written, len);
      }
    }
  } else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}