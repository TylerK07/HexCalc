#ifndef CALCULATOR_H
#define CALCULATOR_H

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
This library works like a calculatror finite state machine that manages
the state of the current calculations on a button by button level.
*/


/*******************************************
* Calculator Class Definition              *
*******************************************/

// Operation Identifiers
#define OP_NONE        0                                                       // Do nothing
#define OP_PLUS        1                                                       // Add current val to stored val
#define OP_MINUS       2                                                       // Subtract current val from stored val
#define OP_MULTIPLY    3                                                       // Multiply current val and stored val
#define OP_DIVIDE      4                                                       // Divide stored val by current val
#define OP_MOD         5                                                       // Remainder of stored val divided by current val
#define OP_ROL         6                                                       // Bit-rotate stored val to the left by current val bits
#define OP_ROR         7                                                       // Bit-rotate stored val to the right by current val bits
#define OP_LEFT_SHIFT  8                                                       // Shift stored val to the left by current val bits
#define OP_RIGHT_SHIFT 9                                                       // Shift stored val to the right by current val bits
#define OP_AND         10                                                      // Logically AND each bit of stored val with each bit of current val
#define OP_OR          11                                                      // Logically OR each bit of stored val with each bit of current val
#define OP_NOR         12                                                      // Logically NOR each bit of stored val with each bit of current val
#define OP_XOR         13                                                      // Logically XOR each bit of stored val with each bit of current val

// Bit Depth Modes:
#define BIT_DEPTH_8  0x00000000000000FF                                        // Bit-mask for an 8-bit number
#define BIT_DEPTH_16 0x000000000000FFFF                                        // Bit-mask for an 16-bit number
#define BIT_DEPTH_24 0x0000000000FFFFFF                                        // Bit-mask for an 24-bit number (used only for 24-bit color mode)
#define BIT_DEPTH_32 0x00000000FFFFFFFF                                        // Bit-mask for an 32-bit number
#define BIT_DEPTH_64 0xFFFFFFFFFFFFFFFF                                        // Bit-mask for an 64-bit number

// RGB Color Modes:
#define RGB_888 0                                                              // 24-bit color mode flag (8 bits each for red, green and blue)
#define RGB_565 1                                                              // 16-bit color mode flag (5 bits red, 6 bits green, 5 bits blue)




class Calculator{
  public:
    uint64_t val_current = 0;                                                  // Current value being edited
    uint64_t val_stored  = 0;                                                  // Value currently stored for an operation
    uint64_t val_result  = 0;                                                  // Value storing the result of the last operation
    uint8_t  op_command = OP_NONE;                                             // Currently selected operator

    uint64_t bitMask = BIT_DEPTH_16;                                           // bitMask for the currently selected bit-depth
    uint8_t  bitDepth = 16;                                                    // currently selected bitDepth (can be 8, 16, 24, 32, 64)
    uint8_t  base = 16;                                                        // currently selected base (can be 8-octal, 10-decimal, 16-hexidecimal)

    uint8_t  color_mode = RGB_888;                                             // currently selected color mode (RGB_888 or RGB_565)
    bool     store_flag = false;                                               // Flag that indicates whether new number keys should trigger a store of the current value
    bool     result_active = false;                                            // Flag that indicates if the equals was just pressed

  public:
    Calculator(){ setBitDepth16(); setBase16(); setColorMode565(); };          // Constructor

    // Mode Selection
    void setBitDepth8(){  bitDepth = 8;  bitMask = BIT_DEPTH_8;   }            // Select a bitDepth of 8-bits
    void setBitDepth16(){ bitDepth = 16; bitMask = BIT_DEPTH_16; }             // Select a bitDepth of 16-bits
    void setBitDepth24(){ bitDepth = 24; bitMask = BIT_DEPTH_24; }             // Select a bitDepth of 24-bits
    void setBitDepth32(){ bitDepth = 32; bitMask = BIT_DEPTH_32; }             // Select a bitDepth of 32-bits
    void setBitDepth64(){ bitDepth = 64; bitMask = BIT_DEPTH_64; }             // Select a bitDepth of 64-bits

    void setBase8(){  base =  8; }                                             // Set base to 8-bit  (octal)
    void setBase10(){ base = 10; }                                             // Set base to 10-bit (decimal)
    void setBase16(){ base = 16; }                                             // Set base to 16-bit (hexidecimal)

    void setColorMode888(){                                                    // Set color mode to 24-bit color mode
      setBitDepth24();                                                         // Change the bit depth to 24 bits
      setBase16();                                                             // Switch into hexidecimal mode for the visualization
      if( color_mode == RGB_565 ){                                             // See if we are currently in 565 color mode and need to translate the current value 
        color_mode = RGB_888;                                                  // Switch color_mode to 24-bit mode
        uint32_t redVal   = (val_current >> 11) & 0b11111;                     // Calculate the red value from the current 565 color value
        uint32_t greenVal = (val_current >> 5)  & 0b111111;                    // Calculate the green value from the current 565 color value
        uint32_t blueVal  = (val_current >> 0)  & 0b11111;                     // Calculate the blue value from the current 565 color value
        val_current = (redVal << 19) | (greenVal << 10) | (blueVal << 3);      // Compile the red, green and blue into a single 24 bit value
      }
    }
    void setColorMode565(){                                                    // Set color mode to 16-bit color mode
      setBase16();                                                             // Switch into hexidecimal mode for the visualization
      if( color_mode == RGB_888 ){                                             // See if we are currently in 888 color mode and need to translate the current value 
        color_mode = RGB_565;                                                  // Switch color_mode to 16-bit mode
        uint32_t redVal   = (val_current >> 19) & 0b11111;                     // Calculate the red value from the current 888 color value
        uint32_t greenVal = (val_current >> 10) & 0b111111;                    // Calculate the green value from the current 888 color value
        uint32_t blueVal  = (val_current >> 3)  & 0b11111;                     // Calculate the blue value from the current 888 color value
        val_current = (redVal << 11) | (greenVal << 5) | (blueVal << 0);       // Compile the red, green and blue into a single 16 bit value
      }
      setBitDepth16();                                                         // Change the bit depth to 16 bits
    }


    // Data Entry
    void enterDigit( uint8_t digit ){                                          // Adds a digit to the current value
      if( store_flag ) store();                                                // If the delayed store flag was set, then we need to store the current value and start a new one
      result_active = false;                                                   // Reset the equals flag so we know that val_current no longer represents the result
      val_current = val_current * base + digit;                                // Multiply by the base to shift the value over one digit and add the new digit
    }
    void store(){    val_stored = val_current; val_current = 0; store_flag    = false;} // Store the current value 
    void clear(){                              val_current = 0; result_active = false;} // Clear the current value
    void allClear(){ val_stored = 0;           val_current = 0; result_active = false; op_command = OP_NONE;} // Clear ALL values

    // Two-step Math Functions
    void plusBy(){       store_flag = true; result_active = false; op_command = OP_PLUS;        } // Set the operator for addition
    void minusBy(){      store_flag = true; result_active = false; op_command = OP_MINUS;       } // Set the operator for 
    void divideBy(){     store_flag = true; result_active = false; op_command = OP_DIVIDE;      } // Set the operator for 
    void modBy(){        store_flag = true; result_active = false; op_command = OP_MOD;         } // Set the operator for 
    void multiplyBy(){   store_flag = true; result_active = false; op_command = OP_MULTIPLY;    } // Set the operator for 

    void leftShiftBy(){  store_flag = true; result_active = false; op_command = OP_LEFT_SHIFT;  } // Set the operator for 
    void rightShiftBy(){ store_flag = true; result_active = false; op_command = OP_RIGHT_SHIFT; } // Set the operator for 
    void rorBy(){        store_flag = true; result_active = false; op_command = OP_ROR;         } // Set the operator for 
    void rolBy(){        store_flag = true; result_active = false; op_command = OP_ROL;         } // Set the operator for 

    void andWith(){      store_flag = true; result_active = false; op_command = OP_AND;         } // Set the operator for 
    void orWith(){       store_flag = true; result_active = false; op_command = OP_OR;          } // Set the operator for 
    void norWith(){      store_flag = true; result_active = false; op_command = OP_NOR;         } // Set the operator for 
    void xorWith(){      store_flag = true; result_active = false; op_command = OP_XOR;         } // Set the operator for 


    // One-step Math Functions
    void leftShift(){  val_current = (val_current << 1) & bitMask; }           // Left shift the current value by one bit
    void rightShift(){ val_current = (val_current >> 1) & bitMask; }           // Right shift the current value by one bit
    void rol(){ val_current = (val_current << 1) | ((val_current >> (bitDepth-1)) & 0b1); }     // Left rotate by one bit
    void ror(){ val_current = (val_current >> 1) | ((val_current << (bitDepth-1)) & bitMask); } // Right rotate by one bit
    void onesCompliment(){ val_current = (~val_current) & bitMask; }           // Calculate the 1's compliment of the current value
    void twosCompliment(){ val_current = (~val_current + 1) & bitMask; }       // Calculate the 2's compliment of the current value

    void byteFlip(){                                                           // Reverse the order of the bytes in current value
      switch( bitDepth ){                                                      // The byte flipping depends on the bit depth
        case 16:                                                               // If 16 bit number, just swap the two bytes. (note: can't byte flip in 8-bit mode)
          val_current = ((val_current << 8) | (val_current >> 8)) & bitMask;   // Move the last byte left by one byte and the second to last byte right
          break;
        case 24:                                                               // If 24 bit number
          val_current = (( val_current & 0xFF0000 ) >> 24 ) |                  // Move byte 2 into byte 0
                        (( val_current & 0x00FF00 ) << 0  ) |                  // Move byte 1 into byte 1
                        (( val_current & 0x0000FF ) << 24 );                   // Move byte 0 into byte 2
          break;
        case 32:                                                               // If 32 bit number
          val_current = (( val_current & 0xFF000000 ) >> 24 ) |                // Move byte 3 into byte 0
                        (( val_current & 0x00FF0000 ) >> 8  ) |                // Move byte 2 into byte 1
                        (( val_current & 0x0000FF00 ) << 8  ) |                // Move byte 1 into byte 2
                        (( val_current & 0x000000FF ) << 24 );                 // Move byte 0 into byte 3
          break;
        case 64:                                                               // If 64 bit number
          val_current = (( val_current & 0xFF00000000000000 ) >> 56 ) |        // Move byte 7 into byte 0
                        (( val_current & 0x00FF000000000000 ) >> 40 ) |        // Move byte 6 into byte 1
                        (( val_current & 0x0000FF0000000000 ) >> 24 ) |        // Move byte 5 into byte 2
                        (( val_current & 0x000000FF00000000 ) >> 8  ) |        // Move byte 4 into byte 3
                        (( val_current & 0x00000000FF000000 ) << 8  ) |        // Move byte 3 into byte 4
                        (( val_current & 0x0000000000FF0000 ) << 24 ) |        // Move byte 2 into byte 5
                        (( val_current & 0x000000000000FF00 ) << 40 ) |        // Move byte 1 into byte 6
                        (( val_current & 0x00000000000000FF ) << 56 );         // Move byte 0 into byte 7
          break;
      }
      val_current = val_current; 
    }

    void wordFlip(){                                                           // Reverse the order of two-byte words in current value
      switch( bitDepth ){                                                      // For 8 and 16 bits, do nothing. But for 32/64 bits:
        case 24:                                                               // If 24 bits - Not really a real word flip, but useful for color swaps (rotates color channels)
          val_current = (( val_current & 0xFF0000 ) >> 16 ) |                  // Move bytes 2 into positions 0 
                        (( val_current & 0x00FFFF ) <<  8 );                   // Move bytes 1 and 0 into positions 2 and 1 
          break;
        case 32:                                                               // If 32 bits
          val_current = (( val_current & 0xFFFF0000 ) >> 16 ) |                // Move bytes 3 and 2 into positions 1 and 0 
                        (( val_current & 0x0000FFFF ) << 16 );                 // Move bytes 1 and 0 into positions 3 and 2 
          break;
        case 64:                                                               // If 64 bits
          val_current = (( val_current & 0xFFFF000000000000 ) >> 48 ) |        // Move bytes 7 and 6 into positions 1 and 0 
                        (( val_current & 0x0000FFFF00000000 ) >> 16 ) |        // Move bytes 5 and 4 into positions 3 and 2 
                        (( val_current & 0x00000000FFFF0000 ) << 16 ) |        // Move bytes 3 and 2 into positions 5 and 4 
                        (( val_current & 0x000000000000FFFF ) << 48 );         // Move bytes 1 and 0 into positions 7 and 6 
          break;
      }
    }

    void incRed(){                                                                      // Increment the portion of val_current related to the red channel
      if( color_mode == RGB_888 ){                                                      // If 24-bit color mode
        val_current = ((val_current + 0x010000) & 0xFF0000) | (val_current & 0x00FFFF); // Only increment bits 16 through 23
      } else {                                                                          // If 16-bit color mode
        val_current = ((val_current + 0x0800) & 0xF800) | (val_current & 0x07FF);       // Only increment bits 11 through 15
      }
    }
    void decRed(){                                                                      // Decrement the portion of val_current related to the red channel
      if( color_mode == RGB_888 ){                                                      // If 24-bit color mode
        val_current = ((val_current - 0x010000) & 0xFF0000) | (val_current & 0x00FFFF); // Only decrement bits 16 through 23
      } else {                                                                          // If 16-bit color mode
        val_current = ((val_current - 0x0800) & 0xF800) | (val_current & 0x07FF);       // Only decrement bits 11 through 15 
      }
    }
    void incGreen(){                                                                    // Increment the portion of val_current related to the green channel
      if( color_mode == RGB_888 ){                                                      // If 24-bit color mode
        val_current = ((val_current + 0x000100) & 0x00FF00) | (val_current & 0xFF00FF); // Only increment bits 8 through 15
      } else {                                                                          // If 16-bit color mode
        val_current = ((val_current + 0x0020) & 0x07E0) | (val_current & 0xF81F);       // Only increment bits 5 through 10
      }
    }
    void decGreen(){                                                                    // Decrement the portion of val_current related to the green channel
      if( color_mode == RGB_888 ){                                                      // If 24-bit color mode
        val_current = ((val_current - 0x000100) & 0x00FF00) | (val_current & 0xFF00FF); // Only decrement bits 8 through 15
      } else {                                                                          // If 16-bit color mode
        val_current = ((val_current - 0x0020) & 0x07E0) | (val_current & 0xF81F);       // Only decrement bits 5 through 10
      }
    }
    void incBlue(){                                                                     // Increment the portion of val_current related to the blue channel
      if( color_mode == RGB_888 ){                                                      // If 24-bit color mode
        val_current = ((val_current + 0x000001) & 0x0000FF) | (val_current & 0xFFFF00); // Only increment bits 0 through 7
      } else {                                                                          // If 16-bit color mode
        val_current = ((val_current + 0x0001) & 0x001F) | (val_current & 0xFFE0);       // Only increment bits 0 through 4
      }
    }
    void decBlue(){                                                                     // Decrement the portion of val_current related to the blue channel
      if( color_mode == RGB_888 ){                                                      // If 24-bit color mode
        val_current = ((val_current - 0x000001) & 0x0000FF) | (val_current & 0xFFFF00); // Only decrement bits 0 through 7
      } else {                                                                          // If 16-bit color mode
        val_current = ((val_current - 0x0001) & 0x001F) | (val_current & 0xFFE0);       // Only decrement bits 0 through 4
      }
    }

    void equals(){                                                                      // Perform the op_command on val_current and val_stored, and save in val_result
      uint64_t l_operand, r_operand;                                                    // Temporary stores for the left and right operands
      if( result_active ){                                                              // See if the result of the last calculation is still stored in val_current
        r_operand = val_stored;                                                         // If it is, then the use the val_stored as the right operand
        l_operand = val_current;                                                        // and val_current as the left operand. This way you can keep repeating the last operation
      } else {                                                                          // Otherwise...
        l_operand = val_stored;                                                         // Use val_stored as the left operand
        r_operand = val_current;                                                        // and val_current as the right operand so it functions like a calculator normally would.
      }
      switch( op_command ){                                                             // Based on the op_command, execute operations that require both the current and stored operators
        case OP_PLUS:        val_result =  (l_operand +  r_operand) & bitMask; break;   // Add two numbers together and truncate based on the bit-depth
        case OP_MINUS:       val_result =  (l_operand -  r_operand) & bitMask; break;   // Subtract right number from left and truncate based on the bit-depth
        case OP_MULTIPLY:    val_result =  (l_operand *  r_operand) & bitMask; break;   // Multiply two numbers together and truncate based on the bit-depth
        case OP_DIVIDE:      val_result =  (l_operand /  r_operand) & bitMask; break;   // Divide right number from left and truncate based on the bit-depth
        case OP_MOD:         val_result =  (l_operand %  r_operand) & bitMask; break;   // Mod right number from left number and truncate based on the bit-depth
        case OP_LEFT_SHIFT:  val_result =  (l_operand << r_operand) & bitMask; break;   // Left shift the left number by right number's bits and truncate based on the bit-depth
        case OP_RIGHT_SHIFT: val_result =  (l_operand >> r_operand) & bitMask; break;   // Right shift the left number by right number's bits and truncate based on the bit-depth
        case OP_AND:         val_result =  (l_operand &  r_operand) & bitMask; break;   // AND two numbers together and truncate based on the bit-depth
        case OP_OR:          val_result =  (l_operand |  r_operand) & bitMask; break;   // OR  two numbers together and truncate based on the bit-depth
        case OP_NOR:         val_result = ~(l_operand |  r_operand) & bitMask; break;   // NOR two numbers together and truncate based on the bit-depth
        case OP_XOR:         val_result =  (l_operand ^  r_operand) & bitMask; break;   // XOR two numbers together and truncate based on the bit-depth
        case OP_ROL: val_result = ((l_operand << r_operand) & bitMask) | ((l_operand >> (bitDepth - r_operand)) & bitMask); break; // Left rotate right number by left number digits
        case OP_ROR: val_result = ((l_operand >> r_operand) & bitMask) | ((l_operand << (bitDepth - r_operand)) & bitMask); break; // Right rotate the right number by left number digits
      }
      if( !result_active ) val_stored = val_current;                                    // If the result isn't already active, then save the current value into the stored value
      result_active = true;                                                             // Set the result_active flag to true
      val_current = val_result;                                                         // Save the result into the val_current
    }

};


#endif