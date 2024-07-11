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
displays all of the visualizations and menus.

*/

Hardware    hw;                                                                // Manages the keyboard presses
Calculator  calc;                                                              // The calculator finite state machine


/*******************************************
* Screen Defaults & Setup                  *
*******************************************/
// SCREEN PINS:
#define PIN_SCREEN_CLOCK PIN_PA6                                               // SPI Clock
#define PIN_SCREEN_DATA  PIN_PA4                                               // SPI Data (MOSI)
#define PIN_SCREEN_DC    PIN_PA3                                               // Data Command
#define PIN_SCREEN_RESET PIN_PA2                                               // Reset
#define PIN_SCREEN_BLK   PIN_PA1                                               // Blank Screen

#define SCREEN_WIDTH   240                                                     // Display width, in pixels
#define SCREEN_HEIGHT  240                                                     // Display height, in pixels

Adafruit_ST7789 screen(&SPI, -1, PIN_SCREEN_DC, PIN_SCREEN_RESET);             // Initiate the screen class

/*******************************************
* STYLES / COLORS                          *
*******************************************/
#define COLOR_NUM_BG 0x0821                                                    // Color used in the background behind the main numbers
#define COLOR_HEX_FG 0x07E0                                                    // Color used in the foreground in hexidecimal mode
#define COLOR_DEC_FG 0x20BF                                                    // Color used in the foreground in decimal mode
#define COLOR_OCT_FG 0xF8C3                                                    // Color used in the foreground in octal mode
#define COLOR_COL_FG 0xEF7D                                                    // Color used for the color chips
#define COLOR_GHOST  0x4162                                                    // Color used for ghost digits

#define COLOR_RED    0xC000                                                    // Color used for Red in the color menu
#define COLOR_GREEN  0x0680                                                    // Color used for Green in the color menu
#define COLOR_BLUE   0x0018                                                    // Color used for Blue in the color menu

uint16_t base_color = COLOR_COL_FG;                                            // Stores the currently selected foreground color

/*******************************************
* Temporary Variables                      *
*******************************************/

uint64_t old_val        = 0xFFFFFFFFFFFFFFFF;                                  // These variables track the state of the finite state
uint64_t old_stored     = 0xFFFFFFFFFFFFFFFF;                                  // machine prior to pressing a button. This allows us
uint8_t  old_op         = 0xFF;                                                // to render only portions of the screen after each key press.
uint8_t  old_base       = 0xFF;                                                // A better way to do this would be to move this into the calculator.h
uint8_t  old_color_mode = 0xFF;                                                // file and just add flags for everything that might have changed
uint8_t  old_bit_depth  = 0xFF;                                                // Maybe I will do that later ### TO DO ###



/*******************************************
* Key Press Event Manager                  *
*******************************************/

#define MENU_BINARY 0                                                          // Binary Menu
#define MENU_DEC    1                                                          // Decimal Menu
#define MENU_COLOR  2                                                          // Color Selector Menu
volatile uint8_t menu_mode = 0;                                                // Tracks which mode the menu is in (BINARY / DEC / COLOR)
volatile uint8_t old_menu_mode = 0xFF;                                         // Tracks the old menu mode to identify changes in the mode between refreshes

uint32_t screen_shutoff_time = 0;                                              // The time that the screen should shut off
#define SCREEN_SHUTOFF_DELAY 30000                                             // Make the screen shutoff time 30 seconds

void manageKeyPress(){                                                         // Keypress Event Handler Function
  if( millis() > screen_shutoff_time ){                                        // See if the screen is currently shut off
    screen_shutoff_time = millis() + SCREEN_SHUTOFF_DELAY;                     // Reset the shutoff timer
    return;                                                                    // Exit out of the keypress. (This just turns the screen back on)
  }

  bool refresh_screen = true;                                                  // Assume that we need to refresh the screen
  screen_shutoff_time = millis() + SCREEN_SHUTOFF_DELAY;                       // Reset the shutoff timer with every keypress

  switch( hw.last_pressed_key ){                                               // Check the value of the last pressed key
    case KEY_0:         calc.enterDigit(0);     break;                         // If the user pressed a number key, then 
    case KEY_1:         calc.enterDigit(1);     break;                         // pass the number to the calculator FSM
    case KEY_2:         calc.enterDigit(2);     break;                         // It will manage the state for us.
    case KEY_3:         calc.enterDigit(3);     break;                         // 
    case KEY_4:         calc.enterDigit(4);     break;                         // 
    case KEY_5:         calc.enterDigit(5);     break;                         // 
    case KEY_6:         calc.enterDigit(6);     break;                         // 
    case KEY_7:         calc.enterDigit(7);     break;                         // 
    case KEY_8:         calc.enterDigit(8);     break;                         // 
    case KEY_9:         calc.enterDigit(9);     break;                         // 
    case KEY_A:         calc.enterDigit(10);    break;                         // 
    case KEY_B:         calc.enterDigit(11);    break;                         // 
    case KEY_C:         calc.enterDigit(12);    break;                         // 
    case KEY_D:         calc.enterDigit(13);    break;                         // 
    case KEY_E:         calc.enterDigit(14);    break;                         // 
    case KEY_F:         calc.enterDigit(15);    break;                         // 
    case KEY_00:        calc.enterDigit(0);  calc.enterDigit(0);  break;       // For double digits like 00 and FF, just run the same
    case KEY_FF:        calc.enterDigit(15); calc.enterDigit(15); break;       // update command twice.

    case KEY_EQUALS:    calc.equals();          break;                         // Most of the math functions just pass straight
    case KEY_MULT:      calc.multiplyBy();      break;                         // through to the calculator's finite state machine.
    case KEY_DIV:       calc.divideBy();        break;                         // Divide stored value by current value
    case KEY_MINUS:     calc.minusBy();         break;                         // Subtract current value from stored value
    case KEY_PLUS:      calc.plusBy();          break;                         // Add stored value to current value
    case KEY_MOD:       calc.modBy();           break;                         // MOD stored value by current value

    case KEY_XOR:       calc.xorWith();         break;                         // XOR current and stored values
    case KEY_AND:       calc.andWith();         break;                         // AND current and stored values
    case KEY_OR:        calc.orWith();          break;                         // OR  current and stored values
    case KEY_NOR:       calc.norWith();         break;                         // NOR current and stored values

    case KEY_ROL:       calc.ror();             break;                         // Rotate Left by 1 bit
    case KEY_ROR:       calc.rol();             break;                         // Rotate Right by 1 bit
    case KEY_LSHIFT:    calc.leftShift();       break;                         // Left Shift by 1 bit
    case KEY_RSHIFT:    calc.rightShift();      break;                         // Right Shift by 1 bit
    case KEY_X_ROL_Y:   calc.rolBy();           break;                         // Rotate Left by N bits
    case KEY_X_ROR_Y:   calc.rorBy();           break;                         // Rotate Right by N bits
    case KEY_X_LS_Y:    calc.leftShiftBy();     break;                         // Left Shift by N bits
    case KEY_X_RS_Y:    calc.rightShiftBy();    break;                         // Right Shift by N bits

    case KEY_1S:        calc.onesCompliment();  break;                         // 1's compliment
    case KEY_2S:        calc.twosCompliment();  break;                         // 2's compliment
    case KEY_BYTE_FLIP: calc.byteFlip();        break;                         // Reverses Byte Order
    case KEY_WORD_FLIP: calc.wordFlip();        break;                         // Reverses Word Order

    case KEY_CLR:       calc.clear();           break;                         // Clears out just the current value
    case KEY_ALL_CLEAR: calc.allClear();        break;                         // Clears out the current value, stored value and operator

    case KEY_R_DN:      calc.decRed();          break;                         // Decrement the red portion of a 16 or 24 bit color code
    case KEY_G_DN:      calc.decGreen();        break;                         // Decrement the green portion of a 16 or 24 bit color code
    case KEY_B_DN:      calc.decBlue();         break;                         // Decrement the blue portion of a 16 or 24 bit color code
    case KEY_R_UP:      calc.incRed();          break;                         // Increment the red portion of a 16 or 24 bit color code
    case KEY_G_UP:      calc.incGreen();        break;                         // Increment the green portion of a 16 or 24 bit color code
    case KEY_B_UP:      calc.incBlue();         break;                         // Increment the blue portion of a 16 or 24 bit color code

    case KEY_8_BIT:     calc.setBitDepth8();    break;                         // Set the current bit depth to 8 bits
    case KEY_16_BIT:    calc.setBitDepth16();   break;                         // Set the current bit depth to 16 bits
    case KEY_32_BIT:    calc.setBitDepth32();   break;                         // Set the current bit depth to 32 bits
    case KEY_64_BIT:    calc.setBitDepth64();   break;                         // Set the current bit depth to 64 bits

    case KEY_RGB_565:   calc.setColorMode565(); menu_mode = MENU_COLOR;  base_color = COLOR_COL_FG; break; // Switch into 565 16-bit color mode
    case KEY_RGB_888:   calc.setColorMode888(); menu_mode = MENU_COLOR;  base_color = COLOR_COL_FG; break; // Switch into 888 24-bit color mode
    case KEY_BASE_8:    calc.setBase8();        menu_mode = MENU_BINARY; base_color = COLOR_OCT_FG; break; // Switch into octal mode
    case KEY_BASE_10:   calc.setBase10();       menu_mode = MENU_DEC;    base_color = COLOR_DEC_FG; break; // Switch into decimal mode
    case KEY_BASE_16:   calc.setBase16();       menu_mode = MENU_BINARY; base_color = COLOR_HEX_FG; break; // Switch into hexidecimal mode
    default: refresh_screen = false;                                           // If the user doesn't press a valid button, we don't need to refresh
  }
  if( refresh_screen ) renderScreen();                                         // Refresh the screen if we need to
}


/*******************************************
* General Setup Function                   *
*******************************************/

void setup() {
  screen.init(SCREEN_WIDTH, SCREEN_HEIGHT, SPI_MODE2);                         // Initialize the screen
  pinMode( PIN_SCREEN_BLK, OUTPUT );                                           // Set the backlight pin to output mode
  digitalWrite( PIN_SCREEN_BLK, HIGH );                                        // Turn the screen's backlight on
  screen_shutoff_time = millis() + SCREEN_SHUTOFF_DELAY;                       // Set the shutoff timer

  delay(10);                                                                   // Give everything a few ms to get situated
  screen.setRotation(3);                                                       // Set the screen rotation to 270 degrees
  screen.fillScreen( ST77XX_BLACK );                                           // Blank out the screen

  hw.setup();                                                                  // Initialize the hardware library (keyboard and such)
  hw.onKeyPress( manageKeyPress );                                             // Add the keyboard handler function to react to keypress events

  renderScreen();                                                              // Do the initial screen render event
}


/*******************************************
* Draw Functions                           *
*******************************************/
void fillBox( uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color ){   // Fill a box
  screen.setAddrWindow( x, y, w, h );                                         // Set the address area of the area to fill
  for( uint16_t i = (w*h); i>0; i--){                                         // Count down from the total area of the box
    screen.SPI_WRITE16( color );                                              // Write the color into the box
  }
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
uint8_t left_char[6]  = { 0x1C, 0x3E, 0x7F, 0x7F, 0x7F, 0x7F };                // A character bitmap that describes the rounded left side of the pill 
uint8_t right_char[6] = { 0x7F, 0x7F, 0x7F, 0x7F, 0x3E, 0x1C };                // A character bitmap that describes the rounded right side of the pill

void drawTag( uint8_t x, uint8_t y, uint8_t scale_x, uint8_t scale_y, uint16_t fg_color, uint16_t bg_color, const char *str, uint8_t str_length, bool right_align = false ){
  uint8_t widget_width  = CHAR_WIDTH  * str_length * scale_x + 2 * CHAR_WIDTH; // Determine the width of the tag widget
  uint8_t widget_height = CHAR_HEIGHT * scale_y + 2;                           // Determine the height of the tag widget
  int16_t str_index = 0;                                                       // Index of the current character in the value string
  uint8_t colStep = 0;                                                         // Keeps track of the number of scaled pixel columns drawn so far
  uint8_t rowStep = 0;                                                         // Keeps track of the number of scaled pixel rows drawn so far
  uint8_t hPixStep = scale_x;                                                  // Keeps track of the number of pixels drawn in a scaled pixel (horizontally)
  uint8_t vPixStep = scale_y;                                                  // Keeps track of the number of pixels drawn in a scaled pixel (vertically)
  uint16_t font_index;                                                         // Points to the current character in the font array
  if( right_align ) x -= widget_width;                                         // Update the starting point depending on whether we need to right-align the text

  uint8_t line_left   =  x + 4;                                                // Calculate the left position of the rule that goes at the top/bottom of the pill
  uint8_t line_length = (CHAR_WIDTH*str_length) * scale_x + 4;                 // Calculate the length of the rule that goes at the top/bottom of the pill
  fillBox( line_left, y, line_length, 1, fg_color );                           // Draw a line on the top of the tag
  fillBox( line_left, y + widget_height - 1, line_length, 1, fg_color );       // Draw a line on the bottom of the tag

  screen.setAddrWindow( x, y+1, widget_width, widget_height-2 );               // Set the address window for the tag's label text

  for( uint8_t row = 0; row<widget_height-2; row++ ){                          // Loop through each row in the tag's label text
    str_index = -1;                                                            // Set the current string index to the beginning
    colStep = CHAR_WIDTH;                                                      // Set the colStep tracker to the end of the character to it resets
    for( uint8_t col = 0; col<widget_width; col++ ){                           // Loop through each column in the tag's label text
      if( colStep == CHAR_WIDTH ){                                             // See if we have reached the end of the current character's columns
        colStep = 0;                                                           // Reset the character column to zero
        str_index++;                                                           // Increment the character index that points to the buffer
        font_index = str[ str_index-1 ] * CHAR_WIDTH;                          // To get the actual string index, subtract one (the 0'th character will be a border character)
      }

      if( str_index == 0 ){                                                    // If it's the 0'th character draw the left_char bitmap
        screen.SPI_WRITE16( (left_char[colStep] & (1<<rowStep)) > 0 ? fg_color : ST77XX_BLACK );
      } else if( str_index == str_length+1){                                   // If it's the last character, draw the right_char bitmap
        screen.SPI_WRITE16( (right_char[colStep] & (1<<rowStep)) > 0 ? fg_color : ST77XX_BLACK );
      } else {                                                                 // For everything else, just draw the character from the font array
        screen.SPI_WRITE16( (font5x7[font_index + colStep] & (1<<rowStep)) > 0 ? bg_color : fg_color );
      }

      if( (str_index == 0) || (str_index == str_length+1) ){                   // We don't want the width of the rounded ends to change with scale_x so if we are
        colStep++;                                                             // in one of those characters, just increment colStep without worrying about hPixStep
      } else {                                                                 // But if we are on one of the actual characters then
        hPixStep--;                                                            // use hPixStep to track how many pixels we still have to render before moving
        if( hPixStep == 0 ){                                                   // to the next column. We start at scale_x and decrement to keep things faster
          colStep++;                                                           // Hit the end of the pixels in the column, so move to the next column in the character
          hPixStep = scale_x;                                                  // Reset the pixel counter to scale_x
        }
      }
    }
    vPixStep--;                                                                // Just like with hPixStep, we track the vertical pixel stepping
    if( vPixStep == 0 ){                                                       // Once vPixStep hits zero, we know we've reached the end of the row
      rowStep++;                                                               // and we are ready to move onto the next row
      vPixStep = scale_y;                                                      // Reset the vPixStep counter to scale_y
    }
  }
}


/*******************************************
* Draw String Function                     *
*******************************************/
// This function draws a series of characters onto the screen in a pre-defined box size.
// x, y          - Location of the top left corner of the bounding box
// width, height - Size of the bounding box
// max_scale     - The maximum horizontal scale factor for text
// kerning       - The spacing between letters in the text
// fg_color      - The color of the text
// bg_color      - The background color behind the text
// ghost_zeros   - Will change the color of leading zeros to COLOR_GHOST
// str           - The pointer to the character array
// str_length    - The number of characters in the array

void drawString( uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t max_scale, uint8_t kerning, uint16_t fg_color, uint16_t bg_color, bool ghost_zeros, const char* str, uint8_t str_length ){

  bool     is_leading_zero = ghost_zeros;                                      // Used during the loop to track if we are rendering a leading zero in the number
  bool     is_comma        = false;                                            // Used during the loop to track if we are currently rendering a comma
  int16_t  str_index = 0;                                                      // Index of the current character in the value string
  uint8_t  col_width  = CHAR_WIDTH + kerning;                                  // The full width of a column with spacing
  uint8_t  scale_x = width / (col_width * str_length);                         // Calculate the largest scale possible for the number of digits
  uint8_t  scale_y = height / CHAR_HEIGHT;                                     // Calculate the largest height possible

  if( scale_x == 0 ){                                                          // See if there are too many numbers for the size of the widget
    scale_x = 1;                                                               // Force horizontal scale to 1
    str_length = width / col_width;                                            // Truncate the number of digits
  }
  if( scale_x > max_scale ) scale_x = max_scale;
  if( scale_y > (scale_x<<1) ) scale_y = scale_x << 1;                         // Ensure we don't breach a 2 to 1 scaling ratio (it's hard to)

  uint8_t  val_width  = (str_length * col_width - kerning) * scale_x;          // The pixel width that the actual characters will consume
  uint8_t  val_height = CHAR_HEIGHT * scale_y;                                 // The pixel height that the actual characters will consume

  uint8_t  colStep = col_width;                                                // Keeps track of the number of scaled pixel columns drawn so far
  uint8_t  rowStep = 0;                                                        // Keeps track of the number of scaled pixel rows drawn so far
  uint8_t  hPixStep = scale_x;                                                 // Keeps track of the number of pixels drawn in a scaled pixel (horizontally)
  uint8_t  vPixStep = scale_y;                                                 // Keeps track of the number of pixels drawn in a scaled pixel (vertically)
  uint16_t font_index;                                                         // Points to the current character in the font array

  fillBox( x, y, width-val_width, height, bg_color );                          // Fill in the box to the left of the type so gets cleared out
  fillBox( x + width-val_width, y, val_width, height-val_height, bg_color );   // Fill in the box above the type so gets cleared out as it gets smaller

  screen.setAddrWindow( x+width-val_width, y+height-val_height, val_width, val_height ); // Set the address area of the window to fill

  for( uint8_t row = 0; row<val_height; row++ ){                               // The outer loop goes through each column
    
    str_index = 0;                                                             // Set the current column to the starting column defined in the select section above
    is_leading_zero = true;                                                    // Since we started the line again, we have to reset the leading zero indicator for this row
    colStep = col_width;                                                       // Reset colStep to the max value so it triggers the if statement at the beginning of the loop
    
    for( uint8_t col = 0; col<val_width; col++ ){                              // The outer loop goes through each column
      
      if( colStep == col_width ){                                              // See if we have reached the end of the columns in a character
        colStep = 0;                                                           // Reset the character column to zero
        font_index = str[ str_index ] * CHAR_WIDTH;                            // Assumes 6 columns per char
        str_index++;                                                           // Increment the character index that points to the buffer

        if( is_leading_zero ){                                                 // If haven't hit a number yet (still leading zeros)
          if( (font_index != '0' * CHAR_WIDTH) && (font_index != ' ' * CHAR_WIDTH)) // See if we hit something other than a zero or a space
            is_leading_zero = false;                                           // If we did, then set the is_leading_zero flag to false
        }
        is_comma = ( font_index == ',' * CHAR_WIDTH );                         // Set the is_comma flag if the current character is a comma
      }

      if( (colStep<CHAR_WIDTH) && ((font5x7[font_index + colStep] & (1<<rowStep)) > 0) ){ // See if the current pixel should be turned on per the font character bitmap
        screen.SPI_WRITE16( (is_leading_zero || is_comma) ? COLOR_GHOST : fg_color );     // If comma / leading zero the use the ghost color. Otherwise use foreground color
      } else {                                                                 // If the pixel isn't active, then we can
        screen.SPI_WRITE16( bg_color );                                        // use the background color
      }
      
      hPixStep--;                                                              // Decrement the horizontal pixel counter (for scaling)
      if( hPixStep == 0 ){                                                     // Once pixel counter hits zero
        colStep++;                                                             // Move to the next pixel
        hPixStep = scale_x;                                                    // Reset the pixel counter to the horizontal scale (scale_x)
      }
    }
    vPixStep--;                                                                // Decrement the vertical pixel counter (for scaling)
    if( vPixStep == 0 ){                                                       // Once pixel counter hits zero
      rowStep++;                                                               // Move to the next row
      vPixStep = scale_y;                                                      // Reset the pixel counter to the vertical scale (scale_y)
    }
  }

}

/*******************************************
* Draw Small Number Function               *
*******************************************/
// This function draws a small version of the number (used for the stored value and the hex/oct values of the decimal screen)
// base          - The numerical base to use to render the number (8, 10, 16)
// x, y          - Location of the top left corner of the bounding box
// width, height - Size of the bounding box
// val           - The 64-bit numberical value to draw

void drawSmallNumber( uint8_t base, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t fg_color, uint64_t val ){
  char buffer[27] = {'0'};                                                     // Create a buffer to hold the string with the value to print to the screen

  if( 16 == base ){                                                            // If we are in base 16 mode
    //Format the 64-bit value into a set of 4-nibble hexidecimal words
    sprintf(buffer, "%04X %04X %04X %04X", uint16_t(val>>48),uint16_t(val>>32),uint16_t(val>>16),uint16_t(val) );
  
    switch( calc.bitDepth ){                                                   // Use bit depth to determine how many digits to draw, and how large to make them
      case 8:  drawString( x, y, width, height, 2, 2, fg_color, ST77XX_BLACK, true, &buffer[17],  2 ); break;     // Draw  8 bit value
      case 16: drawString( x, y, width, height, 2, 1, fg_color, ST77XX_BLACK, true, &buffer[15],  4 ); break;     // Draw 16 bit value
      case 24: drawString( x, y, width, height, 2, 0, fg_color, ST77XX_BLACK, true, &buffer[12],  7 ); break;     // Draw 24 bit value
      case 32: drawString( x, y, width, height, 2, 1, fg_color, ST77XX_BLACK, true, &buffer[10],  9 ); break;     // Draw 32 bit value
      case 64: drawString( x, y, width, height, 2, 1, fg_color, ST77XX_BLACK, true, &buffer[ 0], 19 ); break;     // Draw 64 bit value
    }
  } else if( 8 == base ){
    //Format the 64-bit value into a set of 4-nibble octal words
    sprintf(buffer, "%04o %04o %04o %04o", uint16_t((val>>36)&0xFFF),uint16_t((val>>24)&0xFFF),uint16_t((val>>12)&0xFFF),uint16_t((val)&0xFFF) );

    switch( calc.bitDepth ){                                                   // Use bit depth to determine how many digits to draw, and how large to make them
      case 8:  drawString( x, y, width, height, 2, 2, fg_color, ST77XX_BLACK, true, &buffer[17],  2 ); break;     // Draw  8 bit value
      case 16: drawString( x, y, width, height, 2, 1, fg_color, ST77XX_BLACK, true, &buffer[15],  4 ); break;     // Draw 16 bit value
      case 24: drawString( x, y, width, height, 2, 0, fg_color, ST77XX_BLACK, true, &buffer[12],  7 ); break;     // Draw 24 bit value
      case 32: drawString( x, y, width, height, 2, 1, fg_color, ST77XX_BLACK, true, &buffer[10],  9 ); break;     // Draw 32 bit value
      case 64: drawString( x, y, width, height, 2, 1, fg_color, ST77XX_BLACK, true, &buffer[ 0], 19 ); break;     // Draw 64 bit value
    }
  } else {                                                                     // Format as decimal
    uint8_t  num_digits = 0;                                                   // Reset a counter for the number of digits in the base-10 number
    uint64_t tmp = val;                                                        // Create a temporary storage for val
    for( int8_t i=26; i>=0; i-- ){                                             // Loop through up to 26 digits (maximum allowed in 64-bit number)
      if( tmp > 0 ){                                                           // Ensure tmp hasn't reached zero as we've removed digits
        if( i % 4 == 3 ){                                                      // For every 3 digits, we want to add a comma
          buffer[i] = ',';                                                     // Write a comma into the buffer
        } else {                                                               // If we don't need a comma here
          buffer[i] = 0x30 + (tmp % 10);                                       // Write a numeric character by adding the mod 10 remainder to the ASCII value for "0"  
          tmp /= 10;                                                           // Divide tmp by 10 and do it all again
        }
        num_digits++;                                                          // Keep track of the number of digits
      } else {                                                                 // If tmp is zero now...
        buffer[i] = 0x30;                                                      // Write leading zeros into the buffer
      }
    }
    if( num_digits == 0 ) num_digits = 1;                                      // We still want at least 1 digit even if the number is zero
    drawString( x, y, width, height, 2, 1, fg_color, ST77XX_BLACK, true, &buffer[27-num_digits], num_digits ); // Draw the number to the screen
  }
}

void drawLargeNumber( uint8_t base, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t fg_color, uint64_t val ){
  char buffer[27] = {'0'};                                                     // Create a buffer to hold the string with the value to print to the screen

  if( 16 == base ){                                                            // If we are in base 16 mode
    //Format the 64-bit value into a set of 4-nibble hexidecimal words (no spaces between them)
    sprintf(buffer, "%04X%04X%04X%04X", uint16_t(calc.val_current>>48),uint16_t(calc.val_current>>32),uint16_t(calc.val_current>>16),uint16_t(val) );
    
    switch( calc.bitDepth ){                                                   // Use bit depth to determine how many digits to draw, and how large to make them
      case 8:  drawString( x, y, width, height, 10, 2, fg_color, COLOR_NUM_BG, true, &buffer[14],  2 ); break;     // Draw  8 bit value
      case 16: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[12],  4 ); break;     // Draw 16 bit value
      case 24: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[10],  6 ); break;     // Draw 24 bit value
      case 32: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[ 8],  8 ); break;     // Draw 32 bit value
      case 64: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[ 0], 16 ); break;     // Draw 64 bit value
    }
  } else if( 8 == base ){
    //Format the 64-bit value into a set of 4-nibble octal words (no spaces)
    sprintf(buffer, "%04o%04o%04o%04o", uint16_t((calc.val_current>>36) & 0xFFF),uint16_t((calc.val_current>>24) & 0xFFF),uint16_t((calc.val_current>>12) & 0xFFF),uint16_t(val & 0xFFF) );
    
    switch( calc.bitDepth ){                                                   // Use bit depth to determine how many digits to draw, and how large to make them
      case 8:  drawString( x, y, width, height, 10, 2, fg_color, COLOR_NUM_BG, true, &buffer[13],  3 ); break;     // Draw  8 bit value
      case 16: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[10],  6 ); break;     // Draw 16 bit value
      case 24: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[ 8],  8 ); break;     // Draw 24 bit value
      case 32: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[ 5], 11 ); break;     // Draw 32 bit value
      case 64: drawString( x, y, width, height, 10, 1, fg_color, COLOR_NUM_BG, true, &buffer[ 0], 16 ); break;     // Draw 64 bit value
    }
  } else {                                                                     // Format as decimal
    uint8_t  num_digits = 0;                                                   // Reset a counter for the number of digits in the base-10 number
    uint64_t tmp = val;                                                        // Create a temporary storage for val
    for( int8_t i=26; i>=0; i-- ){                                             // Loop through up to 26 digits (maximum allowed in 64-bit number)
      if( tmp > 0 ){                                                           // Ensure tmp hasn't reached zero as we've removed digits
        if( i % 4 == 3 ){                                                      // For every 3 digits, we want to add a comma
          buffer[i] = ',';                                                     // Write a comma into the buffer
        } else {                                                               // If we don't need a comma here
          buffer[i] = 0x30 + (tmp % 10);                                       // Write a numeric character by adding the mod 10 remainder to the ASCII value for "0"  
          tmp /= 10;                                                           // Divide tmp by 10 and do it all again
        }
        num_digits++;                                                          // Keep track of the number of digits
      } else {                                                                 // If tmp is zero now...
        buffer[i] = 0x30;                                                      // Write leading zeros into the buffer
      }
    }
    if( num_digits == 0 ) num_digits = 1;                                      // We still want at least 1 digit even if the number is zero

    drawString( x, y, width, height, 8, 1, fg_color, ST77XX_BLACK, true, &buffer[27-num_digits], num_digits ); // Draw the number to the screen
  }
}

/*******************************************
* Draw Nibble                              *
*******************************************/
// This function draws a set of dots onto the screen representing a single binary nibble
// x, y     - Location of the top left corner of the bounding box
// num_dots - The number of binary digits (can be either 3 or 4)
// fg_color - The color to use to draw active dots
// val      - The 64-bit numberical value to draw

void drawNibble( uint8_t x, uint8_t y, uint8_t num_dots, uint16_t fg_color, uint8_t val ){
  uint8_t dot_width   = (num_dots == 4) ? 6 : 8;                               // Set the dot width depending on whether num_dots is 3 or 4
  uint8_t dot_spacing = (num_dots == 4) ? 2 : 3;                               // Set the spacing between dots depending on whether num_dots is 3 or 4
  uint16_t widget_width = (dot_width + dot_spacing) * num_dots;                // Determine the total width of the visualization
  uint16_t widget_height = CHAR_HEIGHT * 2;                                    // Determine the total height of the visualization
  char buffer[2] = {0};                                                        // A buffer to store the word value's character

  sprintf(buffer, "%01X", val);                                                // Format val as a 1-digit hexidecimal value
  drawString( x, y, 12, widget_height, 2, 0, fg_color, ST77XX_BLACK, false, buffer, 1 ); // Draw the value onto the screen

  screen.setAddrWindow( x + 15, y, widget_width, widget_height );              // Set the address area of the window to fill

  uint8_t colNum = 0;                                                          // Track the current column
  uint8_t bit_num = num_dots - 1;                                              // Track the current bit
  for( uint16_t i = widget_width * widget_height; i>0; i--){                   // Loop through all of the pixels in the widget's area
    if( val & (1 << bit_num) ){                                                // If the current bit of val is a 1
      screen.SPI_WRITE16( colNum < dot_width ? COLOR_COL_FG : ST77XX_BLACK );  // Then draw the a white dot (or black in the space between the dots)
    } else {                                                                   // If the current bit of val is a zero
      screen.SPI_WRITE16( colNum < dot_width ? COLOR_GHOST : ST77XX_BLACK );   // Then draw the a ghost dot (or black in the space between the dots)
    }
    colNum++;                                                                  // Increment the column
    if( colNum == (dot_width + dot_spacing) ){                                 // If we hit the end of a dot & space 
      colNum = 0;                                                              // Reset the column counter
      if( bit_num-- == 0 ) bit_num = num_dots-1;                               // Decrement the bit_num, and if we hit zero, set it back to the number of dots
    }
  }  
}

/*******************************************
* Draw Hex Binary                          *
*******************************************/
// This function visualizes a hexidecimal binary number as a set of nibbles and their representative characters onto the screen
// x, y         - Location of the top left corner of the bounding box
// full_refresh - Completely re-draws all of the nibbles if true. Otherwise, only draws nibbles that change 
// val          - The 64-bit numberical value to draw

void drawHexBinary( uint8_t x, uint8_t y, bool full_refresh, uint64_t val ){                    
  uint8_t spacing_x = 50;                                                      // Horizontal grid spacing of the nibbles visualizations
  uint8_t spacing_y = 28;                                                      // Vertical grid spacing of the nibble visualizations
  char buffer[3] = {0};                                                        // Buffer for the Byte ASCII visualizations

  uint8_t nibble = 0;                                                          // Set the counter for the current nibble to zero
  for( uint8_t row = 0; row < 4; row++ ){                                      // Loop through the rows of nibbles
    for( uint8_t col = 0; col < 4; col++ ){                                    // Loop through the columns of nibble 
      if( (15-nibble) < (calc.bitDepth >> 2) ){                                // See if we need to show this nibble based on the bit depth
        if( full_refresh || (( (val >> ((15 - (row * 4 + col)) * 4)) & 0xF ) != ( (old_val >> ((15 - (row * 4 + col)) * 4)) & 0xF )) ){ // See if the nibble should be refreshed
          drawNibble( x + col * spacing_x + 40, y + row * spacing_y, 4, COLOR_HEX_FG, (val >> ((15 - (row * 4 + col)) * 4)) & 0xF );    // Draw the nibble
        }
      }
      nibble++;                                                                // Increment the number of nibbles
    }
  }

  for( uint8_t i=0; i<4; i++ ){                                                // Loop through the rows
    buffer[0] = (calc.bitDepth >> 3) > (i*2) + 1 ? (val >> (i * 16 + 8)) & 0xFF : 0x00; // Capture the second byte of val as a character in the buffer
    buffer[1] = (calc.bitDepth >> 3) > (i*2)     ? (val >> (i * 16)) & 0xFF     : 0x00; // Capture the first byte of val as a character in the buffer
    drawString( x, y + spacing_y * (3-i)-6, 30, 24, 3, 1, COLOR_COL_FG, ST77XX_BLACK, false, buffer, 2 ); // Draw the buffer onto the screen
  }
}


/*******************************************
* Draw Octal Binary                        *
*******************************************/
// This function visualizes a hexidecimal binary number as a set of nibbles and their representative characters onto the screen
// x, y         - Location of the top left corner of the bounding box
// full_refresh - Completely re-draws all of the nibbles if true. Otherwise, only draws nibbles that change 
// val          - The 64-bit numberical value to draw

void drawOctalBinary( uint8_t x, uint8_t y, bool full_refresh, uint64_t val ){
  uint8_t spacing_x = 50;                                                      // Horizontal grid spacing of the nibbles visualizations
  uint8_t spacing_y = 28;                                                      // Vertical grid spacing of the nibble visualizations
  char buffer[3] = {0};                                                        // Buffer for the Byte ASCII visualizations

  for( uint8_t row = 0; row < 4; row++ ){                                      // Loop through the rows of nibbles
    for( uint8_t col = 0; col < 4; col++ ){                                    // Loop through the columns of nibble 
      if( full_refresh || (( (val >> ((15 - (row * 4 + col)) * 3)) & 0b111 ) != ( (old_val >> ((15 - (row * 4 + col)) * 3)) & 0b111 )) ){ // See if the nibble should be refreshed
        drawNibble( x + col * spacing_x + 40, y + row * spacing_y, 3, COLOR_OCT_FG, (val >> ((15 - (row * 4 + col)) * 3)) & 0b111 );      // Draw the nibble
      }
    }
  }

  for( uint8_t i=0; i<4; i++ ){
    buffer[0] = (val >> (i * 16 + 8)) & 0xFF;                                  // Capture the second byte of val as a character in the buffer
    buffer[1] = (val >> (i * 16)) & 0xFF;                                      // Capture the first byte of val as a character in the buffer
    drawString( x, y + spacing_y * (3-i)-6, 30, 24, 3, 1, COLOR_COL_FG, ST77XX_BLACK, false, buffer, 2 ); // Draw the buffer onto the screen
  }
}

/*******************************************
* Draw Progress Bar                        *
*******************************************/
// This function draws a progress bar onto the screen
// x, y          - Location of the top left corner of the bounding box
// width, height - Size of the bounding box
// val           - A value between 0 and 255 to represent in the progress bar

void drawProgressBar( uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t fg_color, uint16_t bg_color, uint16_t val ){
  uint8_t bar_width = (val * width) >> 8;                                      // Calculate the inner bar width
  uint8_t colNum = 0;                                                          // Track the current column being rendered
  screen.setAddrWindow( x, y, width, height );                                 // Set the address area of the window to fill
  for( uint16_t i = width * height; i>0; i--){                                 // Loop through the pixels in the window area
    screen.SPI_WRITE16( colNum < bar_width ? fg_color : bg_color );            // Write the foreground color if within the inner box otherwise the background color
    if( ++colNum == width ) colNum = 0;                                        // Increment column and reset to zero if it hits the width of the widget
  }  
}

/*******************************************
* Draw Color Menu                          *
*******************************************/
// This function draws the color visualization menu 
// x, y          - Location of the top left corner of the bounding box
// is565         - Indicates if the color is 16-bit 565 color mode (vs 24-bit 888 color mode)
// val           - A 16-bit or 24-bit color value

void drawColorMenu( uint8_t x, uint8_t y, bool is565, uint64_t val ){
  uint8_t redVal   = is565 ? (val >> 11) & 0b11111  : (val >> 16) & 0xFF;      // Calculate the red value (depending on 565 or 888 encoding)
  uint8_t greenVal = is565 ? (val >> 5)  & 0b111111 : (val >> 8)  & 0xFF;      // Calculate the green value (depending on 565 or 888 encoding)
  uint8_t blueVal  = is565 ? (val >> 0)  & 0b11111  : (val >> 0)  & 0xFF;      // Calculate the blue value (depending on 565 or 888 encoding)
  uint16_t color_565 = is565 ? val & 0xFFFF : ( ((redVal >> 3) << 11) | ((greenVal >> 2)<<5) | (blueVal >> 3) ); // Calculate a new 565 color from the separate RGB values

  char buffer[3] = {0};                                                        // Create a character buffer to hold the R/G/B string values

  sprintf(buffer, "%02X", redVal);                                                                  // Write out the red value into the buffer as a hex value
  drawString( x, y+3, 30, 24, 3, 1,  COLOR_RED,   ST77XX_BLACK, true, buffer, 2 );                  // Draw the red value to the screen
  drawProgressBar(x+40, y+15, 80, 4, COLOR_RED,   COLOR_NUM_BG, is565 ? redVal << 3 : redVal );     // Draw the red progress bar to the screen

  sprintf(buffer, "%02X", greenVal);                                                                // Write out the green value into the buffer as a hex value
  drawString( x, y+38, 30, 24, 3, 1, COLOR_GREEN, ST77XX_BLACK, true, buffer, 2 );                  // Draw the green value to the screen
  drawProgressBar(x+40, y+50, 80, 4, COLOR_GREEN, COLOR_NUM_BG, is565 ? greenVal << 2 : greenVal ); // Draw the green progress bar to the screen

  sprintf(buffer, "%02X", blueVal);                                                                 // Write out the blue value into the buffer as a hex value
  drawString( x, y+73, 30, 24, 3, 1, COLOR_BLUE,  ST77XX_BLACK, true, buffer, 2 );                  // Draw the blue value to the screen
  drawProgressBar(x+40, y+85, 80, 4, COLOR_BLUE,  COLOR_NUM_BG, is565 ? blueVal << 3 : blueVal );   // Draw the blue progress bar to the screen

  fillBox( x+130, y+10, 100, 100, color_565 );                                 // Fill the Color Swatch
}



/*******************************************
* Render Function                          *
*******************************************/

void renderScreen(){

    bool refreshBase    = calc.base != old_base;                               // See if the base changed
    bool refreshBitMode = calc.bitDepth != old_bit_depth;                      // See if the bit depth changed
    bool refreshColor   = calc.color_mode != old_color_mode;                   // See if the color mode changed
    bool refreshOp      = calc.op_command != old_op;                           // See if the operator command changed
    bool refreshValue   = calc.val_current != old_val;                         // See if the current value changed
    bool refreshStored  = calc.val_stored != old_stored;                       // See if the stored value changed
    bool refreshBottom  = menu_mode != old_menu_mode;                          // See if the menu mode changed
    bool refreshBin     = refreshBase || refreshBitMode || refreshBottom;      // See if we need to update the binary info

    digitalWrite(PIN_SCREEN_DC, HIGH);                                         // Set the DC line High so we can send data to the screen
    SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE2));          // Run the SPI transaction at 20 MHZ

    if( refreshBase || refreshColor || refreshBottom ){                        // Update the tags at the top of the screen depending on the mode
      drawTag( 0,   0, 1, 2, (menu_mode != MENU_COLOR) && (calc.base ==  8) ? COLOR_OCT_FG : COLOR_GHOST, ST77XX_BLACK, "OCT", 3 );
      drawTag( 35,  0, 1, 2, (menu_mode != MENU_COLOR) && (calc.base == 10) ? COLOR_DEC_FG : COLOR_GHOST, ST77XX_BLACK, "DEC", 3 );
      drawTag( 70,  0, 1, 2, (menu_mode != MENU_COLOR) && (calc.base == 16) ? COLOR_HEX_FG : COLOR_GHOST, ST77XX_BLACK, "HEX", 3 );
      drawTag( 170, 0, 1, 2, (menu_mode == MENU_COLOR) && (calc.color_mode == RGB_888) ? COLOR_COL_FG : COLOR_GHOST, ST77XX_BLACK, "888", 3 );
      drawTag( 205, 0, 1, 2, (menu_mode == MENU_COLOR) && (calc.color_mode == RGB_565) ? COLOR_COL_FG : COLOR_GHOST, ST77XX_BLACK, "565", 3 );
    }

    if( refreshValue || refreshBase || refreshColor || refreshBitMode ) drawLargeNumber( calc.base, 0, 54, 240, 56, base_color, calc.val_current );   // Update the current val 
    if( refreshStored || refreshBase || refreshColor || refreshBitMode ) drawSmallNumber( calc.base, 0, 20, 180, 28, COLOR_COL_FG, calc.val_stored ); // Update the stored val

    if( refreshOp ){                                                           // See if we need to refresh the operator widget
      fillBox( 190, 30, 30, 16, ST77XX_BLACK );                                // Blank out the left side of the widget since the operator changes size

      switch(  calc.op_command ){                                              // Based on the current operator command
        case OP_NONE:        drawTag( 240, 30, 2, 2, ST77XX_BLACK, ST77XX_BLACK, " ",   1, true ); break; // Draw the operator tag
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

    if( refreshBottom || refreshBitMode || refreshBase ){ fillBox(0, 120, SCREEN_WIDTH, 120, ST77XX_BLACK); } // If we need a full redo of the bottom of the screen, blank it out

    switch( menu_mode ){                                                                     // Based on the menu mode
      case MENU_BINARY:                                                                      // If it's a binary menu (HEX / OCTAL)
        if( refreshBin || refreshValue ){                                                    // If the binary number of value changed
          if( calc.base == 16 )     drawHexBinary( 0, 130, refreshBin, calc.val_current );   // If hex mode, draw the binary number in hex mode
          else if( calc.base == 8 ) drawOctalBinary( 0, 130, refreshBin, calc.val_current ); // If oct mode, draw the binary number in oct mode
        }
        break;

      case MENU_DEC:                                                                         // If it's the decimal menu
        drawString( 0, 130, 60, 24, 2, 1, COLOR_HEX_FG, ST77XX_BLACK, false, "HEX:", 4 );    // Create a label for "HEX:"
        drawSmallNumber( 16, 60, 126, 170, 28, COLOR_HEX_FG, calc.val_current );             // Draw the hex representation for the decimal number

        drawString( 0, 174, 60, 24, 2, 1, COLOR_OCT_FG, ST77XX_BLACK, false, "OCT:", 4 );    // Create a label for "OCT:"
        drawSmallNumber( 8, 60, 170, 170, 28, COLOR_OCT_FG, calc.val_current );              // Draw the octal representation for the decimal number
        break;

      case MENU_COLOR:                                                                       // If it's the color menu
        drawColorMenu( 0, 120, calc.color_mode == RGB_565, calc.val_current );               // Draw the color menu
        break;
    }

    old_val        = calc.val_current;                                                       // Store the various values so we can see if they changed
    old_stored     = calc.val_stored;                                                        // when it's time to render the screen again
    old_base       = calc.base;
    old_color_mode = calc.color_mode;
    old_bit_depth  = calc.bitDepth;
    old_menu_mode  = menu_mode;

    SPI.endTransaction();                                                                    // End the SPI transaction
}

/*******************************************
* Program Loop Function                    *
*******************************************/

void loop() {
  hw.processEvents();
  if( millis() > screen_shutoff_time ){                                        // If the timer has breached the shutoff time
    digitalWrite( PIN_SCREEN_BLK, LOW );                                       // Turn the screen's backlight off
  } else {                                                                     // Otherwise
    digitalWrite( PIN_SCREEN_BLK, HIGH );                                      // Turn the screen's backlight on
  }
}
