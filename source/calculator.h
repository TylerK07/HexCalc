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
This library is basically a calculatror finite state machine that manages
the state of the current calculations on a button by button level.
*/


/*******************************************
* Calculator Class Definition              *
*******************************************/

// Operation Identifiers
#define OP_NONE        0
#define OP_PLUS        1
#define OP_MINUS       2
#define OP_MULTIPLY    3
#define OP_DIVIDE      4
#define OP_MOD         5
#define OP_ROL         6
#define OP_ROR         7
#define OP_LEFT_SHIFT  8
#define OP_RIGHT_SHIFT 9
#define OP_AND         10
#define OP_OR          11
#define OP_NOR         12
#define OP_XOR         13

// Bit Depth Modes:
#define BIT_DEPTH_8  0x00000000000000FF
#define BIT_DEPTH_16 0x000000000000FFFF
#define BIT_DEPTH_24 0x0000000000FFFFFF
#define BIT_DEPTH_32 0x00000000FFFFFFFF
#define BIT_DEPTH_64 0xFFFFFFFFFFFFFFFF

// RGB Color Modes:
#define RGB_888 0
#define RGB_565 1




class Calculator{
  public:
    uint64_t val_current = 0;                                                  // Current value being edited
    uint64_t val_stored  = 0;                                                  // Value currently stored for an operation
    uint64_t val_result  = 0;

    uint8_t  op_command = OP_NONE;

    uint64_t bitMask = BIT_DEPTH_16;
    uint8_t  base = 16;

    uint8_t  bitDepth = 16;
    uint8_t  color_mode = RGB_888;
    bool     store_flag = false;                                               // Flag that indicates whether new number keys should trigger a store of the current value
    bool     result_active = false;                                            // Indicates if the equals was just pressed

  public:

    Calculator(){ setBitDepth16(); setBase16(); setColorMode565(); };          // Constructor

    // Mode Selection
    void setBitDepth8(){  bitDepth = 8;  bitMask = BIT_DEPTH_8;  }
    void setBitDepth16(){ bitDepth = 16; bitMask = BIT_DEPTH_16; }
    void setBitDepth24(){ bitDepth = 24; bitMask = BIT_DEPTH_24; }
    void setBitDepth32(){ bitDepth = 32; bitMask = BIT_DEPTH_32; }
    void setBitDepth64(){ bitDepth = 64; bitMask = BIT_DEPTH_64; }

    void setBase8(){  base =  8; }
    void setBase10(){ base = 10; }
    void setBase16(){ base = 16; }

    void setColorMode888(){ 
      setBitDepth24();
      setBase16();
      if( color_mode == RGB_565 ){
        color_mode = RGB_888; 
        uint32_t redVal   = (val_current >> 11) & 0b11111;
        uint32_t greenVal = (val_current >> 5)  & 0b111111;
        uint32_t blueVal  = (val_current >> 0)  & 0b11111;
        val_current = (redVal << 19) | (greenVal << 10) | (blueVal << 3);
      }
    }
    void setColorMode565(){ 
      setBitDepth16();
      setBase16();
      if( color_mode == RGB_888 ){
        color_mode = RGB_565; 
        uint32_t redVal   = (val_current >> 19) & 0b11111;
        uint32_t greenVal = (val_current >> 10) & 0b111111;
        uint32_t blueVal  = (val_current >> 3)  & 0b11111;
        val_current = (redVal << 11) | (greenVal << 5) | (blueVal << 0); 
      }
    }


    // Data Entry
    void enterDigit( uint8_t digit ){ 
      if( store_flag ) store(); 
      result_active = false;
      val_current = val_current * base + digit; 
    }
    void store(){    val_stored = val_current; val_current = 0; store_flag = false;}
    void clear(){                              val_current = 0; result_active = false;}
    void allClear(){ val_stored = 0;           val_current = 0; result_active = false; op_command = OP_NONE;}

    // Two-step Math Functions
    void plusBy(){       store_flag = true; result_active = false; op_command = OP_PLUS;        }
    void minusBy(){      store_flag = true; result_active = false; op_command = OP_MINUS;       }
    void divideBy(){     store_flag = true; result_active = false; op_command = OP_DIVIDE;      }
    void modBy(){        store_flag = true; result_active = false; op_command = OP_MOD;         }
    void multiplyBy(){   store_flag = true; result_active = false; op_command = OP_MULTIPLY;    }

    void leftShiftBy(){  store_flag = true; result_active = false; op_command = OP_LEFT_SHIFT;  }
    void rightShiftBy(){ store_flag = true; result_active = false; op_command = OP_RIGHT_SHIFT; }
    void rorBy(){        store_flag = true; result_active = false; op_command = OP_ROR;         }
    void rolBy(){        store_flag = true; result_active = false; op_command = OP_ROL;         }

    void andWith(){      store_flag = true; result_active = false; op_command = OP_AND;         }
    void orWith(){       store_flag = true; result_active = false; op_command = OP_OR;          }
    void norWith(){      store_flag = true; result_active = false; op_command = OP_NOR;         }
    void xorWith(){      store_flag = true; result_active = false; op_command = OP_XOR;         }


    // One-step Math Functions
    void leftShift(){  val_current = (val_current << 1) & bitMask; }
    void rightShift(){ val_current = (val_current >> 1) & bitMask; }
    void rol(){ val_current = (val_current << 1) || ((val_current >> (bitDepth-1)) & 0b1); }
    void ror(){ val_current = (val_current >> 1) || ((val_current << (bitDepth-1)) & bitMask); }
    void onesCompliment(){ val_current = (~val_current) & bitMask; }
    void twosCompliment(){ val_current = (~val_current + 1) & bitMask; }
    void byteFlip(){ 
      switch( bitDepth ){
        case 16:
          val_current = ((val_current << 8) | (val_current >> 8)) & bitMask;
          break;
        case 32:
          val_current = (( val_current & 0xFF000000 ) >> 24 ) |
                        (( val_current & 0x00FF0000 ) >> 8  ) |       
                        (( val_current & 0x0000FF00 ) << 8  ) |       
                        (( val_current & 0x000000FF ) << 24 );
          break;
        case 64:
          val_current = (( val_current & 0xFF00000000000000 ) >> 56 ) |
                        (( val_current & 0x00FF000000000000 ) >> 40 ) |       
                        (( val_current & 0x0000FF0000000000 ) >> 24 ) |       
                        (( val_current & 0x000000FF00000000 ) >> 8  ) |
                        (( val_current & 0x00000000FF000000 ) << 8  ) |
                        (( val_current & 0x0000000000FF0000 ) << 24 ) |
                        (( val_current & 0x000000000000FF00 ) << 40 ) |
                        (( val_current & 0x00000000000000FF ) << 56 );
          break;
      }
      
      val_current = val_current; 
    }
    void wordFlip(){
      switch( bitDepth ){
        case 32:
          val_current = (( val_current & 0xFFFF0000 ) >> 16 ) |
                        (( val_current & 0x0000FFFF ) << 16 );
          break;
        case 64:
          val_current = (( val_current & 0xFFFF000000000000 ) >> 48 ) |
                        (( val_current & 0x0000FFFF00000000 ) >> 16 ) |       
                        (( val_current & 0x00000000FFFF0000 ) << 16 ) |
                        (( val_current & 0x000000000000FFFF ) << 48 );
          break;
      }
    }
    void incRed(){ 
      if( color_mode == RGB_888 ){
        val_current = ((val_current + 0x010000) & 0xFF0000) | (val_current & 0x00FFFF); 
      } else {
        val_current = ((val_current + 0x0800) & 0xF800) | (val_current & 0x07FF); 
      }
    }
    void decRed(){ 
      if( color_mode == RGB_888 ){
        val_current = ((val_current - 0x010000) & 0xFF0000) | (val_current & 0x00FFFF); 
      } else {
        val_current = ((val_current - 0x0800) & 0xF800) | (val_current & 0x07FF); 
      }
    }
    void incGreen(){ 
      if( color_mode == RGB_888 ){
        val_current = ((val_current + 0x000100) & 0x00FF00) | (val_current & 0xFF00FF); 
      } else {
        val_current = ((val_current + 0x0020) & 0x07E0) | (val_current & 0xF81F); 
      }
    }
    void decGreen(){ 
      if( color_mode == RGB_888 ){
        val_current = ((val_current - 0x000100) & 0x00FF00) | (val_current & 0xFF00FF); 
      } else {
        val_current = ((val_current - 0x0020) & 0x07E0) | (val_current & 0xF81F); 
      }
    }
    void incBlue(){ 
      if( color_mode == RGB_888 ){
        val_current = ((val_current + 0x000001) & 0x0000FF) | (val_current & 0xFFFF00); 
      } else {
        val_current = ((val_current + 0x0001) & 0x001F) | (val_current & 0xFFE0); 
      }
    }
    void decBlue(){ 
      if( color_mode == RGB_888 ){
        val_current = ((val_current - 0x000001) & 0x0000FF) | (val_current & 0xFFFF00); 
      } else {
        val_current = ((val_current - 0x0001) & 0x001F) | (val_current & 0xFFE0); 
      }
    }

    void equals(){
      uint64_t l_operand, r_operand;
      if( result_active ){
        r_operand = val_stored;
        l_operand = val_current;
      } else {
        l_operand = val_stored;
        r_operand = val_current;
      }
      switch( op_command ){
        case OP_PLUS:        val_result =  (l_operand +  r_operand) & bitMask; break;
        case OP_MINUS:       val_result =  (l_operand -  r_operand) & bitMask; break;
        case OP_MULTIPLY:    val_result =  (l_operand *  r_operand) & bitMask; break;
        case OP_DIVIDE:      val_result =  (l_operand /  r_operand) & bitMask; break;
        case OP_MOD:         val_result =  (l_operand %  r_operand) & bitMask; break;
        case OP_LEFT_SHIFT:  val_result =  (l_operand << r_operand) & bitMask; break;
        case OP_RIGHT_SHIFT: val_result =  (l_operand >> r_operand) & bitMask; break;
        case OP_AND:         val_result =  (l_operand &  r_operand) & bitMask; break;
        case OP_OR:          val_result =  (l_operand |  r_operand) & bitMask; break;
        case OP_NOR:         val_result = ~(l_operand |  r_operand) & bitMask; break;
        case OP_XOR:         val_result =  (l_operand ^  r_operand) & bitMask; break;
        case OP_ROL:  val_result = ((l_operand << r_operand) & bitMask) | ((l_operand >> (bitDepth - r_operand)) & bitMask); break;
        case OP_ROR:  val_result = ((l_operand >> r_operand) & bitMask) | ((l_operand << (bitDepth - r_operand)) & bitMask); break;
      }
      if( !result_active ) val_stored = val_current;
      result_active = true;
      val_current = val_result;
    }

};


#endif