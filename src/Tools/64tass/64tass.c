/*
    Turbo Assembler 6502/65C02/65816/DTV
    Copyright (C) <year>  <name of author>

   6502/65C02 Turbo Assembler  Version 1.3
   (c)1996 Taboo Productions, Marek Matula

   6502/65C02 Turbo Assembler  Version 1.35  ANSI C port
   (c)2000 BiGFooT/BReeZe^2000

   6502/65C02/65816/DTV Turbo Assembler  Version 1.4x
   (c)2001-2011 Soci/Singular (soci@c64.rulez.org)

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

#define _GNU_SOURCE
#define _MAIN_C_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <time.h>

#ifdef WIN32
#define strcasecmp(s1, s2)      _stricmp(s1, s2)
#endif

#include "64tass-opcodes.h"
#include "64tass-misc.h"
#include "64tass-eval.h"
#include "64tass-error.h"
#include "64tass-section.h"
#include "64tass-encoding.h"

static const char *mnemonic;    //mnemonics
static const uint8_t *opcode;    //opcodes

struct memblock_s {size_t p, len;address_t start;}; //starts and sizes

#define nestinglevel 256
static int wrapwarn=0, wrapwarn2=0;
line_t sline, vline_64tass;      //current line
static address_t all_mem;
uint8_t pass=0;      //pass
static int listing=0;   //listing
static struct {size_t p, len;uint8_t *data;} mem = {0, 0, NULL};//Linear memory dump
static size_t memblocklastp = 0;
static address_t memblocklaststart = 0;
static struct {unsigned int p, len;struct memblock_s *data;} memblocks = {0, 0, NULL};
address_t star=0;
const uint8_t *pline, *llist;  //current line data
unsigned int lpoint;              //position in current line
static char path[linelength];   //path
static FILE* flist = NULL;      //listfile
static enum { LIST_NONE, LIST_CODE, LIST_DATA, LIST_EQU } lastl = LIST_CODE;
static int longaccu=0,longindex=0,scpumode=0,dtvmode=0;
static uint8_t databank=0;
static uint16_t dpage=0;
int fixeddig;
static int allowslowbranch=1;
static int longbranchasjmp=0;
static uint8_t outputeor = 0; // EOR value for final output (usually 0, except changed by .eor)

static struct {
    char what;
    line_t line;
    address_t addr;
    address_t laddr;
    struct label_s *label;
    struct section_s *section;
    uint8_t skip;
} waitfor[nestinglevel];

static uint8_t waitforp=0;

static unsigned int last_mnem;

int labelexists;
uint16_t reffile;
uint32_t backr, forwr;
struct file_s *cfile;
struct avltree *star_tree = NULL;
static uint_fast8_t structrecursion;
static int unionmode;
static address_t unionstart, unionend, l_unionstart, l_unionend;

struct {
    uint8_t p, len;
    struct {
        size_t len, size;
        struct {
            size_t len;
            const uint8_t *data;
        } *param, all;
        uint8_t *pline;
    } *params, *current;
} macro_parameters = {0, 0, NULL, NULL};

static const char* command[]={ /* must be sorted, first char is the ID */
    "\x20" "al",
    "\x32" "align",
    "\x1f" "as",
    "\x33" "assert",
    "\x38" "bend",
    "\x18" "binary",
    "\x37" "block",
    "\x05" "byte",
    "\x4c" "cdef",
    "\x30" "cerror",
    "\x06" "char",
    "\x34" "check",
    "\x19" "comment",
    "\x35" "cpu",
    "\x31" "cwarn",
    "\x26" "databank",
    "\x0b" "dint",
    "\x27" "dpage",
    "\x4a" "dsection",
    "\x45" "dstruct",
    "\x48" "dunion",
    "\x0c" "dword",
    "\x4d" "edef",
    "\x13" "else",
    "\x15" "elsif",
    "\x2a" "enc",
    "\x3d" "end",
    "\x1a" "endc",
    "\x2b" "endif",
    "\x0f" "endm",
    "\x1c" "endp",
    "\x44" "ends",
    "\x47" "endu",
    "\x3e" "eor",
    "\x23" "error",
    "\x14" "fi",
    "\x28" "fill",
    "\x10" "for",
    "\x42" "goto",
    "\x1e" "here",
    "\x3c" "hidemac",
    "\x12" "if",
    "\x2d" "ifeq",
    "\x2f" "ifmi",
    "\x2c" "ifne",
    "\x2e" "ifpl",
    "\x17" "include",
    "\x08" "int",
    "\x41" "lbl",
    "\x1d" "logical",
    "\x0a" "long",
    "\x0e" "macro",
    "\x11" "next",
    "\x04" "null",
    "\x0d" "offs",
    "\x36" "option",
    "\x1b" "page",
    "\x25" "pend",
    "\x24" "proc",
    "\x3a" "proff",
    "\x39" "pron",
    "\x01" "ptext",
    "\x16" "rept",
    "\x07" "rta",
    "\x49" "section",
    "\x3f" "segment",
    "\x4b" "send",
    "\x02" "shift",
    "\x03" "shiftl",
    "\x3b" "showmac",
    "\x43" "struct",
    "\x00" "text",
    "\x46" "union",
    "\x40" "var",
    "\x29" "warn",
    "\x09" "word",
    "\x22" "xl",
    "\x21" "xs",
};

enum command_e {
    CMD_TEXT=0, CMD_PTEXT, CMD_SHIFT, CMD_SHIFTL, CMD_NULL, CMD_BYTE, CMD_CHAR,
    CMD_RTA, CMD_INT, CMD_WORD, CMD_LONG, CMD_DINT, CMD_DWORD, CMD_OFFS,
    CMD_MACRO, CMD_ENDM, CMD_FOR, CMD_NEXT, CMD_IF, CMD_ELSE, CMD_FI,
    CMD_ELSIF, CMD_REPT, CMD_INCLUDE, CMD_BINARY, CMD_COMMENT, CMD_ENDC,
    CMD_PAGE, CMD_ENDP, CMD_LOGICAL, CMD_HERE, CMD_AS, CMD_AL, CMD_XS, CMD_XL,
    CMD_ERROR, CMD_PROC, CMD_PEND, CMD_DATABANK, CMD_DPAGE, CMD_FILL, CMD_WARN,
    CMD_ENC, CMD_ENDIF, CMD_IFNE, CMD_IFEQ, CMD_IFPL, CMD_IFMI, CMD_CERROR,
    CMD_CWARN, CMD_ALIGN, CMD_ASSERT, CMD_CHECK, CMD_CPU, CMD_OPTION,
    CMD_BLOCK, CMD_BEND, CMD_PRON, CMD_PROFF, CMD_SHOWMAC, CMD_HIDEMAC,
    CMD_END, CMD_EOR, CMD_SEGMENT, CMD_VAR, CMD_LBL, CMD_GOTO, CMD_STRUCT,
    CMD_ENDS, CMD_DSTRUCT, CMD_UNION, CMD_ENDU, CMD_DUNION, CMD_SECTION,
    CMD_DSECTION, CMD_SEND, CMD_CDEF, CMD_EDEF,
};

// ---------------------------------------------------------------------------

void status(void) {
    freeerrorlist(1);
    errors+=conderrors;
    if (arguments.quiet) {
        address_t start, end;
        unsigned int i;
        char temp[10];
        printf("Error messages:    ");
        if (errors) printf("%u\n", errors); else puts("None");
        printf("Warning messages:  ");
        if (warnings) printf("%u\n", warnings); else puts("None");
        printf("Passes:            %u\n",pass);
        if (memblocks.p) {
            start = memblocks.data[0].start;
            end = memblocks.data[0].start + memblocks.data[0].len;
            for (i=1;i<memblocks.p;i++) {
                if (memblocks.data[i].start != end) {
                    sprintf(temp, "$%04" PRIaddress, start);
                    printf("Memory range:    %7s-$%04" PRIaddress "\n", temp, end-1);
                    start = memblocks.data[i].start;
                }
                end = memblocks.data[i].start + memblocks.data[i].len;
            }
            sprintf(temp, "$%04" PRIaddress, start);
            printf("Memory range:    %7s-$%04" PRIaddress "\n", temp, end-1);
        } else puts("Memory range:      None");
    }
    free(mem.data);		        	// free codemem
	mem.data = NULL;
    free(memblocks.data);				// free memorymap
	memblocks.data = NULL;
    free_values();
    tfree();
    {
        int i;
        for (i = 0; i < macro_parameters.len; i++) {
            free(macro_parameters.params[i].pline);
            free(macro_parameters.params[i].param);
        }
        free(macro_parameters.params);
		
		macro_parameters.params = NULL;
    }
	
	
	wrapwarn=0, wrapwarn2=0;
	pass=0;      //pass
	listing=0;   //listing
	mem.p = 0;
	mem.len = 0;
	mem.data = NULL;
	memblocklastp = 0;
	memblocklaststart = 0;
	memblocks.p = 0;
	memblocks.len = 0;
	memblocks.data = NULL;
	star=0;
	lastl = LIST_CODE;
	longaccu=0,longindex=0,scpumode=0,dtvmode=0;
	databank=0;
	dpage=0;
	allowslowbranch=1;
	longbranchasjmp=0;
	outputeor = 0;
	waitforp=0;
	
	star_tree = NULL;

	macro_parameters.p = 0;
	macro_parameters.len = 0;
	macro_parameters.params = NULL;
	macro_parameters.current = NULL;
	
}

static void printllist(FILE *f) {
    const uint8_t *c = llist, *last, *n;
    uint32_t ch;
    if (c) {
        last = c;
        while ((ch = *c)) {
            if (ch & 0x80) n=c+utf8in(c, &ch); else n=c+1;
            if ((ch < 0x20 || ch > 0x7e) && ch!=9) {
                fwrite(last, c - last, 1, f);
                fprintf(f, "{$%x}", ch);
                last=n;
            }
            c = n;
        }
        while (c > last && (c[-1] == 0x20 || c[-1] == 0x09)) c--;
        fwrite(last, c - last, 1, f);
        llist=NULL;
    }
    putc('\n', f);
}

static void new_waitfor(char what) {
    waitfor[++waitforp].what = what;
    waitfor[waitforp].line = sline;
    waitfor[waitforp].label = NULL;
    waitfor[waitforp].skip=waitfor[waitforp-1].skip & 1;
}

static void set_size(struct label_s *var, size_t size) {
    size &= all_mem;
    if (var->size != size) {
        var->size = size;
        fixeddig = 0;
    }
}

// ---------------------------------------------------------------------------
/*
 * output one byte
 */
static void memjmp(address_t adr) {
    if (mem.p == memblocklastp) {
        memblocklaststart = adr;
        return;
    }
    if (memblocks.p>=memblocks.len) {
        memblocks.len+=64;
        memblocks.data=realloc(memblocks.data, memblocks.len*sizeof(*memblocks.data));
        if (!memblocks.data) err_msg(ERROR_OUT_OF_MEMORY,NULL);
    }
    memblocks.data[memblocks.p].len = mem.p-memblocklastp;
    memblocks.data[memblocks.p].p = memblocklastp;
    memblocks.data[memblocks.p++].start = memblocklaststart;
    memblocklastp = mem.p;
    memblocklaststart = adr;
}

static void memskip(address_t db) {
    if (fixeddig && scpumode) {
        if (((current_section->address + db)^current_section->address) & ~(address_t)0xffff) wrapwarn2=1;
        if (((current_section->l_address + db)^current_section->l_address) & ~(address_t)0xffff) wrapwarn2=1;
    }
    current_section->l_address += db;
    if (current_section->l_address > all_mem) {
        if (fixeddig) wrapwarn=1;
        current_section->l_address &= all_mem;
    }
    current_section->address += db;
    if (current_section->address > all_mem) {
        if (fixeddig) wrapwarn=1;
        current_section->address &= all_mem;
    }
    memjmp(current_section->address);
}

static int memblockcomp(const void *a, const void *b) {
    const struct memblock_s *aa=(struct memblock_s *)a;
    const struct memblock_s *bb=(struct memblock_s *)b;
    return aa->start-bb->start;
}

static void memcomp(void) {
    unsigned int i, j, k;
    memjmp(0);
    if (memblocks.p<2) return;

    for (k = j = 0; j < memblocks.p; j++) {
        struct memblock_s *bj = &memblocks.data[j];
        if (bj->len) {
            for (i = j + 1; i < memblocks.p; i++) if (memblocks.data[i].len) {
                struct memblock_s *bi = &memblocks.data[i];
                if (bj->start <= bi->start && (bj->start + bj->len) > bi->start) {
                    size_t overlap = (bj->start + bj->len) - bi->start;
                    if (overlap > bi->len) overlap = bi->len;
                    memcpy(mem.data + bj->p + (unsigned)(bi->start - bj->start), mem.data + bi->p, overlap);
                    bi->len-=overlap;
                    bi->p+=overlap;
                    bi->start+=overlap;
                    continue;
                }
                if (bi->start <= bj->start && (bi->start + bi->len) > bj->start) {
                    size_t overlap = bi->start + bi->len - bj->start;
                    if (overlap > bj->len) overlap = bj->len;
                    bj->start+=overlap;
                    bj->p+=overlap;
                    bj->len-=overlap;
                    if (!bj->len) break;
                }
            }
            if (bj->len) {
                if (j!=k) memblocks.data[k]=*bj;
                k++;
            }
        }
    }
    memblocks.p = k;
    qsort(memblocks.data, memblocks.p, sizeof(*memblocks.data), memblockcomp);
}

// ---------------------------------------------------------------------------
/*
 * output one byte
 */
static void pokeb(uint8_t byte)
{

    if (fixeddig && current_section->dooutput)
    {
        if (mem.p>=mem.len) {
            mem.len+=0x1000;
            mem.data=realloc(mem.data, mem.len);
            if (!mem.data) err_msg(ERROR_OUT_OF_MEMORY,NULL);
        }
	mem.data[mem.p++] = byte ^ outputeor;
    }
    if (wrapwarn) {err_msg(ERROR_TOP_OF_MEMORY,NULL);wrapwarn=0;}
    if (wrapwarn2) {err_msg(ERROR___BANK_BORDER,NULL);wrapwarn2=0;}
    current_section->address++;current_section->l_address++;current_section->l_address &= all_mem;
    if (current_section->address & ~all_mem) {
	if (fixeddig) wrapwarn=1;
	current_section->address=0;
        memjmp(current_section->address);
    }
    if (fixeddig && scpumode) if (!(current_section->address & 0xffff) || !(current_section->l_address & 0xffff)) wrapwarn2=1;
}

static int lookup_opcode(const char *s) {
    char s2,s3, ch;
    const char *p;
    int s4;
    unsigned int also=0,felso,elozo, no;

    ch=lowcase(s[0]);
    no=(felso=last_mnem)/2;
    if (ch && (s2=lowcase(s[1])) && (s3=lowcase(s[2])) && !s[3])
        for (;;) {  // do binary search
            if (!(s4=ch-*(p=mnemonic+no*3)))
                if (!(s4=s2-*(++p)))
                    if (!(s4=s3-*(++p)))
                    {
                        return no;
                    }

            elozo=no;
            if (elozo==(no=((s4>0) ? (felso+(also=no)) : (also+(felso=no)) )/2)) break;
        }
    return -1;
}

// ---------------------------------------------------------------------------
static int what(int *tempno) {
    char ch;

    ignore();
    switch (ch=whatis[(int)here()]) {
    case WHAT_COMMAND:
	{
            char cmd[20];
            unsigned int no, l, also, felso, elozo;
            int s4;
            lpoint++;
            for (also = l = 0; l < sizeof(cmd)-1; l++) {
                cmd[l]=pline[lpoint+l] | 0x20;
                if (!pline[lpoint+l] || cmd[l] < 'a' || cmd[l] > 'z') {
                    cmd[l]=(cmd[l] >= '0' && cmd[l] <= '9') || cmd[l]=='_';
                    l++;break;
                }
            }
            l--;
            if (!cmd[l]) {
                felso=sizeof(command)/sizeof(command[0]);
                no=felso/2;
                for (;;) {  // do binary search
                    if (!(s4=strcmp(cmd, command[no] + 1))) {
                        lpoint+=l;
                        no = (uint8_t)command[no][0];
                        if (no==CMD_ENDIF) no=CMD_FI; else
                        if (no==CMD_IFNE) no=CMD_IF;
                        *tempno=no;
                        return WHAT_COMMAND;
                    }

                    elozo = no;
                    no = ((s4>0) ? (felso+(also=no)) : (also+(felso=no)))/2;
                    if (elozo == no) break;
                }
            }
	    *tempno=sizeof(command)/sizeof(command[0]);
	    return 0;
	}
    case WHAT_COMA:
	lpoint++;
        ignore();
	switch (get() | 0x20) {
	case 'y': ignore();return WHAT_Y;
	case 'x': ignore();if (here()==')') {lpoint++;ignore();return WHAT_XZ;} else return WHAT_X;
	case 's': ignore();if (here()==')') {lpoint++;ignore();return WHAT_SZ;} else return WHAT_S;
	case 'r': ignore();if (here()==')') {lpoint++;ignore();return WHAT_RZ;} else return WHAT_R;
	default: lpoint--;return WHAT_COMA;
	}
    case WHAT_CHAR:
    case WHAT_LBL:
            *tempno=1;return WHAT_EXPRESSION;
    case WHAT_EXPRESSION://tempno=1 if label, 0 if expression
	    *tempno=0;return WHAT_EXPRESSION;
    case WHAT_COMMENT:
    case WHAT_EOL:return ch;
    default:lpoint++;return ch;
    }
}

static int get_ident2(char *ident) {
    unsigned int i=0;
    uint8_t ch;
    if (arguments.casesensitive)
	while ((whatis[ch=here()]==WHAT_CHAR) || (ch>='0' && ch<='9') || ch=='_') {ident[i++]=ch; lpoint++; }
    else
	while (((ch=lowcase(pline[lpoint]))>='a' && ch<='z') || (ch>='0' && ch<='9') || ch=='_') { ident[i++]=ch; lpoint++; }
    ident[i]=0;
    return i == 0;
}

static int get_ident(char *ident) {
    int code;

    if (what(&code)!=WHAT_EXPRESSION || !code) {
	err_msg(ERROR_EXPRES_SYNTAX,NULL);
	return 1;
    }
    return get_ident2(ident);
}

static int get_hack(void) {
    int q=1;
    unsigned int i=0, i2;
    i2 = i;
    ignore();
    if (here()=='\"') {lpoint++;q=0;}
    while (here() && (here()!=';' || !q) && (here()!='\"' || q) && i<sizeof(path)) path[i++]=get();
    if (i>=sizeof(path) || (!q && here()!='\"')) {err_msg(ERROR_GENERL_SYNTAX,NULL); return 1;}
    if (!q) lpoint++; else while (i && (path[i-1]==0x20 || path[i-1]==0x09)) i--;
    path[i]=0;
    ignore();
    if (i <= i2) {err_msg(ERROR_GENERL_SYNTAX,NULL); return 1;}
    return 0;
}

static int get_path(struct value_s *v, const char *base) {
    unsigned int i;
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
    unsigned int j;

    i = strlen(base);
    j = (((base[0] >= 'A' && base[0] <= 'Z') || (base[0] >= 'a' && base[0] <= 'z')) && base[1]==':') ? 2 : 0;
    while (i > j) {
        if (base[i-1] == '/' || base[i-1] == '\\') break;
        i--;
    }
#else
    char *c;
    c = strrchr(base, '/');
    i = c ? (c - base + 1) : 0;
#endif

    if (i && i < sizeof(path)) memcpy(path, base, i);
    
    if (i + v->u.str.len + 1 >= sizeof(path)) {err_msg(ERROR_CONSTNT_LARGE,NULL); return 1;}
    memcpy(path + i, v->u.str.data, v->u.str.len);
    path[i + v->u.str.len] = 0;
    return 0;
}

//------------------------------------------------------------------------------

/*
 * macro parameter expansion
 *
 * in:
 *   mpr:  parameters, separated by zeros
 *   nprm: number of parameters
 *   cucc: one line of the macro (unexpanded)
 * out:
 *   cucc: one line of the macro (expanded)
*/
static inline void mtranslate()
{
    uint_fast8_t q;
    uint_fast16_t p, j;
    uint8_t ch, *cucc = macro_parameters.current->pline;

    q=p=0;
    for (; (ch = here()); lpoint++) {
        if (ch == '"'  && !(q & 2)) { q^=1; }
        else if (ch == '\'' && !(q & 1)) { q^=2; }
        else if ((ch == ';') && (!q)) { q=4; }
        else if ((ch=='\\') && (!q)) {
            /* normal parameter reference */
            if (((ch=lowcase(pline[lpoint+1])) >= '1' && ch <= '9') || (ch >= 'a' && ch <= 'z')) {
                /* \1..\9, \a..\z */
                if ((j=(ch<='9' ? ch-'1' : ch-'a'+9)) >= macro_parameters.current->len) {err_msg(ERROR_MISSING_ARGUM,NULL); break;}
                if (p + macro_parameters.current->param[j].len >= linelength) err_msg(ERROR_LINE_TOO_LONG,NULL);
                else {
                    memcpy(cucc + p, macro_parameters.current->param[j].data, macro_parameters.current->param[j].len);
                    p += macro_parameters.current->param[j].len;
                }
                lpoint++;continue;
            } else if (ch=='@') {
                /* \@ gives complete parameter list */
                if (p + macro_parameters.current->all.len >= linelength) err_msg(ERROR_LINE_TOO_LONG,NULL);
                else {
                    memcpy(cucc + p, macro_parameters.current->all.data, macro_parameters.current->all.len);
                    p += macro_parameters.current->all.len;
                }
                lpoint++;continue;
            } else ch='\\';
        } else if (ch=='@') {
            /* text parameter reference */
            if (((ch=lowcase(pline[lpoint+1]))>='1' && ch<='9')) {
                /* @1..@9 */
                if ((j=ch-'1') >= macro_parameters.current->len) {err_msg(ERROR_MISSING_ARGUM,NULL); break;}
                if (p + macro_parameters.current->param[j].len >= linelength) err_msg(ERROR_LINE_TOO_LONG,NULL);
                else {
                    memcpy(cucc + p, macro_parameters.current->param[j].data, macro_parameters.current->param[j].len);
                    p += macro_parameters.current->param[j].len;
                }
                lpoint++;continue;
            } else ch='@';
        }
        cucc[p++]=ch;
        if (p>=linelength) err_msg(ERROR_LINE_TOO_LONG,NULL);
    }
    cucc[p]=0;
    pline = cucc; lpoint = 0;
}

//------------------------------------------------------------------------------

static void set_cpumode(uint_fast8_t cpumode) {
    all_mem=0xffff;scpumode=0;dtvmode=0;
    switch (last_mnem=cpumode) {
    case OPCODES_65C02:mnemonic=MNEMONIC65C02;opcode=c65c02;break;
    case OPCODES_6502i:mnemonic=MNEMONIC6502i;opcode=c6502i;break;
    case OPCODES_65816:mnemonic=MNEMONIC65816;opcode=c65816;all_mem=0xffffff;scpumode=1;break;
    case OPCODES_65DTV02:mnemonic=MNEMONIC65DTV02;opcode=c65dtv02;dtvmode=1;break;
    case OPCODES_65EL02:mnemonic=MNEMONIC65EL02;opcode=c65el02;break;
    default: mnemonic=MNEMONIC6502;opcode=c6502;break;
    }
}

void var_assign(struct label_s *tmp, const struct value_s *val, int fix) {
    switch (tmp->value.type) {
    case T_UINT:
    case T_SINT:
    case T_NUM:
        if ((val->type != T_UINT && val->type != T_SINT && val->type != T_NUM) || tmp->value.u.num.val!=val->u.num.val) {
            tmp->value=*val;
            if (val->type == T_STR) {
                tmp->value.u.str.data=malloc(val->u.str.len);
                memcpy(tmp->value.u.str.data,val->u.str.data,val->u.str.len);
            }
            fixeddig=fix;
        } else tmp->value.type = val->type;
        break;
    case T_STR:
        if (val->type != T_STR || tmp->value.u.str.len!=val->u.str.len || memcmp(tmp->value.u.str.data, val->u.str.data, val->u.str.len)) {
            if (val->type == T_STR) {
                tmp->value.type = val->type;
                if (tmp->value.u.str.len != val->u.str.len) {
                    tmp->value.u.str.len = val->u.str.len;
                    tmp->value.u.str.data = realloc(tmp->value.u.str.data, val->u.str.len);
                    if (!tmp->value.u.str.data) err_msg(ERROR_OUT_OF_MEMORY,NULL);
                }
                memcpy(tmp->value.u.str.data, val->u.str.data, val->u.str.len);
            } else {
                free(tmp->value.u.str.data);
                tmp->value=*val;
            }
            fixeddig=fix;
        }
        break;
    case T_NONE:
    case T_GAP:
        if (val->type != tmp->value.type) fixeddig=fix;
        tmp->value=*val;
        if (val->type == T_STR) {
            tmp->value.u.str.data=malloc(val->u.str.len);
            memcpy(tmp->value.u.str.data,val->u.str.data,val->u.str.len);
        }
        break;
    default: /* not possible here */
        exit(1);
        break;
    }
    tmp->upass=pass;
}

static void compile(void);

static void macro_recurse(char t, struct macro_s *tmp2) {
    if (macro_parameters.p>100) {
        err_msg(ERROR__MACRECURSION,"!!!!");
        return;
    }
    if (macro_parameters.p >= macro_parameters.len) {
        macro_parameters.len += 1;
        macro_parameters.params = realloc(macro_parameters.params, sizeof(*macro_parameters.params) * macro_parameters.len);
        if (!macro_parameters.params) err_msg(ERROR_OUT_OF_MEMORY,NULL);
        macro_parameters.params[macro_parameters.p].param = NULL;
        macro_parameters.params[macro_parameters.p].size = 0;
        macro_parameters.params[macro_parameters.p].pline = malloc(linelength);
    }
    macro_parameters.current = &macro_parameters.params[macro_parameters.p];
    macro_parameters.p++;
    {
        uint_fast8_t q = 0, ch;
        unsigned int opoint, npoint;
        size_t p = 0;

        ignore(); opoint = lpoint;
        if (here() && here()!=';') {
            char par[256];
            uint8_t pp = 0;
            do {
                unsigned int opoint, npoint;
                ignore(); opoint = lpoint;
                while ((ch=here()) && (q || (ch!=';' && (ch!=',' || pp)))) {
                    if (ch == '"'  && !(q & 2)) { q^=1; }
                    else if (ch == '\'' && !(q & 1)) { q^=2; }
                    if (!q) {
                        if (ch == '(' || ch =='[') par[pp++]=ch;
                        else if (pp && ((ch == ')' && par[pp-1]=='(') || (ch == ']' && par[pp-1]=='['))) pp--;
                    }
                    lpoint++;
                }
                if (p >= macro_parameters.current->size) {
                    macro_parameters.current->size += 4;
                    macro_parameters.current->param = realloc(macro_parameters.current->param, sizeof(*macro_parameters.current->param) * macro_parameters.current->size);
                    if (!macro_parameters.current->param) err_msg(ERROR_OUT_OF_MEMORY,NULL);
                }
                macro_parameters.current->param[p].data = pline + opoint;
                npoint = lpoint;
                while (npoint > opoint && (pline[npoint-1] == 0x20 || pline[npoint-1] == 0x09)) npoint--;
                macro_parameters.current->param[p].len = npoint - opoint;
                p++;
                if (ch == ',') lpoint++;
            } while (ch == ',');
        }
        macro_parameters.current->len = p;
        macro_parameters.current->all.data = pline + opoint;
        npoint = lpoint;
        while (npoint > opoint && (pline[npoint-1] == 0x20 || pline[npoint-1] == 0x09)) npoint--;
        macro_parameters.current->all.len = npoint - opoint;
    }
    {
        size_t oldpos = tmp2->file->p;
        line_t lin = sline;
        struct file_s *f;
        struct star_s *s = new_star(vline_64tass);
        struct avltree *stree_old = star_tree;
        line_t ovline = vline_64tass;

        if (labelexists && s->addr != star) fixeddig=0;
        s->addr = star;
        star_tree = &s->tree;vline_64tass=0;
        enterfile(tmp2->file->name, sline);
        sline = tmp2->sline;
        new_waitfor(t);
        f = cfile; cfile = tmp2->file;
        cfile->p = tmp2->p;
        compile();
        exitfile(); cfile = f;
        star_tree = stree_old; vline_64tass = ovline;
        sline = lin; tmp2->file->p = oldpos;
    }
    macro_parameters.p--;
    if (macro_parameters.p) macro_parameters.current = &macro_parameters.params[macro_parameters.p - 1];
}

static void compile(void)
{
    int wht,w;
    int prm = 0;
    struct value_s val;

    struct label_s *newlabel = NULL;
    struct macro_s *tmp2 = NULL;
    address_t oaddr = 0;

    uint8_t oldwaitforp = waitforp;
    unsigned wasref;
    int nobreak = 1;
    char labelname[linelength], labelname2[linelength];
    unsigned int epoint;

    while (cfile->len != cfile->p && nobreak) {
        pline = cfile->data + cfile->p; lpoint = 0; sline++;vline_64tass++; cfile->p += strlen((char *)pline) + 1;
        if (macro_parameters.p) mtranslate(); //expand macro parameters, if any
        llist = pline;
        star=current_section->l_address;newlabel = NULL;
        labelname2[0]=wasref=0;ignore();epoint=lpoint;
        if (unionmode) {
            if (current_section->address > unionend) unionend = current_section->address;
            if (current_section->l_address > l_unionend) l_unionend = current_section->l_address;
            current_section->l_address = l_unionstart;
            if (current_section->address != unionstart) {
                current_section->address = unionstart;
                memjmp(current_section->address);
            }
        }
        if ((wht=what(&prm))==WHAT_EXPRESSION) {
            int islabel;
            if (!prm) {
                if (here()=='-' || here()=='+') {
                    labelname2[0]=here();labelname2[1]=0;
                    lpoint++;if (here()!=0x20 && here()!=0x09 && here()!=';' && here()) goto baj;
                    if (labelname2[0]=='-') {
                        sprintf(labelname,"-%x-%x", reffile, backr++);
                    } else {
                        sprintf(labelname,"+%x+%x", reffile, forwr++);
                    }
                    islabel = 1;goto hh;
                }
            baj:
                if (waitfor[waitforp].skip & 1) err_msg2(ERROR_GENERL_SYNTAX,NULL, epoint);
                goto breakerr;
            } //not label
            get_ident(labelname);islabel = (here()==':');
            if (islabel) lpoint++;
            else if ((prm=lookup_opcode(labelname))>=0) {
                if (waitfor[waitforp].skip & 1) goto as_opcode; else continue;
            }
            if (listing) strcpy(labelname2, labelname);
        hh:
            if (!(waitfor[waitforp].skip & 1)) {wht=what(&prm);goto jn;} //skip things if needed
            if ((wht=what(&prm))==WHAT_EQUAL) { //variable
                if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                if (!get_val(&val, T_NONE, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                eval_finish();
                newlabel=new_label(labelname, L_CONST);oaddr=current_section->address;
                if (listing && flist && arguments.source && newlabel->ref) {
                    if (lastl!=LIST_EQU) {putc('\n',flist);lastl=LIST_EQU;}
                    if (val.type == T_UINT || val.type == T_SINT || val.type == T_NUM) {
                        fprintf(flist,"=%" PRIxval "\t\t\t\t\t",(uval_t)val.u.num.val);
                    } else {
                        fputs("=\t\t\t\t\t", flist);
                    }
                    printllist(flist);
                }
                newlabel->ref=0;
                if (pass==1) {
                    if (labelexists) err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                    else {
                        newlabel->requires=current_section->requires;
                        newlabel->conflicts=current_section->conflicts;
                        newlabel->pass=pass;
                        newlabel->value.type=T_NONE;
                        var_assign(newlabel, &val, fixeddig);
                    }
                } else {
                    if (labelexists) {
                        newlabel->requires=current_section->requires;
                        newlabel->conflicts=current_section->conflicts;
                        var_assign(newlabel, &val, 0);
                    }
                }
                goto finish;
            }
            if (wht==WHAT_COMMAND) {
                switch (prm) {
                case CMD_VAR: //variable
                    if (!get_exp(&w, 0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_NONE, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    newlabel=new_label(labelname, L_VAR);oaddr=current_section->address;
                    if (listing && flist && arguments.source && newlabel->ref) {
                        if (lastl!=LIST_EQU) {putc('\n',flist);lastl=LIST_EQU;}
                        if (val.type == T_UINT || val.type == T_SINT || val.type == T_NUM) {
                            fprintf(flist,"=%" PRIxval "\t\t\t\t\t",(uval_t)val.u.num.val);
                        } else {
                            fputs("=\t\t\t\t\t", flist);
                        }
                        printllist(flist);
                    }
                    newlabel->ref=0;
                    if (labelexists) {
                        if (newlabel->type != L_VAR) err_msg2(ERROR_DOUBLE_DEFINE, labelname, epoint);
                        else {
                            newlabel->requires=current_section->requires;
                            newlabel->conflicts=current_section->conflicts;
                            var_assign(newlabel, &val, fixeddig);
                        }
                    } else {
                        newlabel->requires=current_section->requires;
                        newlabel->conflicts=current_section->conflicts;
                        newlabel->pass=pass;
                        newlabel->value.type=T_NONE;
                        var_assign(newlabel, &val, fixeddig);
                        if (val.type == T_NONE) err_msg(ERROR___NOT_DEFINED,"argument used");
                    }
                    goto finish;
                case CMD_LBL:
                    { //variable
                        struct jump_s *tmp2;
                        if (listing && flist && arguments.source) {
                            if (lastl!=LIST_EQU) {putc('\n',flist);lastl=LIST_EQU;}
                            fputs("=\t\t\t\t\t", flist);
                            printllist(flist);
                        }
                        tmp2 = new_jump(labelname);
                        if (labelexists) {
                            if (tmp2->sline != sline
                                    || tmp2->waitforp != waitforp
                                    || tmp2->file != cfile
                                    || tmp2->p != cfile->p
                                    || tmp2->parent != current_context) {
                                err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                            }
                        } else {
                            tmp2->sline = sline;
                            tmp2->waitforp = waitforp;
                            tmp2->file = cfile;
                            tmp2->p = cfile->p;
                            tmp2->parent = current_context;
                        }
                        goto finish;
                    }
                case CMD_MACRO:// .macro
                case CMD_SEGMENT:
                    new_waitfor('m');waitfor[waitforp].skip=0;
                    tmp2=new_macro(labelname);
                    if (labelexists) {
                        if (tmp2->p!=cfile->p
                         || tmp2->sline!=sline
                         || tmp2->type!=prm
                         || tmp2->file!=cfile) {
                            err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                        }
                    } else {
                        tmp2->p=cfile->p;
                        tmp2->sline=sline;
                        tmp2->type=prm;
                        tmp2->file=cfile;
                    }
                    goto finish;
                case CMD_STRUCT:
                case CMD_UNION:
                    {
                        struct label_s *old_context=current_context;
                        struct section_s new;
                        new_waitfor((prm==CMD_STRUCT)?'s':'u');waitfor[waitforp].skip=0;
                        if (!structrecursion) {
                            new.parent = current_section;
                            new.provides=~(uval_t)0;new.requires=new.conflicts=0;
                            new.start=new.l_start=new.address=new.l_address=0;
                            new.dooutput=0;new.name=NULL;memjmp(0);
                            current_section = &new;

                            tmp2=new_macro(labelname);
                            if (labelexists) {
                                if (tmp2->p!=cfile->p
                                        || tmp2->sline!=sline
                                        || tmp2->type!=prm
                                        || tmp2->file!=cfile) {
                                    err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                                }
                            } else {
                                tmp2->p=cfile->p;
                                tmp2->sline=sline;
                                tmp2->type=prm;
                                tmp2->file=cfile;
                            }
                        }
                        newlabel=new_label(labelname, (prm==CMD_STRUCT)?L_STRUCT:L_UNION);oaddr = current_section->address;
                        if (pass==1) {
                            if (labelexists) err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                            else {
                                newlabel->requires=0;
                                newlabel->conflicts=0;
                                newlabel->upass=newlabel->pass=pass;
                                newlabel->value.type=T_UINT;newlabel->value.u.num.val=0;newlabel->value.u.num.len=1;
                            }
                        } else {
                            if (labelexists) {
                                if (newlabel->value.type != T_UINT || newlabel->type != ((prm==CMD_STRUCT)?L_STRUCT:L_UNION)) { /* should not happen */
                                    err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                                } else {
                                    if (newlabel->value.u.num.val != 0) {
                                        newlabel->value.u.num.val=0;newlabel->value.u.num.len=1;
                                        fixeddig=0;
                                    }
                                    newlabel->requires=0;
                                    newlabel->conflicts=0;
                                    newlabel->value.type=T_UINT;
                                }
                            }
                        }
                        current_context=newlabel;
                        newlabel->ref=0;
                        if (listing && flist && arguments.source) {
                            if (lastl!=LIST_DATA) {putc('\n',flist);lastl=LIST_DATA;}
                            fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t":".%06" PRIaddress "\t\t\t\t\t",current_section->address);
                            printllist(flist);
                        }
                        structrecursion++;
                        if (structrecursion<100) {
                            int old_unionmode = unionmode;
                            address_t old_unionstart = unionstart, old_unionend = unionend;
                            address_t old_l_unionstart = l_unionstart, old_l_unionend = l_unionend;
                            unionmode = (prm==CMD_UNION);
                            unionstart = unionend = current_section->address;
                            l_unionstart = l_unionend = current_section->l_address;
                            waitforp--;
                            new_waitfor((prm==CMD_STRUCT)?'S':'U');waitfor[waitforp].skip=1;
                            compile();
                            current_context = old_context;
                            unionmode = old_unionmode;
                            unionstart = old_unionstart; unionend = old_unionend;
                            l_unionstart = old_l_unionstart; l_unionend = old_l_unionend;
                        } else err_msg(ERROR__MACRECURSION,"!!!!");
                        structrecursion--;
                        set_size(newlabel, current_section->address - oaddr);
                        if (!structrecursion) {current_section = new.parent; memjmp(current_section->address);}
			newlabel = NULL;
                        goto finish;
                    }
                case CMD_SECTION:
                    {
                        struct section_s *tmp;
                        char sectionname[linelength];
                        unsigned int epoint;
                        new_waitfor('t');waitfor[waitforp].section=current_section;
                        if (structrecursion) err_msg(ERROR___NOT_ALLOWED, ".SECTION");
                        ignore();epoint=lpoint;
                        if (get_ident2(sectionname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                        tmp=find_new_section(sectionname);
                        if (!tmp->declared) {
                            if (!labelexists) {
                                tmp->start = tmp->address = tmp->r_address = 0;
                                tmp->l_start = tmp->l_address = tmp->r_l_address = 0;
                                fixeddig=0;
                            } else if (pass > 1) {
                                err_msg2(ERROR___NOT_DEFINED,sectionname,epoint); goto breakerr;
                            }
                        } else if (tmp->pass != pass) {
                            tmp->r_address = tmp->address;
                            tmp->address = tmp->start;
                            tmp->r_l_address = tmp->l_address;
                            tmp->l_address = tmp->l_start;
                        }
                        tmp->pass = pass;
                        waitfor[waitforp].what = 'T';
                        current_section = tmp;
                        memjmp(current_section->address);
                        break;
                    }
                }
            }
            newlabel=find_label2(labelname, &current_context->members);
            if (newlabel) labelexists=1;
            else {
                if (!islabel && (tmp2=find_macro(labelname)) && (tmp2->type==CMD_MACRO || tmp2->type==CMD_SEGMENT)) {lpoint--;labelname2[0]=0;goto as_macro;}
                newlabel=new_label(labelname, L_LABEL);
            }
            oaddr=current_section->address;
            if (pass==1) {
                if (labelexists) err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                else {
                    newlabel->requires=current_section->requires;
                    newlabel->conflicts=current_section->conflicts;
                    newlabel->upass=newlabel->pass=pass;
                    set_uint(&newlabel->value, current_section->l_address);
                    newlabel->value.type = T_NUM;
                }
            } else {
                if (labelexists) {
                    if (newlabel->value.type != T_NUM || newlabel->type != L_LABEL) { /* should not happen */
                        err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                    } else {
                        if ((uval_t)newlabel->value.u.num.val != current_section->l_address) {
                            set_uint(&newlabel->value, current_section->l_address);
                            fixeddig=0;
                        }
                        newlabel->requires=current_section->requires;
                        newlabel->conflicts=current_section->conflicts;
                        newlabel->value.type=T_NUM;
                    }
                } else {
                    newlabel->requires=current_section->requires;
                    newlabel->conflicts=current_section->conflicts;
                    newlabel->upass=newlabel->pass=pass;
                    set_uint(&newlabel->value, current_section->l_address);
                    newlabel->value.type = T_NUM;
                }
            }
            if (epoint && !islabel) err_msg2(ERROR_LABEL_NOT_LEF,NULL,epoint);
            if (wht==WHAT_COMMAND) { // .proc
                switch (prm) {
                case CMD_PROC:
                    new_waitfor('r');waitfor[waitforp].label=newlabel;waitfor[waitforp].addr = current_section->address;
                    if (!newlabel->ref && pass != 1) waitfor[waitforp].skip=0;
                    else {
                        current_context=newlabel;
                        if (listing && flist && arguments.source) {
                            if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                            fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t%s\n":".%06" PRIaddress "\t\t\t\t\t%s\n",current_section->address,labelname2);
                        }
                        newlabel->ref=0;
                    }
                    newlabel = NULL;
                    goto finish;
                case CMD_BLOCK: // .block
                    new_waitfor('B');
                    current_context=newlabel;waitfor[waitforp].label=newlabel;waitfor[waitforp].addr = current_section->address;
                    if (newlabel->ref && listing && flist && arguments.source) {
                        if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                        fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t%s\n":".%06" PRIaddress "\t\t\t\t\t%s\n",current_section->address,labelname2);
                    }
                    newlabel->ref=0;
                    newlabel = NULL;
                    goto finish;
                case CMD_DSTRUCT: // .dstruct
                case CMD_DUNION:
                    {
                        struct label_s *oldcontext = current_context;
                        int old_unionmode = unionmode;
                        address_t old_unionstart = unionstart, old_unionend = unionend;
                        address_t old_l_unionstart = l_unionstart, old_l_unionend = l_unionend;
                        unionmode = (prm==CMD_DUNION);
                        unionstart = unionend = current_section->address;
                        l_unionstart = l_unionend = current_section->l_address;
                        current_context=newlabel;
                        if (listing && flist && arguments.source) {
                            if (lastl!=LIST_DATA) {putc('\n',flist);lastl=LIST_DATA;}
                            fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t":".%06" PRIaddress "\t\t\t\t\t",current_section->address);
                            printllist(flist);
                        }
                        newlabel->ref=0;
                        ignore();epoint=lpoint;
                        if (get_ident2(labelname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                        if (!(tmp2=find_macro(labelname)) || tmp2->type!=((prm==CMD_DSTRUCT)?CMD_STRUCT:CMD_UNION)) {err_msg2(ERROR___NOT_DEFINED,labelname,epoint); goto breakerr;}
                        structrecursion++;
                        macro_recurse((prm==CMD_DSTRUCT)?'S':'U',tmp2);
                        structrecursion--;
                        current_context=oldcontext;
                        unionmode = old_unionmode;
                        unionstart = old_unionstart; unionend = old_unionend;
                        l_unionstart = old_l_unionstart; l_unionend = old_l_unionend;
                        goto finish;
                    }
                case CMD_SECTION:
                    waitfor[waitforp].label=newlabel;waitfor[waitforp].addr = current_section->address;
                    if (newlabel->ref && listing && flist && arguments.source) {
                        if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                        fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t%s\n":".%06" PRIaddress "\t\t\t\t\t%s\n",current_section->address,labelname2);
                    }
                    newlabel->ref=0;
                    newlabel = NULL;
                    goto finish;
                }
            }
            wasref=newlabel->ref;newlabel->ref=0;
        }
        jn:
        switch (wht) {
        case WHAT_STAR:if (waitfor[waitforp].skip & 1) //skip things if needed
            {
                ignore();if (here()!='=') {err_msg(ERROR______EXPECTED,"=");goto breakerr;}
                lpoint++;
                wrapwarn=0;wrapwarn2=0;
                if (!get_exp(&w,0)) goto breakerr;
                if (!get_val(&val, T_UINT, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                eval_finish();
                if (listing && flist && arguments.source) {
                    lastl=LIST_NONE;
                    if (wasref)
                        fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t":".%06" PRIaddress "\t\t\t\t\t",current_section->address);
                    else
                        fputs("\n\t\t\t\t\t", flist);
                    printllist(flist);
                }
                if (structrecursion) err_msg(ERROR___NOT_ALLOWED, "*=");
                else if (val.type == T_NONE) fixeddig = 0;
                else {
                    if ((uval_t)val.u.num.val & ~(uval_t)all_mem) {
                        err_msg(ERROR_CONSTNT_LARGE,NULL);
                    } else {
                        address_t ch2=(uval_t)val.u.num.val;
                        if (current_section->address!=ch2 || current_section->l_address!=ch2) {
                            current_section->address=current_section->l_address=ch2;
                            memjmp(current_section->address);
                        }
                    }
                }
            }
            break;
        case WHAT_COMMENT:
        case WHAT_EOL:
            if (listing && flist && arguments.source && (waitfor[waitforp].skip & 1) && wasref) {
                if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t":".%06" PRIaddress "\t\t\t\t\t",current_section->address);
                printllist(flist);
            }
            break;
        case WHAT_COMMAND:
            {
                ignore();
                if (listing && flist && arguments.source && (waitfor[waitforp].skip & 1) && prm>CMD_DWORD) {
                    switch (prm) {
                        case CMD_FILL:
                        case CMD_ALIGN:
                        case CMD_OFFS:
                        case CMD_ENDS:
                        case CMD_STRUCT:
                        case CMD_ENDU:
                        case CMD_UNION:
                            if (lastl!=LIST_DATA) {putc('\n',flist);lastl=LIST_DATA;}
                            fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t":".%06" PRIaddress "\t\t\t\t\t",current_section->address);
                            printllist(flist);
                        case CMD_BINARY:
                            break;
                        case CMD_PROC:break;
                        case CMD_AS:
                        case CMD_AL:
                        case CMD_XS:
                        case CMD_XL:
                        case CMD_DATABANK:
                        case CMD_DPAGE:
                        case CMD_LOGICAL:
                        case CMD_HERE:
                        case CMD_ENC:
                        case CMD_EOR:
                        case CMD_CPU:
                        case CMD_INCLUDE:
                            if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                            if (wasref)
                                fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t":".%06" PRIaddress "\t\t\t\t\t",current_section->address);
                            else
                                fputs("\t\t\t\t\t", flist);
                            printllist(flist);
                            break;
                        default:
                            if (wasref) {
                                if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                                fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t%s\n":".%06" PRIaddress "\t\t\t\t\t%s\n",current_section->address,labelname2);
                            }
                    }
                }
                if (prm==CMD_ENDC) { // .endc
                    if (waitfor[waitforp].what!='c') err_msg(ERROR______EXPECTED,".COMMENT");
                    else waitforp--;
                    break;
                } else if (waitfor[waitforp].what=='c') break;
                if (prm==CMD_FI) // .fi
                {
                    if (waitfor[waitforp].what!='e' && waitfor[waitforp].what!='f') err_msg(ERROR______EXPECTED,".IF");
                    else waitforp--;
                    break;
                }
                if (prm==CMD_ELSE) { // .else
                    if (waitfor[waitforp].what=='f') {err_msg(ERROR______EXPECTED,".FI"); break;}
                    if (waitfor[waitforp].what!='e') {err_msg(ERROR______EXPECTED,".IF"); break;}
                    waitfor[waitforp].skip=waitfor[waitforp].skip >> 1;
                    waitfor[waitforp].what='f';waitfor[waitforp].line=sline;
                    break;
                }
                if (prm==CMD_IF || prm==CMD_IFEQ || prm==CMD_IFPL || prm==CMD_IFMI || prm==CMD_ELSIF) { // .if
                    uint8_t skwait = waitfor[waitforp].skip;
                    if (prm==CMD_ELSIF) {
                        if (waitfor[waitforp].what!='e') {err_msg(ERROR______EXPECTED,".IF"); break;}
                    } else new_waitfor('e');
                    waitfor[waitforp].line=sline;
                    if (((skwait==1) && prm!=CMD_ELSIF) || ((skwait==2) && prm==CMD_ELSIF)) {
                        if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                        if (!get_val(&val, T_NONE, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                        eval_finish();
                        if (val.type == T_NONE) err_msg(ERROR___NOT_DEFINED,"argument used for condition");
                    } else val.type=T_NONE;
                    switch (prm) {
                    case CMD_ELSIF:
                        if (((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && val.u.num.val) || (val.type == T_STR && val.u.str.len)) waitfor[waitforp].skip=waitfor[waitforp].skip >> 1; else
                            waitfor[waitforp].skip=waitfor[waitforp].skip & 2;
                        break;
                    case CMD_IF:
                        if (((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && val.u.num.val) || (val.type == T_STR && val.u.str.len)) waitfor[waitforp].skip=waitfor[waitforp-1].skip & 1; else
                            waitfor[waitforp].skip=(waitfor[waitforp-1].skip & 1) << 1;
                        break;
                    case CMD_IFEQ:
                        if (((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && !val.u.num.val) || (val.type == T_STR && !val.u.str.len)) waitfor[waitforp].skip=waitfor[waitforp-1].skip & 1; else
                            waitfor[waitforp].skip=(waitfor[waitforp-1].skip & 1) << 1;
                        break;
                    case CMD_IFPL:
                        if (((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && (arguments.tasmcomp ? (~val.u.num.val & 0x8000) : (val.u.num.val>=0))) || (val.type == T_STR && val.u.str.len)) waitfor[waitforp].skip=waitfor[waitforp-1].skip & 1; else
                            waitfor[waitforp].skip=(waitfor[waitforp-1].skip & 1) << 1;
                        break;
                    case CMD_IFMI:
                        if ((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && (arguments.tasmcomp ? (val.u.num.val & 0x8000) : (val.u.num.val < 0))) waitfor[waitforp].skip=waitfor[waitforp-1].skip & 1; else
                            waitfor[waitforp].skip=(waitfor[waitforp-1].skip & 1) << 1;
                        break;
                    }
                    break;
                }
                if (prm==CMD_ENDM) { // .endm
                    if (waitfor[waitforp].what=='m') {
                        waitforp--;
                    } else if (waitfor[waitforp].what=='M') {
                        waitforp--; nobreak=0;
                    } else err_msg(ERROR______EXPECTED,".MACRO or .SEGMENT");
                    break;
                }
                if (prm==CMD_NEXT) { // .next
                    if (waitfor[waitforp].what=='n') {
                        waitforp--;
                    } else if (waitfor[waitforp].what=='N') {
                        waitforp--; nobreak=0;
                    } else err_msg(ERROR______EXPECTED,".FOR or .REPT");
                    break;
                }
                if (prm==CMD_PEND) { //.pend
                    if (waitfor[waitforp].what!='r') {err_msg(ERROR______EXPECTED,".PROC"); break;}
                    if (waitfor[waitforp].skip & 1) {
                        if (current_context->parent) {
                            current_context = current_context->parent;
                        } else err_msg(ERROR______EXPECTED,".proc");
			lastl=LIST_NONE;
			if (waitfor[waitforp].label) set_size(waitfor[waitforp].label, current_section->address - waitfor[waitforp].addr);
                    }
                    waitforp--;
                    break;
                }
                if (prm==CMD_ENDS) { // .ends
                    if (waitfor[waitforp].what=='s') {
                        waitforp--;
                    } else if (waitfor[waitforp].what=='S') {
                        waitforp--; nobreak=0;
                    } else err_msg(ERROR______EXPECTED,".STRUCT"); break;
                    break;
                }
                if (prm==CMD_SEND) { // .send
                    if (waitfor[waitforp].what=='t') {
                        waitforp--;get_ident2(labelname);
                    } else if (waitfor[waitforp].what=='T') {
                        ignore();epoint=lpoint;
                        if (!get_ident2(labelname)) {
                            if (strcmp(labelname, current_section->name)) err_msg2(ERROR______EXPECTED,current_section->name,epoint);
                        }
			if (waitfor[waitforp].label) set_size(waitfor[waitforp].label, current_section->address - waitfor[waitforp].addr);
                        current_section = waitfor[waitforp].section;
                        memjmp(current_section->address);
                        waitforp--;
                    } else err_msg(ERROR______EXPECTED,".SECTION or .DSECTION");
                    break;
                }
                if (prm==CMD_ENDU) { // .endu
                    if (waitfor[waitforp].what=='u') {
                        waitforp--; current_section->l_address = l_unionend; if (current_section->address != unionend) {current_section->address = unionend; memjmp(current_section->address);}
                    } else if (waitfor[waitforp].what=='U') {
                        waitforp--; nobreak=0; current_section->l_address = l_unionend; if (current_section->address != unionend) {current_section->address = unionend; memjmp(current_section->address);}
                    } else err_msg(ERROR______EXPECTED,".UNION"); break;
                    break;
                }
                if (prm==CMD_ENDP) { // .endp
                    if (waitfor[waitforp].what=='p') {
                        waitforp--;
                    } else if (waitfor[waitforp].what=='P') {
			if ((current_section->l_address & ~0xff) != (waitfor[waitforp].laddr & ~0xff) && fixeddig) {
				err_msg(ERROR____PAGE_ERROR,(const char *)current_section->l_address);
			}
			if (waitfor[waitforp].label) set_size(waitfor[waitforp].label, current_section->address - waitfor[waitforp].addr);
                        waitforp--;
                    } else err_msg(ERROR______EXPECTED,".PAGE"); break;
                    break;
                }
                if (prm==CMD_HERE) { // .here
                    if (waitfor[waitforp].what=='l') {
                        waitforp--;
                    } else if (waitfor[waitforp].what=='L') {
			current_section->l_address = current_section->address + waitfor[waitforp].laddr;
			if (waitfor[waitforp].label) set_size(waitfor[waitforp].label, current_section->address - waitfor[waitforp].addr);
                        waitforp--;
                    } else err_msg(ERROR______EXPECTED,".LOGICAL"); break;
                    break;
                }
                if (!(waitfor[waitforp].skip & 1)) {
                    switch (prm) {
                    case CMD_BLOCK: new_waitfor('b'); break;
                    case CMD_LOGICAL: new_waitfor('l'); break;
                    case CMD_PAGE: new_waitfor('p'); break;
                    case CMD_STRUCT: new_waitfor('s'); break;
                    case CMD_DSECTION:
                    case CMD_SECTION: new_waitfor('t'); break;
                    case CMD_MACRO:
                    case CMD_SEGMENT: new_waitfor('m'); break;
                    case CMD_FOR:
                    case CMD_REPT: new_waitfor('n'); break;
                    case CMD_COMMENT: new_waitfor('c'); break;
                    case CMD_PROC: new_waitfor('r'); break;
                    }
                    break;//skip things if needed
                }
                if (prm<=CMD_DWORD || prm==CMD_BINARY) { // .byte .text .rta .char .int .word .long
                    size_t ptextaddr=mem.p;
                    unsigned int omemp = memblocks.p;
                    size_t uninit = 0;

                    if (prm<CMD_BYTE) {    // .text .ptext .shift .shift2 .null
                        int16_t ch2=-1;
                        int large=0;
                        if (newlabel) newlabel->esize = 1;
                        if (prm==CMD_PTEXT) ch2=0;
                        if (!get_exp(&w,0)) goto breakerr;
                        while (get_val(&val, T_NONE, &epoint)) {
                            if (val.type != T_STR || val.u.str.len) {
                                size_t i = 0;
                                do {
                                    if (ch2 >= 0) {
                                        if (uninit) { memskip(uninit); uninit = 0; }
                                        pokeb(ch2);
                                    }

                                    switch (val.type) {
                                    case T_GAP:ch2 = -1; uninit++; break;
                                    case T_STR:
                                        ch2 = petascii(&i, &val);
                                        if (ch2 > 255) i = val.u.str.len;
                                        break;
                                    case T_NUM:
                                    case T_UINT:
                                    case T_SINT:
                                        if (val.type != T_NUM || val.u.num.len > 1) {
                                            if ((uval_t)val.u.num.val & ~(uval_t)0xff) large=epoint;
                                        }
                                        ch2 = (uint8_t)val.u.num.val;
                                        break;
                                    default: err_msg_wrong_type(val.type, epoint);
                                    case T_NONE: ch2 = fixeddig = 0;
                                    }

                                    if (prm==CMD_SHIFT || prm==CMD_SHIFTL) {
                                        if (ch2>=0x80) large=epoint;
                                        if (prm==CMD_SHIFTL) ch2<<=1;
                                    } else if (prm==CMD_NULL && !ch2 && val.type != T_NONE) large=epoint;
                                } while (val.type == T_STR && val.u.str.len > i);
                            }
                        }
                        if (uninit) memskip(uninit);
                        if (ch2>=0) {
                            if (prm==CMD_SHIFT) ch2|=0x80;
                            if (prm==CMD_SHIFTL) ch2|=0x01;
                            pokeb(ch2);
                        }
                        if (prm==CMD_NULL) pokeb(0);
                        if (prm==CMD_PTEXT) {
                            if (mem.p-ptextaddr>0x100) large=epoint;

                            if (fixeddig && current_section->dooutput) mem.data[ptextaddr]=mem.p-ptextaddr-1;
                        }
                        if (large) err_msg2(ERROR_CONSTNT_LARGE, NULL, large);
                    } else if (prm<=CMD_DWORD) { // .word .int .rta .long
                        uint32_t ch2;
                        int large=0;
                        if (newlabel) newlabel->esize = 1 + (prm>=CMD_RTA) + (prm>=CMD_LONG) + (prm >= CMD_DINT);
                        if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                        while (get_val(&val, T_NONE, &epoint)) {
                            switch (val.type) {
                            case T_GAP:uninit += 1 + (prm>=CMD_RTA) + (prm>=CMD_LONG) + (prm >= CMD_DINT);continue;
                            case T_STR:
                                if (str_to_num(&val)) large = epoint;
                            case T_NUM:
                            case T_SINT:
                            case T_UINT:
                                switch (prm) {
                                case CMD_CHAR: if ((val.u.num.len > 1 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0x7f) && (~(uval_t)val.u.num.val & ~(uval_t)0x7f)) large=epoint;break;
                                case CMD_BYTE: if ((val.u.num.len > 1 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0xff)) large=epoint; break;
                                case CMD_INT: if ((val.u.num.len > 2 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0x7fff) && (~(uval_t)val.u.num.val & ~(uval_t)0x7fff)) large=epoint;break;
                                case CMD_LONG: if ((val.u.num.len > 3 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0xffffff)) large=epoint; break;
                                case CMD_DINT: if ((val.u.num.len > 4 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0x7fffffff) && (~(uval_t)val.u.num.val & ~(uval_t)0x7fffffff)) large=epoint;break;
                                case CMD_DWORD: if ((val.u.num.len > 4 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0xffffffff)) large=epoint; break;
                                default: if ((val.u.num.len > 2 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0xffff)) large=epoint;
                                }
                                ch2 = (uval_t)val.u.num.val;
                                break;
                            default: err_msg_wrong_type(val.type, epoint);
                            case T_NONE: ch2 = fixeddig = 0;
                            }
                            if (prm==CMD_RTA) ch2--;

                            if (uninit) {memskip(uninit);uninit = 0;}
                            pokeb((uint8_t)ch2);
                            if (prm>=CMD_RTA) pokeb((uint8_t)(ch2>>8));
                            if (prm>=CMD_LONG) pokeb((uint8_t)(ch2>>16));
                            if (prm>=CMD_DINT) pokeb((uint8_t)(ch2>>24));
                        }
                        if (uninit) memskip(uninit);
                        if (large) err_msg2(ERROR_CONSTNT_LARGE, NULL, large);
                    } else if (prm==CMD_BINARY) { // .binary
                        long foffset = 0;
                        int nameok = 0;
                        address_t fsize = all_mem+1;
                        FILE* fil;
                        if (newlabel) newlabel->esize = 1;
                        if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                        if (!get_val(&val, T_NONE, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                        if (val.type == T_NONE) fixeddig = 0;
                        else {
                            if (val.type != T_STR) {err_msg_wrong_type(val.type, epoint);goto breakerr;}
                            if (get_path(&val, cfile->name)) goto breakerr;
                            nameok = 1;
                        }
                        if (get_val(&val, T_UINT, &epoint)) {
                            if (val.type == T_NONE) fixeddig = 0;
                            else {
                                if (val.u.num.val<0) {err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint); goto breakerr;}
                                foffset = val.u.num.val;
                            }
                            if (get_val(&val, T_UINT, &epoint)) {
                                if (val.type == T_NONE) fixeddig = 0;
                                else {
                                    if (val.u.num.val<0 || (address_t)val.u.num.val > fsize) err_msg2(ERROR_CONSTNT_LARGE,NULL, epoint);
                                    else fsize = val.u.num.val;
                                }
                            }
                        }
                        eval_finish();

                        if (nameok) {
                            if ((fil=fopen(path,"rb"))==NULL) {err_msg(ERROR_CANT_FINDFILE,path);goto breakerr;}
                            fseek(fil,foffset,SEEK_SET);
                            for (;fsize;fsize--) {
                                int st=getc(fil);
                                if (st == EOF) break;
                                pokeb((uint8_t)st);
                            }
                            if (ferror(fil)) err_msg(ERROR__READING_FILE,path);
                            fclose(fil);
                        }
                    }

                    if (listing && flist) {
                        unsigned int i, lcol;
                        address_t myaddr;
                        size_t len;
                        for (;omemp <= memblocks.p;omemp++) {
                            lcol=arguments.source?25:49;
                            if (omemp < memblocks.p) {
                                len = memblocks.data[omemp].len - (ptextaddr - memblocks.data[omemp].p);
                                myaddr = (memblocks.data[omemp].start + memblocks.data[omemp].len - len) & all_mem;
                            } else {
                                myaddr = memblocklaststart + (ptextaddr - memblocklastp);
                                len = mem.p - ptextaddr;
                                if (!len) {
                                    if (!llist) continue;
                                    if (omemp) myaddr = (memblocks.data[omemp-1].start + memblocks.data[omemp-1].len) & all_mem;
                                }
                            }
                            if (lastl!=LIST_DATA) {putc('\n',flist);lastl=LIST_DATA;}
                            if (current_section->dooutput) {
                                fprintf(flist,(all_mem==0xffff)?">%04" PRIaddress "\t":">%06" PRIaddress " ", myaddr);
                                while (len) {
                                    if (lcol==1) {
                                        if (arguments.source && llist) {
                                            putc('\t', flist);printllist(flist);
                                        } else putc('\n',flist);
                                        fprintf(flist,(all_mem==0xffff)?">%04" PRIaddress "\t":">%06" PRIaddress " ", myaddr);lcol=49;
                                    }
                                    fprintf(flist," %02x", mem.data[ptextaddr++]);
                                    myaddr = (myaddr + 1) & all_mem;

                                    lcol-=3;
                                    len--;
                                }
                            } else fprintf(flist,(all_mem==0xffff)?">%04" PRIaddress "\t":">%06" PRIaddress " ", current_section->address);

                            if (arguments.source && llist) {
                                for (i=0; i<lcol-1; i+=8) putc('\t',flist);
                                putc('\t', flist);printllist(flist);
                            } else putc('\n',flist);
                        }
                    }
                    break;
                }
                if (prm==CMD_OFFS) {   // .offs
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_SINT, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (val.type == T_NONE) fixeddig = 0;
                    else if (val.u.num.val) {
                        if (fixeddig && scpumode) {
                            if (((current_section->address + val.u.num.val)^current_section->address) & ~(address_t)0xffff) wrapwarn2=1;
                        }
                        if (structrecursion) {
                            if (val.u.num.val < 0) err_msg(ERROR___NOT_ALLOWED, ".OFFS");
                            else {
                                current_section->l_address+=val.u.num.val;
                                current_section->address+=val.u.num.val;
                            }
                        } else current_section->address+=val.u.num.val;
                        if (current_section->address & ~all_mem) {
                            if (fixeddig) wrapwarn=1;
                            current_section->address&=all_mem;
                        }
                        memjmp(current_section->address);
                    }
                    break;
                }
                if (prm==CMD_LOGICAL) { // .logical
                    new_waitfor('L');waitfor[waitforp].laddr = current_section->l_address - current_section->address;waitfor[waitforp].label=newlabel;waitfor[waitforp].addr = current_section->address;
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (structrecursion) err_msg(ERROR___NOT_ALLOWED, ".LOGICAL");
                    if (val.type == T_NONE) fixeddig = 0;
                    else {
                        if ((uval_t)val.u.num.val & ~(uval_t)all_mem) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                        else current_section->l_address=(uval_t)val.u.num.val;
                    }
                    newlabel = NULL;
                    break;
                }
                if (prm==CMD_AS) { // .as
                    longaccu=0;
                    break;
                }
                if (prm==CMD_AL) { // .al
                    longaccu=1;
                    break;
                }
                if (prm==CMD_XS) { // .xs
                    longindex=0;
                    break;
                }
                if (prm==CMD_XL) { // .xl
                    longindex=1;
                    break;
                }
                if (prm==CMD_ERROR) { // .error
                    err_msg(ERROR__USER_DEFINED,(char *)&pline[lpoint]);
                    goto breakerr;
                }
                if (prm==CMD_BLOCK) { // .block
		    new_waitfor('B');
                    sprintf(labelname, ".%" PRIxPTR ".%" PRIxline, (uintptr_t)star_tree, vline_64tass);
                    current_context=new_label(labelname, L_LABEL);
                    current_context->value.type = T_NONE;
                    break;
                }
                if (prm==CMD_BEND) { //.bend
                    if (waitfor[waitforp].what=='b') {
                        waitforp--;
                    } else if (waitfor[waitforp].what=='B') {
			if (waitfor[waitforp].label) set_size(waitfor[waitforp].label, current_section->address - waitfor[waitforp].addr);
			if (current_context->parent) current_context = current_context->parent;
			else err_msg(ERROR______EXPECTED,".block");
			waitforp--;
                    } else err_msg(ERROR______EXPECTED,".BLOCK"); break;
                    break;
                }
                if (prm==CMD_DATABANK) { // .databank
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (val.type == T_NONE) fixeddig = 0;
                    else {
                        if ((val.type != T_NUM || val.u.num.len > 1) && ((uval_t)val.u.num.val & ~(uval_t)0xff)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                        else databank=val.u.num.val;
                    }
                    break;
                }
                if (prm==CMD_DPAGE) { // .dpage
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (val.type == T_NONE) fixeddig = 0;
                    else {
                        if ((val.type != T_NUM || val.u.num.len > 2) && ((uval_t)val.u.num.val & ~(uval_t)0xffff)) err_msg2(ERROR_CONSTNT_LARGE,NULL, epoint);
                        else {
                            if (dtvmode) dpage=val.u.num.val & 0xff00;
                            else dpage=val.u.num.val;
                        }
                    }
                    break;
                }
                if (prm==CMD_FILL) { // .fill
                    address_t db = 0;
                    uint8_t ch;
                    if (newlabel) newlabel->esize = 1;
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    if (val.type == T_NONE) fixeddig = 0;
                    else {
                        db=val.u.num.val;
                        if (db>(all_mem+1)) {err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);goto breakerr;}
                    }
                    if (get_val(&val, T_UINT, &epoint)) {
                        if (val.type == T_NONE) ch = fixeddig = 0;
                        else {
                            if ((val.type != T_NUM || val.u.num.len > 1) && ((uval_t)val.u.num.val & ~(uval_t)0xff)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                            ch = (uint8_t)val.u.num.val;
                        }
                        while (db-->0) pokeb(ch);
                    } else memskip(db);
                    eval_finish();
                    break;
                }
                if (prm==CMD_ASSERT) { // .assert
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    if (val.type == T_NONE) {fixeddig = 0;current_section->provides=~(uval_t)0;}
                    else current_section->provides=val.u.num.val;
                    if (!get_val(&val, T_UINT, NULL)) {err_msg(ERROR______EXPECTED,","); goto breakerr;}
                    if (val.type == T_NONE) fixeddig = current_section->requires = 0;
                    else current_section->requires=val.u.num.val;
                    if (!get_val(&val, T_UINT, NULL)) {err_msg(ERROR______EXPECTED,","); goto breakerr;}
                    if (val.type == T_NONE) fixeddig = current_section->conflicts = 0;
                    else current_section->conflicts=val.u.num.val;
                    eval_finish();
                    break;
                }
                if (prm==CMD_CHECK) { // .check
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    if (val.type == T_NONE) fixeddig = 0;
                    else if ((val.u.num.val & current_section->provides) ^ val.u.num.val) err_msg(ERROR_REQUIREMENTS_,".CHECK");
                    if (!get_val(&val, T_UINT, NULL)) {err_msg(ERROR______EXPECTED,","); goto breakerr;}
                    if (val.type == T_NONE) fixeddig = 0;
                    else if (val.u.num.val & current_section->provides) err_msg(ERROR______CONFLICT,".CHECK");
                    eval_finish();
                    break;
                }
                if (prm==CMD_WARN) { // .warn
                    err_msg(ERROR_WUSER_DEFINED,(char *)&pline[lpoint]);
                    goto breakerr;
                }
                if (prm==CMD_ENC) { // .enc
                    ignore();epoint=lpoint;
                    if (get_ident2(labelname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    actual_encoding = new_encoding(labelname);
                    break;
                }
                if (prm==CMD_CDEF) { // .cdef
                    struct trans_s tmp, *t;
                    struct encoding_s *old = actual_encoding;
                    uint32_t ch;
                    int rc;
                    actual_encoding = NULL;
                    rc = get_exp(&w,0);
                    actual_encoding = old;
                    if (!rc) goto breakerr; //ellenorizve.
                    for (;;) {
                        int endok = 0;

                        actual_encoding = NULL;
                        rc = get_val(&val, T_NONE, &epoint);
                        actual_encoding = old;
                        if (!rc) break;

                        switch (val.type) {
                        case T_NONE: err_msg(ERROR___NOT_DEFINED,"argument used for condition");goto breakerr;
                        case T_NUM: if (val.u.num.len <= 3) {
                            tmp.start = val.u.num.val;
                            break;
                        }
                        case T_UINT:
                        case T_SINT:
                             if ((uval_t)val.u.num.val & ~(uval_t)0xffffff) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                             tmp.start = val.u.num.val;
                             break;
                        case T_STR:
                             if (!val.u.str.len) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                             if (val.u.str.len) {
                                 unsigned int l;
                                 ch = val.u.str.data[0];
                                 if (ch & 0x80) l = utf8in(val.u.str.data, &ch); else l = 1;
                                 val.u.str.len -= l; val.u.str.data += l;
                                 tmp.start = ch;
                             }
                             if (val.u.str.len) {
                                 unsigned int l;
                                 ch = val.u.str.data[0];
                                 if (ch & 0x80) l = utf8in(val.u.str.data, &ch); else l = 1;
                                 val.u.str.len -= l; val.u.str.data += l;
                                 if (tmp.start > ch) {
                                     tmp.end = tmp.start;
                                     tmp.start = ch;
                                 } else tmp.end = ch;
                                 endok = 1;
                             }
                             if (val.u.str.len) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                             break;
                        default:
                            err_msg_wrong_type(val.type, epoint);
                            goto breakerr;
                        }
                        if (!endok) {
                            actual_encoding = NULL;
                            rc = get_val(&val, T_UINT, &epoint);
                            actual_encoding = old;
                            if (!rc) {err_msg(ERROR______EXPECTED,","); goto breakerr;}
                            if (val.type == T_NONE) {err_msg(ERROR___NOT_DEFINED,"argument used for condition");goto breakerr;}
                            if ((val.type != T_NUM || val.u.num.len > 3) && ((uval_t)val.u.num.val & ~(uval_t)0xffffff)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                            if (tmp.start > (uint32_t)val.u.num.val) {
                                tmp.end = tmp.start;
                                tmp.start = val.u.num.val;
                            } else tmp.end = val.u.num.val;
                        }
                        actual_encoding = NULL;
                        rc = get_val(&val, T_UINT, &epoint);
                        actual_encoding = old;
                        if (!rc) {err_msg(ERROR______EXPECTED,","); goto breakerr;}
                        if (val.type == T_NONE) {err_msg(ERROR___NOT_DEFINED,"argument used for condition");goto breakerr;}
                        if ((val.type != T_NUM || val.u.num.len > 1) && ((uval_t)val.u.num.val & ~(uval_t)0xff)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                        tmp.offset = val.u.num.val;
                        t = new_trans(&tmp, actual_encoding);
                        if (t->start != tmp.start || t->end != tmp.end || t->offset != tmp.offset) {
                            err_msg(ERROR_DOUBLE_DEFINE,"range"); goto breakerr;
                        }
                    }
                    eval_finish();
                    break;
                }
                if (prm==CMD_EDEF) { // .edef
                    struct escape_s *t;
                    struct encoding_s *old = actual_encoding;
                    int rc;
                    actual_encoding = NULL;
                    rc = get_exp(&w,0);
                    actual_encoding = old;
                    if (!rc) goto breakerr; //ellenorizve.
                    for (;;) {
                        char expr[linelength];

                        actual_encoding = NULL;
                        rc = get_val(&val, T_NONE, &epoint);
                        actual_encoding = old;
                        if (!rc) break;

                        switch (val.type) {
                        case T_NONE: err_msg(ERROR___NOT_DEFINED,"argument used for condition");goto breakerr;
                        case T_STR:
                             if (!val.u.str.len) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                             memcpy(expr, val.u.str.data, val.u.str.len);
                             expr[val.u.str.len]=0;
                             break;
                        default:
                            err_msg_wrong_type(val.type, epoint);
                            goto breakerr;
                        }
                        actual_encoding = NULL;
                        rc = get_val(&val, T_UINT, &epoint);
                        actual_encoding = old;
                        if (!rc) {err_msg(ERROR______EXPECTED,","); goto breakerr;}
                        if (val.type == T_NONE) {err_msg(ERROR___NOT_DEFINED,"argument used for condition");goto breakerr;}
                        if ((val.type != T_NUM || val.u.num.len > 1) && ((uval_t)val.u.num.val & ~(uval_t)0xff)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                        t = new_escape(expr, (uint8_t)val.u.num.val, actual_encoding);
                        if (t->code != (uint8_t)val.u.num.val) {
                            err_msg(ERROR_DOUBLE_DEFINE,"escape"); goto breakerr;
                        }
                    }
                    eval_finish();
                    break;
                }
                if (prm==CMD_CPU) { // .cpu
                    int def;
                    if (get_hack()) goto breakerr;
                    def=arguments.cpumode;
                    if (!strcmp(path,"6502")) def=OPCODES_6502;
                    else if (!strcasecmp(path,"65c02")) def=OPCODES_65C02;
                    else if (!strcasecmp(path,"6502i")) def=OPCODES_6502i;
                    else if (!strcmp(path,"65816")) def=OPCODES_65816;
                    else if (!strcasecmp(path,"65dtv02")) def=OPCODES_65DTV02;
                    else if (!strcasecmp(path,"65el02")) def=OPCODES_65EL02;
                    else if (strcasecmp(path,"default")) err_msg(ERROR___UNKNOWN_CPU,path);
                    set_cpumode(def);
                    break;
                }
                if (prm==CMD_CERROR || prm==CMD_CWARN) { // .cerror
                    if (!get_exp(&w, 1)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_NONE, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    if (here()==',') {
                        lpoint++;ignore();
                    }
                    if (((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && val.u.num.val) || (val.type == T_STR && val.u.str.len)) err_msg((prm==CMD_CERROR)?ERROR__USER_DEFINED:ERROR_WUSER_DEFINED,(char *)&pline[lpoint]);
                    goto breakerr;
                }
                if (prm==CMD_REPT) { // .rept
                    int cnt;
                    new_waitfor('n');waitfor[waitforp].skip=0;
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (val.type == T_NONE) err_msg2(ERROR___NOT_DEFINED, "repeat count", epoint);
                    cnt = 0;
                    if (val.type != T_NONE) {
                        if (cnt<val.u.num.val) {
                            size_t pos = cfile->p;
                            line_t lin = sline;
                            struct star_s *s = new_star(vline_64tass);
                            struct avltree *stree_old = star_tree;
                            line_t ovline = vline_64tass;

                            waitforp--;
                            if (labelexists && s->addr != star) fixeddig=0;
                            s->addr = star;
                            star_tree = &s->tree;vline_64tass=0;
                            for (; cnt<val.u.num.val; cnt++) {
                                sline=lin;cfile->p=pos;
                                new_waitfor('N');waitfor[waitforp].skip=1;
                                compile();
                            }
                            star_tree = stree_old; vline_64tass = ovline;
                        }
                    }
                    break;
                }
                if (prm==CMD_ALIGN) { // .align
                    int align = 1, fill=-1;
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_UINT, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    if (structrecursion) err_msg(ERROR___NOT_ALLOWED, ".ALIGN");
                    if (val.type == T_NONE) fixeddig = 0;
                    else {
                        if (!val.u.num.val || ((uval_t)val.u.num.val & ~(uval_t)all_mem)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                        else align = val.u.num.val;
                    }
                    if (get_val(&val, T_NUM, &epoint)) {
                        if (val.type == T_NONE) fixeddig = fill = 0;
                        else {
                            if ((val.u.num.len > 1 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0xff)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                            fill = (uint8_t)val.u.num.val;
                        }
                    }
                    eval_finish();
                    if (align>1 && (current_section->l_address % align)) {
                        if (fill>0)
                            while (current_section->l_address % align) pokeb((uint8_t)fill);
                        else {
                            align-=current_section->l_address % align;
                            if (align) memskip(align);
                        }
                    }
                    break;
                }
                if (prm==CMD_EOR) {   // .eor
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_NUM, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (val.type == T_NONE) fixeddig = outputeor = 0;
                    else {
                        if ((val.u.num.len > 1 || val.type != T_NUM) && ((uval_t)val.u.num.val & ~(uval_t)0xff)) err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);
                        else outputeor = val.u.num.val;
                    }
                    break;
                }
                if (prm==CMD_END) {
                    nobreak=0;
                    break;
                }
                if (prm==CMD_PRON) {
                    listing = (flist != NULL);
                    break;
                }
                if (prm==CMD_PROFF) {
                    listing = 0;
                    break;
                }
                if (prm==CMD_SHOWMAC || prm==CMD_HIDEMAC) {
                    err_msg(ERROR_DIRECTIVE_IGN,NULL);
                    break;
                }
                if (prm==CMD_COMMENT) { // .comment
                    new_waitfor('c');waitfor[waitforp].skip=0;
                    break;
                }
                if (prm==CMD_INCLUDE) { // .include
                    struct file_s *f;
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_NONE, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (val.type == T_NONE) err_msg2(ERROR___NOT_DEFINED,"argument used", epoint);
                    else {
                        if (val.type != T_STR) {err_msg_wrong_type(val.type, epoint);goto breakerr;}
                        if (get_path(&val, cfile->name)) goto breakerr;

                        if (listing && flist) {
                            fprintf(flist,"\n;******  Processing file \"%s\"\n",path);
                            lastl=LIST_NONE;
                        }
                        f = cfile;
                        cfile = openfile(path);
                        if (cfile->open>1) {
                            err_msg(ERROR_FILERECURSION,NULL);
                        } else {
                            line_t lin = sline;
                            line_t vlin = vline_64tass;
                            struct avltree *stree_old = star_tree;
                            uint32_t old_backr = backr, old_forwr = forwr;

                            enterfile(cfile->name,sline);
                            sline = vline_64tass = 0; cfile->p=0;
                            star_tree = &cfile->star;
                            backr = forwr = 0;
                            reffile=cfile->uid;
                            compile();
                            sline = lin; vline_64tass = vlin;
                            star_tree = stree_old;
                            backr = old_backr; forwr = old_forwr;
                            exitfile();
                        }
                        closefile(cfile);cfile = f;
                        reffile=cfile->uid;
                        if (listing && flist) {
                            fprintf(flist,"\n;******  Return to file \"%s\"\n",cfile->name);
                            lastl=LIST_NONE;
                        }
                    }
                    break;
                }
                if (prm==CMD_FOR) { // .for
                    size_t pos, xpos;
                    line_t lin, xlin;
                    int apoint, bpoint = -1;
                    uint8_t expr[linelength];
                    struct label_s *var;
                    struct star_s *s;
                    struct avltree *stree_old;
                    line_t ovline;

                    new_waitfor('n');waitfor[waitforp].skip=0;
                    if (strlen((char *)pline)>=linelength) {err_msg(ERROR_LINE_TOO_LONG,NULL);goto breakerr;}
                    if ((wht=what(&prm))==WHAT_EXPRESSION && prm==1) { //label
                        if (get_ident(labelname)) goto breakerr;
                        ignore();if (here()!='=') {err_msg(ERROR______EXPECTED,"=");goto breakerr;}
                        lpoint++;
                        if (!get_exp(&w,1)) goto breakerr; //ellenorizve.
                        if (!get_val(&val, T_NONE, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                        var=new_label(labelname, L_VAR);
                        if (labelexists) {
                            if (var->type != L_VAR) err_msg(ERROR_DOUBLE_DEFINE,labelname);
                            else {
                                var->requires=current_section->requires;
                                var->conflicts=current_section->conflicts;
                                var_assign(var, &val, fixeddig);
                            }
                        } else {
                            var->requires=current_section->requires;
                            var->conflicts=current_section->conflicts;
                            var->pass=pass;
                            var->value.type=T_NONE;
                            var_assign(var, &val, fixeddig);
                            if (val.type == T_NONE) err_msg(ERROR___NOT_DEFINED,"argument used");
                        }
                        wht=what(&prm);
                    }
                    if (wht==WHAT_S || wht==WHAT_Y || wht==WHAT_X || wht==WHAT_R) lpoint--; else
                        if (wht!=WHAT_COMA) {err_msg(ERROR______EXPECTED,","); goto breakerr;}

                    s = new_star(vline_64tass); stree_old = star_tree; ovline = vline_64tass;
                    if (labelexists && s->addr != star) fixeddig=0;
                    s->addr = star;
                    star_tree = &s->tree;vline_64tass=0;
                    xlin=lin=sline; xpos=pos=cfile->p; apoint=lpoint;
                    strcpy((char *)expr, (char *)pline);var = NULL;
                    for (;;) {
                        lpoint=apoint;
                        if (!get_exp(&w,1)) break; //ellenorizve.
                        if (!get_val(&val, T_NONE, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
                        if (val.type == T_NONE) {err_msg(ERROR___NOT_DEFINED,"argument used in condition");break;}
                        if (((val.type == T_UINT || val.type == T_SINT || val.type == T_NUM) && !val.u.num.val) || (val.type == T_STR && !val.u.str.len)) break;
                        if (bpoint < 0) {
                            ignore();if (here()!=',') {err_msg(ERROR______EXPECTED,","); break;}
                            lpoint++;
                            if (get_ident(labelname)) break;
                            ignore();if (here()!='=') {err_msg(ERROR______EXPECTED,"="); break;}
                            lpoint++;
                            ignore();
                            if (!here() || here()==';') bpoint = 0;
                            else {
                                var=new_label(labelname, L_VAR);
                                if (labelexists) {
                                    if (var->type != L_VAR) {
                                        err_msg(ERROR_DOUBLE_DEFINE,labelname);
                                        break;
                                    }
                                    var->requires=current_section->requires;
                                    var->conflicts=current_section->conflicts;
                                } else {
                                    var->requires=current_section->requires;
                                    var->conflicts=current_section->conflicts;
                                    var->upass=var->pass=pass;
                                    var->value.type=T_NONE;
                                }
                                bpoint=lpoint;
                            }
                        }
                        new_waitfor('N');waitfor[waitforp].skip=1;
                        compile();
                        xpos = cfile->p; xlin= sline;
                        pline = expr;
                        sline=lin;cfile->p=pos;
                        if (bpoint) {
                            lpoint=bpoint;
                            if (!get_exp(&w,1)) break; //ellenorizve.
                            if (!get_val(&val, T_NONE, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); break;}
                            var_assign(var, &val, fixeddig);
                        }
                    }
                    if (pos!=xpos || lin!=xlin) waitforp--;
                    sline=xlin;cfile->p=xpos;
                    star_tree = stree_old; vline_64tass = ovline;
                    goto breakerr;
                }
                if (prm==CMD_PAGE) { // .page
                    new_waitfor('P');waitfor[waitforp].addr = current_section->address;waitfor[waitforp].laddr = current_section->l_address;waitfor[waitforp].label=newlabel;
                    newlabel=NULL;
                    break;
                }
                if (prm==CMD_OPTION) { // .option
                    get_ident(labelname);
                    ignore();if (here()!='=') {err_msg(ERROR______EXPECTED,"="); goto breakerr;}
                    lpoint++;
                    if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                    if (!get_val(&val, T_NONE, &epoint)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    eval_finish();
                    if (val.type == T_NONE) {err_msg2(ERROR___NOT_DEFINED,"argument used for option", epoint);goto breakerr;}
                    if (!strcasecmp(labelname,"allow_branch_across_page")) allowslowbranch=(((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && val.u.num.val) || (val.type == T_STR && val.u.str.len));
                    else if (!strcasecmp(labelname,"auto_longbranch_as_jmp")) longbranchasjmp=(((val.type == T_SINT || val.type == T_UINT || val.type == T_NUM) && val.u.num.val) || (val.type == T_STR && val.u.str.len));
                    else err_msg(ERROR_UNKNOWN_OPTIO,labelname);
                    break;
                }
                if (prm==CMD_GOTO) { // .goto
                    struct jump_s *tmp2;
                    int noerr = 1;
                    get_ident(labelname);
                    tmp2 = find_jump(labelname);
                    if (tmp2 && tmp2->file == cfile && tmp2->parent == current_context) {
                        uint8_t oldwaitforp = waitforp;
                        while (tmp2->waitforp < waitforp) {
                            line_t os = sline;
                            sline = waitfor[waitforp].line;
                            switch (waitfor[waitforp--].what) {
                            case 'M':
                            case 'm': err_msg(ERROR______EXPECTED,".ENDM"); noerr = 0; break;
                            case 'N':
                            case 'n': err_msg(ERROR______EXPECTED,".NEXT"); noerr = 0; break;
                            case 'r': err_msg(ERROR______EXPECTED,".PEND"); noerr = 0; break;
			    case 'B':
                            case 'b': err_msg(ERROR______EXPECTED,".BEND"); noerr = 0; break;
                            case 'S':
                            case 's': err_msg(ERROR______EXPECTED,".ENDS"); noerr = 0; break;
                            case 'T':
                            case 't': err_msg(ERROR______EXPECTED,".SEND"); noerr = 0; break;
                            case 'U':
                            case 'u': err_msg(ERROR______EXPECTED,".ENDU"); noerr = 0; break;
			    case 'P':
                            case 'p': err_msg(ERROR______EXPECTED,".ENDP"); noerr = 0; break;
                            case 'L':
                            case 'l': err_msg(ERROR______EXPECTED,".HERE"); noerr = 0; break;
                            }
                            sline = os;
                        }
                        if (noerr) {
                            sline = tmp2->sline;
                            cfile->p = tmp2->p;
                        } else waitforp = oldwaitforp;
                    } else err_msg(ERROR___NOT_DEFINED,labelname);
                    break;
                }
                if (prm==CMD_MACRO || prm==CMD_SEGMENT) {
                    new_waitfor('m');waitfor[waitforp].skip=0;
                    err_msg(ERROR___NOT_DEFINED,"");
                    break;
                }
                if (prm==CMD_PROC) {
                    new_waitfor('r');waitfor[waitforp].skip=0;waitfor[waitforp].addr=current_section->address;
                    err_msg(ERROR___NOT_DEFINED,"");
                    break;
                }
                if (prm==CMD_STRUCT) {
                    int old_unionmode = unionmode;
                    unionmode = 0;
                    new_waitfor('s');waitfor[waitforp].skip=0;
                    structrecursion++;
                    if (structrecursion<100) {
                        waitforp--;
                        new_waitfor('S');waitfor[waitforp].skip=1;
                        compile();
                    } else err_msg(ERROR__MACRECURSION,"!!!!");
                    structrecursion--;
                    unionmode = old_unionmode;
                    break;
                }
                if (prm==CMD_UNION) {
                    int old_unionmode = unionmode;
                    address_t old_unionstart = unionstart, old_unionend = unionend;
                    address_t old_l_unionstart = l_unionstart, old_l_unionend = l_unionend;
                    unionmode = 1;
                    unionstart = unionend = current_section->address;
                    l_unionstart = l_unionend = current_section->l_address;
                    new_waitfor('u');waitfor[waitforp].skip=0;
                    structrecursion++;
                    if (structrecursion<100) {
                        waitforp--;
                        new_waitfor('U');waitfor[waitforp].skip=1;
                        compile();
                    } else err_msg(ERROR__MACRECURSION,"!!!!");
                    structrecursion--;
                    unionmode = old_unionmode;
                    unionstart = old_unionstart; unionend = old_unionend;
                    l_unionstart = old_l_unionstart; l_unionend = old_l_unionend;
                    break;
                }
                if (prm==CMD_DSTRUCT) {
                    int old_unionmode = unionmode;
                    unionmode = 0;
                    ignore();epoint=lpoint;
                    if (get_ident2(labelname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    if (!(tmp2=find_macro(labelname)) || tmp2->type!=CMD_STRUCT) {err_msg2(ERROR___NOT_DEFINED,labelname,epoint); goto breakerr;}
                    structrecursion++;
                    macro_recurse('S',tmp2);
                    structrecursion--;
                    unionmode = old_unionmode;
                    break;
                }
                if (prm==CMD_DUNION) {
                    int old_unionmode = unionmode;
                    address_t old_unionstart = unionstart, old_unionend = unionend;
                    unionmode = 1;
                    unionstart = unionend = current_section->address;
                    ignore();epoint=lpoint;
                    if (get_ident2(labelname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    if (!(tmp2=find_macro(labelname)) || tmp2->type!=CMD_UNION) {err_msg2(ERROR___NOT_DEFINED,labelname,epoint); goto breakerr;}
                    structrecursion++;
                    macro_recurse('U',tmp2);
                    structrecursion--;
                    unionmode = old_unionmode;
                    unionstart = old_unionstart; unionend = old_unionend;
                    break;
                }
                if (prm==CMD_DSECTION) {
                    struct section_s *tmp2;
                    new_waitfor('t');
                    if (structrecursion) err_msg(ERROR___NOT_ALLOWED, ".DSECTION");
                    ignore();epoint=lpoint;
                    if (get_ident2(labelname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    tmp2=new_section(labelname);
                    if (tmp2->declared && pass == 1) err_msg2(ERROR_DOUBLE_DEFINE,labelname,epoint);
                    else {
                        address_t t, t2;
                        waitfor[waitforp].what='T';waitfor[waitforp].section=current_section;
                        if (!tmp2->declared) {
                            tmp2->r_start = tmp2->r_address = current_section->address;
                            tmp2->r_l_start = tmp2->r_l_address = current_section->l_address;
                            if (!labelexists) {
                                tmp2->start = tmp2->address = current_section->address;
                                tmp2->l_start = tmp2->l_address = current_section->l_address;
                            } else {
                                tmp2->address += current_section->address;
                                tmp2->start += current_section->address;
                                tmp2->l_address += current_section->l_address;
                                tmp2->l_start += current_section->l_address;
                            }
                            tmp2->pass = pass;
                            fixeddig = 0;
                            tmp2->declared = 1;
                        }
                        tmp2->provides=~(uval_t)0;tmp2->requires=tmp2->conflicts=0;
                        if (tmp2->pass == pass) {
                            t = tmp2->r_address - tmp2->r_start;
                            t2 = tmp2->address - tmp2->start;
                            if (newlabel) set_size(newlabel, t2 + t);
                            tmp2->start = current_section->address;
                            current_section->address += t2;
                            tmp2->r_start = tmp2->r_address = current_section->address;
                        } else {
                            t = tmp2->address - tmp2->start;
                            if (newlabel) set_size(newlabel, t);
                            tmp2->start = tmp2->r_start = tmp2->address = tmp2->r_address = current_section->address;
                        }
                        current_section->address += t;
                        if (tmp2->pass == pass) {
                            t = tmp2->r_l_address - tmp2->r_l_start;
                            t2 = tmp2->l_address - tmp2->l_start;
                            tmp2->l_start = current_section->l_address;
                            current_section->l_address += t2;
                            tmp2->r_l_start = tmp2->r_l_address = current_section->l_address;
                        } else {
                            t = tmp2->l_address - tmp2->l_start;
                            tmp2->l_start = tmp2->r_l_start = tmp2->l_address = tmp2->r_l_address = current_section->l_address;
                        }
                        current_section->l_address += t;
                        current_section = tmp2;
                        tmp2->pass=pass;
                        memjmp(current_section->address);
                        newlabel=NULL;
                    }
                    break;
                }
                if (prm==CMD_SECTION) {
                    struct section_s *tmp;
                    char sectionname[linelength];
                    new_waitfor('t');waitfor[waitforp].section=current_section;
                    if (structrecursion) err_msg(ERROR___NOT_ALLOWED, ".SECTION");
                    ignore();epoint=lpoint;
                    if (get_ident2(sectionname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    tmp=find_new_section(sectionname);
                    if (!tmp->declared) {
                        if (!labelexists) {
                            tmp->start = tmp->address = tmp->r_address = 0;
                            tmp->l_start = tmp->l_address = tmp->r_l_address = 0;
                            fixeddig=0;
                        } else if (pass > 1) {
                            err_msg2(ERROR___NOT_DEFINED,sectionname,epoint); goto breakerr;
                        }
                    } else if (tmp->pass != pass) {
                        tmp->r_address = tmp->address;
                        tmp->address = tmp->start;
                        tmp->r_l_address = tmp->l_address;
                        tmp->l_address = tmp->l_start;
                    }
                    tmp->pass = pass;
                    waitfor[waitforp].what = 'T';
                    current_section = tmp;
                    memjmp(current_section->address);
                    newlabel = NULL;
                    break;
                }
            }
        case WHAT_HASHMARK:if (waitfor[waitforp].skip & 1) //skip things if needed
            {                   //macro stuff
                struct label_s *old_context;

                if (get_ident2(labelname)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                if (!(tmp2=find_macro(labelname))) {err_msg(ERROR___NOT_DEFINED,labelname); goto breakerr;}
            as_macro:
                if (listing && flist && arguments.source && wasref) {
                    if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                    fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t\t\t\t\t%s\n":".%06" PRIaddress "\t\t\t\t\t%s\n",current_section->address,labelname2);
                }
                if (tmp2->type==CMD_MACRO) {
                    old_context = current_context;
                    if (newlabel) current_context=newlabel;
                    else {
                        sprintf(labelname, "#%" PRIxPTR "#%" PRIxline, (uintptr_t)star_tree, vline_64tass);
                        current_context=new_label(labelname, L_LABEL);
                        current_context->value.type = T_NONE;
                    }
                } else old_context = NULL;
                macro_recurse('M',tmp2);
                if (tmp2->type==CMD_MACRO) current_context = old_context;
                break;
            }
        case WHAT_EXPRESSION:
            if (waitfor[waitforp].skip & 1) {
                enum { AG_ZP, AG_B0, AG_PB, AG_BYTE, AG_DB3, AG_NONE } adrgen;

                get_ident2(labelname);
                if ((prm=lookup_opcode(labelname))>=0) {
                    enum opr_e opr;
                    int mnem, oldlpoint;
                    const uint8_t *cnmemonic; //current nmemonic
                    int_fast8_t ln;
                    uint8_t cod, longbranch;
                    uint32_t adr;
                    int d;
                as_opcode:

                    opr = 0;mnem = prm;
                    oldlpoint = lpoint;
                    cnmemonic = &opcode[prm*26];
                    ln = 0; cod = 0; longbranch = 0; adr = 0; adrgen = AG_NONE;

                    ignore();
                    if (!(wht=here()) || wht==';') {
                        opr=(cnmemonic[ADR_ACCU]==cnmemonic[ADR_IMPLIED])?ADR_ACCU:ADR_IMPLIED;w=ln=0;d=1;
                    }  //clc
                    // 1 Db
                    else if (lowcase(wht)=='a' && cnmemonic[ADR_ACCU]!=____ && (!pline[lpoint+1] || pline[lpoint+1]==';' || pline[lpoint+1]==0x20 || pline[lpoint+1]==0x09))
                    {
                        unsigned int opoint=lpoint;
                        lpoint++;ignore();
                        if (here() && here()!=';') {lpoint=opoint;goto nota;}
                        if (find_label("a")) err_msg(ERROR_A_USED_AS_LBL,NULL);
                        opr=ADR_ACCU;w=ln=0;d=1;// asl a
                    }
                    // 2 Db
                    else if (wht=='#') {
                        if ((cod=cnmemonic[(opr=ADR_IMMEDIATE)])==____ && prm) { // 0x69 hack
                            lpoint += strlen((char *)pline + lpoint);ln=w=d=1;
                        } else {
                            lpoint++;
                            if (!get_exp(&w,0)) goto breakerr; //ellenorizve.
                            if (!get_val(&val, T_NUM, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                            eval_finish();
                            d = (val.type != T_NONE);

                            ln=1;
                            if (cod==0xE0 || cod==0xC0 || cod==0xA2 || cod==0xA0) {// cpx cpy ldx ldy
                                if (longindex && scpumode) ln++;
                            }
                            else if (cod==0xF4) ln=2; //pea #$ffff
                            else if (cod!=0xC2 && cod!=0xE2) {//not sep rep=all accu
                                if (longaccu && scpumode) ln++;
                            }

                            if (w==3) w = ln - 1;
                            else if (w != ln - 1) w = 3;
                            if (val.type != T_NONE) {
                                if (!w && ln == 1 && ((val.u.num.len <= 1 && val.type == T_NUM) || !((uval_t)val.u.num.val & ~(uval_t)0xff))) adr = (uval_t)val.u.num.val;
                                else if (w == 1 && ln == 2 && ((val.u.num.len <= 2 && val.type == T_NUM) || !((uval_t)val.u.num.val & ~(uval_t)0xffff))) adr = (uval_t)val.u.num.val;
                                else w = 3;
                            }
                        }
                    }
                    // 3 Db
                    else {
                        int c;
                        if (whatis[wht]!=WHAT_EXPRESSION && whatis[wht]!=WHAT_CHAR && wht!='_' && wht!='*') {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                    nota:
                        if (!(c=get_exp(&w, cnmemonic[ADR_REL]==____ && cnmemonic[ADR_MOVE]==____))) goto breakerr; //ellenorizve.
                        if (!get_val(&val, T_UINT, NULL)) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                        if (val.type == T_NONE) d = fixeddig = 0;
                        else {adr = val.u.num.val;d = 1;}

                        switch (c) {
                        case 1:
                            switch (what(&prm)) {
                            case WHAT_X:
                                adrgen = AG_DB3; opr=ADR_ZP_X; // lda $ff,x lda $ffff,x lda $ffffff,x
                                break;
                            case WHAT_Y: // lda $ff,y lda $ffff,y lda $ffffff,y
                                if (w==3) {//auto length
                                    if (val.type != T_NONE) {
                                        if (cnmemonic[ADR_ZP_Y]!=____ && !((uval_t)val.u.num.val & ~(uval_t)0xffff) && (uint16_t)(val.u.num.val - dpage) < 0x100) {adr = (uint16_t)(val.u.num.val - dpage);w = 0;}
                                        else if (databank==((uval_t)val.u.num.val >> 16)) {adr = (uval_t)val.u.num.val;w = 1;}
                                    } else w=(cnmemonic[ADR_ADDR_Y]!=____);
                                } else if (val.type != T_NONE) {
                                        if (!w && !((uval_t)val.u.num.val & ~(uval_t)0xffff) && (uint16_t)(val.u.num.val - dpage) < 0x100) adr = (uint16_t)(val.u.num.val - dpage);
                                        else if (w == 1 && databank == ((uval_t)val.u.num.val >> 16)) adr = (uval_t)val.u.num.val;
                                        else w=3;
                                } else if (w > 1) w = 3;
                                opr=ADR_ZP_Y-w;ln=w+1; // ldx $ff,y lda $ffff,y
                                break;
                            case WHAT_S:
                                adrgen = AG_BYTE; opr=ADR_ZP_S; // lda $ff,s
                                break;
                            case WHAT_R:
                                adrgen = AG_BYTE; opr=ADR_ZP_R; // lda $ff,r
                                break;
                            case WHAT_EOL:
                            case WHAT_COMMENT:
                                if (cnmemonic[ADR_MOVE]!=____) {
                                    struct value_s val2;
                                    if (w==3) {//auto length
                                        if (val.type != T_NONE) {
                                            if (!((uval_t)val.u.num.val & ~(uval_t)0xff)) {adr = (uval_t)val.u.num.val << 8; w = 0;}
                                        } else w = 0;
                                    } else if (val.type != T_NONE) {
                                        if (!w && (!((uval_t)val.u.num.val & ~(uval_t)0xff))) adr = (uval_t)val.u.num.val << 8;
                                        else w = 3;
                                    } else if (w) w = 3; // there's no mvp $ffff or mvp $ffffff
                                    if (get_val(&val2, T_UINT, NULL)) {
                                        if (!((uval_t)val.u.num.val & ~(uval_t)0xff)) {adr |= (uint8_t)val.u.num.val;}
                                        else w = 3;
                                    } else err_msg(ERROR_ILLEGAL_OPERA,NULL);
                                    ln = 2; opr=ADR_MOVE;
                                } else if (cnmemonic[ADR_REL]!=____) {
                                    struct star_s *s;
                                    int olabelexists;
                                    int_fast8_t min = 10;
                                    uint32_t joadr = adr;
                                    int_fast8_t joln = 1;
                                    uint8_t jolongbranch = longbranch;
                                    enum opr_e joopr = ADR_REL;
                                    enum errors_e err = ERROR_WUSER_DEFINED;
                                    s=new_star(vline_64tass+1);olabelexists=labelexists;

                                    for (;c;c=get_val(&val, T_UINT, NULL)) {
                                        if (val.type != T_NONE) {
                                            uint16_t oadr = val.u.num.val;
                                            adr = (uval_t)val.u.num.val;
                                            labelexists = olabelexists;

                                            if (labelexists && adr >= s->addr) {
                                                adr=(uint16_t)(adr - s->addr);
                                            } else {
                                                adr=(uint16_t)(adr - current_section->l_address - 2);labelexists=0;
                                            }
                                            ln=1;opr=ADR_REL;longbranch=0;
                                            if (((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff) { err = ERROR_BRANCH_TOOFAR; continue; }
                                            if (adr<0xFF80 && adr>0x007F) {
                                                if (arguments.longbranch && (cnmemonic[ADR_ADDR]==____)) {
                                                    if ((cnmemonic[ADR_REL] & 0x1f)==0x10) {//branch
                                                        longbranch=0x20;ln=4;
                                                        if (scpumode && !longbranchasjmp) {
                                                            if (!labelexists) adr=(uint16_t)(adr-3);
                                                            adr=0x8203+(adr << 16);
                                                        } else {
                                                            adr=0x4C03+(oadr << 16);
                                                        }
                                                    } else {//bra
                                                        if (scpumode && !longbranchasjmp) {
                                                            longbranch=cnmemonic[ADR_REL]^0x82;
                                                            if (!labelexists) adr=(uint16_t)(adr-1);
                                                            ln=2;
                                                        } else if (cnmemonic[ADR_REL] == 0x82 && opcode==c65el02) {
                                                            err = ERROR_BRANCH_TOOFAR;continue; //rer not a branch
                                                        } else {
                                                            longbranch=cnmemonic[ADR_REL]^0x4C;
                                                            adr=oadr;ln=2;
                                                        }
                                                    }
                                                    //err = ERROR___LONG_BRANCH;
                                                } else {
                                                    if (cnmemonic[ADR_ADDR]!=____) {
                                                        if (scpumode && !longbranchasjmp) {
                                                            longbranch=cnmemonic[ADR_REL]^0x82;
                                                            if (!labelexists) adr=(uint16_t)(adr-1);
                                                        } else {
                                                            adr=oadr;
                                                            opr=ADR_ADDR;
                                                        }
                                                        ln=2;
                                                    } else {err = ERROR_BRANCH_TOOFAR;continue;}
                                                }
                                            } else {
                                                if (fixeddig) {
                                                    if (!longbranch && ((uint16_t)(current_section->l_address+2) & 0xff00)!=(oadr & 0xff00)) {
                                                        if (!allowslowbranch) {err=ERROR__BRANCH_CROSS;continue;}
                                                    }
                                                }
                                                if (cnmemonic[ADR_ADDR]!=____) {
                                                    if (adr==0) ln=-1;
                                                    else if (adr==1 && (cnmemonic[ADR_REL] & 0x1f)==0x10) {
                                                        ln=0;longbranch=0x20;adr=0x10000;
                                                    }
                                                }
                                            }
                                            if (ln < min) {
                                                min = ln;
                                                joopr = opr; joadr = adr; joln = ln; jolongbranch = longbranch;
                                            }
                                            err = ERROR_WUSER_DEFINED;
                                        }
                                    }
                                    opr = joopr; adr = joadr; ln = joln; longbranch = jolongbranch;
                                    if (fixeddig && min == 10 && err != ERROR_WUSER_DEFINED) err_msg(err, NULL);
                                    w=0;// bne
                                    if (olabelexists && s->addr != ((star + 1 + ln) & all_mem)) fixeddig=0;
                                    s->addr = (star + 1 + ln) & all_mem;
                                }
                                else if (cnmemonic[ADR_REL_L]!=____) {
                                    if (w==3) {
                                        if (val.type != T_NONE) {
                                            if (!(((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff)) {adr = (uint16_t)(val.u.num.val-current_section->l_address-3); w = 1;}
                                        } else w = 1;
                                    } else if (val.type != T_NONE) {
                                        if (w == 1 && !(((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff)) adr = (uint16_t)(val.u.num.val-current_section->l_address-3);
                                        else w = 3; // there's no jsr ($ffff,x)!
                                    } else if (w != 1) w = 3;
                                    opr=ADR_REL_L; ln = 2; //brl
                                }
                                else if (cnmemonic[ADR_LONG]==0x5C) {
                                    if (w==3) {//auto length
                                        if (val.type != T_NONE) {
                                            if (cnmemonic[ADR_ADDR]!=____ && !(((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff)) {adr = (uval_t)val.u.num.val;w = 1;}
                                            else if (!((uval_t)val.u.num.val & ~(uval_t)0xffffff)) {adr = (uval_t)val.u.num.val; w = 2;}
                                        } else w = (cnmemonic[ADR_ADDR]==____) + 1;
                                    } else if (val.type != T_NONE) {
                                        if (w == 1 && !(((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff)) adr = (uval_t)val.u.num.val;
                                        else if (w == 2 && !((uval_t)val.u.num.val & ~(uval_t)0xffffff)) adr = (uval_t)val.u.num.val;
                                        else w = 3;
                                    }
                                    opr=ADR_ZP-w;ln=w+1; // jml
                                }
                                else if (cnmemonic[ADR_ADDR]==0x20) {
                                    if (fixeddig && (((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff)) {err_msg(ERROR_BRANCH_TOOFAR,NULL);w = 1;};
                                    adrgen = AG_PB; opr=ADR_ADDR; // jsr $ffff
                                } else {
                                    adrgen = AG_DB3; opr=ADR_ZP; // lda $ff lda $ffff lda $ffffff
                                }
                                break;
                            default: err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;
                            }
                            break;
                        case 2:
                            switch (what(&prm)) {
                            case WHAT_SZ:
                                if ((wht=what(&prm))!=WHAT_Y) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                                adrgen = AG_BYTE; opr=ADR_ZP_S_I_Y; // lda ($ff,s),y
                                break;
                            case WHAT_RZ:
                                if ((wht=what(&prm))!=WHAT_Y) {err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;}
                                adrgen = AG_BYTE; opr=ADR_ZP_R_I_Y; // lda ($ff,r),y
                                break;
                            case WHAT_XZ:
                                if (cnmemonic[ADR_ADDR_X_I]==0x7C || cnmemonic[ADR_ADDR_X_I]==0xFC) {// jmp ($ffff,x) jsr ($ffff,x)
                                    adrgen = AG_PB; opr=ADR_ADDR_X_I; // jmp ($ffff,x)
                                } else {
                                    adrgen = AG_ZP; opr=ADR_ZP_X_I; // lda ($ff,x)
                                }
                                break;
                            default: err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;
                            }
                            break;
                        case 3:
                            switch (what(&prm)) {
                            case WHAT_Y:
                                adrgen = AG_ZP; opr=ADR_ZP_I_Y; // lda ($ff),y
                                break;
                            case WHAT_EOL:
                            case WHAT_COMMENT:
                                if (cnmemonic[ADR_ADDR_I]==0x6C) {// jmp ($ffff)
                                    if (fixeddig && opcode!=c65816 && opcode!=c65c02 && opcode!=c65el02 && !(~adr & 0xff)) err_msg(ERROR______JUMP_BUG,NULL);//jmp ($xxff)
                                    adrgen = AG_B0; opr=ADR_ADDR_I; // jmp ($ffff)
                                } else {
                                    adrgen = AG_ZP; opr=ADR_ZP_I; // lda ($ff)
                                }
                                break;
                            default: err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;
                            }
                            break;
                        case 4:
                            switch (what(&prm)) {
                            case WHAT_Y:
                                adrgen = AG_ZP; opr=ADR_ZP_LI_Y; // lda [$ff],y
                                break;
                            case WHAT_EOL:
                            case WHAT_COMMENT:
                                if (cnmemonic[ADR_ADDR_LI]==0xDC) { // jmp [$ffff]
                                    adrgen = AG_B0; opr=ADR_ADDR_LI; // jmp [$ffff]
                                } else {
                                    adrgen = AG_ZP; opr=ADR_ZP_LI; // lda [$ff]
                                }
                                break;
                            default: err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;
                            }
                            break;
                        default: err_msg(ERROR_GENERL_SYNTAX,NULL); goto breakerr;
                        }
                        eval_finish();
                    }
                    switch (adrgen) {
                    case AG_ZP: // zero page address only
                        if (w==3) {//auto length
                            if (val.type == T_NONE) w = 0;
                            else if (!((uval_t)val.u.num.val & ~(uval_t)0xffff) && (uint16_t)(val.u.num.val - dpage) < 0x100) {adr = (uint16_t)(val.u.num.val - dpage);w = 0;}
                        } else if (val.type != T_NONE) {
                            if (!w && !((uval_t)val.u.num.val & ~(uval_t)0xffff) && (uint16_t)(val.u.num.val - dpage) < 0x100) adr = (uint16_t)(val.u.num.val - dpage);
                            else w=3; // there's no $ffff] or $ffffff!
                        } else if (w) w = 3;
                        ln = 1;
                        break;
                    case AG_B0: // bank 0 address only
                        if (w==3) {
                            if (val.type == T_NONE) w = 1;
                            else if (!((uval_t)val.u.num.val & ~(uval_t)0xffff)) {adr = (uint16_t)val.u.num.val; w = 1;}
                        } else if (val.type != T_NONE) {
                            if (w == 1 && !((uval_t)val.u.num.val & ~(uval_t)0xffff)) adr = (uint16_t)val.u.num.val;
                            else w=3; // there's no jmp $ffffff!
                        } else if (w != 1) w = 3;
                        ln = 2;
                        break;
                    case AG_PB: // address in program bank
                        if (w==3) {
                            if (val.type != T_NONE) {
                                if (!(((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff)) {adr = (uint16_t)val.u.num.val; w = 1;}
                            } else w = 1;
                        } else if (val.type != T_NONE) {
                            if (w == 1 && !(((uval_t)current_section->l_address ^ (uval_t)val.u.num.val) & ~(uval_t)0xffff)) adr = (uint16_t)val.u.num.val;
                            else w = 3; // there's no jsr ($ffff,x)!
                        } else if (w != 1) w = 3;
                        ln = 2;
                        break;
                    case AG_BYTE: // byte only
                        if (w==3) {//auto length
                            if (val.type != T_NONE) {
                                if (!((uval_t)val.u.num.val & ~(uval_t)0xff)) {adr = (uval_t)val.u.num.val; w = 0;}
                            } else w = 0;
                        } else if (val.type != T_NONE) {
                            if (!w && !((uval_t)val.u.num.val & ~(uval_t)0xff)) adr = (uval_t)val.u.num.val;
                            else w = 3;
                        } else if (w) w = 3; // there's no lda ($ffffff,s),y or lda ($ffff,s),y!
                        ln = 1;
                        break;
                    case AG_DB3: // 3 choice data bank
                        if (w==3) {//auto length
                            if (val.type != T_NONE) {
                                if (cnmemonic[opr]!=____ && !((uval_t)val.u.num.val & ~(uval_t)0xffff) && (uint16_t)(val.u.num.val - dpage) < 0x100) {adr = (uint16_t)(val.u.num.val - dpage);w = 0;}
                                else if (cnmemonic[opr - 1]!=____ && databank==((uval_t)val.u.num.val >> 16)) {adr = (uval_t)val.u.num.val;w = 1;}
                                else if (!((uval_t)val.u.num.val & ~(uval_t)0xffffff)) {adr = (uval_t)val.u.num.val; w = 2;}
                            } else w=(cnmemonic[opr - 1]!=____);
                        } else if (val.type != T_NONE) {
                            if (!w && !((uval_t)val.u.num.val & ~(uval_t)0xffff) && (uint16_t)(val.u.num.val - dpage) < 0x100) adr = (uint16_t)(val.u.num.val - dpage);
                            else if (w == 1 && databank == ((uval_t)val.u.num.val >> 16)) adr = (uval_t)val.u.num.val;
                            else if (w == 2 && !((uval_t)val.u.num.val & ~(uval_t)0xffffff)) adr = (uval_t)val.u.num.val;
                            else w = 3;
                        }
                        opr -= w;ln = w + 1;
                        break;
                    case AG_NONE:
                        break;
                    }

                    if (d) {
                        if (w==3) {err_msg(ERROR_CONSTNT_LARGE,NULL); goto breakerr;}
                        if ((cod=cnmemonic[opr])==____ && (prm || opr!=ADR_IMMEDIATE)) { // 0x69 hack
                            memcpy(labelname,&mnemonic[mnem*3],3);
                            labelname[3]=0;
                            if ((tmp2=find_macro(labelname)) && (tmp2->type==CMD_MACRO || tmp2->type==CMD_SEGMENT)) {
                                lpoint=oldlpoint;
                                goto as_macro;
                            }
                            err_msg(ERROR_ILLEGAL_OPERA,NULL);
                            goto breakerr;
                        }
                    }
                    if (ln>=0) {
                        uint32_t temp=adr;
                        pokeb(cod ^ longbranch);
                        switch (ln)
                        {
                        case 4:pokeb((uint8_t)temp);temp>>=8;
                        case 3:pokeb((uint8_t)temp);temp>>=8;
                        case 2:pokeb((uint8_t)temp);temp>>=8;
                        case 1:pokeb((uint8_t)temp);
                        }
                    }

                    if (listing && flist) {
                        uint32_t temp=adr;
                        unsigned int i;

                        if (lastl!=LIST_CODE) {putc('\n',flist);lastl=LIST_CODE;}
                        fprintf(flist,(all_mem==0xffff)?".%04" PRIaddress "\t":".%06" PRIaddress " ",(current_section->address-ln-1) & all_mem);
                        if (current_section->dooutput) {
                            if (ln>=0) {
                                fprintf(flist," %02x", cod ^ longbranch ^ outputeor);
                                for (i=0;i<(unsigned)ln;i++) {fprintf(flist," %02x",(uint8_t)temp ^ outputeor);temp>>=8;}
                            }
                            if (ln<2) putc('\t',flist);
                            putc('\t',flist);
                            if (arguments.monitor) {
                                for (i=0;i<3;i++) putc(mnemonic[mnem*3+i],flist);

                                switch (opr) {
                                case ADR_IMPLIED: putc('\t', flist); break;
                                case ADR_ACCU: fputs(" a\t", flist); break;
                                case ADR_IMMEDIATE: {
                                    if (ln==1) fprintf(flist," #$%02x",(uint8_t)adr);
                                    else fprintf(flist," #$%04x",(uint16_t)adr);
                                    break;
                                }
                                case ADR_LONG: fprintf(flist," $%06x",(uint32_t)(adr&0xffffff)); break;
                                case ADR_ADDR: {
                                    if (cnmemonic[ADR_ADDR]==0x20 || cnmemonic[ADR_ADDR]==0x4c)
                                        fprintf(flist,(current_section->l_address&0xff0000)?" $%06" PRIaddress:" $%04" PRIaddress,((uint16_t)adr) | (current_section->l_address & 0xff0000));
                                    else fprintf(flist,databank?" $%06x":" $%04x",(uint16_t)adr | (databank << 16));
                                    break;
                                }
                                case ADR_ZP: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" $%02x\t":" $%04x",(uint16_t)(adr+dpage)); break;
                                case ADR_LONG_X: fprintf(flist," $%06x,x",(uint32_t)(adr&0xffffff)); break;
                                case ADR_ADDR_X: fprintf(flist,databank?" $%06x,x":" $%04x,x",(uint16_t)adr | (databank << 16)); break;
                                case ADR_ZP_X: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" $%02x,x":" $%04x,x",(uint16_t)(adr+dpage)); break;
                                case ADR_ADDR_X_I: fprintf(flist,(current_section->l_address&0xff0000)?" ($%06" PRIaddress ",x)":" ($%04" PRIaddress ",x)",((uint16_t)adr) | (current_section->l_address&0xff0000)); break;
                                case ADR_ZP_X_I: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" ($%02x,x)":" ($%04x,x)",(uint16_t)(adr+dpage)); break;
                                case ADR_ZP_S: fprintf(flist," $%02x,s",(uint8_t)adr); break;
                                case ADR_ZP_S_I_Y: fprintf(flist," ($%02x,s),y",(uint8_t)adr); break;
                                case ADR_ZP_R: fprintf(flist," $%02x,r",(uint8_t)adr); break;
                                case ADR_ZP_R_I_Y: fprintf(flist," ($%02x,r),y",(uint8_t)adr); break;
                                case ADR_ADDR_Y: fprintf(flist,databank?" $%06x,y":" $%04x,y",(uint16_t)adr | (databank << 16)); break;
                                case ADR_ZP_Y: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" $%02x,y":" $%04x,y",(uint16_t)(adr+dpage)); break;
                                case ADR_ZP_LI_Y: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" [$%02x],y":" [$%04x],y",(uint16_t)(adr+dpage)); break;
                                case ADR_ZP_I_Y: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" ($%02x),y":" ($%04x),y",(uint16_t)(adr+dpage)); break;
                                case ADR_ADDR_LI: fprintf(flist," [$%04x]",(uint16_t)adr); break;
                                case ADR_ZP_LI: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" [$%02x]":" [$%04x]",(uint16_t)(adr+dpage)); break;
                                case ADR_ADDR_I: fprintf(flist," ($%04x)",(uint16_t)adr); break;
                                case ADR_ZP_I: fprintf(flist,((uint16_t)(adr+dpage)<0x100)?" ($%02x)":" ($%04x)",(uint16_t)(adr+dpage)); break;
                                case ADR_REL: {
                                    if (ln==1) fprintf(flist,(current_section->l_address&0xff0000)?" $%06" PRIaddress:" $%04" PRIaddress,(uint16_t)(((int8_t)adr)+current_section->l_address) | (current_section->l_address & 0xff0000));
                                    else if (ln==2) {
                                        if ((cod ^ longbranch)==0x4C)
                                            fprintf(flist,(current_section->l_address&0xff0000)?" $%06" PRIaddress:" $%04" PRIaddress,((uint16_t)adr) | (current_section->l_address & 0xff0000));
                                        else
                                            fprintf(flist,(current_section->l_address&0xff0000)?" $%06" PRIaddress:" $%04" PRIaddress,(uint16_t)(adr+current_section->l_address) | (current_section->l_address & 0xff0000));
                                    }
                                    else {
                                        if ((uint16_t)adr==0x4C03)
                                            fprintf(flist,(current_section->l_address&0xff0000)?" $%06" PRIaddress:" $%04" PRIaddress,((uint16_t)(adr >> 16)) | (current_section->l_address & 0xff0000));
                                        else
                                            fprintf(flist,(current_section->l_address&0xff0000)?" $%06" PRIaddress:" $%04" PRIaddress,(uint16_t)((adr >> 16)+current_section->l_address) | (current_section->l_address & 0xff0000));
                                    }
                                    break;
                                }
                                case ADR_REL_L: fprintf(flist,(current_section->l_address & 0xff0000)?" $%06" PRIaddress:" $%04" PRIaddress,(uint16_t)(adr+current_section->l_address) | (current_section->l_address & 0xff0000)); break;
                                case ADR_MOVE: fprintf(flist," $%02x,$%02x",(uint8_t)adr,(uint8_t)(adr>>8));
                                }
                            } else if (arguments.source) putc('\t',flist);
                        } else if (arguments.source) fputs("\t\t\t", flist);
                        if (arguments.source) {
                            putc('\t', flist);printllist(flist);
                        } else putc('\n',flist);
                    }
                    break;
                }
                if ((tmp2=find_macro(labelname)) && (tmp2->type==CMD_MACRO || tmp2->type==CMD_SEGMENT)) goto as_macro;
            }            // fall through
        default: if (waitfor[waitforp].skip & 1) err_msg(ERROR_GENERL_SYNTAX,NULL); //skip things if needed
        }
    finish:
        ignore();if (here() && here()!=';' && (waitfor[waitforp].skip & 1)) err_msg(ERROR_EXTRA_CHAR_OL,NULL);
    breakerr:
        if (newlabel) set_size(newlabel, current_section->address - oaddr);
        continue;
    }

    while (oldwaitforp < waitforp) {
        line_t os = sline;
        sline = waitfor[waitforp].line;
        switch (waitfor[waitforp--].what) {
        case 'e':
        case 'f': err_msg(ERROR______EXPECTED,".FI"); break;
	case 'P':
        case 'p': err_msg(ERROR______EXPECTED,".ENDP"); break;
        case 'M':
        case 'm': err_msg(ERROR______EXPECTED,".ENDM"); break;
        case 'N':
        case 'n': err_msg(ERROR______EXPECTED,".NEXT"); break;
        case 'r': err_msg(ERROR______EXPECTED,".PEND"); break;
	case 'B':
        case 'b': err_msg(ERROR______EXPECTED,".BEND"); break;
        case 'c': err_msg(ERROR______EXPECTED,".ENDC"); break;
        case 'S':
        case 's': err_msg(ERROR______EXPECTED,".ENDS"); break;
        case 'T':
        case 't': err_msg(ERROR______EXPECTED,".SEND"); break;
        case 'U':
        case 'u': err_msg(ERROR______EXPECTED,".ENDU"); break;
        case 'L':
        case 'l': err_msg(ERROR______EXPECTED,".HERE"); break;
        }
        sline = os;
    }
    return;
}

int main_64tass(int argc,char *argv[]) {
    time_t t;
    FILE* fout;
    int optind, i;
    struct file_s *fin;

    tinit();

    fin = openfile("");
    optind = testarg(argc,argv, fin, 0);
    init_encoding(arguments.toascii);

    if (arguments.quiet)
        puts("6502/65C02 Turbo Assembler Version 1.3  Copyright (c) 1997 Taboo Productions\n"
             "6502/65C02 Turbo Assembler Version 1.35 ANSI C port by BiGFooT/BReeZe^2000\n"
             "6502/65C02/65816/DTV TASM Version " VERSION " Fixing by Soci/Singular 2001-2012\n"
             "65EL02 support by BiGFooT/BReeZe^Chorus^Resource 2012\n"
             "64TASS comes with ABSOLUTELY NO WARRANTY; This is free software, and you\n"
             "are welcome to redistribute it under certain conditions; See LICENSE!\n");

    /* assemble the input file(s) */
    do {
        if (pass++>20) {err_msg(ERROR_TOO_MANY_PASS, NULL);break;}
        fixeddig=1;conderrors=warnings=0;freeerrorlist(0);
        mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
        for (i = optind - 1; i<argc; i++) {
            set_cpumode(arguments.cpumode);
            star=databank=dpage=longaccu=longindex=0;wrapwarn=0;actual_encoding=new_encoding("none");wrapwarn2=0;
            structrecursion=0;allowslowbranch=1;
            waitfor[waitforp=0].what=0;waitfor[0].skip=1;sline=vline_64tass=0;outputeor=0;forwr=backr=0;
            current_context=&root_label;
            current_section=&root_section;
            current_section->provides=~(uval_t)0;current_section->requires=current_section->conflicts=0;
            current_section->start=current_section->l_start=current_section->address=current_section->l_address=0;
            current_section->dooutput=1;
            macro_parameters.p = 0;
            /*	listing=1;flist=stderr;*/
            if (i == optind - 1) {
                enterfile("<command line>",0);
                fin->p = 0; cfile = fin;
                star_tree=&fin->star;
                reffile=cfile->uid;
                compile();
                exitfile();
                mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
                continue;
            }
            memjmp(current_section->address);
            enterfile(argv[i],0);
            cfile = openfile(argv[i]);
            if (cfile) {
                cfile->p = 0;
                star_tree=&cfile->star;
                reffile=cfile->uid;
                compile();
                closefile(cfile);
            }
            exitfile();
        }
        if (errors) {memcomp();status();return 1;}
        if (conderrors && !arguments.list && pass==1) fixeddig=0;
    } while (!fixeddig || pass==1);

    /* assemble again to create listing */
    if (arguments.list) {
        char **argv2 = argv;
        listing=1;
        if (arguments.list[0] == '-' && !arguments.list[1]) {
            flist = stdout;
        } else {
            if (!(flist=fopen(arguments.list,"wt"))) err_msg(ERROR_CANT_DUMP_LST,arguments.list);
        }
	fputs("\n; 64tass Turbo Assembler Macro V" VERSION " listing file\n;", flist);
        if (*argv2) {
            char *new = strrchr(*argv2, '/');
            if (new) *argv2 = new + 1;
        }
        while (*argv2) fprintf(flist," %s", *argv2++);
	time(&t); fprintf(flist,"\n; %s",ctime(&t));

        pass++;
        fixeddig=1;conderrors=warnings=0;freeerrorlist(0);
        mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
        for (i = optind - 1; i<argc; i++) {
            if (i >= optind) {fprintf(flist,"\n;******  Processing input file: %s\n", argv[i]);}
            lastl=LIST_NONE;
            set_cpumode(arguments.cpumode);
            star=databank=dpage=longaccu=longindex=0;wrapwarn=0;actual_encoding=new_encoding("none");wrapwarn2=0;
            structrecursion=0;allowslowbranch=1;
            waitfor[waitforp=0].what=0;waitfor[0].skip=1;sline=vline_64tass=0;outputeor=0;forwr=backr=0;
            current_context=&root_label;
            current_section=&root_section;
            current_section->provides=~(uval_t)0;current_section->requires=current_section->conflicts=0;
            current_section->start=current_section->l_start=current_section->address=current_section->l_address=0;
            current_section->dooutput=1;
            macro_parameters.p = 0;

            if (i == optind - 1) {
                enterfile("<command line>",0);
                fin->p = 0; cfile = fin;
                star_tree=&fin->star;
                reffile=cfile->uid;
                compile();
                exitfile();
                mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
                continue;
            }
            memjmp(current_section->address);

            enterfile(argv[i],0);
            cfile = openfile(argv[i]);
            if (cfile) {
                cfile->p = 0;
                star_tree=&cfile->star;
                reffile=cfile->uid;
                compile();
                closefile(cfile);
            }
            exitfile();
        }
	fputs("\n;******  End of listing\n", flist);
	if (flist != stdout) fclose(flist);
    }
    if (!fixeddig) err_msg(ERROR_TOO_MANY_PASS, NULL);
    memcomp();

    set_cpumode(arguments.cpumode);

    if (arguments.label) labelprint();

    if (errors || conderrors) {status();return 1;}

    /* output file */
    if (mem.p) {
        address_t start;
        size_t size;
        unsigned int i, last;
        if (arguments.output[0] == '-' && !arguments.output[1]) {
            fout = stdout;
        } else {
            if ((fout=fopen(arguments.output,"wb"))==NULL) err_msg(ERROR_CANT_WRTE_OBJ,arguments.output);
        }
        clearerr(fout);
        if (memblocks.p) {
            start = memblocks.data[0].start;
            size = memblocks.data[0].len;
            last = 0;
            for (i=1;i<memblocks.p;i++) {
                if (memblocks.data[i].start != start + size) {
                    if (arguments.nonlinear) {
                        putc(size,fout);
                        putc(size >> 8,fout);
                        if (scpumode) putc(size >> 16,fout);
                    }
                    if ((!arguments.stripstart && !last) || arguments.nonlinear) {
                        putc(start,fout);
                        putc(start >> 8,fout);
                        if (scpumode) putc(start >> 16,fout);
                    }
                    while (last<i) {
                        fwrite(mem.data+memblocks.data[last].p,memblocks.data[last].len,1,fout);
                        last++;
                    }
                    if (!arguments.nonlinear) {
                        size = memblocks.data[i].start - start - size;
                        while (size--) putc(0, fout);
                    }
                    start = memblocks.data[i].start;
                    size = 0;
                }
                size += memblocks.data[i].len;
            }
            if (arguments.nonlinear) {
                putc(size,fout);
                putc(size >> 8,fout);
                if (scpumode) putc(size >> 16,fout);
            }
            if ((!arguments.stripstart && !last) || arguments.nonlinear) {
                putc(start,fout);
                putc(start >> 8,fout);
                if (scpumode) putc(start >> 16,fout);
            }
            while (last<i) {
                fwrite(mem.data+memblocks.data[last].p,memblocks.data[last].len,1,fout);
                last++;
            }
        }
        if (arguments.nonlinear) {
            putc(0,fout);
            putc(0,fout);
            if (scpumode) putc(0 ,fout);
        }
        if (ferror(fout)) err_msg(ERROR_CANT_WRTE_OBJ,arguments.output);
	if (fout != stdout) fclose(fout);
    }
    status();
    return 0;
}

static unsigned char *assemble_64tass_bufferOut;
static int assemble_64tass_bufferOutIndex = 0;
static void *assemble_64tass_userData;

void buffer_putc(int c)
{
	unsigned char *v = c;
	assemble_64tass_bufferOut[assemble_64tass_bufferOutIndex++] = v;
}

void c64debugger_set_assemble_result_to_memory(void *userData, int addr, unsigned char v);

void buffer_fwrite(unsigned char *buf, int addrStart, int len)
{
	for (int i = 0; i < len; i++)
	{
		c64debugger_set_assemble_result_to_memory(assemble_64tass_userData, addrStart + i, buf[i]);
		
		buffer_putc(buf[i]);
	}
}

void assemble_64tass_setquiet(int isQuiet)
{
	arguments.quiet = isQuiet ? 0:1;
}

unsigned char *assemble_64tass(void *userData, char *assembleText, int assembleTextSize, int *codeStartAddr, int *codeSize)
{
	assemble_64tass_userData = userData;
	
	*codeStartAddr = 0;
	*codeSize = 0;
	
	time_t t;
	int optind = 0, i = 0;
	struct file_s *fin;
	
	pass=0;
	
	tinit();
	
	fin = openfile("");
//	optind = testarg(argc,argv, fin, 0);
	init_encoding(arguments.toascii);
	
	if (arguments.quiet)
		puts("6502/65C02 Turbo Assembler Version 1.3  Copyright (c) 1997 Taboo Productions\n"
			 "6502/65C02 Turbo Assembler Version 1.35 ANSI C port by BiGFooT/BReeZe^2000\n"
			 "6502/65C02/65816/DTV TASM Version " VERSION " Fixing by Soci/Singular 2001-2012\n"
			 "65EL02 support by BiGFooT/BReeZe^Chorus^Resource 2012\n"
			 "64TASS comes with ABSOLUTELY NO WARRANTY; This is free software, and you\n"
			 "are welcome to redistribute it under certain conditions; See LICENSE!\n");
	
	do {
		if (pass++>20) {err_msg(ERROR_TOO_MANY_PASS, NULL);break;}
				
		fixeddig=1;conderrors=warnings=0;freeerrorlist(0);
		mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
//		for (i = optind - 1; i<argc; i++)
		{
			set_cpumode(arguments.cpumode);
			star=databank=dpage=longaccu=longindex=0;wrapwarn=0;actual_encoding=new_encoding("none");wrapwarn2=0;
			structrecursion=0;allowslowbranch=1;
			waitfor[waitforp=0].what=0;waitfor[0].skip=1;sline=vline_64tass=0;outputeor=0;forwr=backr=0;
			current_context=&root_label;
			current_section=&root_section;
			current_section->provides=~(uval_t)0;current_section->requires=current_section->conflicts=0;
			current_section->start=current_section->l_start=current_section->address=current_section->l_address=0;
			current_section->dooutput=1;
			macro_parameters.p = 0;
			//	listing=1;flist=stderr;
			if (i == optind - 1) {
				enterfile("<command line>",0);
				fin->p = 0; cfile = fin;
				star_tree=&fin->star;
				reffile=cfile->uid;
				compile();
				exitfile();
				mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
				continue;
			}
			memjmp(current_section->address);
			enterfile("asm",0);
			char *fileName = (char*)malloc(8);
			sprintf(fileName, "asm");
			cfile = openfile_from_buffer(fileName, assembleText, assembleTextSize);
			if (cfile) {
				cfile->p = 0;
				star_tree=&cfile->star;
				reffile=cfile->uid;
				compile();
				closefile(cfile);
			}
			exitfile();
		}
		if (errors) {memcomp();status();return NULL;}
		if (conderrors && !arguments.list && pass==1) fixeddig=0;
	} while (!fixeddig || pass==1);
	
//	// assemble again to create listing
//	if (arguments.list) {
//		char **argv2 = argv;
//		listing=1;
//		if (arguments.list[0] == '-' && !arguments.list[1]) {
//			flist = stdout;
//		} else {
//			if (!(flist=fopen(arguments.list,"wt"))) err_msg(ERROR_CANT_DUMP_LST,arguments.list);
//		}
//		fputs("\n; 64tass Turbo Assembler Macro V" VERSION " listing file\n;", flist);
//		if (*argv2) {
//			char *new = strrchr(*argv2, '/');
//			if (new) *argv2 = new + 1;
//		}
//		while (*argv2) fprintf(flist," %s", *argv2++);
//		time(&t); fprintf(flist,"\n; %s",ctime(&t));
//		
//		pass++;
//		fixeddig=1;conderrors=warnings=0;freeerrorlist(0);
//		mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
//		for (i = optind - 1; i<argc; i++) {
//			if (i >= optind) {fprintf(flist,"\n;******  Processing input file: %s\n", argv[i]);}
//			lastl=LIST_NONE;
//			set_cpumode(arguments.cpumode);
//			star=databank=dpage=longaccu=longindex=0;wrapwarn=0;actual_encoding=new_encoding("none");wrapwarn2=0;
//			structrecursion=0;allowslowbranch=1;
//			waitfor[waitforp=0].what=0;waitfor[0].skip=1;sline=vline_64tass=0;outputeor=0;forwr=backr=0;
//			current_context=&root_label;
//			current_section=&root_section;
//			current_section->provides=~(uval_t)0;current_section->requires=current_section->conflicts=0;
//			current_section->start=current_section->l_start=current_section->address=current_section->l_address=0;
//			current_section->dooutput=1;
//			macro_parameters.p = 0;
//			
//			if (i == optind - 1) {
//				enterfile("<command line>",0);
//				fin->p = 0; cfile = fin;
//				star_tree=&fin->star;
//				reffile=cfile->uid;
//				compile();
//				exitfile();
//				mem.p=0;memblocklastp=0;memblocks.p=0;memblocklaststart=0;
//				continue;
//			}
//			memjmp(current_section->address);
//			
//			enterfile(argv[i],0);
//			cfile = openfile(argv[i]);
//			if (cfile) {
//				cfile->p = 0;
//				star_tree=&cfile->star;
//				reffile=cfile->uid;
//				compile();
//				closefile(cfile);
//			}
//			exitfile();
//		}
//		fputs("\n;******  End of listing\n", flist);
//		if (flist != stdout) fclose(flist);
//	}
	if (!fixeddig) err_msg(ERROR_TOO_MANY_PASS, NULL);
	memcomp();
	
	set_cpumode(arguments.cpumode);
	
	if (arguments.label) labelprint();
	
	if (errors || conderrors) {status();return NULL;}

	// output file
	assemble_64tass_bufferOutIndex = 0;
	assemble_64tass_bufferOut = (unsigned char *)malloc(65536);
	
	if (mem.p)
	{
		address_t start;
		size_t size;
		unsigned int i, last;
		
		if (memblocks.p) {
			start = memblocks.data[0].start;
			size = memblocks.data[0].len;
			last = 0;
			
			for (i=1;i<memblocks.p;i++) {
				if (memblocks.data[i].start != start + size) {
					
//					if (arguments.nonlinear) {
//						buffer_putc(size);
//						buffer_putc(size >> 8);
//						if (scpumode) buffer_putc(size >> 16);
//					}
					if ((!arguments.stripstart && !last) || arguments.nonlinear) {
//						buffer_putc(start);
//						buffer_putc(start >> 8);
//						if (scpumode) buffer_putc(start >> 16);
						
						*codeStartAddr = start;
					}
					
					while (last<i) {
						buffer_fwrite(mem.data+memblocks.data[last].p, start, memblocks.data[last].len);
						last++;
					}
					if (!arguments.nonlinear) {
						size = memblocks.data[i].start - start - size;
						while (size--) buffer_putc(0);
					}
					start = memblocks.data[i].start;
					size = 0;
				}
				size += memblocks.data[i].len;
			}
			
//			if (arguments.nonlinear) {
//				buffer_putc(size);
//				buffer_putc(size >> 8);
//				if (scpumode) buffer_putc(size >> 16);
//			}
			if ((!arguments.stripstart && !last) || arguments.nonlinear) {
//				buffer_putc(start);
//				buffer_putc(start >> 8);
//				if (scpumode) buffer_putc(start >> 16);
				*codeStartAddr = start;
			}
			
			while (last<i) {
				buffer_fwrite(mem.data+memblocks.data[last].p, start, memblocks.data[last].len);
				last++;
			}
		}
		if (arguments.nonlinear) {
			buffer_putc(0);
			buffer_putc(0);
			if (scpumode) buffer_putc(0);
		}
//		if (ferror(fout)) err_msg(ERROR_CANT_WRTE_OBJ,arguments.output);
//		if (fout != stdout) fclose(fout);

		*codeSize = assemble_64tass_bufferOutIndex;

	}
	status();
	
	return assemble_64tass_bufferOut;
}

