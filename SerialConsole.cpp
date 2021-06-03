#ifndef SerialConsole_cpp
#define SerialConsole_cpp
// ADDED FOR COMPATIBILITY WITH WIRING ??
//extern "C" {
//  #include <stdlib.h>
//}

#include "SerialConsole.h"
#include <Streaming.h>



// ========== Class functions ============= 

void SerialConsole::SC_list_commands() {
    Serial << F("Available commands:") << endl;
    for(int x=0;x<_numCallbacks;x++) {
        Serial << _callbacks[x].fnName << " ";
    }
    Serial << endl;
}

SerialConsole::SerialConsole(HardwareSerial &comms) {
    _comms = &comms;
    _HotKeysEnabled = _Debug = _JustTabCompleted = false;
    _buff.reserve(kSC_InputBuffSize);
    _numHotKeyCallbacks = kSC_MaxNumHotKeyCommands;

    //printf helper, attaches STDOUT to serial
    fdevopen( &printf_to_serial, 0);
}
int printf_to_serial( char c, FILE *t){
  Serial.write( c );
}

//for future frustration avoidance: arrays are always passed by reference...
void SerialConsole::setup(SC_callbackFunc callbacks[], int numCallbacks) {
    _comms->println(F(kSC_Banner));
    _callbacks = callbacks;
    _numCallbacks = numCallbacks;
    //_numCallbacks = sizeof(&callbacks) / sizeof(SC_callbackFunc);
    //Serial << "Callbacks size =" << (int)sizeof(callbacks) << ", struct size =" << (int)sizeof(SC_callbackFunc);
}

void SerialConsole::SetHotkeys(SC_callbackFunc callbacks[]) {
    _HotKeyCallbacks = callbacks;
    _HotKeysEnabled = true;
}
void SerialConsole::DisableHotkeys() {
    _HotKeysEnabled = false;
}

void SerialConsole::RecieveInput() {
    bool CheckForMatch = false;
    while (_comms->available()) {
        // get the new byte:
        char inChar = (char)_comms->read();
        //echo back to the console (note: at some point might add override for pwds)
        if(!_HotKeysEnabled) {
            if(inChar == '\t') { //lets try command completion!
                int x;
                bool TabRecycleX = false;
                if(_JustTabCompleted) {
                    x = _TabCompleteNextCallback+1; //begin looking at the next option in the list
                    _buff=_buff.substring(0,_TabCompleteBuffIndex); //reset the buffer to what it was before
                    TabRecycleX = true; //allow us to look from the beginning if naught was found
                    //_JustTabCompleted = false;
                } else {
                    x = 0;
                }
                for(;x<_numCallbacks;x++) {
                    if(_Debug) Serial <<F("Checking callback x=")<<x<<F(" (of ")<<_numCallbacks<<F(") for a tab completion match against \"")<< _buff << "\"" << endl;
                    if(_buff == _callbacks[x].fnName.substring(0,_buff.length())) {
                        _JustTabCompleted = true;
                        _TabCompleteBuffIndex = _buff.length();
                        _TabCompleteNextCallback = x;
                        //Serial << _callbacks[x].fnName.substring(_buff.length()); //put the remainder of the command on the screen
                        //below fixes above in all cases inc. a second tab
                        Serial.write(27);
                        Serial.print("[2K"); // clear line
                        Serial.write(27);
                        Serial.print("[1G"); // Move cursor to start of line
                        Serial << _callbacks[x].fnName;
                        
                        _buff = _callbacks[x].fnName;
                        if(_Debug) Serial << endl << F("We found a match for \"") << _callbacks[x].fnName << "\"! (press enter to fire it)" << endl;
                        Serial << kSC_prompt;
                        break;
                    }
                    if(TabRecycleX && (x==_numCallbacks-1)) {
                        if(_Debug) Serial << endl << F("Rolling X over on tab completion (x =") << x << ")" << endl;
                        x = -1; //if we hit the end while tabbing without finding anything, begin again
                    }
                }
                //nb: since we continue on all tabs even if we didn't find anything, this means tab can't otherwise be used (seems ok?)
                continue;
            } else if(inChar != 27) {
                _JustTabCompleted = false;   
            }
            
            if(inChar == 27) { //Abort! YOLO!    
                if(_Debug) Serial << F("Escape hit! clearing line, returning character to the start of the line.") << endl;
                Serial.write(27);
                Serial.print("[2K"); // clear line
                Serial.write(27);
                Serial.print("[1G"); // Move cursor to start of line (hopefully??)
                if(_JustTabCompleted) {
                    _buff=_buff.substring(0,_TabCompleteBuffIndex);
                    Serial << _buff;
                    _JustTabCompleted = false; //allows escape 2x to clear all
                } else {
                    _buff = "";
                }
                continue;
            }
            _comms->write(inChar);
            
            //handle backspace
            if(inChar == (char)127) {
                if(_buff.length()>0) { //if the buffer is empty, just ignore.
                    //Serial << "removing last char:" << _buff;
                    _buff = _buff.substring(0, _buff.length()-1); //remove last char from buff...
                    //Serial << ", now:" << _buff;
                }
                continue;
            }
        }
        
        if (inChar == '\n' || inChar == '\r') {
            if( (char)_comms->peek() == '\n' ) { //this means we either got \r\n or \n\n
                _comms->read();                 //in either case, lets ignore the second one
            }
            _comms->println();
            CheckForMatch = true;
        }    
                
        if(!_HotKeysEnabled) {       
            if(CheckForMatch) { 
                String cmd_only = _buff.substring(0, _buff.indexOf(' '));
                //if(kSC_CaseInsensitive) cmd_only.toLowerCase(); //note Arduino and their crackhead ways, toLowerCase() actually modifies the source string
                bool FoundMatch = false;
                //note buffer flush moved up here because of issues w/ recieveinput inside of a func called from here
                String buffCopy = _buff;
                _buff = ""; //flush the buff!!!
                for(int x=0;x<_numCallbacks;x++) {
                    if(_Debug) Serial << F("Checking callback x=") << x << endl;
                    if(cmd_only == _callbacks[x].fnName) {
                        FoundMatch = true;
                        if(_Debug) Serial << F("Trying to run callback \"") << _callbacks[x].fnName << "\"" << endl;
                        (*_callbacks[x].fn)(&buffCopy);
                        Serial << kSC_prompt;
                        break;
                    }
                }
                if(FoundMatch == false) { //no command was found, fire default
                    if(_Debug) _comms->println(F("Trying to run default callback"));
                    (*_callbacks[0].fn)(&buffCopy);
                    Serial << kSC_prompt;
                }
                
            } else {
                //don't actually want the newline in the buffer...
                //TODO: if inChar == [backspace] pop last char off _buff (or ignore on empty buff)
                //TODO: confirm buffer isn't full...
                // add it to the inputString:
                _buff += inChar;
                
            }
        } else { // we're in hotkey mode
            bool FoundMatch = false;
            String buffCopy(inChar);
            for(int x=0;x<_numHotKeyCallbacks;x++) {
                if(_Debug) Serial << F("Checking hotkey x=") << x << endl;
                if(buffCopy == _HotKeyCallbacks[x].fnName) {
                    FoundMatch = true;
                    if(_Debug) Serial << F("Trying to run hotkey callback '") << _HotKeyCallbacks[x].fnName << "'" << endl;
                    (*_HotKeyCallbacks[x].fn)(&buffCopy);
                    break;
                }
            }
            if(FoundMatch == false) { //no command was found, fire default
                if(_Debug) _comms->println(F("Trying to run default hotkey callback"));
                (*_callbacks[0].fn)(&buffCopy);
            }
        }
  }
}

// ========== Built in serial command (functions) ============
void SC_DefaultResponse(String *arg) {
  Serial << F("Unrecognized command: ") << *arg << endl;
}
void SC_TestCmd(String *arg) {
  Serial << F("TestCmd fired with [") << *arg << "]" << endl;
}
/*
void listCommands(String *arg) {
  console.SC_list_commands();
}*/

void SC_cls(String *arg) {
  Serial.write(27);
  Serial.print("[2J"); // clear screen
  Serial.write(27); // ESC
  Serial.print("[H"); // cursor to home
}
#endif
