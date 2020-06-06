/*
 *******************************
 * Intel Hex Parser Library    *
 * Version: 0.99 "Benja"       *
 *******************************
 */

#ifndef _IHEX_PARSER_H_
#define _IHEX_PARSER_H_

#include <string>

using namespace std;

namespace hexparser {
    
#define IHEX_START_CODE ':'

class HexParser {
   public:
    HexParser();
    ~HexParser();
    bool ParseIHexFormat(string serialized_file, uint8_t *payload);
    uint16_t GetIHexSize(string serialized_file);

   protected:
   private:
};

}  // namespace hexparser

#endif  // _IHEX_PARSER_H_