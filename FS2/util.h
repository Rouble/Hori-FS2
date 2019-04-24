/*
 * util.h  -- definition of util.c
 *
 * $Id: util.h,v 1.4 2004/06/06 17:44:54 hos Exp $
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__


struct uint_ptr_pair {
    unsigned long key;
    void *ptr;
};

void *assq_pair(const struct uint_ptr_pair *pair,
                unsigned long key, void *default_ptr);
unsigned long rassq_pair(const struct uint_ptr_pair *pair,
                         void *ptr, unsigned long default_val);


#include <wchar.h>

typedef unsigned long ucs4_t;

wchar_t wc_from_u4(ucs4_t u4);
wchar_t *wcs_dup_from_u4s(const ucs4_t *u4s);
wchar_t *wcs_dup_from_u8s(const unsigned char *u8s,
                          const unsigned char **rest_u8s);

ucs4_t u4_from_wc(wchar_t wc);
ucs4_t *u4s_dup_from_wcs(const wchar_t *wcs);
ucs4_t *u4s_dup_from_u8s(const unsigned char *u8s,
                         const unsigned char **rest_u8s);

unsigned char *u8s_dup_from_u4s(const ucs4_t *u4s);
unsigned char *u8s_dup_from_wcs(const wchar_t *wcs);


#endif /* __UTIL_H__ */
