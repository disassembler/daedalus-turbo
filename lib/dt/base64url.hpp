/* This file is part of Daedalus Turbo project: https://github.com/sierkov/daedalus-turbo/
 * Copyright (c) 2022-2024 Alex Sierkov (alex dot sierkov at gmail dot com)
 * This code is distributed under the license specified in:
 * https://github.com/sierkov/daedalus-turbo/blob/main/LICENSE */
#ifndef DAEDALUS_TURBO_BASE64URL_HPP
#define DAEDALUS_TURBO_BASE64URL_HPP

#include <string_view>

#include <dt/util.hpp>

namespace daedalus_turbo::base64url {

    static uint8_vector decode(const std::string_view &in)
    {
        static const std::array<signed char, 128> codes {
            /* 0x00 */ -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
            /* 0x10 */ -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
            /* 0x20 */ -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  62,  -1,  -1,
            /* 0x30 */ 52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  -1,  -1,  -1,  -1,  -1,  -1,
            /* 0x40 */ -1,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
            /* 0x50 */ 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  -1,  -1,  -1,  -1,  63,
            /* 0x60 */ -1,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
            /* 0x70 */ 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  -1,  -1,  -1,  -1,  -1
        };
        uint8_vector out {};
        int val = 0, valb = -8;
        for (signed char c: in) {
            if (c == '=') break;
            if (c < 0 || codes[c] == -1) throw error("unsupported base64url character: '{}' in {}!", c, in);
            val = (val << 6) + codes[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

}

#endif // !DAEDALUS_TURBO_BASE64URL_HPP