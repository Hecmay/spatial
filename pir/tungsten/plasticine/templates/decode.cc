#include "decode.h"
// compression constants
namespace decodeConstants {
  extern uint8_t const MASK_FORMAT  	= 0b1100'0000;
  extern uint8_t const MASK_OFF  	= 0b0000'0011;
  extern uint8_t const MASK_BASE 	= 0b0001'1000;

  extern uint8_t const MASK_ORIG       	= 0b1000'0000; // original format
  //extern uint8_t const MASK_BROADCAST  	= 0b1010'0000; // base only, only one value in vector
  extern uint8_t const MASK_BASEOFF    	= 0b0100'0000; // base and offset 
  extern uint8_t const MASK_SEQUENTIAL 	= 0b1100'0000; // sequential storage
  extern uint8_t const MASK_PREVBASE   	= 0b0010'0000; // has offset only 

  extern uint8_t const MASK_BITS1  =  0b0000'0100;
  extern uint8_t const MASK_BITS2  =  0b0000'0101;
  extern uint8_t const MASK_BITS4  =  0b0000'0111;
  extern uint8_t const MASK_BYTES1 =  0b0000'0000;
  extern uint8_t const MASK_BYTES2 =  0b0000'0001;
  extern uint8_t const MASK_BYTES3 =  0b0000'0010;
  extern uint8_t const MASK_BYTES4 =  0b0000'0011;
  extern int const MASK_BIT = 2;

  extern uint8_t const MASK_BASE_BYTES1 = 0b0000'0000;
  extern uint8_t const MASK_BASE_BYTES2 = 0b0000'1000;
  extern uint8_t const MASK_BASE_BYTES3 = 0b0001'0000;
  extern uint8_t const MASK_BASE_BYTES4 = 0b0001'1000;

  extern int const TAG_BYTES = 1;
  extern uint8_t const TOP_MASK = 0b0111'1111;
}

