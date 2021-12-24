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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
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
     ERROR = 264,
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
#define ERROR 264
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




/* Copy the first part of user declarations.  */
// #line 1 "asm.y"

#include "gt-int.h"
#include "gt-parse.h"
#include "gt-vec.h"
#include "gt-membuf.h"
#include "gt-log.h"
#include <stdio.h>

#define YYERROR_VERBOSE

static struct vec asm_atoms[1];


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
// #line 110 "asm.y"
typedef union YYSTYPE {
    i32 num;
    char *str;
    struct atom *atom;
    struct expr *expr;
} YYSTYPE;
/* Line 196 of yacc.c.  */
// #line 282 "asm.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
// #line 294 "asm.tab.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  212
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   591

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  90
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  17
/* YYNRULES -- Number of rules. */
#define YYNRULES  193
/* YYNRULES -- Number of states. */
#define YYNSTATES  307

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   344

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     6,     8,    11,    15,    20,    25,    30,
      35,    40,    45,    47,    49,    51,    58,    63,    68,    73,
      80,    89,    93,    95,    98,   101,   104,   107,   110,   113,
     116,   119,   122,   125,   128,   131,   134,   137,   140,   143,
     146,   149,   152,   155,   158,   161,   164,   167,   170,   173,
     176,   179,   182,   185,   188,   191,   194,   197,   200,   203,
     206,   209,   212,   215,   218,   221,   224,   227,   230,   233,
     236,   239,   242,   245,   248,   251,   254,   257,   260,   263,
     266,   269,   272,   275,   278,   281,   284,   287,   290,   293,
     296,   299,   302,   305,   308,   311,   314,   317,   320,   323,
     326,   329,   332,   335,   338,   341,   344,   347,   350,   352,
     354,   356,   358,   360,   362,   364,   366,   368,   370,   372,
     374,   376,   378,   380,   382,   385,   388,   391,   394,   397,
     400,   403,   406,   409,   412,   414,   416,   418,   420,   423,
     426,   429,   432,   435,   438,   441,   444,   446,   449,   452,
     455,   458,   460,   463,   466,   469,   472,   474,   477,   480,
     483,   486,   488,   491,   494,   497,   500,   503,   505,   509,
     513,   516,   521,   526,   532,   538,   542,   546,   550,   554,
     558,   561,   565,   572,   574,   576,   580,   584,   587,   591,
     595,   599,   603,   607
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      91,     0,    -1,    91,    92,    -1,    92,    -1,    66,    74,
      -1,    66,    87,   105,    -1,     4,    71,   106,    72,    -1,
       8,    71,   105,    72,    -1,     9,    71,    67,    72,    -1,
      10,    71,    67,    72,    -1,     3,    71,    67,    72,    -1,
       6,    71,    67,    72,    -1,    93,    -1,     7,    -1,    95,
      -1,    13,    71,   105,    73,   105,    72,    -1,    14,    71,
      94,    72,    -1,    15,    71,    94,    72,    -1,    11,    71,
      67,    72,    -1,    11,    71,    67,    73,   105,    72,    -1,
      11,    71,    67,    73,   105,    73,   105,    72,    -1,    94,
      73,   105,    -1,   105,    -1,    16,    96,    -1,    16,   100,
      -1,    16,   101,    -1,    16,    97,    -1,    16,    98,    -1,
      16,    99,    -1,    16,   103,    -1,    16,   104,    -1,    17,
      96,    -1,    17,   100,    -1,    17,   102,    -1,    17,    97,
      -1,    17,    99,    -1,    18,    96,    -1,    18,   100,    -1,
      18,   101,    -1,    18,    97,    -1,    18,    99,    -1,    19,
     100,    -1,    19,   101,    -1,    19,    97,    -1,    19,    98,
      -1,    19,    99,    -1,    19,   103,    -1,    19,   104,    -1,
      20,   100,    -1,    20,   102,    -1,    20,    97,    -1,    21,
     100,    -1,    21,   101,    -1,    21,    97,    -1,    22,    96,
      -1,    22,   100,    -1,    22,   101,    -1,    22,    97,    -1,
      22,    98,    -1,    22,    99,    -1,    22,   103,    -1,    22,
     104,    -1,    23,    96,    -1,    23,   100,    -1,    23,   101,
      -1,    23,    97,    -1,    23,    98,    -1,    23,    99,    -1,
      23,   103,    -1,    23,   104,    -1,    24,    96,    -1,    24,
     100,    -1,    24,   101,    -1,    24,    97,    -1,    24,    98,
      -1,    24,    99,    -1,    24,   103,    -1,    24,   104,    -1,
      25,    96,    -1,    25,   100,    -1,    25,   101,    -1,    25,
      97,    -1,    25,    98,    -1,    25,    99,    -1,    25,   103,
      -1,    25,   104,    -1,    26,    96,    -1,    26,   100,    -1,
      26,   101,    -1,    26,    97,    -1,    26,    98,    -1,    26,
      99,    -1,    26,   103,    -1,    26,   104,    -1,    27,    96,
      -1,    27,   100,    -1,    27,   101,    -1,    27,    97,    -1,
      27,    98,    -1,    27,    99,    -1,    27,   103,    -1,    27,
     104,    -1,    28,    96,    -1,    28,   100,    -1,    28,    97,
      -1,    29,    96,    -1,    29,   100,    -1,    29,    97,    -1,
      31,    -1,    30,    -1,    32,    -1,    33,    -1,    34,    -1,
      35,    -1,    36,    -1,    37,    -1,    38,    -1,    39,    -1,
      40,    -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,
      45,    -1,    46,    97,    -1,    47,    97,    -1,    48,    97,
      -1,    49,    97,    -1,    50,    97,    -1,    51,    97,    -1,
      52,    97,    -1,    53,    97,    -1,    54,    97,    -1,    55,
      97,    -1,    56,    -1,    57,    -1,    58,    -1,    59,    -1,
      60,   100,    -1,    60,   101,    -1,    60,    97,    -1,    60,
      98,    -1,    61,   100,    -1,    61,   101,    -1,    61,    97,
      -1,    61,    98,    -1,    62,    -1,    62,   100,    -1,    62,
     101,    -1,    62,    97,    -1,    62,    98,    -1,    63,    -1,
      63,   100,    -1,    63,   101,    -1,    63,    97,    -1,    63,
      98,    -1,    64,    -1,    64,   100,    -1,    64,   101,    -1,
      64,    97,    -1,    64,    98,    -1,    65,    -1,    65,   100,
      -1,    65,   101,    -1,    65,    97,    -1,    65,    98,    -1,
      77,   105,    -1,   105,    -1,   105,    73,    75,    -1,   105,
      73,    76,    -1,    83,   105,    -1,    83,   105,    73,    75,
      -1,    83,   105,    73,    76,    -1,    71,   105,    73,    75,
      72,    -1,    71,   105,    72,    73,    76,    -1,   105,    78,
     105,    -1,   105,    79,   105,    -1,   105,    80,   105,    -1,
     105,    81,   105,    -1,   105,    82,   105,    -1,    79,   105,
      -1,    71,   105,    72,    -1,    12,    71,    67,    73,   105,
      72,    -1,    88,    -1,    66,    -1,   106,    69,   106,    -1,
     106,    68,   106,    -1,    70,   106,    -1,    71,   106,    72,
      -1,   105,    83,   105,    -1,   105,    84,   105,    -1,   105,
      85,   105,    -1,   105,    86,   105,    -1,     5,    71,    66,
      72,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   144,   144,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   156,   157,   158,   159,   160,   162,
     164,   167,   168,   170,   171,   172,   173,   174,   175,   176,
     177,   179,   180,   181,   182,   183,   185,   186,   187,   188,
     189,   191,   192,   193,   194,   195,   196,   197,   199,   200,
     201,   203,   204,   205,   207,   208,   209,   210,   211,   212,
     213,   214,   216,   217,   218,   219,   220,   221,   222,   223,
     225,   226,   227,   228,   229,   230,   231,   232,   234,   235,
     236,   237,   238,   239,   240,   241,   243,   244,   245,   246,
     247,   248,   249,   250,   252,   253,   254,   255,   256,   257,
     258,   259,   261,   262,   263,   264,   265,   266,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   294,   296,   297,   298,   299,   301,   302,
     303,   304,   306,   307,   308,   309,   311,   312,   313,   314,
     315,   317,   318,   319,   320,   321,   323,   324,   325,   326,
     327,   329,   330,   331,   332,   333,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   345,   346,   347,   348,   349,
     350,   351,   352,   354,   355,   357,   358,   359,   360,   361,
     362,   363,   364,   366
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INCLUDE", "IF", "DEFINED", "MACRO",
  "MACRO_STRING", "ORG", "ERROR", "ECHO", "INCBIN", "INCWORD", "RES",
  "WORD", "BYTE", "LDA", "LDX", "LDY", "STA", "STX", "STY", "AND", "ORA",
  "EOR", "ADC", "SBC", "CMP", "CPX", "CPY", "TSX", "TXS", "PHA", "PLA",
  "PHP", "PLP", "SEI", "CLI", "NOP", "TYA", "TAY", "TXA", "TAX", "CLC",
  "SEC", "RTS", "JSR", "JMP", "BEQ", "BNE", "BCC", "BCS", "BPL", "BMI",
  "BVC", "BVS", "INX", "DEX", "INY", "DEY", "INC", "DEC", "LSR", "ASL",
  "ROR", "ROL", "SYMBOL", "STRING", "LAND", "LOR", "LNOT", "LPAREN",
  "RPAREN", "COMMA", "COLON", "X", "Y", "HASH", "PLUS", "MINUS", "MULT",
  "DIV", "MOD", "LT", "GT", "EQ", "NEQ", "ASSIGN", "NUMBER", "vNEG",
  "$accept", "stmts", "stmt", "atom", "exprs", "op", "am_im", "am_a",
  "am_ax", "am_ay", "am_zp", "am_zpx", "am_zpy", "am_ix", "am_iy", "expr",
  "lexpr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    90,    91,    91,    92,    92,    92,    92,    92,    92,
      92,    92,    92,    92,    93,    93,    93,    93,    93,    93,
      93,    94,    94,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   106,   106,   106,   106,   106,
     106,   106,   106,   106
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     1,     2,     3,     4,     4,     4,     4,
       4,     4,     1,     1,     1,     6,     4,     4,     4,     6,
       8,     3,     1,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     1,     1,     1,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     2,     2,
       2,     1,     2,     2,     2,     2,     1,     2,     2,     2,
       2,     1,     2,     2,     2,     2,     2,     1,     3,     3,
       2,     4,     4,     5,     5,     3,     3,     3,     3,     3,
       2,     3,     6,     1,     1,     3,     3,     2,     3,     3,
       3,     3,     3,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,    13,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   109,   108,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   134,   135,   136,   137,     0,     0,   146,   151,
     156,   161,     0,     0,     3,    12,    14,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   184,     0,
       0,     0,     0,   183,    23,    26,    27,    28,    24,    25,
      29,    30,   167,     0,     0,    31,    34,    35,    32,    33,
     167,    36,    39,    40,    37,    38,    43,    44,    45,    41,
      42,    46,    47,    50,    48,    49,   167,    53,    51,    52,
      54,    57,    58,    59,    55,    56,    60,    61,    62,    65,
      66,    67,    63,    64,    68,    69,    70,    73,    74,    75,
      71,    72,    76,    77,    78,    81,    82,    83,    79,    80,
      84,    85,    86,    89,    90,    91,    87,    88,    92,    93,
      94,    97,    98,    99,    95,    96,   100,   101,     0,   102,
     104,   103,   105,   107,   106,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   140,   141,   138,   139,   167,
     144,   145,   142,   143,   149,   150,   147,   148,   154,   155,
     152,   153,   159,   160,   157,   158,   164,   165,   162,   163,
       4,     0,     1,     2,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    22,     0,     0,
       0,   166,   180,   170,     0,     0,     0,     0,     0,     0,
       0,   170,     0,   170,     0,     5,    10,     0,   187,     0,
       0,     0,     0,     0,     0,     0,     0,     6,    11,     7,
       8,     9,    18,     0,     0,    16,     0,    17,     0,   181,
       0,     0,   168,   169,   175,   176,   177,   178,   179,   181,
       0,     0,   188,   189,   190,   191,   192,   186,   185,     0,
       0,    21,     0,     0,     0,   171,   172,   193,    19,     0,
      15,     0,   174,   173,     0,   182,    20
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    63,    64,    65,   226,    66,    84,    85,    86,    87,
      88,    89,    99,    90,    91,   116,   219
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -202
static const short int yypact[] =
{
     339,   -67,   -28,   -14,  -202,   -10,    -6,    -5,     1,     4,
       5,     9,     8,    22,    23,    41,    48,    59,     8,     8,
       8,     8,     8,     8,    38,    38,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,  -202,  -202,  -202,  -202,    59,    59,    59,    59,
      59,    59,   -69,   275,  -202,  -202,  -202,    -8,     7,    16,
      78,    17,    25,    30,    78,    78,    78,    27,  -202,    78,
      78,    78,    78,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,   441,    78,    78,  -202,  -202,  -202,  -202,  -202,
     451,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,   102,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,    78,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,   461,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,  -202,
    -202,    78,  -202,  -202,    31,    37,     7,     7,   188,    65,
      44,   399,    46,    50,   -59,   471,   -51,   102,   -36,    56,
     -40,   102,  -202,   481,   -24,    78,    78,    78,    78,    78,
     410,   491,    52,   102,    57,   102,  -202,    47,  -202,   -55,
      71,    78,    78,    78,    78,     7,     7,  -202,  -202,  -202,
    -202,  -202,  -202,    78,    78,  -202,    78,  -202,    62,    68,
      73,    80,  -202,  -202,    70,    70,  -202,  -202,  -202,  -202,
      69,    74,  -202,   102,   102,   102,   102,  -202,   117,   394,
     415,   102,    78,   112,   124,  -202,  -202,  -202,  -202,    78,
    -202,   426,  -202,  -202,   431,  -202,  -202
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -202,  -202,   134,  -202,   122,  -202,   140,   393,   404,   242,
     154,   172,   184,   559,   568,   -12,  -201
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned short int yytable[] =
{
      92,   100,   100,    92,    67,   210,    92,    92,    92,    92,
      92,    92,   215,   262,   263,   248,   250,   279,   211,    77,
      77,   265,   266,   235,   236,   237,   238,   239,   251,   252,
     253,   254,   269,   270,    77,    77,   267,   266,   235,   236,
     237,   238,   239,    68,   189,   189,   189,   189,   189,   189,
      77,   272,   273,    77,   287,   288,   218,    69,   221,   214,
      77,    70,   225,   227,   227,    71,    72,   230,   231,   232,
     233,    77,    73,    78,    78,    74,    75,   216,   217,    79,
      76,   240,   241,   220,   222,    80,    81,    81,    78,    78,
      77,    82,   223,    93,    93,    83,    83,   224,   229,    80,
      80,    81,    81,   246,    78,    94,    82,    78,   247,    93,
      83,    83,    79,   281,    78,    80,   258,    81,   260,    93,
      81,   168,   261,   268,    82,    78,    83,    81,   273,    83,
      93,    94,   272,   255,   256,   292,    83,   257,    81,   255,
     256,   293,    82,   282,    78,   296,   297,    83,   294,    93,
     237,   238,   239,    95,   101,   295,   243,    81,   120,   128,
     136,   144,   152,   160,   169,   172,    83,    98,   104,   109,
     114,   118,   124,   132,   140,   148,   156,   164,   171,   174,
     235,   236,   237,   238,   239,   255,   105,   110,   302,   119,
     125,   133,   141,   149,   157,   165,   303,   213,   228,   245,
     115,     0,     0,     0,   218,   249,     0,     0,     0,     0,
     187,   192,   196,   200,   204,   208,     0,     0,     0,     0,
       0,     0,     0,   274,   275,   276,   277,   278,   188,   193,
     197,   201,   205,   209,     0,     0,     0,     0,     0,   283,
     284,   285,   286,   218,   218,     0,     0,     0,     0,     0,
       0,   289,   290,     0,   291,    97,   103,   108,     0,     0,
     123,   131,   139,   147,   155,   163,   235,   236,   237,   238,
     239,   251,   252,   253,   254,   212,     0,     0,     1,     2,
     301,     3,     4,     5,     6,     7,     8,   304,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,     1,     2,     0,     3,     4,     5,     6,     7,
       8,     0,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    96,   102,   106,   113,
     117,   121,   129,   137,   145,   153,   161,   170,   173,   107,
       0,     0,   122,   130,   138,   146,   154,   162,     0,     0,
       0,     0,     0,     0,     0,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,     0,     0,     0,     0,   185,
     190,   194,   198,   202,   206,     0,     0,     0,     0,     0,
     186,   191,   195,   199,   203,   207,   298,   299,     0,     0,
       0,   259,   235,   236,   237,   238,   239,   235,   236,   237,
     238,   239,   279,     0,     0,     0,     0,   300,   235,   236,
     237,   238,   239,   235,   236,   237,   238,   239,   305,     0,
       0,     0,     0,   306,   235,   236,   237,   238,   239,   235,
     236,   237,   238,   239,   234,     0,     0,     0,     0,   235,
     236,   237,   238,   239,   242,     0,     0,     0,     0,   235,
     236,   237,   238,   239,   244,     0,     0,     0,     0,   235,
     236,   237,   238,   239,   264,     0,     0,     0,     0,   235,
     236,   237,   238,   239,   271,     0,     0,     0,     0,   235,
     236,   237,   238,   239,   280,     0,     0,     0,     0,   235,
     236,   237,   238,   239,   111,     0,     0,   126,   134,   142,
     150,   158,   166,   112,     0,     0,   127,   135,   143,   151,
     159,   167
};

static const short int yycheck[] =
{
      12,    13,    14,    15,    71,    74,    18,    19,    20,    21,
      22,    23,     5,    72,    73,   216,   217,    72,    87,    12,
      12,    72,    73,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    72,    73,    12,    12,    72,    73,    78,    79,
      80,    81,    82,    71,    56,    57,    58,    59,    60,    61,
      12,    75,    76,    12,   255,   256,    68,    71,    70,    67,
      12,    71,    74,    75,    76,    71,    71,    79,    80,    81,
      82,    12,    71,    66,    66,    71,    71,    70,    71,    71,
      71,    93,    94,    67,    67,    77,    79,    79,    66,    66,
      12,    83,    67,    71,    71,    88,    88,    67,    71,    77,
      77,    79,    79,    72,    66,    83,    83,    66,    71,    71,
      88,    88,    71,    66,    66,    77,    72,    79,    72,    71,
      79,    83,    72,    67,    83,    66,    88,    79,    76,    88,
      71,    83,    75,    68,    69,    73,    88,    72,    79,    68,
      69,    73,    83,    72,    66,    76,    72,    88,    75,    71,
      80,    81,    82,    13,    14,    75,   168,    79,    18,    19,
      20,    21,    22,    23,    24,    25,    88,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      78,    79,    80,    81,    82,    68,    14,    15,    76,    17,
      18,    19,    20,    21,    22,    23,    72,    63,    76,   211,
      16,    -1,    -1,    -1,   216,   217,    -1,    -1,    -1,    -1,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   235,   236,   237,   238,   239,    56,    57,
      58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,   251,
     252,   253,   254,   255,   256,    -1,    -1,    -1,    -1,    -1,
      -1,   263,   264,    -1,   266,    13,    14,    15,    -1,    -1,
      18,    19,    20,    21,    22,    23,    78,    79,    80,    81,
      82,    83,    84,    85,    86,     0,    -1,    -1,     3,     4,
     292,     6,     7,     8,     9,    10,    11,   299,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,     3,     4,    -1,     6,     7,     8,     9,    10,
      11,    -1,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    15,
      -1,    -1,    18,    19,    20,    21,    22,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    -1,    -1,    -1,    -1,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
      56,    57,    58,    59,    60,    61,    72,    73,    -1,    -1,
      -1,    72,    78,    79,    80,    81,    82,    78,    79,    80,
      81,    82,    72,    -1,    -1,    -1,    -1,    72,    78,    79,
      80,    81,    82,    78,    79,    80,    81,    82,    72,    -1,
      -1,    -1,    -1,    72,    78,    79,    80,    81,    82,    78,
      79,    80,    81,    82,    73,    -1,    -1,    -1,    -1,    78,
      79,    80,    81,    82,    73,    -1,    -1,    -1,    -1,    78,
      79,    80,    81,    82,    73,    -1,    -1,    -1,    -1,    78,
      79,    80,    81,    82,    73,    -1,    -1,    -1,    -1,    78,
      79,    80,    81,    82,    73,    -1,    -1,    -1,    -1,    78,
      79,    80,    81,    82,    73,    -1,    -1,    -1,    -1,    78,
      79,    80,    81,    82,    15,    -1,    -1,    18,    19,    20,
      21,    22,    23,    15,    -1,    -1,    18,    19,    20,    21,
      22,    23
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    10,    11,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    91,    92,    93,    95,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    12,    66,    71,
      77,    79,    83,    88,    96,    97,    98,    99,   100,   101,
     103,   104,   105,    71,    83,    96,    97,    99,   100,   102,
     105,    96,    97,    99,   100,   101,    97,    98,    99,   100,
     101,   103,   104,    97,   100,   102,   105,    97,   100,   101,
      96,    97,    98,    99,   100,   101,   103,   104,    96,    97,
      98,    99,   100,   101,   103,   104,    96,    97,    98,    99,
     100,   101,   103,   104,    96,    97,    98,    99,   100,   101,
     103,   104,    96,    97,    98,    99,   100,   101,   103,   104,
      96,    97,    98,    99,   100,   101,   103,   104,    83,    96,
      97,   100,    96,    97,   100,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    98,   100,   101,   105,
      97,    98,   100,   101,    97,    98,   100,   101,    97,    98,
     100,   101,    97,    98,   100,   101,    97,    98,   100,   101,
      74,    87,     0,    92,    67,     5,    70,    71,   105,   106,
      67,   105,    67,    67,    67,   105,    94,   105,    94,    71,
     105,   105,   105,   105,    73,    78,    79,    80,    81,    82,
     105,   105,    73,   105,    73,   105,    72,    71,   106,   105,
     106,    83,    84,    85,    86,    68,    69,    72,    72,    72,
      72,    72,    72,    73,    73,    72,    73,    72,    67,    72,
      73,    73,    75,    76,   105,   105,   105,   105,   105,    72,
      73,    66,    72,   105,   105,   105,   105,   106,   106,   105,
     105,   105,    73,    73,    75,    75,    76,    72,    72,    73,
      72,   105,    76,    72,   105,    72,    72
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(gtyychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (gtyychar == YYEMPTY && yylen == 1)				\
    {								\
      gtyychar = (Token);						\
      gtyylval = (Value);						\
      yytoken = YYTRANSLATE (gtyychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (gtyydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (gtyydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (gtyydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (gtyydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int gtyydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int gtyyparse (void *YYPARSE_PARAM);
# else
int gtyyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int gtyyparse (void);
#else
int gtyyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

void yycleanup();

/* The look-ahead symbol.  */
int gtyychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE gtyylval;

/* Number of syntax errors so far.  */
int gtyynerrs;



/*----------.
| gtyyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int gtyyparse (void *YYPARSE_PARAM)
# else
int gtyyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
gtyyparse (void)
#else
int
gtyyparse ()
   // ;
#endif
#endif
{

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  gtyynerrs = 0;
  gtyychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (gtyychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      gtyychar = YYLEX;
    }

  if (gtyychar <= YYEOF)
    {
      gtyychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (gtyychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &gtyylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &gtyylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (gtyychar != YYEOF)
    gtyychar = YYEMPTY;

  *++yyvsp = gtyylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
// #line 145 "asm.y"
    { gtnew_label((yyvsp[-1].str)); ;}
    break;

  case 5:
// #line 146 "asm.y"
    { new_symbol_expr((yyvsp[-2].str), (yyvsp[0].expr)); ;}
    break;

  case 6:
// #line 147 "asm.y"
    { push_if_state((yyvsp[-1].expr)); ;}
    break;

  case 7:
// #line 148 "asm.y"
    { set_org((yyvsp[-1].expr)); ;}
    break;

  case 8:
// #line 149 "asm.y"
    { asm_error((yyvsp[-1].str)); ;}
    break;

  case 9:
// #line 150 "asm.y"
    { asm_echo((yyvsp[-1].str)); ;}
    break;

  case 10:
// #line 151 "asm.y"
    { asm_include((yyvsp[-1].str)); ;}
    break;

  case 11:
// #line 152 "asm.y"
    { push_macro_state((yyvsp[-1].str)); ;}
    break;

  case 12:
// #line 153 "asm.y"
    { vec_push(asm_atoms, &(yyvsp[0].atom)); ;}
    break;

  case 13:
// #line 154 "asm.y"
    { macro_append((yyvsp[0].str)) ;}
    break;

  case 14:
// #line 156 "asm.y"
    { (yyval.atom) = (yyvsp[0].atom);}
    break;

  case 15:
// #line 157 "asm.y"
    { (yyval.atom) = new_res((yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 16:
// #line 158 "asm.y"
    { (yyval.atom) = exprs_to_word_exprs((yyvsp[-1].atom)); ;}
    break;

  case 17:
// #line 159 "asm.y"
    { (yyval.atom) = exprs_to_byte_exprs((yyvsp[-1].atom)); ;}
    break;

  case 18:
// #line 160 "asm.y"
    {
            (yyval.atom) = new_incbin((yyvsp[-1].str), NULL, NULL); ;}
    break;

  case 19:
// #line 162 "asm.y"
    {
            (yyval.atom) = new_incbin((yyvsp[-3].str), (yyvsp[-1].expr), NULL); ;}
    break;

  case 20:
// #line 164 "asm.y"
    {
            (yyval.atom) = new_incbin((yyvsp[-5].str), (yyvsp[-3].expr), (yyvsp[-1].expr)); ;}
    break;

  case 21:
// #line 167 "asm.y"
    { (yyval.atom) = exprs_add((yyvsp[-2].atom), (yyvsp[0].expr)); ;}
    break;

  case 22:
// #line 168 "asm.y"
    { (yyval.atom) = new_exprs((yyvsp[0].expr)); ;}
    break;

  case 23:
// #line 170 "asm.y"
    { (yyval.atom) = new_op(0xA9, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 24:
// #line 171 "asm.y"
    { (yyval.atom) = new_op(0xA5, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 25:
// #line 172 "asm.y"
    { (yyval.atom) = new_op(0xB5, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 26:
// #line 173 "asm.y"
    { (yyval.atom) = new_op(0xAD, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 27:
// #line 174 "asm.y"
    { (yyval.atom) = new_op(0xBD, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 28:
// #line 175 "asm.y"
    { (yyval.atom) = new_op(0xB9, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 29:
// #line 176 "asm.y"
    { (yyval.atom) = new_op(0xA1, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 30:
// #line 177 "asm.y"
    { (yyval.atom) = new_op(0xB1, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 31:
// #line 179 "asm.y"
    { (yyval.atom) = new_op(0xA2, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 32:
// #line 180 "asm.y"
    { (yyval.atom) = new_op(0xA6, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 33:
// #line 181 "asm.y"
    { (yyval.atom) = new_op(0xB6, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 34:
// #line 182 "asm.y"
    { (yyval.atom) = new_op(0xAE, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 35:
// #line 183 "asm.y"
    { (yyval.atom) = new_op(0xBE, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 36:
// #line 185 "asm.y"
    { (yyval.atom) = new_op(0xA0, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 37:
// #line 186 "asm.y"
    { (yyval.atom) = new_op(0xA4, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 38:
// #line 187 "asm.y"
    { (yyval.atom) = new_op(0xB4, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 39:
// #line 188 "asm.y"
    { (yyval.atom) = new_op(0xAC, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 40:
// #line 189 "asm.y"
    { (yyval.atom) = new_op(0xBC, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 41:
// #line 191 "asm.y"
    { (yyval.atom) = new_op(0x85, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 42:
// #line 192 "asm.y"
    { (yyval.atom) = new_op(0x95, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 43:
// #line 193 "asm.y"
    { (yyval.atom) = new_op(0x8D, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 44:
// #line 194 "asm.y"
    { (yyval.atom) = new_op(0x9D, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 45:
// #line 195 "asm.y"
    { (yyval.atom) = new_op(0x99, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 46:
// #line 196 "asm.y"
    { (yyval.atom) = new_op(0x81, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 47:
// #line 197 "asm.y"
    { (yyval.atom) = new_op(0x91, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 48:
// #line 199 "asm.y"
    { (yyval.atom) = new_op(0x86, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 49:
// #line 200 "asm.y"
    { (yyval.atom) = new_op(0x96, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 50:
// #line 201 "asm.y"
    { (yyval.atom) = new_op(0x8e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 51:
// #line 203 "asm.y"
    { (yyval.atom) = new_op(0x84, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 52:
// #line 204 "asm.y"
    { (yyval.atom) = new_op(0x94, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 53:
// #line 205 "asm.y"
    { (yyval.atom) = new_op(0x8c, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 54:
// #line 207 "asm.y"
    { (yyval.atom) = new_op(0x29, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 55:
// #line 208 "asm.y"
    { (yyval.atom) = new_op(0x25, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 56:
// #line 209 "asm.y"
    { (yyval.atom) = new_op(0x35, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 57:
// #line 210 "asm.y"
    { (yyval.atom) = new_op(0x2d, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 58:
// #line 211 "asm.y"
    { (yyval.atom) = new_op(0x3d, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 59:
// #line 212 "asm.y"
    { (yyval.atom) = new_op(0x39, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 60:
// #line 213 "asm.y"
    { (yyval.atom) = new_op(0x21, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 61:
// #line 214 "asm.y"
    { (yyval.atom) = new_op(0x31, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 62:
// #line 216 "asm.y"
    { (yyval.atom) = new_op(0x09, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 63:
// #line 217 "asm.y"
    { (yyval.atom) = new_op(0x05, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 64:
// #line 218 "asm.y"
    { (yyval.atom) = new_op(0x15, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 65:
// #line 219 "asm.y"
    { (yyval.atom) = new_op(0x0d, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 66:
// #line 220 "asm.y"
    { (yyval.atom) = new_op(0x1d, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 67:
// #line 221 "asm.y"
    { (yyval.atom) = new_op(0x19, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 68:
// #line 222 "asm.y"
    { (yyval.atom) = new_op(0x01, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 69:
// #line 223 "asm.y"
    { (yyval.atom) = new_op(0x11, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 70:
// #line 225 "asm.y"
    { (yyval.atom) = new_op(0x49, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 71:
// #line 226 "asm.y"
    { (yyval.atom) = new_op(0x45, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 72:
// #line 227 "asm.y"
    { (yyval.atom) = new_op(0x55, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 73:
// #line 228 "asm.y"
    { (yyval.atom) = new_op(0x4d, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 74:
// #line 229 "asm.y"
    { (yyval.atom) = new_op(0x5d, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 75:
// #line 230 "asm.y"
    { (yyval.atom) = new_op(0x59, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 76:
// #line 231 "asm.y"
    { (yyval.atom) = new_op(0x41, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 77:
// #line 232 "asm.y"
    { (yyval.atom) = new_op(0x51, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 78:
// #line 234 "asm.y"
    { (yyval.atom) = new_op(0x69, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 79:
// #line 235 "asm.y"
    { (yyval.atom) = new_op(0x65, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 80:
// #line 236 "asm.y"
    { (yyval.atom) = new_op(0x75, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 81:
// #line 237 "asm.y"
    { (yyval.atom) = new_op(0x6D, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 82:
// #line 238 "asm.y"
    { (yyval.atom) = new_op(0x7D, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 83:
// #line 239 "asm.y"
    { (yyval.atom) = new_op(0x79, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 84:
// #line 240 "asm.y"
    { (yyval.atom) = new_op(0x61, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 85:
// #line 241 "asm.y"
    { (yyval.atom) = new_op(0x71, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 86:
// #line 243 "asm.y"
    { (yyval.atom) = new_op(0xe9, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 87:
// #line 244 "asm.y"
    { (yyval.atom) = new_op(0xe5, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 88:
// #line 245 "asm.y"
    { (yyval.atom) = new_op(0xf5, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 89:
// #line 246 "asm.y"
    { (yyval.atom) = new_op(0xeD, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 90:
// #line 247 "asm.y"
    { (yyval.atom) = new_op(0xfD, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 91:
// #line 248 "asm.y"
    { (yyval.atom) = new_op(0xf9, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 92:
// #line 249 "asm.y"
    { (yyval.atom) = new_op(0xe1, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 93:
// #line 250 "asm.y"
    { (yyval.atom) = new_op(0xf1, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 94:
// #line 252 "asm.y"
    { (yyval.atom) = new_op(0xc9, ATOM_TYPE_OP_ARG_UI8, (yyvsp[0].expr)); ;}
    break;

  case 95:
// #line 253 "asm.y"
    { (yyval.atom) = new_op(0xc5, ATOM_TYPE_OP_ARG_U8,  (yyvsp[0].expr)); ;}
    break;

  case 96:
// #line 254 "asm.y"
    { (yyval.atom) = new_op(0xd5, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 97:
// #line 255 "asm.y"
    { (yyval.atom) = new_op(0xcD, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 98:
// #line 256 "asm.y"
    { (yyval.atom) = new_op(0xdD, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 99:
// #line 257 "asm.y"
    { (yyval.atom) = new_op(0xd9, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 100:
// #line 258 "asm.y"
    { (yyval.atom) = new_op(0xc1, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 101:
// #line 259 "asm.y"
    { (yyval.atom) = new_op(0xd1, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 102:
// #line 261 "asm.y"
    { (yyval.atom) = new_op(0xe0, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 103:
// #line 262 "asm.y"
    { (yyval.atom) = new_op(0xe4, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 104:
// #line 263 "asm.y"
    { (yyval.atom) = new_op(0xec, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 105:
// #line 264 "asm.y"
    { (yyval.atom) = new_op(0xc0, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 106:
// #line 265 "asm.y"
    { (yyval.atom) = new_op(0xc4, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 107:
// #line 266 "asm.y"
    { (yyval.atom) = new_op(0xcc, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 108:
// #line 268 "asm.y"
    { (yyval.atom) = new_op0(0x9A); ;}
    break;

  case 109:
// #line 269 "asm.y"
    { (yyval.atom) = new_op0(0xBA); ;}
    break;

  case 110:
// #line 270 "asm.y"
    { (yyval.atom) = new_op0(0x48); ;}
    break;

  case 111:
// #line 271 "asm.y"
    { (yyval.atom) = new_op0(0x68); ;}
    break;

  case 112:
// #line 272 "asm.y"
    { (yyval.atom) = new_op0(0x08); ;}
    break;

  case 113:
// #line 273 "asm.y"
    { (yyval.atom) = new_op0(0x28); ;}
    break;

  case 114:
// #line 274 "asm.y"
    { (yyval.atom) = new_op0(0x78); ;}
    break;

  case 115:
// #line 275 "asm.y"
    { (yyval.atom) = new_op0(0x58); ;}
    break;

  case 116:
// #line 276 "asm.y"
    { (yyval.atom) = new_op0(0xea); ;}
    break;

  case 117:
// #line 277 "asm.y"
    { (yyval.atom) = new_op0(0x98); ;}
    break;

  case 118:
// #line 278 "asm.y"
    { (yyval.atom) = new_op0(0xa8); ;}
    break;

  case 119:
// #line 279 "asm.y"
    { (yyval.atom) = new_op0(0x8a); ;}
    break;

  case 120:
// #line 280 "asm.y"
    { (yyval.atom) = new_op0(0xaa); ;}
    break;

  case 121:
// #line 281 "asm.y"
    { (yyval.atom) = new_op0(0x18); ;}
    break;

  case 122:
// #line 282 "asm.y"
    { (yyval.atom) = new_op0(0x38); ;}
    break;

  case 123:
// #line 283 "asm.y"
    { (yyval.atom) = new_op0(0x60); ;}
    break;

  case 124:
// #line 285 "asm.y"
    { (yyval.atom) = new_op(0x20, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 125:
// #line 286 "asm.y"
    { (yyval.atom) = new_op(0x4c, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 126:
// #line 287 "asm.y"
    { (yyval.atom) = new_op(0xf0, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 127:
// #line 288 "asm.y"
    { (yyval.atom) = new_op(0xd0, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 128:
// #line 289 "asm.y"
    { (yyval.atom) = new_op(0x90, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 129:
// #line 290 "asm.y"
    { (yyval.atom) = new_op(0xb0, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 130:
// #line 291 "asm.y"
    { (yyval.atom) = new_op(0x10, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 131:
// #line 292 "asm.y"
    { (yyval.atom) = new_op(0x30, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 132:
// #line 293 "asm.y"
    { (yyval.atom) = new_op(0x50, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 133:
// #line 294 "asm.y"
    { (yyval.atom) = new_op(0x70, ATOM_TYPE_OP_ARG_I8,  (yyvsp[0].expr)); ;}
    break;

  case 134:
// #line 296 "asm.y"
    { (yyval.atom) = new_op0(0xe8); ;}
    break;

  case 135:
// #line 297 "asm.y"
    { (yyval.atom) = new_op0(0xca); ;}
    break;

  case 136:
// #line 298 "asm.y"
    { (yyval.atom) = new_op0(0xc8); ;}
    break;

  case 137:
// #line 299 "asm.y"
    { (yyval.atom) = new_op0(0x88); ;}
    break;

  case 138:
// #line 301 "asm.y"
    { (yyval.atom) = new_op(0xe6, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 139:
// #line 302 "asm.y"
    { (yyval.atom) = new_op(0xf6, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 140:
// #line 303 "asm.y"
    { (yyval.atom) = new_op(0xee, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 141:
// #line 304 "asm.y"
    { (yyval.atom) = new_op(0xfe, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 142:
// #line 306 "asm.y"
    { (yyval.atom) = new_op(0xc6, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 143:
// #line 307 "asm.y"
    { (yyval.atom) = new_op(0xd6, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 144:
// #line 308 "asm.y"
    { (yyval.atom) = new_op(0xce, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 145:
// #line 309 "asm.y"
    { (yyval.atom) = new_op(0xde, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 146:
// #line 311 "asm.y"
    { (yyval.atom) = new_op0(0x4a); ;}
    break;

  case 147:
// #line 312 "asm.y"
    { (yyval.atom) = new_op(0x46, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 148:
// #line 313 "asm.y"
    { (yyval.atom) = new_op(0x56, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 149:
// #line 314 "asm.y"
    { (yyval.atom) = new_op(0x4e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 150:
// #line 315 "asm.y"
    { (yyval.atom) = new_op(0x5e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 151:
// #line 317 "asm.y"
    { (yyval.atom) = new_op0(0x0a); ;}
    break;

  case 152:
// #line 318 "asm.y"
    { (yyval.atom) = new_op(0x06, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 153:
// #line 319 "asm.y"
    { (yyval.atom) = new_op(0x16, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 154:
// #line 320 "asm.y"
    { (yyval.atom) = new_op(0x0e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 155:
// #line 321 "asm.y"
    { (yyval.atom) = new_op(0x1e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 156:
// #line 323 "asm.y"
    { (yyval.atom) = new_op0(0x6a); ;}
    break;

  case 157:
// #line 324 "asm.y"
    { (yyval.atom) = new_op(0x66, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 158:
// #line 325 "asm.y"
    { (yyval.atom) = new_op(0x76, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 159:
// #line 326 "asm.y"
    { (yyval.atom) = new_op(0x6e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 160:
// #line 327 "asm.y"
    { (yyval.atom) = new_op(0x7e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 161:
// #line 329 "asm.y"
    { (yyval.atom) = new_op0(0x2a); ;}
    break;

  case 162:
// #line 330 "asm.y"
    { (yyval.atom) = new_op(0x26, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 163:
// #line 331 "asm.y"
    { (yyval.atom) = new_op(0x36, ATOM_TYPE_OP_ARG_U8, (yyvsp[0].expr)); ;}
    break;

  case 164:
// #line 332 "asm.y"
    { (yyval.atom) = new_op(0x2e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 165:
// #line 333 "asm.y"
    { (yyval.atom) = new_op(0x3e, ATOM_TYPE_OP_ARG_U16, (yyvsp[0].expr)); ;}
    break;

  case 166:
// #line 335 "asm.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 167:
// #line 336 "asm.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 168:
// #line 337 "asm.y"
    { (yyval.expr) = (yyvsp[-2].expr); ;}
    break;

  case 169:
// #line 338 "asm.y"
    { (yyval.expr) = (yyvsp[-2].expr); ;}
    break;

  case 170:
// #line 339 "asm.y"
    { (yyval.expr) = (yyvsp[0].expr); ;}
    break;

  case 171:
// #line 340 "asm.y"
    { (yyval.expr) = (yyvsp[-2].expr); ;}
    break;

  case 172:
// #line 341 "asm.y"
    { (yyval.expr) = (yyvsp[-2].expr); ;}
    break;

  case 173:
// #line 342 "asm.y"
    { (yyval.expr) = (yyvsp[-3].expr); ;}
    break;

  case 174:
// #line 343 "asm.y"
    { (yyval.expr) = (yyvsp[-3].expr); ;}
    break;

  case 175:
// #line 345 "asm.y"
    { (yyval.expr) = new_expr_op2(PLUS, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 176:
// #line 346 "asm.y"
    { (yyval.expr) = new_expr_op2(MINUS, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 177:
// #line 347 "asm.y"
    { (yyval.expr) = new_expr_op2(MULT, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 178:
// #line 348 "asm.y"
    { (yyval.expr) = new_expr_op2(DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 179:
// #line 349 "asm.y"
    { (yyval.expr) = new_expr_op2(MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 180:
// #line 350 "asm.y"
    { (yyval.expr) = new_expr_op1(vNEG, (yyvsp[0].expr)); ;}
    break;

  case 181:
// #line 351 "asm.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 182:
// #line 352 "asm.y"
    {
            (yyval.expr) = new_expr_incword((yyvsp[-3].str), (yyvsp[-1].expr)); ;}
    break;

  case 183:
// #line 354 "asm.y"
    { (yyval.expr) = new_expr_number((yyvsp[0].num)); ;}
    break;

  case 184:
// #line 355 "asm.y"
    { (yyval.expr) = new_expr_symref((yyvsp[0].str)); ;}
    break;

  case 185:
// #line 357 "asm.y"
    { (yyval.expr) = new_expr_op2(LOR, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 186:
// #line 358 "asm.y"
    { (yyval.expr) = new_expr_op2(LAND, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 187:
// #line 359 "asm.y"
    { (yyval.expr) = new_expr_op1(LNOT, (yyvsp[0].expr)); ;}
    break;

  case 188:
// #line 360 "asm.y"
    { (yyval.expr) = (yyvsp[-1].expr); ;}
    break;

  case 189:
// #line 361 "asm.y"
    { (yyval.expr) = new_expr_op2(LT, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 190:
// #line 362 "asm.y"
    { (yyval.expr) = new_expr_op2(GT, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 191:
// #line 363 "asm.y"
    { (yyval.expr) = new_expr_op2(EQ, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 192:
// #line 364 "asm.y"
    { (yyval.expr) = new_expr_op2(NEQ, (yyvsp[-2].expr), (yyvsp[0].expr)); ;}
    break;

  case 193:
// #line 366 "asm.y"
    { (yyval.expr) = new_is_defined((yyvsp[-1].str)); ;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
// #line 2596 "asm.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++gtyynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (gtyychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (gtyychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (gtyychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &gtyylval);
	  gtyychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = gtyylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (gtyychar != YYEOF && gtyychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &gtyylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


// #line 368 "asm.y"


int
yyerror (char *s)
{
    fprintf (stderr, "line %d, %s\n", num_lines, s);
    return 0;
}

void asm_set_source(struct membuf *buffer);

int assemble(struct membuf *source, struct membuf *dest)
{
    int val;

    LOG_INIT_CONSOLE(LOG_NORMAL);
    parse_init();
    gtyydebug = 0;
    asm_src_buffer_push(source);
    vec_init(asm_atoms, sizeof(struct atom*));
    val = gtyyparse();
    if(val == 0)
    {
        output_atoms(dest, asm_atoms);
    }
    parse_free();
    vec_free(asm_atoms, NULL);
    yycleanup();
    LOG_FREE;

    return val;
}

