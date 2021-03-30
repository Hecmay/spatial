#include "compress.h"

void get_tag_base(uint8_t& tag, int& base_bits){
  if(base_bits <= 8){
    tag = tag | MASK_BASE_BYTES1;
    base_bits = 8;
  } else if (base_bits <= 16){
    tag = tag | MASK_BASE_BYTES2;
    base_bits = 16; 
  } else if (base_bits <= 24){
    tag = tag | MASK_BASE_BYTES3;
    base_bits = 24; 
  } else {
    tag = tag | MASK_BASE_BYTES4;
    base_bits = 32;
  }
  return;
}

void get_tag_off(uint8_t& tag, int& offset_bits){
  if(offset_bits <= 1){
    tag = tag | MASK_BITS1;
    offset_bits = 1;
  } else if (offset_bits <= 2){
    tag = tag | MASK_BITS2; 
    offset_bits = 2;
  } else if (offset_bits <= 4){
    tag = tag | MASK_BITS4; 
    offset_bits = 4;
  } else if (offset_bits <= 8){
    tag = tag | MASK_BYTES1;
    offset_bits = 8;
  } else if (offset_bits <= 16){
    tag = tag | MASK_BYTES2; 
    offset_bits = 16;
  } else if (offset_bits <= 24){
    tag = tag | MASK_BYTES3; 
    offset_bits = 24;
  } else {
    tag = tag | MASK_BYTES4;
    offset_bits = 32;
  }
  return;
}

