/*

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#ifndef _OPCODES_H_
#define _OPCODES_H_
#include <stdint.h>

enum opr_e {
    ADR_IMPLIED=0, ADR_ACCU, ADR_IMMEDIATE, ADR_LONG, ADR_ADDR, ADR_ZP,
    ADR_LONG_X, ADR_ADDR_X, ADR_ZP_X, ADR_ADDR_X_I, ADR_ZP_X_I, ADR_ZP_S,
    ADR_ZP_S_I_Y, ADR_ADDR_Y, ADR_ZP_Y, ADR_ZP_LI_Y, ADR_ZP_I_Y, ADR_ADDR_LI,
    ADR_ZP_LI, ADR_ADDR_I, ADR_ZP_I, ADR_REL_L, ADR_REL, ADR_MOVE,
    ADR_ZP_R, ADR_ZP_R_I_Y
};

#define OPCODES_65816 111
#define OPCODES_6502 68
#define OPCODES_65C02 79
#define OPCODES_6502i 98
#define OPCODES_65DTV02 89
#define OPCODES_65EL02 118
#define ____ 0x69
// 0x42 =WDM
extern const uint8_t c65816[];
#define MNEMONIC65816 "adcandaslbccbcsbeqbgebitbltbmibnebplbrabrkbrlbvcbvsclccldcliclvcmpcopcpxcpydeadecdexdeyeorgccgcsgeqggegltgmignegplgragvcgvsinaincinxinyjmljmpjsljsrldaldxldylsrmvnmvpnoporapeapeiperphaphbphdphkphpphxphyplaplbpldplpplxplyreprolrorrtirtlrtssbcsecsedseisepstastpstxstystzswatadtastaxtaytcdtcstdatdctrbtsatsbtsctsxtxatxstxytyatyxwaixbaxce"
//                      1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100101102103104105106107108109110111
extern const uint8_t c6502[];
#define MNEMONIC6502 "adcandaslbccbcsbeqbgebitbltbmibnebplbrkbvcbvsclccldcliclvcmpcpxcpydecdexdeyeorgccgcsgeqggegltgmignegplgvcgvsincinxinyjmpjsrldaldxldylsrnoporaphaphpplaplprolrorrtirtssbcsecsedseistastxstytaxtaytsxtxatxstya";
//                     1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68
extern const uint8_t c65c02[];
#define MNEMONIC65C02 "adcandaslbccbcsbeqbgebitbltbmibnebplbrabrkbvcbvsclccldcliclvcmpcpxcpydeadecdexdeyeorgccgcsgeqggegltgmignegplgragvcgvsinaincinxinyjmpjsrldaldxldylsrnoporaphaphpphxphyplaplpplxplyrolrorrtirtssbcsecsedseistastxstystztaxtaytrbtsbtsxtxatxstya";
//                      1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79
extern const uint8_t c6502i[];
#define MNEMONIC6502i "adcahxalrancandanearraslasraxsbccbcsbeqbgebitbltbmibnebplbrkbvcbvsclccldcliclvcmpcpxcpydcmdcpdecdexdeyeorgccgcsgeqggegltgmignegplgvcgvsincinsinxinyisbiscjamjmpjsrlaelaslaxldaldsldxldylsrlxanoporaphaphpplaplprlarolrorrrartirtssaxsbcsbxsecsedseishashsshxshyslosrestastxstytastaxtaytsxtxatxstyaxaa";
//                      1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98
extern const uint8_t c65dtv02[];
#define MNEMONIC65DTV02 "adcalrandanearraslasrbccbcsbeqbgebitbltbmibnebplbrabrkbvcbvsclccldcliclvcmpcpxcpydcmdcpdecdexdeyeorgccgcsgeqggegltgmignegplgragvcgvsincinsinxinyisbiscjmpjsrlaxldaldxldylsrlxanoporaphaphpplaplprlarolrorrrartirtssacsaxsbcsecsedseisirslosrestastxstytaxtaytsxtxatxstyaxaa";
//                        1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89
extern const uint8_t c65el02[];
#define MNEMONIC65EL02 "adcandaslbccbcsbeqbgebitbltbmibnebplbrabrkbvcbvsclccldcliclvcmpcpxcpydeadecdexdeydiventeorgccgcsgeqggegltgmignegplgragvcgvsinaincinxinyjmpjsrldaldxldylsrmmumulnopnxanxtorapeapeiperphaphdphpphxphyplapldplpplxplyreareireprerrharhirhxrhyrlarlirlxrlyrolrorrtirtssbcseasecsedseisepstastpstxstystzswatadtaxtaytdatixtrbtrxtsbtsxtxatxitxrtxstxytyatyxwaixbaxcezea"
//                      1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100101102103104105106107108109110111112113114115116117118

#endif
