/*
 *******************************
 * Intel Hex Parser Library    *
 * Version: 0.99 "Benja"       *
 *******************************
 */

#ifndef _IHEX_PARSER_H_
#define _IHEX_PARSER_H_

#include <Arduino.h>

class HexParser {
   public:
    HexParser();
    ~HexParser();
    void GetIHexPayload(String serialized_file, uint8_t *payload);
    uint16_t GetIHexSize(String serialized_file);

   protected:
   private:
};

#endif  // _IHEX_PARSER_H_