#ifndef SerialConsole_h
#define SerialConsole_h

#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Stream.h"

// ========== Constants ============

#if !defined(kSC_Banner)
#define kSC_Banner "ASC - Arduino Serial Console\r\nv0.2 - by Kevin White   -   Now with tab completion!" // default modt banner, printed at startup
#endif
#if !defined(kSC_prompt)
#define kSC_prompt "" //default text prompt... also, as of v0.2 this doesn't really work right (display issues)
#endif

#define kSC_InputBuffSize 50 //Buffer size
//#define kSC_MaxNumCommands 15 //increase as needed, consumes more memory per each
//note: the above now superceeded by improved code, see helper below:
template<typename T, size_t SIZE>
size_t getSize(T (&)[SIZE]) {
    return SIZE;
}
#define kSC_MaxNumHotKeyCommands 3 //increase as needed, consumes more memory per each

//note the 'console.SC_list_commands' part is kind of hackey, should fix one day...
//#define SC_builtin_commands {"", SC_DefaultResponse}, {"test", SC_TestCmd}, {"?", console.SC_list_commands}, {"cls", SC_cls}
#define SC_builtin_commands {"", SC_DefaultResponse}, {"test", SC_TestCmd}, {"cls", SC_cls}

#define kSC_CaseInsensitive true //ignore case on commands

extern "C" {
  typedef void (*SerialConsoleCallbackFunc)(void);
}

typedef struct {
  String fnName;
  void (*fn)(String *arg);
  //void (*fn)(string cmd, string arg);  //not sure if I can overload like this?... future version :)
} SC_callbackFunc;

class SerialConsole {  
    protected:
        HardwareSerial *_comms;
        String _buff;
        SC_callbackFunc *_callbacks, *_HotKeyCallbacks;
        int _numCallbacks, _numHotKeyCallbacks;
        bool _HotKeysEnabled;
        bool _JustTabCompleted;
        int _TabCompleteBuffIndex;
        int _TabCompleteNextCallback;
    
    public:
        SerialConsole(HardwareSerial &comms);
        void RecieveInput();
        void SetHotkeys(SC_callbackFunc callbacks[]);
        void DisableHotkeys();
        void setup(SC_callbackFunc callbacks[], int numCallbacks);
        void SC_list_commands();
        //void AttachCommand(char *string, SerialConsoleCallbackFunc CallBackFunc);
        
        bool _Debug;
};

//=========== printf helper ===========
int printf_to_serial( char c, FILE *t);

// ========== Built in serial command (functions) ============
void SC_DefaultResponse(String *arg);
void SC_TestCmd(String *arg);
//void listCommands(String *arg);
void SC_cls(String *arg);

#endif
