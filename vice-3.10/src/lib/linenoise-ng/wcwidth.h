/** \file   wcwidth.h
 *
 * \brief   Header file for wcwidth.c
 *
 * This header fixes some prototype warnings.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

#ifndef LINENOISE_NG_WCWIDTH_H
#define LINENOISE_NG_WCWIDTH_H

#include <stddef.h>
#include <stdint.h>

namespace linenoise_ng {

int mk_wcwidth(char32_t ucs);
int mk_wcswidth(const char32_t* pwcs, size_t n);
int mk_wcwidth_cjk(wchar_t ucs);
int mk_wcswidth_cjk(const wchar_t *pwcs, size_t n);

}

#endif
