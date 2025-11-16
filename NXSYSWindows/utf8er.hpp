#if !TEST_UTF8ER
#pragma once
#endif

#include <bit>
#include <string>

/* 14 Nov 2025 -- the native unicode-encoding handling facilities were removed from the language. */

class utf8er {
    static constexpr char16_t SUB = 0xFFFD;
    static constexpr char32_t LEGAL_BASES[3]{0x80, 0x800, 0x10000};
public:
    template<class InputCharType>
    static std::string to_utf8 (const InputCharType& input) { /* anything iterable */
        std::string output;
        for (uint32_t  IC : input) {
            if (IC < 0x80)
                output += (char)IC;
            else {
                if (IC >= 0x110000) { /* not all 32-bit #'s legit */
                    output += to_utf8(std::basic_string<char16_t> {SUB});
                    continue;
                }
                uint8_t bit_length = 32 - std::countl_zero(IC);
                /* > BMP0 must use 4-byte model, even though 3 could fit*/
                int byte_length = (bit_length > 16) ? 4 : (bit_length + 5) / 6;
                for (int j = 0; j < byte_length; j++) {
                    int k = byte_length - 1 - j;
                    char unsigned b = (IC >> k*6) & 0x3F;
                    if (j == 0)
                        b |= ((unsigned char)0xF0) << (4 - byte_length);
                    else
                        b |= 0x80;
                    output += b;
                }
                
            }
        }
        return output;
    }
    template<class OutputCharType, class InputCharType>
    static std::basic_string<OutputCharType>to_wchar(const std::basic_string<InputCharType>& S) {
        std::basic_string<OutputCharType> result;
        auto p = S.begin();
        auto pend = S.end();
        while (p != pend) {
            char unsigned b0 = *p++;
            /* nones = "number of (leading) 1 bits" */
            int nones = std::min(4, std::countl_zero((char unsigned)(b0 ^ 0xFF)));
            if (nones == 0) {  // Single simple ASCII byte < 0x80
                result += b0;
                continue;
            }
            else if (nones == 1 || (pend - p + 1 < nones)) {
                result += SUB; //trail byte where lead byte expected, or not enough
                continue;
            }
            OutputCharType OutChar = b0 & (0xFF >> (nones+1));  //collect lead-byte properly
            for (int i = 1; i < nones; i++) {
                unsigned char b = *p++;
                if ((b & 0xC0) != 0x80) { //must be trail byte 0x80+pl
                    OutChar = SUB;
                    break;
                }
                OutChar = (OutChar << 6) | (b & 0x3F); //incorporate trail-byte payload
            }
            // assert (nones >= 2); nones=0 handled above, nones=1 illegal and handled.
            if (OutChar < LEGAL_BASES[nones-2])  // Trap "overlong fake aliasing"
                OutChar = SUB;

            result += OutChar;
        }
        return result;
    }
};


#if TEST_UTF8ER
#define CYRILLIC_LETTER_DE  0x0414
#define CYRILLIC_LETTER_SHA 0x0428
#define CYRILLIC_LETTER_YU  0x042E
#define BLACK_TELEPHONE 0x260E
#define FACE_WITH_TONGUE 0x1F61B
#define PIG_NOSE 0x1F43D
#define SUB 0xFFFD


using WSTR=std::basic_string<char32_t>;
using UCSTR=std::basic_string<char unsigned>;

void dump_ws(const char* header, const WSTR& S) {
    printf("\n%s:\n", header);
    int i = 0;
    for (auto c : S)
        printf("%2d %04x\n", i++, c);
}
template<class ICT>
void dump_bytes(const char* header, const std::basic_string<ICT>& S) {
    printf ("%s:\n", header);
    for (unsigned char c : S)
        printf (" %02x ", c);
    printf("\n");
}

int main (int argc, const char** argv) {
    WSTR S {
        'a',
        'E',
        0x3FFFFFFF, // deliberate out-of-range
        '%',
        CYRILLIC_LETTER_YU,
        CYRILLIC_LETTER_DE,
        CYRILLIC_LETTER_SHA,
        BLACK_TELEPHONE,
        PIG_NOSE,
        SUB,
        CYRILLIC_LETTER_DE // don't let last be SUB, so can test that.
};
    dump_ws("Original utf-32 characters", S);
    auto answer = utf8er::to_utf8(S);
    dump_bytes("UTF-8 bytes of result", answer);
    printf("Result as string: %s\n", answer.c_str());
    dump_ws("Back-translation of utf-8 result", utf8er::to_wchar<char32_t>(answer));
    answer.pop_back(); /* test premature end detection */
    dump_ws("Same with last utf8 byte deliberately clipped", utf8er::to_wchar<char32_t>(answer));
    UCSTR bogo_ovl {0xC1, 0x85}; //0x45 (E) expressed as 2 bytes where only 1 required
    dump_bytes("Deliberate overlength encoding of 0x45 'E'", bogo_ovl);
    dump_ws("Back-translation of overlength encoding (should give 0xfffd)",
            utf8er::to_wchar<char32_t>(bogo_ovl));
}
#endif // TEST_UTF8ER

