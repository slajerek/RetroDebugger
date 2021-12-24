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
#ifndef _ERROR_H_
#define _ERROR_H_
#include <stdint.h>
#include "64tass-misc.h"

extern unsigned int errors, conderrors, warnings;

// ---------------------------------------------------------------------------
// $00-$3f warning
// $40-$7f error
// $80-$bf fatal
enum errors_e {
    ERROR_TOP_OF_MEMORY=0x00,
    ERROR_A_USED_AS_LBL,
    ERROR___BANK_BORDER,
    ERROR______JUMP_BUG,
    ERROR___LONG_BRANCH,
    ERROR_DIRECTIVE_IGN,
    ERROR_LABEL_NOT_LEF,
    ERROR_WUSER_DEFINED,

    ERROR_DOUBLE_DEFINE=0x40,
    ERROR___NOT_DEFINED,
    ERROR_EXTRA_CHAR_OL,
    ERROR_CONSTNT_LARGE,
    ERROR_GENERL_SYNTAX,
    ERROR______EXPECTED,
    ERROR_EXPRES_SYNTAX,
    ERROR_BRANCH_TOOFAR,
    ERROR_MISSING_ARGUM,
    ERROR_ILLEGAL_OPERA,
    ERROR_REQUIREMENTS_,
    ERROR______CONFLICT,
    ERROR_DIVISION_BY_Z,
    ERROR____WRONG_TYPE,
    ERROR___UNKNOWN_CHR,
    ERROR___NOT_ALLOWED,
    ERROR____PAGE_ERROR,
    ERROR__BRANCH_CROSS,

    ERROR_CANT_FINDFILE=0x80,
    ERROR__READING_FILE,
    ERROR_OUT_OF_MEMORY,
    ERROR_CANT_WRTE_OBJ,
    ERROR_LINE_TOO_LONG,
    ERROR_CANT_DUMP_LST,
    ERROR_CANT_DUMP_LBL,
    ERROR__USER_DEFINED,
    ERROR_FILERECURSION,
    ERROR__MACRECURSION,
    ERROR___UNKNOWN_CPU,
    ERROR_UNKNOWN_OPTIO,
    ERROR_TOO_MANY_PASS,
    ERROR__TOO_MANY_ERR
};

extern void err_msg(enum errors_e, const char*);
extern void err_msg2(enum errors_e, const char*, unsigned int);
extern void err_msg_wrong_type(enum type_e, unsigned int);
extern void freeerrorlist(int);
extern void enterfile(const char *, line_t);
extern void exitfile(void);
extern void err_destroy(void);

#endif
