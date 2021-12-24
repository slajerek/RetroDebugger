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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "64tass-eval.h"
#include "64tass-misc.h"
#include "64tass-error.h"
#include "64tass-section.h"
#include "64tass-encoding.h"

struct encoding_s *actual_encoding;

void set_uint(struct value_s *v, uval_t val) {
    v->u.num.val = val;
    v->u.num.len = 1;
    while (val & ~(uval_t)0xff) {val>>=8;v->u.num.len++;}
    v->type = T_UINT;
}

void set_int(struct value_s *v, ival_t val) {
    v->u.num.val = val;
    v->u.num.len = 1;
    if (val < 0) val = ~val;
    while ((uval_t)val & ~(uval_t)0x7f) {val >>= 8;v->u.num.len++;}
    v->type = T_SINT;
}

static int get_hex(struct value_s *v) {
    uval_t val = 0;
    unsigned int start;
    ignore();
    while (here() == 0x30) lpoint++;
    start = lpoint;
    while ((here() ^ 0x30) < 10 || (uint8_t)((here() | 0x20) - 0x61) < 6 ) {
        val = (val << 4) + (here() & 15);
        if (here() & 0x40) val += 9;
        lpoint++;
    }
    v->u.num.val = val;
    v->u.num.len = (lpoint - start + 1) / 2;
    v->u.num.len |= !v->u.num.len;
    v->type = T_NUM;
    return v->u.num.len > sizeof(val);
}

static int get_bin(struct value_s *v) {
    uval_t val = 0;
    unsigned int start;
    ignore();
    while (here() == 0x30) lpoint++;
    start = lpoint;
    while ((here() & 0xfe) == '0') {
        val = (val << 1) | (here() & 1);
        lpoint++;
    }
    v->u.num.val = val;
    v->u.num.len = (lpoint - start + 7) / 8;
    v->u.num.len |= !v->u.num.len;
    v->type = T_NUM;
    return v->u.num.len > sizeof(val);
}

static int get_dec(struct value_s *v) {
    uval_t val = 0;
    int large = 0;
    while (here() == '0') lpoint++;
    while ((uint8_t)(here() ^ '0') < 10) {
        if (val >= ((uval_t)1 << (8 * sizeof(val) - 1)) / 5) {
            if (val == ((uval_t)1 << (8 * sizeof(val) - 1)) / 5) {
               if ((uval_t)(here() & 15) > (((uval_t)1 << (8 * sizeof(val) - 1)) % 5) * 2) large = 1;
            } else large = 1;
        }
        val=(val * 10) + (here() & 15);
        lpoint++;
    }
    set_uint(v, val);
    return large;
}

uint_fast16_t petascii(size_t *i, struct value_s *v) {
    uint32_t ch, rc2;
    uint8_t *text = v->u.str.data + *i;
    uint16_t rc;

    rc2 = find_escape((char *)text, actual_encoding);
    if (rc2) {
        *i = (rc2 >> 8) + text - v->u.str.data;
        return rc2 & 0xff;
    }
    ch = text[0];
    if (ch & 0x80) (*i) += utf8in(text, &ch); else (*i)++;
    rc = find_trans(ch, actual_encoding);
    if (rc < 256) return rc;
    err_msg(ERROR___UNKNOWN_CHR, (char *)ch);
    ch = 0;
    return ch;
}

int str_to_num(struct value_s *v) {
    uint16_t ch;
    unsigned int large = 0;
    size_t i = 0;
    uval_t val = 0;

    if (actual_encoding) {
        while (v->u.str.len > i) {
            if (large >= sizeof(val)) {
                if (v->type == T_TSTR) free(v->u.str.data);
                v->type = T_NONE;return 1;
            }

            ch = petascii(&i, v);
            if (ch > 255) {
                if (v->type == T_TSTR) free(v->u.str.data);
                v->type = T_NONE;return 1;
            }

            val |= (uint8_t)ch << (8 * large);
            large++;
        }
    } else if (v->u.str.len) {
        uint32_t ch;

        ch = v->u.str.data[0];
        if (ch & 0x80) i = utf8in(v->u.str.data, &ch); else i=1;

        if (v->u.str.len > i) {
            if (v->type == T_TSTR) free(v->u.str.data);
            v->type = T_NONE;return 1;
        }
        val = ch;
    } else {
        if (v->type == T_TSTR) free(v->u.str.data);
        v->type = T_NONE;return 1;
    }
    if (v->type == T_TSTR) free(v->u.str.data);
    v->u.num.val = val;
    v->u.num.len = large;
    v->u.num.len |= !v->u.num.len;
    v->type = T_NUM;
    return 0;
}

static void get_string(struct value_s *v, uint8_t ch) {
    unsigned int i;
    uint32_t ch2;

    i = lpoint;
    for (;;) {
        if (!(ch2 = here())) {err_msg(ERROR______EXPECTED,"End of string"); v->type = T_NONE; return;}
        if (ch2 & 0x80) lpoint += utf8in(pline + lpoint, &ch2); else lpoint++;
        if (ch2 == ch) {
            if (here() == ch && !arguments.tasmcomp) lpoint++; // handle 'it''s'
            else break; // end of string;
        }
    }
    v->type = T_STR;
    v->u.str.len = lpoint - i - 1;
    v->u.str.data = (uint8_t *)pline + i;
    return;
}

static enum type_e touch_label(struct label_s *tmp) {

    if (tmp) {
        tmp->ref=1;tmp->pass=pass;
        if (tmp->type != L_VAR || tmp->upass==pass) return T_IDENTREF;
    }
    return T_UNDEF;
}

static void copy_name(struct value_s *val, char *ident) {
    unsigned int len = val->u.ident.len;
    if (len > linelength - 1) len = linelength - 1;
    if (arguments.casesensitive) memcpy(ident, val->u.ident.name, len);
    else {
        unsigned int i;
        for (i=0;i < len;i++) ident[i]=lowcase(val->u.ident.name[i]);
    }
    ident[len] = 0;
}

static void try_resolv_ident(struct value_s *val) {
    char ident[linelength];
    if (val->type == T_FORWR) {
        sprintf(ident,"+%x+%x", reffile, forwr + val->u.ref - 1);
        goto ident;
    }
    if (val->type == T_BACKR) {
        sprintf(ident,"-%x-%x", reffile, backr - val->u.ref);
        goto ident;
    }
    if (val->type == T_IDENT) {
        copy_name(val, ident);
    ident:
        val->u.label = find_label(ident);
        val->type = touch_label(val->u.label);
    }
}

static void try_resolv_identref(struct value_s *val) {
    if (val->type == T_IDENTREF) {
        if (pass != 1) {
            if ((val->u.label->requires & current_section->provides)!=val->u.label->requires) err_msg(ERROR_REQUIREMENTS_,val->u.label->name);
            if (val->u.label->conflicts & current_section->provides) err_msg(ERROR______CONFLICT,val->u.label->name);
        }
        *val = val->u.label->value;
    } else if (val->type == T_UNDEF && pass == 1) val->type = T_NONE;
}

static enum type_e try_resolv(struct value_s *val) {
    try_resolv_ident(val);
    try_resolv_identref(val);
    return val->type;
}

static void get_star(struct value_s *v) {
    struct star_s *tmp;

    tmp=new_star(vline_64tass);
    if (labelexists && tmp->addr != star) {
        fixeddig=0;
    }
    tmp->addr=star;
    set_uint(v, star);
}

/*
 * get priority for operator in an expression
 */
static int priority(char ch)
{
    switch (ch) {
    default:
    case 'I':          // a[
    case '[':          // [a]
    case '(':return 0;
    case ',':return 1;
    case 'l':          // <
    case 'h':          // >
    case 'H':          // `
    case 'S':return 2; // ^
    case 'O':return 3; // ||
    case 'X':return 4; // ^^
    case 'A':return 5; // &&
    case '=':
    case 'o':          // !=
    case '<':
    case '>':
    case 'g':          // >=
    case 's':return 6; // <=
    case '|':return 7;
    case '^':return 8;
    case '&':return 9;
    case 'm':          // <<
    case 'D':          // >>
    case 'd':return 10; // >>>
    case '+':
    case '-':return 11;
    case '*':
    case '/':
    case '%':return 12;// %
    case 'E':return 13;// **
    case '.':return 14;// .
    case 'n':          // -
    case 'p':return 15;// +
    case '~':return 16;// ~
    case '!':return 17;// !
    }
}

static struct values_s **values = NULL;
static size_t values_size=0;
static size_t values_len=0;
static size_t values_p=0;

static int get_exp_compat(int *wd, int stop) {// length in bytes, defined
    int cd;
    unsigned int i;
    char ch, conv;

    static struct values_s o_out[256];
    char o_oper[256] = {0};
    uint8_t outp = 0, operp = 0, vsp;
    int large=0;
    enum type_e t1, t2;
    unsigned int epoint, cpoint = 0;
    values_len = values_p = 0;

    *wd=3;    // 0=byte 1=word 2=long 3=negative/too big
    cd=0;     // 0=error, 1=ok, 2=(a, 3=()
rest:
    ignore();
    conv = 0;
    switch (here()) {
    case '!':*wd=1;lpoint++;break;
    case '<':
    case '>': conv = here();cpoint = lpoint; lpoint++;break; 
    }
    for (;;) {
        ignore();ch = here(); epoint=lpoint;

        switch (ch) {
        case '(': o_oper[operp++] = ch; lpoint++;continue;
        case '$': lpoint++;if (get_hex(&o_out[outp].val)) goto pushlarge;goto pushval;
        case '%': lpoint++;if (get_bin(&o_out[outp].val)) goto pushlarge;goto pushval;
        case '"': lpoint++;get_string(&o_out[outp].val, ch);goto pushval;
        case '*': lpoint++;get_star(&o_out[outp].val);goto pushval;
        }
        if (ch>='0' && ch<='9') { if (get_dec(&o_out[outp].val)) goto pushlarge;
        pushval:
            if ((o_out[outp].val.type == T_SINT || o_out[outp].val.type == T_UINT || o_out[outp].val.type == T_NUM) && (o_out[outp].val.u.num.val & ~0xffff)) {
            pushlarge:
                err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);large=1;
                o_out[outp].val.u.num.val = 0xffff;
            }
        } else {
            while ((((ch=here()) | 0x20) >= 'a' && (ch | 0x20) <= 'z') || (ch>='0' && ch<='9') || ch=='_') lpoint++;
            if (epoint == lpoint) goto syntaxe;
            o_out[outp].val.u.ident.name = pline + epoint;
            o_out[outp].val.u.ident.len = lpoint - epoint;
            o_out[outp].val.type = T_IDENT;
        }
        o_out[outp++].epoint=epoint;
    other:
        ignore();ch = here(); epoint=lpoint;
	
        while (operp && o_oper[operp-1] != '(') {o_out[outp].val.type = T_OPER; o_out[outp++].val.u.oper=o_oper[--operp];}
        switch (ch) {
        case ',':
            if (conv) {
                o_out[outp].epoint = cpoint;
                o_out[outp].val.type = T_OPER;
                o_out[outp++].val.u.oper = conv;
            }
            if (stop) break;
            o_oper[operp++] = ch;
            lpoint++;
            goto rest;
        case '&':
        case '.':
        case ':':
        case '*':
        case '/':
        case '+':
        case '-': 
            o_oper[operp++] = ch;
            lpoint++;
            continue;
        case ')':
            if (!operp) {err_msg(ERROR______EXPECTED,"("); goto error;}
            lpoint++;
            operp--;
            goto other;
        case 0:
        case ';':
            if (conv) {
                o_out[outp].epoint = cpoint;
                o_out[outp].val.type = T_OPER;
                o_out[outp++].val.u.oper = conv;
            }
            break;
        default: goto syntaxe;
        }
        if (stop && o_oper[0]=='(') {
            if (!operp) {cd=3;break;}
            if (operp==1 && ch == ',') {cd=2; break;}
        }
        if (!operp) {cd=1;break;}
        err_msg(ERROR______EXPECTED,")"); goto error;
    syntaxe:
        err_msg(ERROR_EXPRES_SYNTAX,NULL);
    error:
        for (i=0; i<outp; i++) if (o_out[i].val.type == T_TSTR) free(o_out[i].val.u.str.data);
        return 0;
    }
    vsp = 0;
    for (i=0;i<outp;i++) {
	if (o_out[i].val.type != T_OPER) {
            if (vsp >= values_size) {
                values_size += 16;
                values = realloc(values, sizeof(struct values_s *)*values_size);
            }
            values[vsp++]=&o_out[i];
            continue;
        }
        ch = o_out[i].val.u.oper;
        if (ch==',') continue;
        if (vsp < 1) goto syntaxe;
        t1 = try_resolv(&values[vsp-1]->val);
        if (ch == '<' || ch == '>') {
            switch (t1) {
            case T_UINT:
            case T_SINT:
            case T_NUM:
                {
                    uint16_t val1 = values[vsp-1]->val.u.num.val;

                    switch (ch) {
                    case '>': val1 >>= 8;
                    case '<': val1 = (uint8_t)val1; break;
                    }
                    set_uint(&values[vsp-1]->val, val1);
                    break;
                }
            case T_TSTR:
                free(values[vsp-1]->val.u.str.data);
            default:
                err_msg_wrong_type(t1, values[vsp-1]->epoint);
                values[vsp-1]->val.type = T_NONE; 
            case T_NONE:break;
            }
            values[vsp-1]->epoint = o_out[i].epoint;
            continue;
        }
        if (vsp < 2) goto syntaxe;
        t2 = try_resolv(&values[vsp-2]->val);
        switch (t1) {
        case T_SINT:
        case T_UINT:
        case T_NUM:
            switch (t2) {
            case T_UINT:
            case T_SINT:
            case T_NUM:
                {
                    uint16_t val1 = values[vsp-1]->val.u.num.val;
                    uint16_t val2 = values[vsp-2]->val.u.num.val;

                    switch (ch) {
                    case '*': val1 *= val2; break;
                    case '/': if (!val1) {err_msg2(ERROR_DIVISION_BY_Z, NULL, values[vsp-1]->epoint); val1 = 0xffff;large=1;} else val1=val2 / val1; break;
                    case '+': val1 += val2; break;
                    case '-': val1 = val2 - val1; break;
                    case '&': val1 &= val2; break;
                    case '.': val1 |= val2; break;
                    case ':': val1 ^= val2; break;
                    }
                    vsp--;
                    set_uint(&values[vsp-1]->val, val1);
                    continue;
                }
            default: err_msg_wrong_type(t2, values[vsp-2]->epoint);
            case T_NONE:break;
            }
            break;
        case T_TSTR:
            free(values[vsp-1]->val.u.str.data);
        default:
            err_msg_wrong_type(t1, values[vsp-1]->epoint);
        case T_NONE:break;
        }
        if (t2 == T_TSTR) free(values[vsp-2]->val.u.str.data);
        vsp--; values[vsp-1]->val.type = T_NONE; continue;
    }
    if (large) cd=0;
    values_len = vsp;
    return cd;
}

void eval_finish(void) {
    if (values_p >= values_len) return;
    lpoint = values[values_p]->epoint;
}

int get_val(struct value_s *v, enum type_e type, unsigned int *epoint) {// length in bytes, defined
    static uint8_t line[linelength];  //current line data

    if (values_p >= values_len) return 0;

    if (epoint) *epoint = values[values_p]->epoint;
    *v=values[values_p++]->val;
    try_resolv(v);

    switch (v->type) {
    case T_TSTR:
        if (v->u.str.len <= linelength) memcpy(line, v->u.str.data, v->u.str.len);
        free(v->u.str.data);
        v->u.str.data = line;
        v->type = T_STR;
    case T_STR:
    case T_SINT:
    case T_UINT:
    case T_NUM:
    case T_GAP:
        if (type == T_NONE) return 1;
        if (type == T_SINT || type == T_UINT || type == T_NUM) {
            switch (v->type) {
            case T_STR:
                if (arguments.tasmcomp) break;
                if (str_to_num(v)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, values[values_p-1]->epoint);
                    return 1;
                }
                v->type = type;
            case T_UINT:
            case T_SINT:
            case T_NUM:
                return 1;
            default:
                break;
            }
        }
    default:
        err_msg_wrong_type(v->type, values[values_p-1]->epoint);
        v->type = T_NONE;break;
    case T_NONE: break;
    }
    return 1;
}

static void functions(struct values_s **vals, unsigned int args) {
    struct value_s *v1 = &vals[0]->val;

#if 0
    // len(a) - length of string in characters
    if (v1->u.ident.len == 3 && !memcmp(v1->u.ident.name, "len", 3)) {
        if (args != 1) err_msg2(ERROR_ILLEGAL_OPERA,NULL, vals[0]->epoint); else
        switch (try_resolv(&vals[1]->val)) {
        case T_TSTR:
        case T_STR:
            set_uint(v1, vals[1]->val.u.str.len);
            break;
        default: err_msg_wrong_type(vals[1]->val.type, vals[1]->epoint);
        case T_NONE: return;
        }
        return;
    }
#endif
    // min(a, b, ...) - minimum value
    if (v1->u.ident.len == 3 && !memcmp(v1->u.ident.name, "min", 3)) {
        ival_t min = 0;
        if (args < 1) err_msg2(ERROR_ILLEGAL_OPERA,NULL, vals[0]->epoint);
        else {
            int volt = 1, t = 1;
            while (args) {
                switch (try_resolv(&vals[args]->val)) {
                case T_SINT:
                    if (volt || vals[args]->val.u.num.val < min) {min = vals[args]->val.u.num.val;t = 1;}
                    volt = 0;
                    break;
                case T_UINT:
                case T_NUM:
                    if (volt || (uval_t)vals[args]->val.u.num.val < (uval_t)min) {min = vals[args]->val.u.num.val; t = 0;}
                    volt = 0;
                    break;
                default: err_msg_wrong_type(vals[args]->val.type, vals[args]->epoint);
                case T_NONE:
                    return;
                }
                args--;
            }
            if (t) set_int(v1, min); else set_uint(v1, min);
        }
        return;
    } // max(a, b, ...) - maximum value
    else if (v1->u.ident.len == 3 && !memcmp(v1->u.ident.name, "max", 3)) {
        ival_t max = 0;
        if (args < 1) err_msg2(ERROR_ILLEGAL_OPERA,NULL, vals[0]->epoint);
        else {
            int volt = 1;
            while (args) {
                switch (try_resolv(&vals[args]->val)) {
                case T_SINT:
                case T_UINT:
                case T_NUM:
                    if (volt || vals[args]->val.u.num.val > max) max = vals[args]->val.u.num.val;
                    volt = 0;
                    break;
                default: err_msg_wrong_type(vals[args]->val.type, vals[args]->epoint);
                case T_NONE:
                    return;
                }
                args--;
            }
            set_int(v1, max);
        }
        return;
    } // size(a) - size of data structure at location
    else if (v1->u.ident.len == 4 && !memcmp(v1->u.ident.name, "size", 4)) {
        if (args != 1) err_msg2(ERROR_ILLEGAL_OPERA,NULL, vals[0]->epoint);
        else {
	    try_resolv_ident(&vals[1]->val);
	    switch (vals[1]->val.type) {
	    case T_IDENTREF:
                switch (vals[1]->val.u.label->type) {
                case L_LABEL:
                case L_STRUCT:
                case L_UNION:
                    set_uint(v1, vals[1]->val.u.label->size);
                    break;
                default: try_resolv_identref(&vals[1]->val);err_msg_wrong_type(vals[1]->val.type, vals[1]->epoint);
                }
                break;
	    default: err_msg_wrong_type(vals[1]->val.type, vals[1]->epoint);
	    case T_NONE:
		return;
            case T_UNDEF:
                if (pass == 1) v1->type = T_NONE; else v1->type = T_UNDEF;
	    }
        }
        return;
    }
    err_msg2(ERROR___NOT_DEFINED,"function", vals[0]->epoint);
}

int get_exp(int *wd, int stop) {// length in bytes, defined
    int cd;
    unsigned int i;
    char ch;
    static uint8_t line[linelength];  //current line data

    static struct values_s o_out[256];
    struct values_s *v1, *v2;
    char o_oper[256] = {0};
    unsigned int epoints[256];
    uint8_t outp = 0, operp = 0, vsp, prec, db;
    int large=0;
    ival_t val;
    enum type_e t1, t2;
    unsigned int epoint;

    if (arguments.tasmcomp) {
        return get_exp_compat(wd, stop);
    }
    values_len = values_p = 0;

    *wd=3;    // 0=byte 1=word 2=long 3=negative/too big
    cd=0;    // 0=error, 1=ok, 2=(a, 3=(), 4=[]

    ignore();
    switch (here()) {
    case '@':
	switch (pline[++lpoint] | 0x20) {
	case 'b':*wd=0;break;
	case 'w':*wd=1;break;
	case 'l':*wd=2;break;
	default:err_msg(ERROR______EXPECTED,"@B or @W or @L"); return 0;
	}
        lpoint++;
        break;
    }
    for (;;) {
        ignore();ch = here(); epoint = lpoint;
        switch (ch) {
        case '[':
        case '(': o_oper[operp++] = ch; lpoint++;continue;
        case '+': ch = 'p'; break;
        case '-': ch = 'n'; break;
        case '!': break;
        case '~': break;
        case '<': ch = 'l'; break;
        case '>': ch = 'h'; break;
        case '`': ch = 'H'; break;
        case '^': ch = 'S'; break;
        case '$': lpoint++;if (get_hex(&o_out[outp].val)) goto pushlarge;goto pushval;
        case '%': lpoint++;if (get_bin(&o_out[outp].val)) goto pushlarge;goto pushval;
        case '"':
        case '\'': lpoint++;get_string(&o_out[outp].val, ch);goto pushval;
        case '*': lpoint++;get_star(&o_out[outp].val);goto pushval;
        case '?': lpoint++;o_out[outp].val.type = T_GAP;goto pushval;
        default: 
            if (ch>='0' && ch<='9') {
                if (get_dec(&o_out[outp].val)) {
                pushlarge:
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, epoint);large=1;
                }
            pushval: 
                o_out[outp++].epoint=epoint;
                goto other;
            }
            while ((((ch=here()) | 0x20) >= 'a' && (ch | 0x20) <= 'z') || (ch>='0' && ch<='9') || ch=='_') lpoint++;
            if (epoint != lpoint) {
                o_out[outp].val.u.ident.name = pline + epoint;
                o_out[outp].val.u.ident.len = lpoint - epoint;
                o_out[outp].val.type = (ch=='(')?T_FUNC : T_IDENT;
                o_out[outp++].epoint=epoint;
                if (ch=='(') {
                    o_oper[operp++] = 'F'; continue;
                }
                goto other;
            }
            db = operp;
            while (operp && o_oper[operp-1] == 'p') operp--;
            if (db != operp) {
                o_out[outp].val.u.ref = db - operp;
                o_out[outp].val.type = T_FORWR;
                epoint = epoints[operp];
                goto pushval;
            }
            while (operp && o_oper[operp-1] == 'n') operp--;
            if (db != operp) {
                o_out[outp].val.u.ref = db - operp;
                o_out[outp].val.type = T_BACKR;
                epoint = epoints[operp];
                goto pushval;
            }
            goto syntaxe;
        }
        lpoint++;
        prec = priority(ch);
        while (operp && prec < priority(o_oper[operp-1])) {o_out[outp].val.type = T_OPER;o_out[outp].epoint=epoints[--operp];o_out[outp++].val.u.oper=o_oper[operp];}
        epoints[operp] = epoint;
        o_oper[operp++] = ch;
        continue;
    other:
        ignore();ch = here(); epoint = lpoint;
        switch (ch) {
        case ',': if (stop) break;goto push2;
        case '[': ch = 'I';goto push2;
        case '&': if (pline[lpoint+1] == '&') {lpoint++;ch = 'A';} goto push2;
        case '|': if (pline[lpoint+1] == '|') {lpoint++;ch = 'O';} goto push2;
        case '^': if (pline[lpoint+1] == '^') {lpoint++;ch = 'X';} goto push2;
        case '*': if (pline[lpoint+1] == '*') {lpoint++;ch = 'E';} goto push2;
        case '%': goto push2;
        case '/': if (pline[lpoint+1] == '/') {lpoint++;ch = '%';} goto push2;
        case '+': goto push2;
        case '-': goto push2;
        case '.': goto push2;
        case '=': if (pline[lpoint+1] == '=') lpoint++;
        push2:
            prec = priority(ch);
            while (operp && prec <= priority(o_oper[operp-1])) {o_out[outp].val.type = T_OPER;o_out[outp].epoint=epoints[--operp];o_out[outp++].val.u.oper=o_oper[operp];}
            o_oper[operp++] = ch;
            lpoint++;
            continue;
        case '<': 
            switch (pline[lpoint+1]) {
            case '>': lpoint++;ch = 'o'; break;
            case '<': lpoint++;ch = 'm'; break;
            case '=': lpoint++;ch = 's'; break;
            }
            goto push2;
        case '>':
            switch (pline[lpoint+1]) {
            case '<': lpoint++;ch = 'o'; break;
            case '>': lpoint++;if (pline[lpoint+1] == '>') {lpoint++;ch = 'd';} else ch = 'D'; break;
            case '=': lpoint++;ch = 'g'; break;
            }
            goto push2;
        case '!':
            if (pline[lpoint+1]=='=') {lpoint++;ch = 'o';goto push2;}
            goto syntaxe;
        case ')':
            while (operp && o_oper[operp-1] != '(') {
                if (o_oper[operp-1]=='[' || o_oper[operp-1]=='I') {err_msg(ERROR______EXPECTED,"("); goto error;}
                o_out[outp].val.type = T_OPER;o_out[outp].epoint=epoints[--operp];o_out[outp++].val.u.oper=o_oper[operp];
            }
            lpoint++;
            if (!operp) {err_msg(ERROR______EXPECTED,"("); goto error;}
            operp--;
            if (operp && o_oper[operp-1]=='F') {
                o_out[outp].val.type = T_OPER;o_out[outp].epoint=epoints[--operp];o_out[outp++].val.u.oper=o_oper[operp];
            }
            goto other;
        case ']':
            while (operp && o_oper[operp-1] != '[') {
                if (o_oper[operp-1]=='(') {err_msg(ERROR______EXPECTED,"["); goto error;}
                o_out[outp].val.type = T_OPER;o_out[outp].epoint=epoints[--operp];o_out[outp++].val.u.oper=o_oper[operp];
                if (o_oper[operp] == 'I') break;
            }
            lpoint++;
            if (o_oper[operp] == 'I') goto other;
            if (!operp) {err_msg(ERROR______EXPECTED,"["); goto error;}
            operp--;
            goto other;
        case 0:
        case ';': break;
        default: goto syntaxe;
        }
        if (stop && o_oper[0]=='(') {
            if (!operp) {cd=3;break;}
            if (ch == ',') {
                while (operp && o_oper[operp-1] != '(') {
                    if (o_oper[operp-1]=='[' || o_oper[operp-1]=='I') {err_msg(ERROR______EXPECTED,"("); goto error;}
                    o_out[outp].val.type = T_OPER;o_out[outp].epoint=epoints[--operp];o_out[outp++].val.u.oper=o_oper[operp];
                }
                if (operp==1) {cd=2; break;}
            }
            err_msg(ERROR______EXPECTED,")"); goto error;
        } else if (stop && o_oper[0]=='[') {
            if (!operp) {cd=4;break;}
            err_msg(ERROR______EXPECTED,"]"); goto error;
        } else {
            while (operp) {
                if (o_oper[operp-1] == '(') {err_msg(ERROR______EXPECTED,")"); goto error;}
                if (o_oper[operp-1] == '[' || o_oper[operp-1]=='I') {err_msg(ERROR______EXPECTED,"]"); goto error;}
                o_out[outp].val.type = T_OPER;o_out[outp].epoint=epoints[--operp];o_out[outp++].val.u.oper=o_oper[operp];
            }
            if (!operp) {cd=1;break;}
        }
    syntaxe:
        err_msg(ERROR_EXPRES_SYNTAX,NULL);
    error:
        for (i=0; i<outp; i++) if (o_out[i].val.type == T_TSTR) free(o_out[i].val.u.str.data);
        return 0;
    }
    vsp = 0;
    for (i=0;i<outp;i++) {
	if (o_out[i].val.type != T_OPER) {
            if (vsp >= values_size) {
                values_size += 16;
                values = realloc(values, sizeof(struct values_s *)*values_size);
            }
            values[vsp++]=&o_out[i];
            continue;
        }
        ch = o_out[i].val.u.oper;
        if (ch==',') continue;
        if (vsp == 0) goto syntaxe;
        v1 = values[vsp-1];
        switch (ch) {
        case '.':
            {
                char ident[linelength];
                v2 = v1; v1 = values[--vsp-1];
                if (vsp == 0) goto syntaxe;
                if (v1->val.type == T_NONE) goto errtype;
                if (v1->val.type == T_IDENT) {
                    copy_name(&v1->val, ident);
                    v1->val.u.label = find_label(ident);
                    v1->val.type = touch_label(v1->val.u.label);
                    if (v1->val.type == T_UNDEF && pass == 1) {
                        v1->val.type = T_NONE;
                        goto errtype;
                    }
                }
                if (v1->val.type == T_IDENTREF) {
                    if (v2->val.type == T_IDENT) {
                        copy_name(&v2->val, ident);
                        v1->val.u.label = find_label2(ident, &v1->val.u.label->members);
                        v1->val.type = touch_label(v1->val.u.label);
                        v1->epoint=v2->epoint;
                        continue;
                    } else err_msg_wrong_type(v2->val.type, v2->epoint);
                } else err_msg_wrong_type(v1->val.type, v1->epoint);
                goto errtype;
            }
        case 'F':
            {
                unsigned int args = 0;
                while (v1->val.type != T_FUNC) {
                    args++;
                    v1 = values[vsp-1-args];
                }
                v1->val.type = T_NONE;
                functions(&values[vsp-1-args], args);
                while (args--) {
                    if (values[vsp-1]->val.type == T_TSTR) free(values[vsp-1]->val.u.str.data);
                    vsp--;
                }
                continue;
            }
        }

        switch (ch) {
        case 'l': // <
            switch (try_resolv(&v1->val)) {
            case T_TSTR:
            case T_STR:
                if (str_to_num(&v1->val)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                }
            case T_UINT:
            case T_SINT: v1->val.type = T_NUM; 
            case T_NUM: v1->val.u.num.val = (uint8_t)v1->val.u.num.val;v1->val.u.num.len=1;v1->epoint = o_out[i].epoint;continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        case 'h': // >
            switch (try_resolv(&v1->val)) {
            case T_TSTR:
            case T_STR:
                if (str_to_num(&v1->val)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                }
            case T_UINT:
            case T_SINT: v1->val.type = T_NUM; 
            case T_NUM: v1->val.u.num.val = (uint8_t)(v1->val.u.num.val >> 8);v1->val.u.num.len=1;v1->epoint = o_out[i].epoint;continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        case 'H': // `
            switch (try_resolv(&v1->val)) {
            case T_TSTR:
            case T_STR:
                if (str_to_num(&v1->val)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                }
            case T_UINT:
            case T_SINT: v1->val.type = T_NUM; 
            case T_NUM: v1->val.u.num.val = (uint8_t)(v1->val.u.num.val >> 16);v1->val.u.num.len=1;v1->epoint = o_out[i].epoint;continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        case 'S': // ^
            switch (try_resolv(&v1->val)) {
            case T_TSTR:
            case T_STR:
                if (str_to_num(&v1->val)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                }
            case T_UINT:
            case T_SINT:
            case T_NUM:
                sprintf((char *)line, (v1->val.type == T_SINT) ? "%" PRIdval : "%" PRIuval, v1->val.u.num.val);
                v1->val.type = T_TSTR;
                v1->val.u.str.len=strlen((char *)line);
                v1->val.u.str.data=malloc(v1->val.u.str.len);
                memcpy(v1->val.u.str.data, line, v1->val.u.str.len);
                v1->epoint = o_out[i].epoint;
                continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        case 'p': // +
            switch (try_resolv(&v1->val)) {
            case T_TSTR:
            case T_STR:
                if (str_to_num(&v1->val)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                }
            case T_UINT: v1->val.type = T_SINT;
            case T_SINT:
            case T_NUM: v1->epoint = o_out[i].epoint;continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        case 'n': // -
            switch (try_resolv(&v1->val)) {
            case T_TSTR:
            case T_STR:
                if (str_to_num(&v1->val)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                }
            case T_UINT: v1->val.type = T_SINT;
            case T_SINT:
            case T_NUM: v1->val.u.num.val = -v1->val.u.num.val;v1->epoint = o_out[i].epoint;continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        case '~':
            switch (try_resolv(&v1->val)) {
            case T_TSTR:
            case T_STR:
                if (str_to_num(&v1->val)) {
                    err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                }
            case T_SINT:
            case T_UINT: v1->val.type = T_NUM; 
            case T_NUM: v1->val.u.num.val = ~v1->val.u.num.val;v1->epoint = o_out[i].epoint;continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        case '!':
            switch (try_resolv(&v1->val)) {
            case T_SINT:
            case T_NUM: v1->val.type = T_UINT;
            case T_UINT: v1->val.u.num.val = !v1->val.u.num.val;v1->val.u.num.len=1;v1->epoint = o_out[i].epoint;continue;
            case T_TSTR: free(v1->val.u.str.data);
            case T_STR:
                v1->val.type = T_UINT;
                v1->val.u.num.val = !v1->val.u.str.len;
                v1->val.u.num.len = 1;
                continue;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;
            case T_NONE: continue;
            }
        }
        v2 = v1; v1 = values[--vsp-1];
        if (vsp == 0) goto syntaxe;
        t2 = try_resolv(&v2->val);
        try_resolv_ident(&v1->val);
        if (ch == 'I' && v1->val.type == T_IDENTREF
            && v1->val.u.label->type == L_LABEL && v1->val.u.label->esize && t2 <= T_SINT) {
            uint8_t size = v1->val.u.label->esize;
            try_resolv_identref(&v1->val);
            v1->val.u.num.val += v2->val.u.num.val * size;
            continue;
        }
        try_resolv_identref(&v1->val);
        t1 = v1->val.type;

        if (t1 == T_NONE || t2 == T_NONE) {
        errtype:
            if (v1->val.type == T_TSTR) free(v1->val.u.str.data);
            if (v2->val.type == T_TSTR) free(v2->val.u.str.data);
            v1->val.type = T_NONE;
            continue;
        }

        if (t1 <= T_SINT && (t2 == T_STR || t2 == T_TSTR)) {
            if (str_to_num(&v2->val)) {
                err_msg2(ERROR_CONSTNT_LARGE, NULL, v2->epoint); large=1;
            }
            t2 = v2->val.type;
        }
        if (t2 <= T_SINT && (t1 == T_STR || t1 == T_TSTR)) {
            if (str_to_num(&v1->val)) {
                err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
            }
            t1 = v1->val.type;
        }
    strretr:

        if ((t1 <= T_SINT && t2 <= T_SINT)) {
            ival_t val1 = v1->val.u.num.val;
            ival_t val2 = v2->val.u.num.val;

            if (t2 < t1) t2 = t1;

            switch (ch) {
            case '=': val1 = ( val1 == val2);break;
            case 'o': val1 = ( val1 != val2);break;
            case '<': val1 = ( val1 <  val2);break;
            case '>': val1 = ( val1 >  val2);break;
            case 's': val1 = ( val1 <= val2);break;
            case 'g': val1 = ( val1 >= val2);break;
            case 'A': val1 = ( val1 && val2);break;
            case 'O': val1 = ( val1 || val2);break;
            case 'X': val1 = (!val1 ^ !val2);break;
            case '*': val1 = ( val1 *  val2);break;
            case '/': if (!val2) {err_msg2(ERROR_DIVISION_BY_Z, NULL, v2->epoint); val1 = (~(uval_t)0) >> 1; large=1;}
                else if (t2==T_SINT) val1 = ( val1 / val2); else val1 = ( (uval_t)val1 / (uval_t)val2);  break;
            case '%': if (!val2) {err_msg2(ERROR_DIVISION_BY_Z, NULL, v2->epoint); val1 = (~(uval_t)0) >> 1; large=1;}
                else if (t2==T_SINT) val1 = ( val1 % val2); else val1 = ( (uval_t)val1 % (uval_t)val2); break;
            case '+': val1 = ( val1 +  val2);break;
            case '-': val1 = ( val1 -  val2);break;
            case '&': val1 = ( val1 &  val2);break;
            case '|': val1 = ( val1 |  val2);break;
            case '^': val1 = ( val1 ^  val2);break;
            case 'm':
                if (val2 >= (ival_t)sizeof(val1)*8 || val2 <= -(ival_t)sizeof(val1)*8) val1=0;
                else val1 = (val2 > 0) ? (val1 << val2) : (ival_t)((uval_t)val1 >> (-val2));
                break;
            case 'D': 
                if (t1 == T_SINT) {
                    if (val2 >= (ival_t)sizeof(val1)*8) val1 = (val1 > 0) ? 0 : -1;
                    if (val2 <= -(ival_t)sizeof(val1)*8) val1 = 0;
                    else if (val1 >= 0) val1 = (val2 > 0) ? (val1 >> val2) : (val1 << (-val2));
                    else val1 = ~((val2 > 0) ? ((~val1) >> val2) : ((~val1) << (-val2)));
                    break;
                }
            case 'd': 
                if (val2 >= (ival_t)sizeof(val1)*8 || val2 <= -(ival_t)sizeof(val1)*8) val1=0;
                else val1 = (val2 > 0) ? (ival_t)((uval_t)val1 >> val2) : (val1 << (-val2));
                break;
            case 'E': 
                {
                    ival_t res = 1;

                    if (val2 < 0) {
                        if (!val1) {err_msg2(ERROR_DIVISION_BY_Z, NULL, v2->epoint); res = (~(uval_t)0) >> 1; large=1;}
                        else res = 0;
                    } else {
                        while (val2) {
                            if (val2 & 1) res *= val1;
                            val1 *= val1;
                            val2 >>= 1;
                        }
                    }
                    val1 = res;
                }
                break;
            default: err_msg_wrong_type(v1->val.type, v1->epoint); v1->val.type = T_NONE;continue;
            }
            if (t1 == T_SINT) set_int(&v1->val, val1); else {set_uint(&v1->val, val1); v1->val.type = t1;}
            continue;
        }

        if (t1 == T_TSTR) t1 = T_STR;
        if (t2 == T_TSTR) t2 = T_STR;
        if (t1 == T_STR && t2 == T_STR) {
            switch (ch) {
                case '=':
                    val = (v1->val.u.str.len == v2->val.u.str.len) && !memcmp(v1->val.u.str.data, v2->val.u.str.data, v1->val.u.str.len);
                strcomp:
                    if (v1->val.type == T_TSTR) free(v1->val.u.str.data);
                    if (v2->val.type == T_TSTR) free(v2->val.u.str.data);
                    v1->val.type = T_UINT; v1->val.u.num.val = val; v1->val.u.num.len = 1;
                    continue;
                case 'o':
                    val = (v1->val.u.str.len != v2->val.u.str.len) || memcmp(v1->val.u.str.data, v2->val.u.str.data, v1->val.u.str.len);
                    goto strcomp;
                case '<':
                    val = memcmp(v1->val.u.str.data, v2->val.u.str.data, (v1->val.u.str.len < v2->val.u.str.len) ? v1->val.u.str.len:v2->val.u.str.len);
                    if (!val) val = v1->val.u.str.len < v2->val.u.str.len; else val = val < 0;
                    goto strcomp;
                case '>':
                    val = memcmp(v1->val.u.str.data, v2->val.u.str.data, (v1->val.u.str.len < v2->val.u.str.len) ? v1->val.u.str.len:v2->val.u.str.len);
                    if (!val) val = v1->val.u.str.len > v2->val.u.str.len; else val = val > 0;
                    goto strcomp;
                case 's':
                    val = memcmp(v1->val.u.str.data, v2->val.u.str.data, (v1->val.u.str.len < v2->val.u.str.len) ? v1->val.u.str.len:v2->val.u.str.len);
                    if (!val) val = v1->val.u.str.len <= v2->val.u.str.len; else val = val <= 0;
                    goto strcomp;
                case 'g':
                    val = memcmp(v1->val.u.str.data, v2->val.u.str.data, (v1->val.u.str.len < v2->val.u.str.len) ? v1->val.u.str.len:v2->val.u.str.len);
                    if (!val) val = v1->val.u.str.len >= v2->val.u.str.len; else val = val >= 0;
                    goto strcomp;
                case 'A': val = (v1->val.u.str.len && v2->val.u.str.len); goto strcomp;
                case 'O': val = (v1->val.u.str.len || v2->val.u.str.len); goto strcomp;
                case 'X': val = (!v1->val.u.str.len ^ !v2->val.u.str.len); goto strcomp;
                case '*': 
                case '/':
                case '%':
                case '+':
                case '-':
                case '&':
                case '|':
                case '^':
                case 'm':
                case 'D': 
                case 'd': 
                case 'E': 
                    if (str_to_num(&v1->val)) {
                        err_msg2(ERROR_CONSTNT_LARGE, NULL, v1->epoint); large=1;
                    }
                    if (str_to_num(&v2->val)) {
                        err_msg2(ERROR_CONSTNT_LARGE, NULL, v2->epoint); large=1;
                    }
                    t1 = v1->val.type;
                    t2 = v2->val.type;
                    goto strretr;
                default: err_msg_wrong_type(v1->val.type, v1->epoint); goto errtype;
            }
        }
#if 0
        if (t1 == T_STR && (t2 == T_SINT || t2 == T_UINT || t2 == T_NUM)) {
            if (ch=='I') {
                val=0;
                if (v2->val.u.num.val >= 0) {
                    if ((uval_t)v2->val.u.num.val < v1->val.u.str.len) val = v1->val.u.str.data[v2->val.u.num.val];
                    else err_msg2(ERROR_CONSTNT_LARGE, NULL, v2->epoint);
                } else {
                    if ((uval_t)-v2->val.u.num.val <= v1->val.u.str.len) val = v1->val.u.str.data[v1->val.u.str.len + v2->val.u.num.val];
                    else err_msg2(ERROR_CONSTNT_LARGE, NULL, v2->epoint);
                }
                if (v1->val.type == T_TSTR) free(v1->val.u.str.data);
                v1->val.type = T_CHR; v1->val.u.num.val = val;v1->val.u.num.len = 1;continue;
            }
        }
#endif
        if (t2 == T_UNDEF) err_msg_wrong_type(v2->val.type, v2->epoint); 
        else err_msg_wrong_type(v1->val.type, v1->epoint);
        goto errtype;
    }
    if (large) cd=0;
    values_len = vsp;
    return cd;
}

void free_values(void) {
    free(values);
	values = NULL;
	values_size=0;
	values_len=0;
	values_p=0;
}
