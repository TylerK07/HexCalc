#ifndef HARDWARE_H
#define HARDWARE_H

/*
  ___ ___                _________        .__          
 /   |   \   ____ ___  __\_   ___ \_____  |  |   ____  
/   -~-   \_/ __ \\  \/  /    \  \/\__  \ |  | _/ ___\ 
\    |    /\  ___/ >    <\     \____/ __ \|  |_\  \___ 
 \___|_  /  \___  >__/\_ \\______  (____  /____/\___  >
       \/       \/      \/       \/     \/          \/ 
        
HexCalc Firmware source code designed to run on the AVR128DA28.
Copyright (C) 2024 Tyler Klein (Things Made Simple)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.


--- Description: ---
This library abstracts the hardware specifics of the HexCalc module 
into a set of events and values that can be used by the rest of the
code.
*/


/*******************************************
* Hardware Input Tracking                  *
*******************************************/

// KEYPAD PINS:
#define PIN_COL_0        PIN_PA7      // Keypad Column Output 0
#define PIN_COL_1        PIN_PC0      // Keypad Column Output 1
#define PIN_COL_2        PIN_PC1      // Keypad Column Output 2
#define PIN_COL_3        PIN_PC2      // Keypad Column Output 3
#define PIN_COL_4        PIN_PC3      // Keypad Column Output 4

#define PIN_ROW_0        PIN_PD0      // Keypad Row Input 0
#define PIN_ROW_1        PIN_PD1      // Keypad Row Input 1
#define PIN_ROW_2        PIN_PD2      // Keypad Row Input 2
#define PIN_ROW_3        PIN_PD3      // Keypad Row Input 3
#define PIN_ROW_4        PIN_PD4      // Keypad Row Input 4
#define PIN_ROW_5        PIN_PD5      // Keypad Row Input 5
#define PIN_ROW_6        PIN_PD6      // Keypad Row Input 6

volatile uint8_t key_cols[5] = { PIN_COL_0, PIN_COL_1, PIN_COL_2, PIN_COL_3, PIN_COL_4 };
#define KEY_NUM_COLS     5
#define KEY_NUM_ROWS     7

#define UPDATE_PERIOD    100          // Number of ms between analog reads
#define LONG_PRESS_TIME  500

/*******************************************
* Mapped Key Names                         *
*******************************************/
// Maps the button number (calculated by row * KEY_NUM_COLS + col) to a more desirable order shown in the list below
//volatile uint8_t button_map[35] = {8,12,7,2,11,6,1,10,5,0,22,17,21,16,20,15,3,13,4,9,14,19,24,26,31,30,28,27,25,32,29,23,18,33,34};
volatile uint8_t button_map[35] = {9,6,3,16,18,8,5,2,0,19,7,4,1,17,20,15,13,11,32,21,14,12,10,31,22,28,23,27,26,30,25,24,29,33,34};

#define KEY_0         0
#define KEY_1         1
#define KEY_2         2
#define KEY_3         3
#define KEY_4         4
#define KEY_5         5
#define KEY_6         6
#define KEY_7         7
#define KEY_8         8
#define KEY_9         9
#define KEY_A         10
#define KEY_B         11
#define KEY_C         12
#define KEY_D         13
#define KEY_E         14
#define KEY_F         15
#define KEY_00        16
#define KEY_FF        17
#define KEY_EQUALS    18
#define KEY_MULT      19
#define KEY_DIV       20
#define KEY_MINUS     21
#define KEY_PLUS      22
#define KEY_XOR       23
#define KEY_AND       24
#define KEY_OR        25
#define KEY_NOR       26
#define KEY_ROL       27
#define KEY_ROR       28
#define KEY_LSHIFT    29
#define KEY_RSHIFT    30
#define KEY_1S        31
#define KEY_2S        32
#define KEY_ALT       33
#define KEY_CLR       34
#define KEY_R_DN      39
#define KEY_G_DN      40
#define KEY_B_DN      41
#define KEY_R_UP      42
#define KEY_G_UP      43
#define KEY_B_UP      44
#define KEY_BYTE_FLIP 45
#define KEY_WORD_FLIP 46
#define KEY_8_BIT     47
#define KEY_16_BIT    48
#define KEY_32_BIT    49
#define KEY_64_BIT    50
#define KEY_MOD       55
#define KEY_RGB_565   57
#define KEY_RGB_888   58
#define KEY_BASE_8    59
#define KEY_BASE_10   60
#define KEY_BASE_16   61
#define KEY_X_ROL_Y   62
#define KEY_X_ROR_Y   63
#define KEY_X_LS_Y    64
#define KEY_X_RS_Y    65
#define KEY_ALL_CLEAR 69


#define KEY_1         1
#define KEY_2         2
#define KEY_3         3
#define KEY_00        16
#define KEY_0         0
#define KEY_FF        17





/*******************************************
 * Primary Hardware Class Definition       *
 *******************************************/

class Hardware{
  private:
    uint32_t next_update = 0;
    void (*cb_keyPress)()   = NULL;                                            // Event function pointer for when a key gets pressed

  public:

    Hardware();                                                                // Constructor
    void setup();                                                              // Setup function
    void processEvents();                                                      // Process Events Function

    void onKeyPress( void (*fn)() ){ cb_keyPress = fn; }                       // Assign callback function for pressing a key

    int8_t key_state[35] = {0};
    int8_t last_pressed_button = -1;                                           // Contains the raw button code of the last pressed button
    int8_t last_pressed_key = -1;                                              // Contains the raw key code of the last pressed button
};

Hardware::Hardware(){};



/*******************************************
* SETUP                                    *
*******************************************/

void Hardware::setup(){
  // Screen Setup
  pinMode( PIN_COL_0, INPUT_PULLUP );
  pinMode( PIN_COL_1, INPUT_PULLUP );
  pinMode( PIN_COL_2, INPUT_PULLUP );
  pinMode( PIN_COL_3, INPUT_PULLUP );
  pinMode( PIN_COL_4, INPUT_PULLUP );
  pinMode( PIN_ROW_0, OUTPUT );
  pinMode( PIN_ROW_1, OUTPUT );
  pinMode( PIN_ROW_2, OUTPUT );
  pinMode( PIN_ROW_3, OUTPUT );
  pinMode( PIN_ROW_4, OUTPUT );
  pinMode( PIN_ROW_5, OUTPUT );
  pinMode( PIN_ROW_6, OUTPUT );
}


void Hardware::processEvents(){
  uint8_t row;
  uint8_t buttonIndex;

  if( millis() < next_update ) return;
  next_update += UPDATE_PERIOD;

  for( row=0; row < KEY_NUM_ROWS; row++ ){
    digitalWrite( 12+row, HIGH );
  }

  for( row=0; row < KEY_NUM_ROWS; row++ ){
    digitalWrite( 12+row, LOW );                                               // Set the row to low which will pull all of the connected columns to zero
    
    for( uint8_t col=0; col < KEY_NUM_COLS; col++ ){
      buttonIndex = row * KEY_NUM_COLS + col;                                  // Determine the button index
      if( digitalRead( key_cols[col] ) ){                                      // Key not pressed
        key_state[buttonIndex] = 0;                                            // Reset the key state entry to show the button is turned off
        if( last_pressed_button == buttonIndex ) last_pressed_button = -1;     // Reset last pressed button to show it's turned off
      } else {                                                                 // 
        switch( key_state[buttonIndex] ){                                      // Using a switch statement here so I can maybe add more advanced debouncing or press and hold
          case 0:                                                              // If the button is currently off
            key_state[buttonIndex] = 1;                                        // Set it to on
            last_pressed_button = buttonIndex;                                 // Update the last pressed button
            if( last_pressed_button != KEY_ALT ){                              // If we pressed the alt key, don't trigger an event
              last_pressed_key = button_map[last_pressed_button] + (key_state[KEY_ALT] == 0 ? 0 : 35); // Determine the key code
              if( cb_keyPress ) cb_keyPress();                                 // Trigger the callback keypressd function if it's been hooked up
            }
            break;
        }
      }
    }
    digitalWrite( 12+row, HIGH );                                              // Set the row back to high so it doesn't conflict
  }

}

#endif