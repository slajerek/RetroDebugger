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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "64tass-error.h"
#include "64tass-misc.h"

unsigned int errors=0,conderrors=0,warnings=0;

static struct {
    size_t p;
    size_t len;
    struct {
        line_t line;
        const char *name;
    } *data;
} file_list = {0,0,NULL};

static struct {
    size_t p;
    size_t len;
    char *data;
} error_list = {0,0,NULL};

void enterfile(const char *name, line_t line) {

    if (file_list.p >= file_list.len)
	{
        file_list.len += 16;
		if (file_list.data == NULL)
		{
			file_list.data = malloc(file_list.len * sizeof(*file_list.data));
		}
		else
		{
			file_list.data = realloc(file_list.data, file_list.len * sizeof(*file_list.data));
		}
        if (!file_list.data) {fputs("Out of memory\n", stderr);exit(1);}
    }
    file_list.data[file_list.p].name=name;
    file_list.data[file_list.p].line=line;
    file_list.p++;
}

void exitfile(void) {
    if (file_list.p) file_list.p--;
}

static void adderror(const char *s) {
    unsigned int len;

    len = strlen(s) + 1;
    if (len + error_list.p > error_list.len) {
        error_list.len += (len > 0x200) ? len : 0x200;
        error_list.data = realloc(error_list.data, error_list.len);
        if (!error_list.data) {fputs("Out of memory\n", stderr);exit(1);}
    }
    memcpy(error_list.data + error_list.p, s, len);
    error_list.p += len - 1;
}

static const char *terr_warning[]={
    "Top of memory excedeed",
    "Possibly incorrectly used A",
    "Memory bank excedeed",
    "Possible jmp ($xxff) bug",
    "Long branch used",
    "Directive ignored",
    "Label not on left side",
};
static const char *terr_error[]={
    "Double defined %s",
    "Not defined %s",
    "Extra characters on line",
    "Constant too large",
    "General syntax",
    "%s expected",
    "Expression syntax",
    "Branch too far",
    "Missing argument",
    "Illegal operand",
    "Requirements not met: %s",
    "Conflict: %s",
    "Division by zero",
    "Wrong type %s",
    "Unknown character $%02x",
    "Not allowed here: %s",
};
static const char *terr_fatal[]={
    "Can't locate file: %s\n",
    "Error reading file: %s\n",
    "Out of memory\n",
    "Can't write object file: %s\n",
    "Line too long\n",
    "Can't write listing file: %s\n",
    "Can't write label file: %s\n",
    "%s\n",
    "File recursion\n",
    "Macro recursion too deep\n",
    "Unknown CPU: %s\n",
    "Unknown option: %s\n",
    "Too many passes\n",
    "Too many errors\n"
};

void err_msg2(enum errors_e no, const char* prm, unsigned int lpoint) {
    char line[linelength];
    unsigned int i;

    if (errors+conderrors==99 && no>=0x40) no=ERROR__TOO_MANY_ERR;

    if (!arguments.warning && no<0x40) {
        warnings++;
        return;
    }

    if (file_list.p) {
        adderror(file_list.data[file_list.p - 1].name);
	sprintf(line,":%" PRIuline ":%u: ", sline, lpoint + 1); adderror(line);
    }

    for (i = file_list.p; i > 1; i--) {
        adderror("(");
        adderror(file_list.data[i - 2].name);
        sprintf(line,":%" PRIuline ") ", file_list.data[i - 1].line);
        adderror(line);
    }

    if (no<0x40) {
        adderror("warning: ");
        if (no == ERROR_WUSER_DEFINED) adderror(prm); else adderror(terr_warning[no]);
        warnings++;
    }
    else if (no<0x80) {
        if (no==ERROR____PAGE_ERROR) {
            adderror("Page error at $"); 
            sprintf(line,"%06" PRIaddress,(address_t)prm); adderror(line);
            conderrors++;
        }
        else if (no==ERROR__BRANCH_CROSS) {
            adderror("Branch crosses page");
            conderrors++;
        }
        else {
            snprintf(line,linelength,terr_error[no & 63],prm);
            if (no==ERROR_BRANCH_TOOFAR || no==ERROR_CONSTNT_LARGE) conderrors++;
            else errors++;
            adderror(line);
        }
    }
    else {
        adderror("[**Fatal**] ");
        snprintf(line,linelength,terr_fatal[no & 63],prm);
        adderror(line);
        if (no==ERROR__USER_DEFINED) conderrors++; else
        {
            errors++;
            status();exit(1);
        }
    }
    adderror("\n");
}

void err_msg(enum errors_e no, const char* prm) {
    err_msg2(no,prm, lpoint);
}

void err_msg_wrong_type(enum type_e type, unsigned int epoint) {
    const char *name = NULL;
    switch (type) {
    case T_SINT: name = "<sint>";break;
    case T_UINT: name = "<uint>";break;
    case T_NUM: name = "<num>";break;
    case T_TSTR:
    case T_STR: name = "<string>";break;
    case T_UNDEF: err_msg2(ERROR___NOT_DEFINED, "", epoint);return;
    case T_IDENT: name = "<ident>";break;
    case T_IDENTREF: name = "<identref>";break;
    case T_NONE: name = "<none>";break;
    case T_BACKR: name = "<backr>";break;
    case T_FORWR: name = "<forwr>";break;
    case T_OPER: name = "<operator>";break;
    case T_FUNC: name = "<function>";break;
    case T_GAP: name = "<uninit>";break;
    }
    err_msg2(ERROR____WRONG_TYPE, name, epoint);
}

void freeerrorlist(int print) {
    if (print) {
        fwrite(error_list.data, error_list.p, 1, stderr);
    }
    error_list.p = 0;
}

void err_destroy(void) {
    free(file_list.data);
    free(error_list.data);
	file_list.data = NULL;
	file_list.p = 0;
	file_list.len = 0;
	error_list.p = 0;
	error_list.len = 0;
}
