// Class to convert binary data buffer to a string and back again

#ifndef CONVERT_FORMAT
#define CONVERT_FORMAT

#include <iostream>
#include <string>


struct Conv {

    // not the best way to use a string
    static std::string toString(char *pData, int64_t length) {
        std::string str;
        for (int64_t i = 0; i < length; i++) {
            char ch[2];
            char t = pData[i];
            str += toChar((pData[i] >> 4) & 0x0F);
            str += toChar(pData[i] & 0x0F);
        }

        return str;
    }

    static bool fromString(std::string str, char *pData) {
        bool ret = true;
        int64_t length = str.length();
        if (length % 2 == 1)
            return false;

        for (int64_t i = 0; i < length; i += 2) {
            pData[i / 2] = 0;
            pData[i / 2] = (((toHex(str[i])) & 0x0F) << 4) | (toHex(str[i + 1]) & 0x0F);
        }

        return ret;
    }

    static char toChar(char c) {
        if (c <= 9) {
            return c + '0';
        } else if (c >= 10 && c <= 15) {
            return c + '7';
        }
    }

    static char toHex(char c) {
        if (c >= '0' && c <= '9') {
            return c - '0';
        } else if (c >= 'A' && c <= 'F') {
            return c - 55;
        } else if (c >= 'a' && c <= 'f') {
            return c - 87;
        } else return 0;
    }


};

#endif