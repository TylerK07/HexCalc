#include "hardware.h"
#include "calculator.h"
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include "font.h"


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
This file provides the main primary loop of the HexCalc module and
displays all of the visualizations and menus. The code is definitely
in the "make it work" stage and could probably use a pretty significant
overhaul.

*/

Hardware    hw;         // Manages the keyboard presses
Calculator  calc;


/*******************************************
* Screen Defaults & Setup                  *
*******************************************/

// The pins for I2C are defined by the Wire-library. 
// Library Assumes: SDA - PA2, SCL - PA3
#define SCREEN_WIDTH   240            // OLED display width, in pixels
#define SCREEN_HEIGHT  240            // OLED display height, in pixels
#define OLED_RESET     -1             // Reset pin # (or -1 if sharing Arduino reset pin)

#define SCREEN_VISIBLE_COLS 21        // The number of visible columns on the screen (not the entire buffer)
#define SCREEN_BUFFER_COLS  42        // The number of columns in the buffer
#define SCREEN_BUFFER_ROWS   8        // The number of rows in the buffer


// SCREEN PINS:
#define PIN_SCREEN_CLOCK PIN_PA6      // SPI Clock
#define PIN_SCREEN_DATA  PIN_PA4      // SPI Data (MOSI)
#define PIN_SCREEN_DC    PIN_PA3      // Data Command
#define PIN_SCREEN_RESET PIN_PA2      // Reset
#define PIN_SCREEN_BLK   PIN_PA1      // Blank Screen

Adafruit_ST7789 screen(&SPI, -1, PIN_SCREEN_DC, PIN_SCREEN_RESET);

/*******************************************
* STYLES / COLORS                          *
*******************************************/
#define COLOR_NUM_BG 0x0821           // Color used in the background behind the main numbers
#define COLOR_HEX_FG 0x07E0           // Color used in the foreground in hexidecimal mode
#define COLOR_DEC_FG 0x20BF           // Color used in the foreground in decimal mode
#define COLOR_OCT_FG 0xF8C3           // Color used in the foreground in octal mode
#define COLOR_COL_FG 0xEF7D           // Color used for the color chips
#define COLOR_GHOST  0x4162           // Color used for ghost digits

#define COLOR_RED    0xC000           // Color used for Red in the color menu
#define COLOR_GREEN  0x0680           // Color used for Green in the color menu
#define COLOR_BLUE   0x0018           // Color used for Blue in the color menu


/*******************************************
* General Setup Function                   *
*******************************************/

void setup() {
  screen.init(SCREEN_WIDTH, SCREEN_HEIGHT, SPI_MODE2);
  pinMode( PIN_SCREEN_BLK, OUTPUT );
  digitalWrite( PIN_SCREEN_BLK, HIGH );

  delay(10);
  screen.setRotation(3);
  screen.cp437(true);
  screen.setCursor(0, 0);
  screen.fillScreen(   ST77XX_BLACK );
  screen.setTextColor( ST77XX_GREEN );
  screen.setTextWrap(true);


  hw.setup();                                                                  // Initialize the hardware library
  hw.onKeyPress( manageKeyPress );

  renderScreen();
}

#define MENU_BINARY 0 // Binary Menu
#define MENU_DEC    1 // Decimal Menu
#define MENU_COLOR  2 // Color Selector Menu
volatile uint8_t menu_mode = 0;
volatile uint8_t old_menu_mode = 0xFF;


void manageKeyPress(){


  bool refresh_screen = true;

  switch( hw.last_pressed_key ){
    case KEY_0:         calc.enterDigit(0);     break;
    case KEY_1:         calc.enterDigit(1);     break;
    case KEY_2:         calc.enterDigit(2);     break;
    case KEY_3:         calc.enterDigit(3);     break;
    case KEY_4:         calc.enterDigit(4);     break;
    case KEY_5:         calc.enterDigit(5);     break;
    case KEY_6:         calc.enterDigit(6);     break;
    case KEY_7:         calc.enterDigit(7);     break;
    case KEY_8:         calc.enterDigit(8);     break;
    case KEY_9:         calc.enterDigit(9);     break;
    case KEY_A:         calc.enterDigit(10);    break;
    case KEY_B:         calc.enterDigit(11);    break;
    case KEY_C:         calc.enterDigit(12);    break;
    case KEY_D:         calc.enterDigit(13);    break;
    case KEY_E:         calc.enterDigit(14);    break;
    case KEY_F:         calc.enterDigit(15);    break;
    case KEY_00:        calc.enterDigit(0);  calc.enterDigit(0);  break;
    case KEY_FF:        calc.enterDigit(15); calc.enterDigit(15); break;

    case KEY_EQUALS:    calc.equals();          break;
    case KEY_MULT:      calc.multiplyBy();      break;
    case KEY_DIV:       calc.divideBy();        break;
    case KEY_MINUS:     calc.minusBy();         break;
    case KEY_PLUS:      calc.plusBy();          break;
    case KEY_MOD:       calc.modBy();           break;

    case KEY_XOR:       calc.xorWith();         break;
    case KEY_AND:       calc.andWith();         break;
    case KEY_OR:        calc.orWith();          break;
    case KEY_NOR:       calc.norWith();         break;

    case KEY_ROL:       calc.ror();             break;
    case KEY_ROR:       calc.rol();             break;
    case KEY_LSHIFT:    calc.leftShift();       break;
    case KEY_RSHIFT:    calc.rightShift();      break;
    case KEY_X_ROL_Y:   calc.rolBy();           break;
    case KEY_X_ROR_Y:   calc.rorBy();           break;
    case KEY_X_LS_Y:    calc.leftShiftBy();     break;
    case KEY_X_RS_Y:    calc.rightShiftBy();    break;

    case KEY_1S:        calc.onesCompliment();  break;
    case KEY_2S:        calc.twosCompliment();  break;
    case KEY_BYTE_FLIP: calc.byteFlip();        break;
    case KEY_WORD_FLIP: calc.wordFlip();        break;

    case KEY_CLR:       calc.clear();           break;
    case KEY_ALL_CLEAR: calc.allClear();        break;

    case KEY_R_DN:      calc.decRed();          break;
    case KEY_G_DN:      calc.decGreen();        break;
    case KEY_B_DN:      calc.decBlue();         break;
    case KEY_R_UP:      calc.incRed();          break;
    case KEY_G_UP:      calc.incGreen();        break;
    case KEY_B_UP:      calc.incBlue();         break;

    case KEY_8_BIT:     calc.setBitDepth8();    break;
    case KEY_16_BIT:    calc.setBitDepth16();   break;
    case KEY_32_BIT:    calc.setBitDepth32();   break;
    case KEY_64_BIT:    calc.setBitDepth64();   break;

    case KEY_RGB_565:   calc.setColorMode565(); menu_mode = MENU_COLOR;  break;
    case KEY_RGB_888:   calc.setColorMode888(); menu_mode = MENU_COLOR;  break;
    case KEY_BASE_8:    calc.setBase8();        menu_mode = MENU_BINARY; break;
    case KEY_BASE_10:   calc.setBase10();       menu_mode = MENU_DEC;    break;
    case KEY_BASE_16:   calc.setBase16();       menu_mode = MENU_BINARY; break;
    default: refresh_screen = false;
  }

  if( refresh_screen ) renderScreen();
}

/*******************************************
* Draw Tag    Functions                    *
*******************************************/
/* LEFT CAP:
   0x1C: 0001 1100
   0x3E: 0011 1110
   0x7F: 0111 1111
   0x7F: 0111 1111
   0x7F: 0111 1111
   0x7F: 0111 1111
 
 * RIGHT CAP:
   0x7F: 0111 1111
   0x7F: 0111 1111
   0x7F: 0111 1111
   0x7F: 0111 1111
   0x3E: 0011 1110
   0x1C: 0001 1100
*/
uint8_t left_char[6]  = { 0x1C, 0x3E, 0x7F, 0x7F, 0x7F, 0x7F };
uint8_t right_char[6] = { 0x7F, 0x7F, 0x7F, 0x7F, 0x3E, 0x1C };

uint64_t old_val        = 0xFFFFFFFFFFFFFFFF;
uint64_t old_stored     = 0xFFFFFFFFFFFFFFFF;
uint8_t  old_op         = 0xFF;
uint8_t  old_base       = 0xFF;
uint8_t  old_color_mode = 0xFF;
uint8_t  old_bit_depth  = 0xFF;



// Before running this command make sure DC is high
void drawTag( uint8_t x, uint8_t y, uint8_t scale_x, uint8_t scale_y, uint16_t fg_color, uint16_t bg_color, const char *lbl, uint8_t label_length, bool right_align = false ){
  uint8_t widget_width  = CHAR_WIDTH  * label_length * scale_x + 2 * CHAR_WIDTH;
  uint8_t widget_height = CHAR_HEIGHT * scale_y + 2;
  int16_t colChar = 0;                                                         // 
  uint8_t colStep = 0;
  uint8_t rowStep = 0;                                                         // 
  uint8_t hPixStep = scale_x;
  uint8_t vPixStep = scale_y;
  uint16_t charIndex = 0;
  if( right_align ) x -= widget_width;

  digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen


  uint8_t line_left   =  x + 4;
  uint8_t line_length = (CHAR_WIDTH*label_length) * scale_x + 4;
  screen.setAddrWindow( line_left, y, line_length, 1 );
  for( uint16_t i = line_length; i>0; i--){ screen.SPI_WRITE16( fg_color ); }  // Draw a line on top of the tag

  screen.setAddrWindow( line_left, y + widget_height - 1, line_length, 1 );    // Draw a line on the bottom of the tag
  for( uint16_t i = line_length; i>0; i--){ screen.SPI_WRITE16( fg_color ); }  

  screen.setAddrWindow( x, y+1, widget_width, widget_height-2 ); 
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ

  for( uint8_t row = 0; row<widget_height-2; row++ ){                              // The outer loop goes through each column
    colChar = -1;                                                              // Set the current column to zero
    colStep = CHAR_WIDTH;
    for( uint8_t col = 0; col<widget_width; col++ ){                           // 
      if( colStep == CHAR_WIDTH ){                                             // See if we have reached the end of the columns in a character
        colStep = 0;                                                           // Reset the character column to zero
        colChar++;                                                             // Increment the character index that points to the buffer
        charIndex = lbl[ colChar-1 ] * CHAR_WIDTH;                             // Assumes 32 characters per row, 5 columns per char
      }

      if( colChar == 0 ){
        if( (left_char[colStep] & (1<<rowStep)) > 0 ){
          screen.SPI_WRITE16( fg_color );
        } else {
          screen.SPI_WRITE16( ST77XX_BLACK );
        }
      } else if( colChar == label_length+1){
        if( (right_char[colStep] & (1<<rowStep)) > 0 ){
          screen.SPI_WRITE16( fg_color );
        } else {
          screen.SPI_WRITE16( ST77XX_BLACK );
        }
      } else {
        if( (font5x7[charIndex + colStep] & (1<<rowStep)) > 0 ){
          screen.SPI_WRITE16( bg_color );
        } else {
          screen.SPI_WRITE16( fg_color );
        }
      }
      if( (colChar == 0) || (colChar == label_length+1) ){
        colStep++;
      } else {
        hPixStep--;
        if( hPixStep == 0 ){
          colStep++;
          hPixStep = scale_x;
        }
      }
    }
    vPixStep--;
    if( vPixStep == 0 ){
      rowStep++;
      vPixStep = scale_y;
    }
  }
  SPI.endTransaction();                                                        // Close out the SPI transaction

}




/*******************************************
* Draw Number Functions                    *
*******************************************/
void drawSmallNumber( uint8_t base, uint8_t x, uint8_t y, uint16_t fg_color, uint64_t val ){
  char buffer[27] = {'0'};


  // Set the display settings based on the bit depth (number of digits) and the base. These will all be overwritten
  uint8_t widget_top    = 20;
  uint8_t widget_height = 28;           // The height of the numbers widget
  uint8_t widget_width  = 180;
  uint8_t pixel_width   = 1;            // The width of a font pixel (used for scaling the numbers)
  uint8_t pixel_height  = 1;            // The height of a font pixel (used for scaling the nubmers)
  uint8_t num_digits    = 1;            // The number of digits in the number
  uint8_t spacing       = 1;            // The number of additional pixels to put between each number

  // Temporary scratch variables
  uint64_t tmp          = 0;            // A temporary variable used to hold the current value as it is continuously divided by 10
  uint8_t  rem          = 0;            // A temporary variable used to hold the remainder of the tmp division
  int16_t  colCharStart = -1;           // Tracks the current character position in the buffer that we are rendering

  // Override the settings for the render loop based on the current state of the calculator (base & bitDepth)
  if( 16 == base ){
    sprintf(buffer, "%04X %04X %04X %04X", uint16_t(val>>48),uint16_t(val>>32),uint16_t(val>>16),uint16_t(val) );
    switch( calc.bitDepth ){
      case 8:  pixel_width = 2;  pixel_height = 3; num_digits = 2;   colCharStart = 16; spacing = 2; break;
      case 16: pixel_width = 2;  pixel_height = 3; num_digits = 4;   colCharStart = 14; spacing = 1; break;
      case 24: pixel_width = 2;  pixel_height = 3; num_digits = 7;   colCharStart = 11; spacing = 0; break;
      case 32: pixel_width = 2;  pixel_height = 3; num_digits = 9;   colCharStart =  9; spacing = 1; break;
      case 64: pixel_width = 1;  pixel_height = 2; num_digits = 19;  colCharStart = -1; spacing = 1; break;
    }
  } else if( 8 == base ){
    sprintf(buffer, "%04o %04o %04o %04o", uint16_t((val>>36) & 0xFFF),uint16_t((val>>24) & 0xFFF),uint16_t((val>>12) & 0xFFF),uint16_t((val) & 0xFFF) );

    switch( calc.bitDepth ){
      case 8:  pixel_width = 2;  pixel_height = 3; num_digits = 2;   colCharStart = 16; spacing = 2; break;
      case 16: pixel_width = 2;  pixel_height = 3; num_digits = 4;   colCharStart = 14; spacing = 1; break;
      case 24: pixel_width = 2;  pixel_height = 3; num_digits = 7;   colCharStart = 11; spacing = 0; break;
      case 32: pixel_width = 2;  pixel_height = 3; num_digits = 9;   colCharStart =  9; spacing = 1; break;
      case 64: pixel_width = 1;  pixel_height = 2; num_digits = 19;  colCharStart = -1; spacing = 1; break;
    }
  } else {
    num_digits = 0;
    tmp = val;
    for( int8_t i=26; i>=0; i-- ){
      if( tmp > 0 ){
        if( i % 4 == 3 ){
          buffer[i] = ',';
        } else {
          rem = tmp % 10;
          buffer[i] = 0x30 + rem; // character for "0" + [0..9]
          tmp /= 10;
        }
        num_digits++;
      } else {
        buffer[i] = 0x30;        // add a leading zero
      }
    }
    if( num_digits == 0 ) num_digits = 1;
    if(        num_digits > 17 ){ pixel_width = 1; pixel_height = 2; spacing = 1; }
      else if( num_digits > 11 ){ pixel_width = 2; pixel_height = 2; spacing = 1; }
      else if( num_digits > 8  ){ pixel_width = 2; pixel_height = 3; spacing = 1; }
      else if( num_digits > 4  ){ pixel_width = 2; pixel_height = 3; spacing = 1; }
      else                      { pixel_width = 2; pixel_height = 3; spacing = 1; }

    colCharStart = 26 - num_digits;
  }

  // Define some more temporary scratch variables to use in the rendering loop
  bool     isLeadingZero = true;                                               // Used during the loop to track if we are rendering a leading zero in the number
  bool     isComma       = false;                                              // Used during the loop to track if we are currently rendering a comma
  int16_t  colChar = colCharStart;                                             // 
  uint8_t  rowStep = 0;                                                        // 
  uint8_t  charWidth = (CHAR_WIDTH + spacing) * pixel_width;                   // 
  uint8_t  val_width = num_digits * charWidth - (spacing * pixel_width);       //
  uint8_t  val_height = CHAR_HEIGHT * pixel_height;                            // Max of 4*7 = 28

  uint8_t  colStep = CHAR_WIDTH + spacing;
  uint8_t  hPixStep = pixel_width;
  uint8_t  vPixStep = pixel_height;
  uint16_t charIndex;

  digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen
  
  // Fill in the box to the left of the type so gets cleared out
  screen.setAddrWindow( x, y, widget_width-val_width, widget_height );         // Set the address area of the window to fill
  for( uint16_t i = (widget_width-val_width) * widget_height; i>0; i--){
        screen.SPI_WRITE16( ST77XX_BLACK );    
  }
  // Fill in the box above the type so gets cleared out as it gets smaller
  screen.setAddrWindow( x+widget_width-val_width, y, val_width, widget_height-val_height ); // Set the address area of the window to fill
  for( uint16_t i = val_width * (widget_height-val_height); i>0; i--){
        screen.SPI_WRITE16( ST77XX_BLACK );    
  }

  screen.setAddrWindow( x+widget_width-val_width, y+widget_height-val_height, val_width, val_height );   // Set the address area of the window to fill

  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ


  for( uint8_t row = 0; row<val_height; row++ ){                               // The outer loop goes through each column
    colChar = colCharStart;                                                    // Set the current column to the starting column defined in the select section above
    isLeadingZero = true;                                                      // Since we started the line again, we have to reset the leading zero indicator for this row
    colStep = CHAR_WIDTH + spacing;                                            // Reset colStep to the max value so it triggers the if statement at the beginning of the loop
    for( uint8_t col = 0; col<val_width; col++ ){                              // The outer loop goes through each column
      if( colStep == CHAR_WIDTH + spacing ){                                   // See if we have reached the end of the columns in a character
        colStep = 0;                                                           // Reset the character column to zero
        colChar++;                                                             // Increment the character index that points to the buffer
        charIndex = buffer[ colChar ] * 6;                                     // Assumes 32 characters per row, 5 columns per char

        if( isLeadingZero ){                                                   // If haven't hit a number yet
          if( (charIndex != '0' * CHAR_WIDTH) && (charIndex != ' ' * CHAR_WIDTH)) isLeadingZero = false; // See if the current charIndex is pointing to a zero
        }
        isComma = ( charIndex == ',' * CHAR_WIDTH );
      }

      if( (val != 0) && (colStep<CHAR_WIDTH) && ((font5x7[charIndex + colStep] & (1<<rowStep)) > 0) ){
        screen.SPI_WRITE16( (isLeadingZero || isComma) ? COLOR_GHOST : fg_color );
      } else {
        screen.SPI_WRITE16( ST77XX_BLACK );
      }
      hPixStep--;
      if( hPixStep == 0 ){
        colStep++;
        hPixStep = pixel_width;
      }
    }
    vPixStep--;
    if( vPixStep == 0 ){
      rowStep++;
      vPixStep = pixel_height;
    }
  }
  SPI.endTransaction();                                                        // Close out the SPI transaction

}


void drawLargeNumber(){
  char buffer[27] = {'0'};


  // Set the display settings based on the bit depth (number of digits) and the base. These will all be overwritten
  uint8_t widget_top    = 54;           // The vertical position of the numbers widget
  uint8_t widget_height = 56;           // The height of the numbers widget
  uint8_t pixel_width   = 1;            // The width of a font pixel (used for scaling the numbers)
  uint8_t pixel_height  = 1;            // The height of a font pixel (used for scaling the nubmers)
  uint8_t num_digits    = 1;            // The number of digits in the number
  uint8_t spacing       = 1;            // The number of additional pixels to put between each number
  uint16_t fg_color     = COLOR_HEX_FG; // Holds the current foreground color to use when rendering the number

  // Temporary scratch variables
  uint64_t tmp          = 0;            // A temporary variable used to hold the current value as it is continuously divided by 10
  uint8_t  rem          = 0;            // A temporary variable used to hold the remainder of the tmp division
  int16_t  colCharStart = -1;           // Tracks the current character position in the buffer that we are rendering

  // Override the settings for the render loop based on the current state of the calculator (base & bitDepth)
  if( 16 == calc.base ){
    sprintf(buffer, "%04X%04X%04X%04X", uint16_t(calc.val_current>>48),uint16_t(calc.val_current>>32),uint16_t(calc.val_current>>16),uint16_t(calc.val_current) );
    fg_color = (menu_mode == MENU_COLOR) ? COLOR_COL_FG : COLOR_HEX_FG;
    switch( calc.bitDepth ){
      case 8:  pixel_width = 16; pixel_height = 8; num_digits = 2;   colCharStart = 13; spacing = 2; break;
      case 16: pixel_width = 8;  pixel_height = 8; num_digits = 4;   colCharStart = 11; spacing = 1; break;
      case 24: pixel_width = 6;  pixel_height = 8; num_digits = 6;   colCharStart =  9; spacing = 0; break;
      case 32: pixel_width = 4;  pixel_height = 8; num_digits = 8;   colCharStart =  7; spacing = 1; break;
      case 64: pixel_width = 2;  pixel_height = 5; num_digits = 16;  colCharStart = -1; spacing = 1; break;
    }
  } else if( 8 == calc.base ){
    sprintf(buffer, "%04o%04o%04o%04o", uint16_t((calc.val_current>>36) & 0xFFF),uint16_t((calc.val_current>>24) & 0xFFF),uint16_t((calc.val_current>>12) & 0xFFF),uint16_t((calc.val_current) & 0xFFF) );
    fg_color = (menu_mode == MENU_COLOR) ? COLOR_COL_FG : COLOR_OCT_FG;
    switch( calc.bitDepth ){
      case 8:  pixel_width = 8;  pixel_height = 8; num_digits = 3;   colCharStart = 12; spacing = 2; break;
      case 16: pixel_width = 5;  pixel_height = 8; num_digits = 6;   colCharStart =  9; spacing = 1; break;
      case 24: pixel_width = 4;  pixel_height = 8; num_digits = 8;   colCharStart =  7; spacing = 1; break;
      case 32: pixel_width = 3;  pixel_height = 8; num_digits = 11;  colCharStart =  4; spacing = 1; break;
      case 64: pixel_width = 2;  pixel_height = 5; num_digits = 16;  colCharStart = -1; spacing = 1; break;
    }
  } else {
    num_digits = 0;
    tmp = calc.val_current;
    fg_color = (menu_mode == MENU_COLOR) ? COLOR_COL_FG : COLOR_DEC_FG;
    for( int8_t i=26; i>=0; i-- ){
      if( tmp > 0 ){
        if( i % 4 == 3 ){
          buffer[i] = ',';
        } else {
          rem = tmp % 10;
          buffer[i] = 0x30 + rem; // character for "0" + [0..9]
          tmp /= 10;
        }
        num_digits++;
      } else {
        buffer[i] = 0x30;        // add a leading zero
      }
    }
    if( num_digits == 0 ) num_digits = 1;
    if(        num_digits > 17 ){ pixel_width = 1; pixel_height = 3; spacing = 1; }
      else if( num_digits > 11 ){ pixel_width = 2; pixel_height = 4; spacing = 1; }
      else if( num_digits > 8  ){ pixel_width = 3; pixel_height = 5; spacing = 1; }
      else if( num_digits > 4  ){ pixel_width = 4; pixel_height = 8; spacing = 1; }
      else                      { pixel_width = 8; pixel_height = 8; spacing = 1; }

    colCharStart = 26 - num_digits;
  }

  // Define some more temporary scratch variables to use in the rendering loop
  bool     isLeadingZero = true;                                               // Used during the loop to track if we are rendering a leading zero in the number
  bool     isComma       = false;                                              // Used during the loop to track if we are currently rendering a comma
  int16_t  colChar = colCharStart;                                             // 
  uint8_t  rowStep = 0;                                                        // 
  uint8_t  charWidth = (CHAR_WIDTH + spacing) * pixel_width;                   // 
  uint8_t  val_width = num_digits * charWidth - (spacing * pixel_width);       //
  uint8_t  val_height = CHAR_HEIGHT * pixel_height;                            // Max of 4*7 = 28

  uint8_t  colStep = CHAR_WIDTH + spacing;
  uint8_t  hPixStep = pixel_width;
  uint8_t  vPixStep = pixel_height;
  uint16_t charIndex;

  digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen
  
  // Fill in the box to the left of the type so gets cleared out
  screen.setAddrWindow( 0, widget_top, SCREEN_WIDTH-val_width, widget_height );         // Set the address area of the window to fill
  for( uint16_t i = (240-val_width) * widget_height; i>0; i--){
        screen.SPI_WRITE16( COLOR_NUM_BG );    
  }
  // Fill in the box above the type so gets cleared out as it gets smaller
  screen.setAddrWindow( SCREEN_WIDTH-val_width, widget_top, val_width, widget_height-val_height ); // Set the address area of the window to fill
  for( uint16_t i = val_width * (widget_height-val_height); i>0; i--){
        screen.SPI_WRITE16( COLOR_NUM_BG );    
  }

  screen.setAddrWindow( SCREEN_WIDTH-val_width, widget_top+widget_height-val_height, val_width, val_height );   // Set the address area of the window to fill

  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ


  for( uint8_t row = 0; row<val_height; row++ ){                               // The outer loop goes through each column
    colChar = colCharStart;                                                    // Set the current column to the starting column defined in the select section above
    isLeadingZero = true;                                                      // Since we started the line again, we have to reset the leading zero indicator for this row
    colStep = CHAR_WIDTH + spacing;                                            // Reset colStep to the max value so it triggers the if statement at the beginning of the loop
    for( uint8_t col = 0; col<val_width; col++ ){                              // The outer loop goes through each column
      if( colStep == CHAR_WIDTH + spacing ){                                   // See if we have reached the end of the columns in a character
        colStep = 0;                                                           // Reset the character column to zero
        colChar++;                                                             // Increment the character index that points to the buffer
        charIndex = uint16_t(buffer[ colChar ]) * 6;                           // Assumes 32 characters per row, 5 columns per char

        if( isLeadingZero ){                                                   // If haven't hit a number yet
          if( charIndex != '0' * CHAR_WIDTH ) isLeadingZero = false;           // See if the current charIndex is pointing to a zero
        }
        isComma = ( charIndex == ',' * CHAR_WIDTH );
      }

      if( (colStep<CHAR_WIDTH) && ((font5x7[charIndex + colStep] & (1<<rowStep)) > 0) ){
        screen.SPI_WRITE16( (isLeadingZero || isComma) ? COLOR_GHOST : fg_color );
      } else {
        screen.SPI_WRITE16( COLOR_NUM_BG );
      }
      hPixStep--;
      if( hPixStep == 0 ){
        colStep++;
        hPixStep = pixel_width;
      }
    }
    vPixStep--;
    if( vPixStep == 0 ){
      rowStep++;
      vPixStep = pixel_height;
    }
  }
  SPI.endTransaction();                                                        // Close out the SPI transaction

}


// Draw a set of characters to the screen
void drawChar( uint8_t x, uint8_t y, uint8_t scale_x, uint8_t scale_y, uint8_t spacing, uint16_t fg_color, uint16_t bg_color, const char *lbl, uint8_t label_length ){
  uint8_t widget_width  = CHAR_WIDTH * label_length * scale_x + spacing * label_length;
  uint8_t widget_height = CHAR_HEIGHT * scale_y;
  uint8_t colChar = 0;                                                         // 
  uint8_t colStep = 0;
  uint8_t rowStep = 0;                                                         // 
  uint8_t hPixStep = scale_x;
  uint8_t vPixStep = scale_y;
  uint16_t charIndex = 0;

  digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen

  screen.setAddrWindow( x, y, widget_width, widget_height ); 
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ

  for( uint8_t row = 0; row<widget_height; row++ ){                            // The outer loop goes through each column
    colChar = 0;                                                              // Set the current column to zero
    colStep = CHAR_WIDTH + spacing;
    for( uint8_t col = 0; col<widget_width; col++ ){                           // 
      if( colStep == (CHAR_WIDTH + spacing) ){                                 // See if we have reached the end of the columns in a character
        colStep = 0;                                                           // Reset the character column to zero
        charIndex = uint8_t(lbl[ colChar ]) * CHAR_WIDTH;                     // Assumes 32 characters per row, 5 columns per char
        colChar++;                                                             // Increment the character index that points to the buffer
      }

      if( (colStep < CHAR_WIDTH) && ((font5x7[colStep + charIndex] & (1<<rowStep)) > 0) ){
        screen.SPI_WRITE16( fg_color );
      } else {
        screen.SPI_WRITE16( bg_color );
      }

      hPixStep--;
      if( hPixStep == 0 ){
        colStep++;
        hPixStep = scale_x;
      }
    }
    vPixStep--;
    if( vPixStep == 0 ){
      rowStep++;
      vPixStep = scale_y;
    }
  }
  SPI.endTransaction();                                                        // Close out the SPI transaction

}


void drawBinWord( uint8_t x, uint8_t y, uint8_t num_dots, uint8_t val, uint16_t color_fg ){
  uint8_t dot_width   = (num_dots == 4) ? 6 : 8;
  uint8_t dot_spacing = (num_dots == 4) ? 2 : 3;
  uint16_t widget_width = (dot_width + dot_spacing) * num_dots;
  uint16_t widget_height = CHAR_HEIGHT * 2;
  char buffer[2] = {0};
  sprintf(buffer, "%01X", val);

  drawChar( x, y, 2, 2, 0, color_fg, ST77XX_BLACK, buffer, 1 );

  digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen
  screen.setAddrWindow( x + 15, y, widget_width, widget_height );              // Set the address area of the window to fill
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ

  uint8_t colNum = 0;
  uint8_t bit_num = num_dots - 1;
  for( uint16_t i = widget_width * widget_height; i>0; i--){
    if( val & (1 << bit_num) ){
      screen.SPI_WRITE16( colNum < dot_width ? COLOR_COL_FG : ST77XX_BLACK );
    } else {
      screen.SPI_WRITE16( colNum < dot_width ? COLOR_GHOST : ST77XX_BLACK );
    }
    colNum++;
    if( colNum == (dot_width + dot_spacing) ){
      colNum = 0;
      if( bit_num-- == 0 ) bit_num = num_dots-1;
    }
  }  
  SPI.endTransaction();                                                        // Close out the SPI transaction
}

void drawBinary( uint8_t x, uint8_t y, bool full_refresh ){
  uint8_t spacing_x = 50;
  uint8_t spacing_y = 28;
  char buffer[3] = {0};

  uint8_t word = 0;
  for( uint8_t row = 0; row < 4; row++ ){
    for( uint8_t col = 0; col < 4; col++ ){
      if( (15-word) < (calc.bitDepth >> 2) ){
        if( full_refresh || (( (calc.val_current >> ((15 - (row * 4 + col)) * 4)) & 0xF ) != ( (old_val >> ((15 - (row * 4 + col)) * 4)) & 0xF )) ){
          drawBinWord( x + col * spacing_x + 40, y + row * spacing_y, 4, (calc.val_current >> ((15 - (row * 4 + col)) * 4)) & 0xF, COLOR_HEX_FG );
        }
      }
      word++;
    }
  }

  for( uint8_t i=0; i<4; i++ ){
    buffer[0] = (calc.bitDepth >> 3) > (i*2) + 1 ? (calc.val_current >> (i * 16 + 8)) & 0xFF : 0x00;
    buffer[1] = (calc.bitDepth >> 3) > (i*2)     ? (calc.val_current >> (i * 16)) & 0xFF     : 0x00;
    drawChar( x, y + spacing_y * (3-i), 2, 2, 2, COLOR_COL_FG, ST77XX_BLACK, buffer, 2 );
  }
}

void drawOctalBinary( uint8_t x, uint8_t y, bool full_refresh ){
  uint8_t spacing_x = 50;
  uint8_t spacing_y = 28;
  char buffer[3] = {0};

  for( uint8_t row = 0; row < 4; row++ ){
    for( uint8_t col = 0; col < 4; col++ ){
      if( full_refresh || (( (calc.val_current >> ((15 - (row * 4 + col)) * 3)) & 0b111 ) != ( (old_val >> ((15 - (row * 4 + col)) * 3)) & 0b111 )) ){
        drawBinWord( x + col * spacing_x + 40, y + row * spacing_y, 3, (calc.val_current >> ((15 - (row * 4 + col)) * 3)) & 0b111, COLOR_OCT_FG );
      }
    }
  }

  for( uint8_t i=0; i<4; i++ ){
    buffer[0] = (calc.val_current >> (i * 16 + 8)) & 0xFF;
    buffer[1] = (calc.val_current >> (i * 16)) & 0xFF;
    drawChar( x, y + spacing_y * (3-i), 2, 2, 2, COLOR_COL_FG, COLOR_NUM_BG, buffer, 2 );
  }
}

/*******************************************
* Color Menu Render Function               *
*******************************************/

// Value needs to be between 0 and 255
void drawProgressBar( uint8_t x, uint8_t y, uint8_t height, uint8_t width, uint16_t val, uint16_t fg_color, uint16_t bg_color ){
  digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen
  screen.setAddrWindow( x, y, width, height );                                 // Set the address area of the window to fill
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ

  uint8_t bar_width = (val * width) >> 8;

  uint8_t colNum = 0;
  for( uint16_t i = width * height; i>0; i--){
    screen.SPI_WRITE16( colNum < bar_width ? fg_color : bg_color );
    if( ++colNum == width ) colNum = 0;
  }  
  SPI.endTransaction();   
}

void drawColorMenu( uint8_t x, uint8_t y){
  bool is565 = calc.color_mode == RGB_565;
  uint8_t redVal   = is565 ? (calc.val_current >> 11) & 0b11111  : (calc.val_current >> 16) & 0xFF;
  uint8_t greenVal = is565 ? (calc.val_current >> 5)  & 0b111111 : (calc.val_current >> 8)  & 0xFF;
  uint8_t blueVal  = is565 ? (calc.val_current >> 0)  & 0b11111  : (calc.val_current >> 0)  & 0xFF;
  
  uint16_t color_565 = is565 ? calc.val_current & 0xFFFF : ( ((redVal >> 3) << 11) | ((greenVal >> 2)<<5) | (blueVal >> 3) );

  char buffer[3] = {0};

  sprintf(buffer, "%02X", redVal);
  drawChar( x, y+10, 2, 2, 1, COLOR_RED, ST77XX_BLACK, buffer, 2 );
  drawProgressBar(x+40, y+15, 4, 80, is565 ? redVal   << 3 : redVal,   COLOR_RED,   COLOR_NUM_BG );

  sprintf(buffer, "%02X", greenVal);
  drawChar( x, y+45, 2, 2, 1, COLOR_GREEN, ST77XX_BLACK, buffer, 2 );
  drawProgressBar(x+40, y+50, 4, 80, is565 ? greenVal << 2 : greenVal, COLOR_GREEN, COLOR_NUM_BG );

  sprintf(buffer, "%02X", blueVal);
  drawChar( x, y+80, 2, 2, 1, COLOR_BLUE, ST77XX_BLACK, buffer, 2 );
  drawProgressBar(x+40, y+85, 4, 80, is565 ? blueVal  << 3 : blueVal,  COLOR_BLUE,  COLOR_NUM_BG );

  digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen
  screen.setAddrWindow( x+130, y+10, 100, 100 );                           // Set the address area of the window to fill
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ
  for( uint16_t i = 100*100; i>0; i--){
    screen.SPI_WRITE16( color_565 );    
  }

}



/*******************************************
* Render Function                          *
*******************************************/

void renderScreen(){

    bool refreshBase    = calc.base != old_base;
    bool refreshBitMode = calc.bitDepth != old_bit_depth;
    bool refreshColor   = calc.color_mode != old_color_mode;
    bool refreshOp      = calc.op_command != old_op;
    bool refreshValue   = calc.val_current != old_val;
    bool refreshStored  = calc.val_stored != old_stored;
    bool refreshBottom  = menu_mode != old_menu_mode;
    bool refreshBin     = refreshBase || refreshBitMode || refreshBottom;

    if( refreshBase || refreshColor || refreshBottom ){
      drawTag( 0,   0, 1, 2, (menu_mode != MENU_COLOR) && (calc.base ==  8) ? COLOR_OCT_FG : COLOR_GHOST, ST77XX_BLACK, "OCT", 3 );
      drawTag( 35,  0, 1, 2, (menu_mode != MENU_COLOR) && (calc.base == 10) ? COLOR_DEC_FG : COLOR_GHOST, ST77XX_BLACK, "DEC", 3 );
      drawTag( 70,  0, 1, 2, (menu_mode != MENU_COLOR) && (calc.base == 16) ? COLOR_HEX_FG : COLOR_GHOST, ST77XX_BLACK, "HEX", 3 );
      drawTag( 170, 0, 1, 2, (menu_mode == MENU_COLOR) && (calc.color_mode == RGB_888) ? COLOR_COL_FG : COLOR_GHOST, ST77XX_BLACK, "888", 3 );
      drawTag( 205, 0, 1, 2, (menu_mode == MENU_COLOR) && (calc.color_mode == RGB_565) ? COLOR_COL_FG : COLOR_GHOST, ST77XX_BLACK, "565", 3 );
    }

    if( refreshValue || refreshBase || refreshColor || refreshBitMode ) drawLargeNumber();
    if( refreshStored || refreshBase || refreshColor || refreshBitMode ) drawSmallNumber( calc.base, 0, 20, COLOR_COL_FG, calc.val_stored );

    if( refreshOp ){
      screen.setAddrWindow( 190, 30, 30, 16 );
      for( uint16_t i = 30*16; i>0; i--){ screen.SPI_WRITE16( ST77XX_BLACK ); }

      switch(  calc.op_command ){
        case OP_NONE:        drawTag( 240, 30, 2, 2, ST77XX_BLACK, ST77XX_BLACK, " ",   1, true ); break;
        case OP_PLUS:        drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "+",   1, true ); break;
        case OP_MINUS:       drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "-",   1, true ); break;
        case OP_MULTIPLY:    drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "X",   1, true ); break;
        case OP_DIVIDE:      drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "/",   1, true ); break;
        case OP_MOD:         drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "MOD", 3, true ); break;
        case OP_ROL:         drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "RoL", 3, true ); break;
        case OP_ROR:         drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "RoR", 3, true ); break;
        case OP_LEFT_SHIFT:  drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "<<",  2, true ); break;
        case OP_RIGHT_SHIFT: drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, ">>",  2, true ); break;
        case OP_AND:         drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "AND", 3, true ); break;
        case OP_OR:          drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "OR",  2, true ); break;
        case OP_NOR:         drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "NOR", 3, true ); break;
        case OP_XOR:         drawTag( 240, 30, 2, 2, COLOR_GHOST,  COLOR_COL_FG, "XOR", 3, true ); break;
      }
    }

    if( refreshBottom || refreshBitMode || refreshBase ){
      digitalWrite(PIN_SCREEN_DC, HIGH);                                           // Set the DC line High so we can send data to the screen
      screen.setAddrWindow( 0, 120, SCREEN_WIDTH, 120 );                           // Set the address area of the window to fill
      SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));            // Run the SPI transaction at 8MHZ
      for( uint16_t i = SCREEN_WIDTH * 120; i>0; i--){
        screen.SPI_WRITE16( ST77XX_BLACK );    
      }
    }

    switch( menu_mode ){
      case MENU_BINARY:
        if( refreshBin || refreshValue ){
          if( calc.base == 16 )     drawBinary( 0, 130, refreshBin );
          else if( calc.base == 8 ) drawOctalBinary( 0, 130, refreshBin );
        }
        break;
      case MENU_DEC:
        drawChar( 0, 140, 2, 2, 0, COLOR_HEX_FG, ST77XX_BLACK, "HEX:", 4 );
        drawSmallNumber( 16, 50, 126, COLOR_HEX_FG, calc.val_current );

        drawChar( 0, 184, 2, 2, 0, COLOR_OCT_FG, ST77XX_BLACK, "OCT:", 4 );
        drawSmallNumber( 8, 50, 170, COLOR_OCT_FG, calc.val_current );

        break;
      case MENU_COLOR:
        drawColorMenu(0, 120);
        break;
    }

    old_val        = calc.val_current;
    old_stored     = calc.val_stored;
    old_base       = calc.base;
    old_color_mode = calc.color_mode;
    old_bit_depth  = calc.bitDepth;
    old_menu_mode  = menu_mode;

}




/*******************************************
* Program Loop Function                    *
*******************************************/

void loop() {
  hw.processEvents();
}
