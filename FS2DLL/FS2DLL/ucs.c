/*
 * ucs.c  -- Universal Character Set
 *
 * $Id: ucs.c,v 1.1 2004/06/01 09:35:50 hos Exp $
 *
 */


#include <stdlib.h>
#include "util.h"


/* length of ucs-4 string */
static int u4s_len(const ucs4_t *u4s)
{
    int i;

    for(i = 0; u4s[i] != 0; i++) ;

    return i;
}

/* length of wchar string */
static int wcs_len(const wchar_t *wcs)
{
    int i;

    for(i = 0; wcs[i] != 0; i++) ;

    return i;
}


/* ucs-4 string from utf-8 string (without alloc) */
static int u4s_from_u8s(ucs4_t *u4s,
                        const unsigned char *u8s,
                        const unsigned char **rest_u8s)
{
    int i, j, n;
    int s, ss;
    ucs4_t c;
    const unsigned char *last;

    n = 0;
    s = ss = 0;
    c = 0;
    last = u8s;
    for(i = 0; u8s[i] != 0; i++) {
        if(s == 0) {
            static struct {
                unsigned char mask;
                unsigned char val;
                int trail;
            } state_map[] = {
                {0x80, 0x00, 0},
                {0xe0, 0xc0, 1},
                {0xf0, 0xe0, 2},
                {0xf8, 0xf0, 3},
                {0xfc, 0xf8, 4},
                {0xfe, 0xfc, 5},

                {0x00, 0x00, 0}
            };

            for(j = 0; state_map[j].mask != 0; j++) {
                if((u8s[i] & state_map[j].mask) == state_map[j].val) {
                    s = ss = state_map[j].trail;
                    c = (u8s[i] & ~state_map[j].mask);
                    break;
                }
            }
            if(state_map[j].mask == 0) {
                /* invalid lead char */
                s = 0;
                continue;
            }
        } else {
            if((u8s[i] & 0xc0) != 0x80) {
                /* invalid sequence */
                s = 0;
                continue;
            }

            s -= 1;
            c = (c << 6) | (u8s[i] & ~0xc0);
        }

        if(s == 0) {
            last = u8s + i + 1;

            if(ss != 0 && c == 0) {
                /* invalid sequence */
                continue;
            }

            if(u4s != NULL) {
                u4s[n] = c;
            }
            n += 1;
        }
    }

    if(u4s != NULL) {
        u4s[n] = 0;
    }

    if(rest_u8s != NULL) {
        *rest_u8s = last;
    }

    return n;
}

/* utf-8 string from ucs-4 string */
static int u8s_from_u4s(unsigned char *u8s, const ucs4_t *u4s)
{
    int i, n;

    n = 0;
    for(i = 0; u4s[i] != 0; i++) {
        if(u4s[i] < 0x80) {
            if(u8s != NULL) {
                u8s[n + 0] = ((u4s[i] >> (6 * 0)) & ~0x80) | 0x00;
            }
            n += 1;
        } else if(u4s[i] < 0x800) {
            if(u8s != NULL) {
                u8s[n + 0] = ((u4s[i] >> (6 * 1)) & ~0xe0) | 0xc0;
                u8s[n + 1] = ((u4s[i] >> (6 * 0)) & ~0xc0) | 0x80;
            }
            n += 2;
        } else if(u4s[i] < 0x10000) {
            if(u8s != NULL) {
                u8s[n + 0] = ((u4s[i] >> (6 * 2)) & ~0xf0) | 0xe0;
                u8s[n + 1] = ((u4s[i] >> (6 * 1)) & ~0xc0) | 0x80;
                u8s[n + 2] = ((u4s[i] >> (6 * 0)) & ~0xc0) | 0x80;
            }
            n += 3;
        } else if(u4s[i] < 0x200000) {
            if(u8s != NULL) {
                u8s[n + 0] = ((u4s[i] >> (6 * 3)) & ~0xf8) | 0xf0;
                u8s[n + 1] = ((u4s[i] >> (6 * 2)) & ~0xc0) | 0x80;
                u8s[n + 2] = ((u4s[i] >> (6 * 1)) & ~0xc0) | 0x80;
                u8s[n + 3] = ((u4s[i] >> (6 * 0)) & ~0xc0) | 0x80;
            }
            n += 4;
        } else if(u4s[i] < 0x4000000) {
            if(u8s != NULL) {
                u8s[n + 0] = ((u4s[i] >> (6 * 4)) & ~0xfc) | 0xf8;
                u8s[n + 1] = ((u4s[i] >> (6 * 3)) & ~0xc0) | 0x80;
                u8s[n + 2] = ((u4s[i] >> (6 * 2)) & ~0xc0) | 0x80;
                u8s[n + 3] = ((u4s[i] >> (6 * 1)) & ~0xc0) | 0x80;
                u8s[n + 4] = ((u4s[i] >> (6 * 0)) & ~0xc0) | 0x80;
            }
            n += 5;
        } else {
            if(u8s != NULL) {
                u8s[n + 0] = ((u4s[i] >> (6 * 5)) & ~0xfe) | 0xfc;
                u8s[n + 1] = ((u4s[i] >> (6 * 4)) & ~0xc0) | 0x80;
                u8s[n + 2] = ((u4s[i] >> (6 * 3)) & ~0xc0) | 0x80;
                u8s[n + 3] = ((u4s[i] >> (6 * 2)) & ~0xc0) | 0x80;
                u8s[n + 4] = ((u4s[i] >> (6 * 1)) & ~0xc0) | 0x80;
                u8s[n + 5] = ((u4s[i] >> (6 * 0)) & ~0xc0) | 0x80;
            }
            n += 6;
        }
    }

    if(u8s != NULL) {
        u8s[n] = 0;
    }

    return n;
}


/* wchar from ucs-4 */
wchar_t wc_from_u4(ucs4_t u4)
{
    return (wchar_t)u4;
}

/* ucs-4 from wchar */
ucs4_t u4_from_wc(wchar_t wc)
{
    return (ucs4_t)wc;
}


/* wchar string from ucs-4 string */
wchar_t *wcs_dup_from_u4s(const ucs4_t *u4s)
{
    wchar_t *wcs;
    int i;

    wcs = (wchar_t *)malloc(sizeof(wchar_t) * (u4s_len(u4s) + 1));
    if(wcs == NULL) {
        return NULL;
    }

    for(i = 0; u4s[i] != 0; i++) {
        wcs[i] = wc_from_u4(u4s[i]);
    }
    wcs[i] = 0;

    return wcs;
}

/* ucs-4 string from wchar string */
ucs4_t *u4s_dup_from_wcs(const wchar_t *wcs)
{
    ucs4_t *u4s;
    int i;

    u4s = (ucs4_t *)malloc(sizeof(ucs4_t) * (wcs_len(wcs) + 1));
    if(u4s == NULL) {
        return NULL;
    }

    for(i = 0; wcs[i] != 0; i++) {
        u4s[i] = u4_from_wc(wcs[i]);
    }
    u4s[i] = 0;

    return u4s;
}


/* ucs-4 string from utf-8 string */
ucs4_t *u4s_dup_from_u8s(const unsigned char *u8s,
                         const unsigned char **rest_u8s)
{
    ucs4_t *u4s;

    u4s = (ucs4_t *)
          malloc(sizeof(ucs4_t) * (u4s_from_u8s(NULL, u8s, NULL) + 1));
    if(u4s == NULL) {
        return NULL;
    }

    u4s_from_u8s(u4s, u8s, rest_u8s);

    return u4s;
}

/* utf-8 string from ucs-4 string */
unsigned char *u8s_dup_from_u4s(const ucs4_t *u4s)
{
    unsigned char *u8s;

    u8s = (unsigned char *)
          malloc(sizeof(unsigned char) * (u8s_from_u4s(NULL, u4s) + 1));
    if(u8s == NULL) {
        return NULL;
    }

    u8s_from_u4s(u8s, u4s);

    return u8s;
}


/* wchar string from utf-8 string */
wchar_t *wcs_dup_from_u8s(const unsigned char *u8s,
                          const unsigned char **rest_u8s)
{
    ucs4_t *u4s;
    wchar_t *wcs;

    u4s = u4s_dup_from_u8s(u8s, rest_u8s);
    if(u4s == NULL) {
        return NULL;
    }

    wcs = wcs_dup_from_u4s(u4s);

    free(u4s);

    return wcs;
}

/* utf-8 string from wchar string */
unsigned char *u8s_dup_from_wcs(const wchar_t *wcs)
{
    ucs4_t *u4s;
    unsigned char *u8s;

    u4s = u4s_dup_from_wcs(wcs);
    if(u4s == NULL) {
        return NULL;
    }

    u8s = u8s_dup_from_u4s(u4s);

    free(u4s);

    return u8s;
}
