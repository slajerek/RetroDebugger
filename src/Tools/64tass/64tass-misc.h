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
#ifndef _MISC_H_
#define _MISC_H_
#include "64tass-libtree.h"
#include <stdint.h>
#include <inttypes.h>
#define VERSION "1.46"

enum what_e { WHAT_EXPRESSION, WHAT_HASHMARK, WHAT_X, WHAT_Y, WHAT_XZ, WHAT_R, WHAT_RZ,
    WHAT_COMMAND, WHAT_EQUAL, WHAT_EOL, WHAT_STAR, WHAT_COMA, WHAT_COLON,
    WHAT_S, WHAT_SZ, WHAT_COMMENT, WHAT_CHAR, WHAT_LBL
};

typedef uint_fast32_t line_t;
#define PRIuline PRIuFAST32
#define PRIxline PRIxFAST32
typedef uint_fast32_t address_t;
#define PRIaddress PRIxFAST32
typedef int32_t ival_t;
#define PRIdval PRId32
typedef uint32_t uval_t;
#define PRIxval PRIx32
#define PRIuval PRIu32

static inline char lowcase(char cch) {return (cch<'A' || cch>'Z')?cch:(cch|0x20);}

enum type_e {
    T_NONE, T_NUM, T_UINT, T_SINT, T_STR, T_TSTR, T_GAP, T_IDENT, T_FUNC, T_IDENTREF, T_FORWR, T_BACKR, T_UNDEF, T_OPER
};

struct value_s {
    enum type_e type;
    union {
        struct {
            uint8_t len;
            ival_t val;
        } num;
        struct {
            size_t len;
            uint8_t *data;
        } str;
        struct {
            size_t len;
            const uint8_t *name;
        } ident;
        struct label_s *label;
        char oper;
        uint8_t ref;
    } u;
};

enum label_e {
    L_LABEL, L_CONST, L_VAR, L_STRUCT, L_UNION
};

struct label_s {
    const char *name;
    enum label_e type;
    struct avltree_node node;

    struct value_s value;
    size_t size;
    uval_t requires;
    uval_t conflicts;
    unsigned ref:1;
    uint8_t esize;
    uint8_t pass;
    uint8_t upass;
    struct label_s *parent;
    struct avltree members;
};

struct star_s {
    line_t line;
    address_t addr;
    struct avltree tree;
    struct avltree_node node;
};

struct file_s {
    const char *name;
    uint8_t *data;    /* data */
    size_t len;       /* length */
    size_t p;         /* current point */
    uint16_t open;    /* open/not open */
    uint16_t uid;     /* uid */
    struct avltree star;
    struct avltree_node node;
};

struct macro_s {
    const char *name;
    size_t p;
    line_t sline;
    struct file_s *file;
    int type;
    struct avltree_node node;
};

struct jump_s {
    const char *name;
    size_t p;
    line_t sline;
    uint8_t waitforp;
    const struct file_s *file;
    const struct label_s *parent;
    struct avltree_node node;
};

struct arguments_s {
    unsigned warning:1;
    unsigned quiet:1;
    unsigned nonlinear:1;
    unsigned stripstart:1;
    unsigned toascii:1;
    unsigned monitor:1;
    unsigned source:1;
    unsigned casesensitive:1;
    unsigned longbranch:1;
    unsigned wordstart:1;
    unsigned tasmcomp:1;
    const char *output;
    uint8_t cpumode;
    const char *label;
    const char *list;
};

extern line_t sline, vline_64tass;
extern unsigned int lpoint; 
extern struct file_s *cfile; 
extern struct avltree *star_tree;
extern int fixeddig;
extern unsigned int errors,conderrors,warnings;
extern address_t star;
extern const uint8_t *pline;
extern int labelexists;
extern void status(void);
extern uint16_t reffile;
extern uint32_t backr, forwr;
extern uint8_t pass;

#define ignore() while(pline[lpoint]==0x20 || pline[lpoint]==0x09) lpoint++
#define get() pline[lpoint++]
#define here() pline[lpoint]
#define linelength 4096

extern const uint8_t whatis[256];
extern struct label_s *find_label(const char*);
extern struct label_s *find_label2(const char*, const struct avltree *);
extern struct label_s *new_label(const char*, enum label_e);
extern struct section_s *new_section(const char*);
extern struct section_s *find_section(const char*);
extern struct macro_s *find_macro(const char*);
extern struct macro_s *new_macro(const char*);
extern struct jump_s *find_jump(const char*);
extern struct jump_s *new_jump(const char*);
extern struct star_s *new_star(line_t);
extern struct file_s *openfile(const char*);
extern struct file_s *openfile_from_buffer(const char* name, const char *buffer, int buffer_size);
extern void closefile(struct file_s*);
extern void tfree(void);
extern void tinit(void);
extern void labelprint(void);
extern int testarg(int,char **,struct file_s *, int skipCheck);
extern struct arguments_s arguments;
extern struct label_s *current_context, root_label;
extern unsigned int utf8in(const uint8_t *c, uint32_t *out);
extern struct encoding_s *actual_encoding;

#endif
