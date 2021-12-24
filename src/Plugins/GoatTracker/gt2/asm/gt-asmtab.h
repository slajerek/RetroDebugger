/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
# define YYTOKENTYPE
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INCLUDE = 258,
     IF = 259,
     DEFINED = 260,
     MACRO = 261,
     MACRO_STRING = 262,
     ORG = 263,
     ASMERROR = 264,
     ECHO = 265,
     INCBIN = 266,
     INCWORD = 267,
     RES = 268,
     WORD = 269,
     BYTE = 270,
     LDA = 271,
     LDX = 272,
     LDY = 273,
     STA = 274,
     STX = 275,
     STY = 276,
     AND = 277,
     ORA = 278,
     EOR = 279,
     ADC = 280,
     SBC = 281,
     CMP = 282,
     CPX = 283,
     CPY = 284,
     TSX = 285,
     TXS = 286,
     PHA = 287,
     PLA = 288,
     PHP = 289,
     PLP = 290,
     SEI = 291,
     CLI = 292,
     NOP = 293,
     TYA = 294,
     TAY = 295,
     TXA = 296,
     TAX = 297,
     CLC = 298,
     SEC = 299,
     RTS = 300,
     JSR = 301,
     JMP = 302,
     BEQ = 303,
     BNE = 304,
     BCC = 305,
     BCS = 306,
     BPL = 307,
     BMI = 308,
     BVC = 309,
     BVS = 310,
     INX = 311,
     DEX = 312,
     INY = 313,
     DEY = 314,
     INC = 315,
     DEC = 316,
     LSR = 317,
     ASL = 318,
     ROR = 319,
     ROL = 320,
     SYMBOL = 321,
     STRING = 322,
     LAND = 323,
     LOR = 324,
     LNOT = 325,
     LPAREN = 326,
     RPAREN = 327,
     COMMA = 328,
     COLON = 329,
     X = 330,
     Y = 331,
     HASH = 332,
     PLUS = 333,
     MINUS = 334,
     MULT = 335,
     DIV = 336,
     MOD = 337,
     LT = 338,
     GT = 339,
     EQ = 340,
     NEQ = 341,
     ASSIGN = 342,
     NUMBER = 343,
     vNEG = 344
   };
#endif
/* Tokens.  */
#define INCLUDE 258
#define IF 259
#define DEFINED 260
#define MACRO 261
#define MACRO_STRING 262
#define ORG 263
#define ASMERROR 264
#define ECHO 265
#define INCBIN 266
#define INCWORD 267
#define RES 268
#define WORD 269
#define BYTE 270
#define LDA 271
#define LDX 272
#define LDY 273
#define STA 274
#define STX 275
#define STY 276
#define AND 277
#define ORA 278
#define EOR 279
#define ADC 280
#define SBC 281
#define CMP 282
#define CPX 283
#define CPY 284
#define TSX 285
#define TXS 286
#define PHA 287
#define PLA 288
#define PHP 289
#define PLP 290
#define SEI 291
#define CLI 292
#define NOP 293
#define TYA 294
#define TAY 295
#define TXA 296
#define TAX 297
#define CLC 298
#define SEC 299
#define RTS 300
#define JSR 301
#define JMP 302
#define BEQ 303
#define BNE 304
#define BCC 305
#define BCS 306
#define BPL 307
#define BMI 308
#define BVC 309
#define BVS 310
#define INX 311
#define DEX 312
#define INY 313
#define DEY 314
#define INC 315
#define DEC 316
#define LSR 317
#define ASL 318
#define ROR 319
#define ROL 320
#define SYMBOL 321
#define STRING 322
#define LAND 323
#define LOR 324
#define LNOT 325
#define LPAREN 326
#define RPAREN 327
#define COMMA 328
#define COLON 329
#define X 330
#define Y 331
#define HASH 332
#define PLUS 333
#define MINUS 334
#define MULT 335
#define DIV 336
#define MOD 337
#define LT 338
#define GT 339
#define EQ 340
#define NEQ 341
#define ASSIGN 342
#define NUMBER 343
#define vNEG 344




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 110 "asm.y"
typedef union YYSTYPE {
    i32 num;
    char *str;
    struct atom *atom;
    struct expr *expr;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 223 "asm.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



