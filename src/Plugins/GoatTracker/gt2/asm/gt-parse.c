/*
 * Copyright (c) 2005 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

#include "gt-parse.h"
#include "gt-asmtab.h"
#include "gt-log.h"
#include "gt-chnkpool.h"
#include "gt-namedbuf.h"
#include "gt-pc.h"

#include <stdlib.h>


static struct chunkpool s_atom_pool[1];
static struct chunkpool s_vec_pool[1];
static struct vec s_sym_table[1];
static const char *s_macro_name;

struct sym_entry
{
    const char *symbol;
    struct expr *expr;
};

static int sym_entry_cmp(const void *a, const void *b)
{
    struct sym_entry *sym_a;
    struct sym_entry *sym_b;
    int val;

    sym_a = (struct sym_entry*)a;
    sym_b = (struct sym_entry*)b;

    val = strcmp(sym_a->symbol, sym_b->symbol);

    return val;
}

void scanner_init(void);
void scanner_free(void);

void parse_init()
{
    scanner_init();
    chunkpool_init(s_atom_pool, sizeof(struct atom));
    chunkpool_init(s_vec_pool, sizeof(struct vec));
    expr_init();
    vec_init(s_sym_table, sizeof(struct sym_entry));
    pc_unset();
    named_buffer_init();
}

static void free_vec_pool(struct vec *v)
{
    vec_free(v, NULL);
}

void parse_free()
{
    named_buffer_free();
    chunkpool_free(s_atom_pool);
    chunkpool_free2(s_vec_pool, (cb_free*)free_vec_pool);
    expr_free();
    vec_free(s_sym_table, NULL);
    scanner_free();
}

int is_valid_i8(i32 value)
{
    return (value >= -128 && value <= 127);
}

int is_valid_u8(i32 value)
{
    return (value >= 0 && value <= 255);
}

int is_valid_ui8(i32 value)
{
    return (value >= -128 && value <= 255);
}

int is_valid_u16(i32 value)
{
    return (value >= 0 && value <= 65535);
}

int is_valid_ui16(i32 value)
{
    return (value >= -32768 && value <= 65535);
}

void dump_sym_entry(int level, struct sym_entry *se)
{
    LOG(level, ("sym_entry 0x%08X symbol %s, expr 0x%08X\n",
                (u32)se, se->symbol, (u32)se->expr));
}

struct expr *new_is_defined(const char *symbol)
{
    struct expr *val;
    struct sym_entry e[1];
    int pos;
    int expr_val = 0;

    e->symbol = symbol;
    pos = vec_find(s_sym_table, sym_entry_cmp, e);
    if(pos >= 0)
    {
        /* found */
        expr_val = 1;
    }
    val = new_expr_number(expr_val);
    return val;
}

void new_symbol_expr(const char *symbol, struct expr *arg)
{
    struct sym_entry e[1];
    struct sym_entry *se;
    int pos;

    e->symbol = symbol;
    pos = vec_find(s_sym_table, sym_entry_cmp, e);
    if(pos > -1)
    {
        /* error, symbol redefinition not allowed */
        LOG(LOG_ERROR, ("not allowed to redefine symbol %s\n", symbol));
        exit(1);
    }
    if(pos == -1)
    {
        /* error, find failed */
        LOG(LOG_ERROR, ("new_symbol_expr: vec_find() internal error\n"));
        exit(1);
    }
    e->expr = arg;

    se = vec_insert(s_sym_table, -(pos + 2), e);
    LOG(LOG_DEBUG, ("creating symdef: "));
    dump_sym_entry(LOG_DEBUG, se);
}

void new_symbol(const char *symbol, i32 value)
{
    struct expr *e;

    e = new_expr_number(value);
    new_symbol_expr(symbol, e);
}

const char *find_symref(const char *symbol, struct expr **expp)
{
    struct sym_entry e[1];
    struct sym_entry *ep;
    struct expr *exp;
    int pos;
    const char *p;

    p = NULL;
    e->symbol = symbol;
    pos = vec_find(s_sym_table, sym_entry_cmp, e);
    if(pos < -1)
    {
        static char buf[1024];
        /* error, symbol not found */
        sprintf(buf, "symbol %s not found", symbol);
        p = buf;
        LOG(LOG_DEBUG, ("%s\n", p));
        return p;
    }
    if(pos == -1)
    {
        /* error, find failed */
        LOG(LOG_ERROR, ("find_symref: vec_find() internal error\n"));
        exit(-1);
    }
    ep = vec_get(s_sym_table, pos);
    exp = ep->expr;

    LOG(LOG_DEBUG, ("found: "));
    dump_sym_entry(LOG_DEBUG, ep);

    if(expp != NULL)
    {
        *expp = exp;
    }

    return p;
}

void gtnew_label(const char *label)
{
    struct sym_entry e[1];
    struct sym_entry *se;
    int pos;

    e->symbol = label;
    pos = vec_find(s_sym_table, sym_entry_cmp, e);
    if(pos > -1)
    {
        /* error, symbol redefinition not allowed */
        LOG(LOG_ERROR, ("not allowed to redefine label %s\n", label));
        exit(1);
    }
    if(pos == -1)
    {
        /* error, find failed */
        LOG(LOG_ERROR, ("new_label: vec_find() internal error\n"));
        exit(1);
    }

    e->expr = pc_get();

    se = vec_insert(s_sym_table, -(pos + 2), e);
    LOG(LOG_DEBUG, ("creating label: "));
    dump_sym_entry(LOG_DEBUG, se);
}

static void dump_sym_table(int level, struct vec *v)
{
    struct vec_iterator i[1];
    struct sym_entry *se;

    vec_get_iterator(v, i);
    while((se = vec_iterator_next(i)) != NULL)
    {
        LOG(level, ("sym_table: %s\n", se->symbol));
    }
}

static const char *resolve_expr2(struct expr *e, i32 *valp)
{
    struct expr *e2;
    i32 value;
    i32 value2;
    const char *p;

    p = NULL;
    LOG(LOG_DEBUG, ("resolve_expr: "));

    expr_dump(LOG_DEBUG, e);

    switch (e->expr_op)
    {
    case NUMBER:
        /* we are already resolved */
        value = e->type.number;
        break;
    case vNEG:
        p = resolve_expr2(e->type.arg1, &value);
        if(p != NULL) break;
        value = -value;
        break;
    case LNOT:
        p = resolve_expr2(e->type.arg1, &value);
        if(p != NULL) break;
        value = !value;
        break;
    case SYMBOL:
        p = find_symref(e->type.symref, &e2);
        if(p != NULL) break;
        if(e2 == NULL)
        {
            static char buf[1024];
            /* error, symbol not found */
            sprintf(buf, "symbol %s has no value.", e->type.symref);
            p = buf;
            LOG(LOG_DEBUG, ("%s\n", p));
            break;
        }
        p = resolve_expr2(e2, &value);
        break;
    default:
        LOG(LOG_DEBUG, ("binary op %d\n", e->expr_op));

        p = resolve_expr2(e->type.arg1, &value);
        if(p != NULL) break;

        /* short circuit the logical operators */
        if(e->expr_op == LOR)
        {
            value = (value != 0);
            if(value) break;
        }
        else if(e->expr_op == LAND)
        {
            value = (value != 0);
            if(!value) break;
        }

        p = resolve_expr2(e->expr_arg2, &value2);
        if(p != NULL) break;

        switch(e->expr_op)
        {
        case MINUS:
            value -= value2;
            break;
        case PLUS:
            value += value2;
            break;
        case MULT:
            value *= value2;
            break;
        case DIV:
            value /= value2;
            break;
        case MOD:
            value %= value2;
            break;
        case LT:
            value = (value < value2);
            break;
        case GT:
            value = (value > value2);
            break;
        case EQ:
            value = (value == value2);
            break;
        case NEQ:
            value = (value != value2);
            break;
        case LOR:
            value = (value2 != 0);
            break;
        case LAND:
            value = (value2 != 0);
            break;
        default:
            LOG(LOG_ERROR, ("unsupported op %d\n", e->expr_op));
            exit(1);
        }
    }
    if(p == NULL)
    {
        if(e->expr_op != NUMBER)
        {
            /* shortcut future recursion */
            e->expr_op = NUMBER;
            e->type.number = value;
        }
        if(valp != NULL)
        {
            *valp = value;
        }
    }

    return p;
}

static i32 resolve_expr(struct expr *e)
{
    i32 val;
    const char *p;

    p = resolve_expr2(e, &val);
    if(p != NULL)
    {
        LOG(LOG_ERROR, ("%s\n", p));
        exit(-1);
    }
    return val;
}

struct expr *new_expr_incword(const char *name, struct expr *skip)
{
    i32 word;
    i32 offset;
    long length;
    struct membuf *in;
    struct expr *expr;
    unsigned char *p;

    offset = resolve_expr(skip);
    in = get_named_buffer(name);
    length = membuf_memlen(in);
    if(offset < 0)
    {
        offset += length;
    }
    if(offset < 0 || offset > length - 2)
    {
        LOG(LOG_ERROR,
            ("Can't read word from offset %d in file \"%s\".\n",
             offset, name));
        exit(-1);
    }
    p = membuf_get(in);
    p += offset;
    word = *p++;
    word |= *p++ << 8;

    expr = new_expr_number(word);
    return expr;
}

void set_org(struct expr *arg)
{
    /* org assembler directive */
    pc_set_expr(arg);
    LOG(LOG_DEBUG, ("setting .org to ???\n"));
    return;
}

void push_macro_state(const char *name)
{
    s_macro_name = name;
    push_state_macro = 1;
    new_named_buffer(name);
}

void macro_append(const char *text)
{
    struct membuf *mb;

    LOG(LOG_DEBUG, ("appending >>%s<< to macro\n", text));

    mb = get_named_buffer(s_macro_name);
    membuf_append(mb, text, strlen(text));
}

void push_if_state(struct expr *arg)
{
    int val;
    LOG(LOG_DEBUG, ("resolving if expression\n"));
    val = resolve_expr(arg);
    LOG(LOG_DEBUG, ("if expr resolved to %d\n", val));
    if(val)
    {
        push_state_init = 1;
    }
    else
    {
        push_state_skip = 1;
    }
}

struct atom *new_op(u8 op_code, u8 atom_op_type, struct expr *op_arg)
{
    struct atom *atom;

    atom = chunkpool_malloc(s_atom_pool);
    atom->type = atom_op_type;
    atom->u.op.code = op_code;
    atom->u.op.arg = op_arg;

    switch(atom_op_type)
    {
    case ATOM_TYPE_OP_ARG_NONE:
        pc_add(1);
        break;
    case ATOM_TYPE_OP_ARG_U8:
        pc_add(2);
        break;
    case ATOM_TYPE_OP_ARG_U16:
        pc_add(3);
        break;
    case ATOM_TYPE_OP_ARG_I8:
        pc_add(2);
        atom->u.op.arg = new_expr_op2(MINUS, atom->u.op.arg, pc_get());
        break;
    case ATOM_TYPE_OP_ARG_UI8:
        pc_add(2);
        break;
    default:
        LOG(LOG_ERROR, ("invalid op arg range %d\n", atom_op_type));
        exit(1);
    }
    pc_dump(LOG_DEBUG);

    return atom;
}

struct atom *new_op0(u8 op_code)
{
    struct atom *atom;
    atom = new_op(op_code, ATOM_TYPE_OP_ARG_NONE, NULL);
    return atom;
}

struct atom *new_exprs(struct expr *arg)
{
    struct atom *atom;

    atom = chunkpool_malloc(s_atom_pool);
    atom->type = ATOM_TYPE_EXPRS;
    atom->u.exprs = chunkpool_malloc(s_vec_pool);
    vec_init(atom->u.exprs, sizeof(struct expr*));
    exprs_add(atom, arg);
    return atom;
}

struct atom *exprs_add(struct atom *atom, struct expr *arg)
{
    if(atom->type != ATOM_TYPE_EXPRS)
    {
        LOG(LOG_ERROR, ("can't add expr to atom of type %d\n", atom->type));
        exit(1);
    }
    vec_push(atom->u.exprs, &arg);
    return atom;
}

struct atom *exprs_to_byte_exprs(struct atom *atom)
{
    if(atom->type != ATOM_TYPE_EXPRS)
    {
        LOG(LOG_ERROR, ("can't convert atom of type %d to byte exprs.\n",
                        atom->type));
        exit(1);
    }
    atom->type = ATOM_TYPE_BYTE_EXPRS;

    pc_add(vec_count(atom->u.exprs));
    return atom;
}

struct atom *exprs_to_word_exprs(struct atom *atom)
{
    if(atom->type != ATOM_TYPE_EXPRS)
    {
        LOG(LOG_ERROR, ("can't convert exprs of type %d to word exprs.\n",
                        atom->type));
        exit(1);
    }
    atom->type = ATOM_TYPE_WORD_EXPRS;

    pc_add(vec_count(atom->u.exprs) * 2);
    return atom;
}

struct atom *new_res(struct expr *len, struct expr *value)
{
    struct atom *atom;

    atom = chunkpool_malloc(s_atom_pool);
    atom->type = ATOM_TYPE_RES;
    atom->u.res.length = len;
    atom->u.res.value = value;

    pc_add_expr(len);
    return atom;
}

struct atom *new_incbin(const char *name, struct expr *skip, struct expr *len)
{
    struct atom *atom;
    long length;
    i32 len32;
    i32 skip32;
    struct membuf *in;

    /* find out how long the file is */
    in = get_named_buffer(name);
    length = membuf_memlen(in);

    skip32 = 0;
    if(skip != NULL)
    {
        skip32 = resolve_expr(skip);
    }
    if(skip32 < 0)
    {
        skip32 += length;
    }
    if(skip32 < 0 || skip32 > length)
    {
        LOG(LOG_ERROR,
            ("Can't read from offset %d in file \"%s\".\n", skip32, name));
        exit(-1);
    }
    length -= skip32;

    len32 = 0;
    if(len != NULL)
    {
        len32 = resolve_expr(len);
    }
    if(len32 < 0)
    {
        len32 += length;
    }
    if(len32 < 0 || len32 > length)
    {
        LOG(LOG_ERROR,
            ("Can't read %d bytes from offset %d from file \"%s\".\n",
             len32, skip32, name));
        exit(-1);
    }

    atom = chunkpool_malloc(s_atom_pool);
    atom->type = ATOM_TYPE_BUFFER;
    atom->u.buffer.name = name;
    atom->u.buffer.length = len32;
    atom->u.buffer.skip = skip32;

    if(len != NULL)
    {
        pc_add(len32);
    }
    return atom;
}


void asm_error(const char *msg)
{
    LOG(LOG_ERROR, ("Error: %s\n", msg));
    exit(1);
}

void asm_echo(const char *msg)
{
    fprintf(stdout, "%s\n", msg);
}

void asm_include(const char *msg)
{
    struct membuf *src;

    src = get_named_buffer(msg);
    asm_src_buffer_push(src);
}

void symbol_dump_resolved(int level, const char *symbol)
{
    i32 value;
    struct expr *e;
    const char *p;
    p = find_symref(symbol, &e);
    if(p == NULL)
    {
        if(e != NULL)
        {
            value = resolve_expr(e);
            LOG(level, ("symbol \"%s\" resolves to %d ($%04X)\n",
                        symbol, value, value));
        }
        else
        {
            LOG(level, ("symbol \"%s\" is defined but has no value\n",
                        symbol));
        }
    }
    else
    {
        LOG(level, ("symbol \"%s\" not found\n", symbol));
    }
}

void output_atoms(struct membuf *out, struct vec *atoms)
{
    struct vec_iterator i[1];
    struct vec_iterator i2[1];
    struct atom **atomp;
    struct atom *atom;
    struct expr **exprp;
    struct expr *expr;
    struct membuf *in;
    const char *p;
    i32 value;
    i32 value2;

    dump_sym_table(LOG_DEBUG, s_sym_table);

    vec_get_iterator(atoms, i);
    while((atomp = vec_iterator_next(i)) != NULL)
    {
        atom = *atomp;

        LOG(LOG_DEBUG, ("yadda\n"));

        switch(atom->type)
        {
        case ATOM_TYPE_OP_ARG_NONE:
            LOG(LOG_DEBUG, ("output: $%02X\n", atom->u.op.code));
            membuf_append_char(out, atom->u.op.code);
            break;
        case ATOM_TYPE_OP_ARG_U8:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_u8(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, atom));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: $%02X $%02X\n",
                            atom->u.op.code, value & 255));
            membuf_append_char(out, atom->u.op.code);
            membuf_append_char(out, value);
            break;
        case ATOM_TYPE_OP_ARG_I8:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_i8(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, atom));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: $%02X $%02X\n",
                            atom->u.op.code, value & 255));
            membuf_append_char(out, atom->u.op.code);
            membuf_append_char(out, value);
            break;
        case ATOM_TYPE_OP_ARG_UI8:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_ui8(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, atom));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: $%02X $%02X\n",
                            atom->u.op.code, value & 255));
            membuf_append_char(out, atom->u.op.code);
            membuf_append_char(out, value);
            break;
        case ATOM_TYPE_OP_ARG_U16:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_u16(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, atom));
                exit(1);
            }
            value2 = value / 256;
            value = value % 256;
            LOG(LOG_DEBUG, ("output: $%02X $%02X $%02X\n",
                            atom->u.op.code,
                            value, value2));
            membuf_append_char(out, atom->u.op.code);
            membuf_append_char(out, value);
            membuf_append_char(out, value2);
            break;
        case ATOM_TYPE_RES:
            /* reserve memory statement */
            value = resolve_expr(atom->u.res.length);
            if(!is_valid_u16(value))
            {
                LOG(LOG_ERROR, ("length %d for .res(length, value) "
                                "is out of range\n", value));
                exit(1);
            }
            value2 = resolve_expr(atom->u.res.value);
            if(!is_valid_ui8(value2))
            {
                LOG(LOG_ERROR, ("value %d for .res(length, value) "
                                "is out of range\n", value));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: .RES %d, %d\n", value, value2));
            while(--value >= 0)
            {
                membuf_append_char(out, value2);
            }
            break;
        case ATOM_TYPE_BUFFER:
            /* include binary file statement */
            value = atom->u.buffer.skip;
            if(!is_valid_u16(value))
            {
                LOG(LOG_ERROR, ("value %d for .res(length, value) "
                                "is out of range\n", value));
                exit(1);
            }
            value2 = atom->u.buffer.length;
            if(!is_valid_u16(value2))
            {
                LOG(LOG_ERROR, ("length %d for .incbin(name, skip, length) "
                                "is out of range\n", value2));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: .INCBIN \"%s\", %d, %d\n",
                            atom->u.buffer.name, value, value2));
            in = get_named_buffer(atom->u.buffer.name);
            p = membuf_get(in);
            p += value;
            while(--value2 >= 0)
            {
                membuf_append_char(out, *p++);
            }
            break;
        case ATOM_TYPE_WORD_EXPRS:
            vec_get_iterator(atom->u.exprs, i2);
            while((exprp = vec_iterator_next(i2)) != NULL)
            {
                expr = *exprp;
                value = resolve_expr(expr);
                if(!is_valid_ui16(value))
                {
                    LOG(LOG_ERROR, ("value %d for .word(value, ...) "
                                    "is out of range\n", value));
                }
                value2 = value / 256;
                value = value % 256;
                membuf_append_char(out, value);
                membuf_append_char(out, value2);
            }
            LOG(LOG_DEBUG, ("output: %d words\n", vec_count(atom->u.exprs)));
            break;
        case ATOM_TYPE_BYTE_EXPRS:
            vec_get_iterator(atom->u.exprs, i2);
            while((exprp = vec_iterator_next(i2)) != NULL)
            {
                expr = *exprp;
                value = resolve_expr(expr);
                if(!is_valid_ui8(value))
                {
                    LOG(LOG_ERROR, ("value %d for .byte(value, ...) "
                                    "is out of range\n", value));
                }
                membuf_append_char(out, value);
            }
            LOG(LOG_DEBUG, ("output: %d bytes\n", vec_count(atom->u.exprs)));
            break;
        default:
            LOG(LOG_ERROR, ("invalid atom_type %d @%p\n",
                            atom->type, atom));
            exit(1);
        }
    }
}
