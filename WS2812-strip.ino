#include <Streaming.h>  
#include <SerialConsole.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, A0, NEO_GRB + NEO_KHZ800);



bool outputAppData = false;
bool appDebug = false;

SerialConsole console = SerialConsole(Serial);
void listCommands(String *arg) {
  console.SC_list_commands();
}

//================ Serial Hotkey Command callbacks =================
//this needs to be high up here in case any serial command callback functions reference hotkeys.
SC_callbackFunc HotkeyCommandCallbacks[kSC_MaxNumHotKeyCommands] = { 
  {"", HotkeyDefaultResponse}, 
  {"q", HotkeyQuit}
  //note: max of 3 presently in kSC_MaxNumHotKeyCommands (SerialConsole.h)
};

//================ Command callbacks =================

SC_callbackFunc CommandCallbacks[kSC_MaxNumCommands] = { SC_builtin_commands, {"?", listCommands}, //"built in" commands
  //{"test", TestCmd},  
  {"set-lights", set_lights},
  {"lightshow", lightshow},
  {"set-brightness", set_brightness},
  
  {"testchar", SC_TestChar},
  {"debug", set_debug}
};

void SC_TestChar(String *arg) {
  String data = (*arg).substring((*arg).indexOf(' ')+1);
  Serial << "Char # = " << (int)(*data.c_str()) << endl;
}

//================ Command callback functions =================

void DefaultResponse(String *arg) {
  Serial.println("Unrecognized command.");
}

void set_lights(String *arg) {
  // set-lights N-NN FFFFFF
  // set-lights N FFFFFF
  int firstSpace = (*arg).indexOf(' ');
  int secondSpace = (*arg).indexOf(' ', firstSpace+1);
  String range = (*arg).substring(firstSpace+1,secondSpace);
  int rangeStart, rangeEnd;
  if(range.indexOf('-')== -1) { //the '-' was not found
    rangeStart = rangeEnd = range.toInt();
  } else {
    rangeStart = range.substring(0,range.indexOf('-')).toInt();
    rangeEnd = range.substring(range.indexOf('-')+1).toInt();
  }
  uint32_t colour = strtoul((*arg).substring(secondSpace+1).c_str(), NULL, 16);
  if(appDebug) Serial << "Testing [" << (*arg).substring(secondSpace+1).c_str() << "] as hex string, got " << colour << " as int" << endl;
  Serial << "Setting lights " << rangeStart <<" to " << rangeEnd << ", to colour " << colour << endl;

  for(int x=rangeStart;x<rangeEnd+1;x++) {
    strip.setPixelColor(x, colour);
  }
  strip.show();
}

void set_brightness(String *arg) {
  int data = ((*arg).substring((*arg).indexOf(' ')+1)).toInt();
  if(data<0 || data>255) {
    Serial << F("Invalid brightness value. Acceptable values are 0-255 (Reccomended: 16-128)") << endl;
  } else {
    strip.setBrightness(data);
    strip.show(); // Initialize all pixels to 'off'
  }
}


void lightshow(String *arg) {
    //delay(500);
    Serial << F("Testing the entire strip. Press 'q' to end.") << endl;
    outputAppData = true;
    console.SetHotkeys(HotkeyCommandCallbacks);
}

void set_debug(String *arg) { 
  bool value = true;
  bool error = false;
  String data = ((*arg).substring((*arg).indexOf(' ')+1)); // strip out the 'debug ' part
  String what = data.substring(0, data.indexOf(' '));
  if(data.indexOf(' ') != -1) { //a value was included, assume 'off'
    value = false;
  }
  
  if(what == "serial") {
    console._Debug = value;
  } else if(what == "app") {
    appDebug = value;
  } else if(what == "all") {
    console._Debug = value;
    appDebug = value;
  } else {
    error = true;
    Serial << F("Useage is debug {all|serial|app} [off].") << endl;
  }
  if(!error) { 
    Serial << F("Debug messages for ") << what << F(" are ") << (value ? F("enabled.") : F("disabled.")) << endl;
  }
}

//================ Serial Hotkey Command callback functions =================
void HotkeyDefaultResponse(String *arg) {
  //in this case, no action (user must hit q to exit)
}
void HotkeyQuit(String *arg) {
  outputAppData = false;
  console.DisableHotkeys();
  
  //clean up the display
  Serial.write(27);
  Serial.print("[2J"); // clear screen
  Serial.write(27); // ESC
  Serial.print("[H"); // cursor to home
}

//================ Setup =================
void setup() {
  Serial.begin(115200);
  console.setup(CommandCallbacks); //must be after serial is opened, sends motd

  strip.begin();
  strip.setBrightness(16);
  strip.show(); // Initialize all pixels to 'off'
}


//================ Main Loop =================
uint16_t i, j;
unsigned long next_increment=0, now;
void loop() {
  console.RecieveInput();


  if(outputAppData) {
    now=millis();
    if(now>=next_increment) {
      next_increment= now + 50;
   
      if( j>255 ) {
        j=0;
      } else {
        for(i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, Wheel((i+j) & 255));
        }
        strip.show();
        j++;
      }
    }
  }
}


//================ Misc Functions =================

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
