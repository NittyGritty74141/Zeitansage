/*
  ESP 8266-based talking clock
  created 06 Sep 2017
  by Dirk Spiller

  This sketch gets the Time via NTP and speaks it out loud using DFPlayer MP3 module and several voice files as mp3 (e.g. in German language).
  It utilizes an IR distance sensor TCRT5000
  (eg. https://www.amazon.de/Ecloud-TCRT5000-Reflective-Barrier-Leichtathletik-Lichtschranke/dp/B06WGQJ2ZW/ref=sr_1_4?s=ce-de&ie=UTF8&qid=1505044834&sr=1-4&keywords=TCRT5000+modul)
  to recognize if the user holds his hand close to the sensor and then speaks the time again.

  It uses:
  - Standard ESP8266 WiFi Libs for Arduino IDE: https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
  - TimeLib: https://github.com/PaulStoffregen/Time
  - Timezone Library for correct handling of DST: https://github.com/JChristensen/Timezone
  - The DFPlayer Mini Lib: https://github.com/DFRobot/DFRobotDFPlayerMini
  - The 4x7-segment display TM1637Display lib: https://github.com/avishorp/TM1637

  The mp3 handling is inspired by the mp3 talking clock from LAGSILVA: http://www.instructables.com/id/Talking-Clock-With-Arduino/
  Instead of setting the initial Time manually, it uses NTP as a Sync Provider as in the TimeNTP_ESP8266WiFi example from TimeLib // https://github.com/PaulStoffregen/Time/tree/master/examples/TimeNTP_ESP8266WiFi
  Instead of saying the time every ten or so minutes, it says the time, when you hold your hand close to the IR sensor

   -------- MP3 Voice output via DFPlayer module -------
   On microSD card, all sound files have to be put in a folder called "mp3"
   The following files are expected:
   1.mp3 - 24-mp3  : "zero hours", "one hours", "two hours" to "23 hours"; In German it is required to say "Uhr" meaning "hours" between hours and minutes of a timestamp!
   25.mp3 - 84.mp3 : "zero" to "fifty-nine" for the minutes
   85.mp3          : "to"
   86.mp3          : "past"
   87.mp3          : "quarter"
   88.mp3          : "half"
   85.mp3 - 88.mp3 are meant for colloquial time telling (future expansion, not yet implemented!)

   Decent mp3 files can easily be created online using the following strategy:

   1. Create a text file containing all required words with breaks, like in German:
      0 Uhr, 1 Uhr, 2 Uhr, 3 Uhr, 4 Uhr, 5 Uhr, 6 Uhr, 7 Uhr, 8 Uhr, 9 Uhr, 10 Uhr, 11 Uhr, 12 Uhr, 13 Uhr, 14 Uhr, 15 Uhr, 16 Uhr, 17 Uhr, 18 Uhr, 19 Uhr, 20 Uhr, 21 Uhr, 22 Uhr, 23 Uhr, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, Nach, Vor, Viertel, Halb,
   2. Choose an online Text-To-Speech Service like https://www.naturalreaders.com/online/
   3. Paste the text file contents to the input box
   4. Choose language and voice
   5. Use audacity for recording the output directly as explained under http://manual.audacityteam.org/man/tutorial_recording_computer_playback_on_windows.html
   6. Label recording into separate tracks as described at http://manual.audacityteam.org/man/silence_finder_and_sound_finder.html
   7. Export separate sound files from labels as described here: http://manual.audacityteam.org/man/splitting_a_recording_into_separate_tracks.html
   8. Now click "ok" 88 times ;-) -> the resulting files are cut and named correctly by default.

  Possible Expansions:
  - Add a TM1637-based 4 Digit module to also display the time (eg https://www.amazon.de/TM1637-Digit-7-Segment-Display-Modul/dp/B0117C1332) >Already contained in source code!
  - Add a DS3231 RTC module to overcome powerloss and network failures (eg. https://www.amazon.de/DaoRier-Precision-Modul-Speichermodul-Raspberry-Mikrocontroller/dp/B06W2PLFJY/ref=sr_1_5?s=ce-de&ie=UTF8&qid=1505029576&sr=1-5&keywords=rtc+module+ds3231)

  This code is in the public domain.
*/
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>       // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <WiFiUdp.h>           // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <DFRobotDFPlayerMini.h>   // https://github.com/DFRobot/DFRobotDFPlayerMini
#include <TimeLib.h>           // https://github.com/PaulStoffregen/Time
#include <Timezone.h>          // https://github.com/JChristensen/Timezone

#define PIN_BUSY D5 // Connect to DFPlayer Module Pin 8 for busy detection (doesn't work too good)
#define PIN_NEAR D3 // Connect to IR Module Near detection pin.


SoftwareSerial mySoftwareSerial(D6, D7); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void printDetail(uint8_t type, int value);

char ssid[] = "Powerplay";  //  your network SSID (name)
char pass[] = "PiratensenderPowerplay2106";  // your network password

// NTP Servers:
// IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
// IPAddress timeServer(129, 6, 15, 28);   // time.nist.gov NTP server

// Don't hardwire the IP address or we won't get the benefits of the pool.
// Lookup the IP address for the host name instead */
IPAddress timeServer;                    // time.nist.gov NTP server address will be retrieved using WiFi.byHost() command
const char* ntpServerName = "pool.ntp.org";

//Timezone
//Central European Time (Frankfurt, Paris)
TimeChangeRule myDST = { "CEST", Last, Sun, Mar, 2, 120 };     // Central European Summer Time
TimeChangeRule mySTD = { "CET ", Last, Sun, Oct, 3, 60 };      // Central European Standard Time
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;                                           // pointer to the time change rule, use to get the TZ abbrev
time_t local;         // local time for display / voice output
time_t prevDisplay = 0;                                        // when the digital clock was displayed
time_t Ansage;

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets



int bsy = 0;
int z = 0;
uint32_t beginWait;
int Folder, Datei = 0;

char* weekdays[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };

// --------------------------------------------------
void VarDelay(uint32_t Verz)
{
  uint32_t beginWait1;
  beginWait1 = millis();
  while ( (millis() - beginWait1 < Verz )  ) delay(0) ;
}

void waitend()
{
  int bsyflag = 0;
  for ( z = 0; z <= 100; z++) {
    bsy = digitalRead(PIN_BUSY);
    delay(50);
    if (bsyflag == 1 && bsy == 1) {
      break;
    }
    if (bsy == 0) {
      bsyflag = 1;
    }
  }
}

// ------------------------------------------------------------

void Track(int Folder, int Datei)
{
//  char cb [64];
//  sprintf(cb, "Folder: %1d, Datei: %1d", Folder, Datei ) ;
//  Serial.println(cb);
  myDFPlayer.playFolder(Folder, Datei);
  waitend();
}

void setup()
{
  const char compile_date[] = __DATE__ " " __TIME__ " " __FILE__;

  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  Serial.println();
  Serial.println(compile_date);
  Serial.println("ESP Talking Clock");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

    Serial.print(".");

  }

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");

  Udp.begin(localPort);

  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("Waiting for sync");

  setSyncProvider(getNtpTime);
  while (timeStatus() == timeNotSet) {
    // wait until the time is set by the sync provider
    Serial.println("Time not set!");
  }
  local = myTZ.toLocal(now(), &tcr);
  Ansage = myTZ.toLocal(now() + 10, &tcr);

  pinMode(PIN_BUSY, INPUT_PULLUP);
  pinMode(PIN_NEAR, INPUT_PULLUP);

  Serial.println();
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(mySoftwareSerial, false)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1. Please recheck the connection!"));
    Serial.println(F("2. Please insert the SD card!"));
    while (true) delay(0);
  }
  Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  VarDelay(500);

  byte DFData[] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x1D, 0xEF};
  for (int i = 0; i <= 7; i++) {
    mySoftwareSerial.write(DFData[i]);
  }
  VarDelay(500);
  
  Serial.println(myDFPlayer.readVolume());
  //----Set volume----
  myDFPlayer.volume(14);  //Set volume value (0~30).
  VarDelay(500);

  Serial.println(myDFPlayer.readVolume());

  // myDFPlayer.volumeUp(); //Volume Up
  // myDFPlayer.volumeDown(); //Volume Down

  //----Set different EQ----
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  //  myDFPlayer.EQ(DFPLAYER_EQ_POP);
  //  myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
  //  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
  //  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
  //  myDFPlayer.EQ(DFPLAYER_EQ_BASS);

  //----Set device we use SD as default----
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_U_DISK);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_AUX);
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SLEEP);
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_FLASH);

  //----Mp3 control----
  //  myDFPlayer.sleep();     //sleep
  //  myDFPlayer.reset();     //Reset the module
  //  myDFPlayer.enableDAC();  //Enable On-chip DAC
  //  myDFPlayer.disableDAC();  //Disable On-chip DAC
  //  myDFPlayer.outputSetting(true, 15); //output setting, enable the output and set the gain to 15
}

void loop()
{
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }

  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {      //update the display only if time has changed

      prevDisplay = now();
      local = myTZ.toLocal(now(), &tcr);
      digitalClockDisplay();
    }
/*    
 *     
 if (digitalRead(PIN_NEAR) != 1) {

      Serial.print("**** NEAR= ");
      Serial.println(digitalRead(PIN_NEAR));


      SayTime();
    }
*/
      SayTime();
  }
}

/* ------- Time display on Serial port for debugging ------------*/
void digitalClockDisplay() {
  // digital clock display of the time

  char cb [64];

  sprintf(cb, "%s, %02d.%02d.%04d %02d:%02d:%02d",
          (weekdays[weekday(local) - 1]),
          day(local), month(local), year(local),
          hour(local), minute(local), second(local)
         ) ;
  Serial.println(cb);

}

/* ------- Time display on Serial port for debugging ------------*/
void digitalClockDisplay1() {
  // digital clock display of the time + 10 seconds
  char cb [64];
  sprintf(cb, "%s, %02d.%02d.%04d %02d:%02d:%02d",
          (weekdays[weekday(Ansage) - 1]),
          day(Ansage), month(Ansage), year(Ansage),
          hour(Ansage), minute(Ansage), second(Ansage)
         ) ;
  Serial.println(cb);
}

// ------------------------------------------------------------

void SayTime()
{
  uint8_t Sekunden;

  do {
    Ansage = myTZ.toLocal(now() + 10, &tcr);
    Sekunden = (second(Ansage) % 10);
    // Serial.print(Sekunden);
    delay(10);
  }
  while (Sekunden != 0);

  Serial.println("Now in SayTime");
  Track(3, 100);         // "Beim nächsten Ton ist es"

  Ansage = myTZ.toLocal(now() + 10, &tcr);
  Track( 3, (hour(Ansage) + 1) );
  Track( 4, (minute(Ansage) + 1) );

  Sekunden = ((second(local) / 10 ) + 1);
  if (Sekunden > 5) {
    Sekunden = 0;
  }
  Track( 5, Sekunden + 1 );

  do {
    Ansage = myTZ.toLocal(now() + 10, &tcr);
    Sekunden = (second(Ansage) % 10);
    // Serial.print(Sekunden);
    delay(10);
  }
  while (Sekunden != 0);

  digitalClockDisplay();
  Track(3, 101);         // "Piep"
}

/* -------- NTP code ----------
   based on https://github.com/PaulStoffregen/Time/tree/master/examples/TimeNTP_ESP8266WiFi
   added debug code to test if DST change works correctly
*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets

  Serial.println("Transmit NTP Request");

  WiFi.hostByName(ntpServerName, timeServer);
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {

      Serial.println("Receive NTP Response");

      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }

  Serial.println("No NTP Response :-(");

  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
