/*
 * read_s_exp.c  -- read s-expression
 *
 * $Id: read_s_exp.c,v 1.11 2004/06/10 08:42:54 hos Exp $
 *
 */


/*
 * This syntax is subset of R5RS external representations.
 *
 * <datum> = <boolean>
 *         | <number>
 *         | <symbol>
 *         | <string>
 *         | <list>
 * 
 * <boolean> = #t | #f
 * 
 * <number> = <number 2>
 *          | <number 8>
 *          | <number 10>
 *          | <number 16>
 * 
 * <number R> = <radix R> <integer R>
 * 
 * <integer R> = <sign> <digit R>+
 * 
 * <sign> = <empty> | + | -
 * 
 * <radix 2> = #b
 * <radix 8> = #o
 * <radix 10> = <empty> | #d
 * <radix 16> = #x
 * 
 * <digit 2> = 0 | 1
 * <digit 8> = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7
 * <digit 10> = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
 * <digit 16> = <digit 10> | a | b | c | e | f
 * 
 * <symbol> = <identifier>
 * 
 * <identifier> = <initial> <subsequent>*
 *              | <peculiar identifier>
 * 
 * <initial> = <letter> | <special initial>
 * <subsequent> = <initial> | <digit> | <special subsequent>
 * 
 * <letter> = a | b | c | ... | z
 * <special initial> = ! | $ | % | & | * | / | : | < | = | > | ? | ^ | _ | ~
 * <digit> = <digit 10>
 * <special subsequent> = + | - | . | @
 * <peculiar identifier> = + | - | ...
 * 
 * <string> = " <string element>* "
 * 
 * <string element> = <any character other than " or \> | \" | \\
 * 
 * <list> = ( <datum>* )
 *        | ( <datum>+ . <datum> )
 * 
 * <empty> = 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <wchar.h>
#include <stdarg.h>
#include <limits.h>

#include "s_exp.h"
#include "util.h"


#define INBUF_SIZE 64

struct s_exp_read_context {
    FILE *fp;
    char *file_name;

    int line_no;

    ucs4_t pushback_char;
    ucs4_t buf[INBUF_SIZE];
    int buf_idx;
    unsigned char pending[8];
};


enum {
    CHAR_TYPE_OTHER = 0,
    CHAR_TYPE_SPACE,
    CHAR_TYPE_LETTER,
    CHAR_TYPE_SPECIAL_INITIAL,
    CHAR_TYPE_DIGIT,
    CHAR_TYPE_SPECIAL_SUBSEQENT
};


s_exp_data_t s_exp_true = { S_EXP_TYPE_TRUE, 0 };
s_exp_data_t s_exp_false = { S_EXP_TYPE_FALSE, 0 };
s_exp_data_t s_exp_nil = { S_EXP_TYPE_NIL, 0 };


static void fill_read_buf(s_exp_read_context_t *ctx);
static int read_char(s_exp_read_context_t *ctx);
static int read_char_lower(s_exp_read_context_t *ctx);
static int skip_ws_read_char(s_exp_read_context_t *ctx);
static void pushback_char(s_exp_read_context_t *ctx, int c);
static int get_char_type(int c);
static void skip_line(s_exp_read_context_t *ctx);

static int char_to_number(int c, int base);

static s_exp_data_t *read_list(s_exp_read_context_t *ctx);
static s_exp_data_t *read_string(s_exp_read_context_t *ctx);
static s_exp_data_t *read_integer(s_exp_read_context_t *ctx, int base);
static s_exp_data_t *read_symbol(s_exp_read_context_t *ctx);
static s_exp_data_t *read_word(s_exp_read_context_t *ctx);

static s_exp_data_t *s_exp_read_error(s_exp_read_context_t *ctx, char *fmt, ...);
static s_exp_data_t *s_exp_sys_error(char *name, int en);

static s_exp_data_t *s_exp_append(s_exp_data_t *head, s_exp_data_t **tail,
                                  s_exp_data_t *data);
static s_exp_data_t *s_exp_empty_string(void);
static s_exp_data_t *s_exp_append_char_to_string(s_exp_data_t *data, int c);
static s_exp_data_t *s_exp_integer(long val);
static s_exp_data_t *s_exp_intern_symbol(s_exp_data_t *str);

static s_exp_data_t *s_exp_alloc(int type);


s_exp_read_context_t *open_s_exp_read_context(const char *file)
{
    FILE *fp;
    s_exp_read_context_t *ctx;

    fp = fopen(file, "rb");
    if(fp == NULL) {
        return NULL;
    }

    ctx = open_s_exp_read_context_f(fp, file);
    if(ctx == NULL) {
        fclose(fp);
        return NULL;
    }

    return ctx;
}

s_exp_read_context_t *open_s_exp_read_context_f(FILE *fp, const char *name)
{
    s_exp_read_context_t *ctx;

    ctx = (s_exp_read_context_t *)malloc(sizeof(s_exp_read_context_t));
    if(ctx == NULL) {
        return NULL;
    }

    memset(ctx, 0, sizeof(s_exp_read_context_t));
    ctx->fp = fp;
    ctx->file_name = strdup(name);
    ctx->line_no = 1;
    ctx->pushback_char = (ucs4_t)-1;

    return ctx;
}

int close_s_exp_read_context(struct s_exp_read_context *ctx)
{
    if(ctx != NULL) {
        if(ctx->fp != NULL) fclose(ctx->fp);
        if(ctx->file_name != NULL) free(ctx->file_name);
    }

    return 1;
}


/* read s-exp */
s_exp_data_t *read_s_exp(s_exp_read_context_t *ctx)
{
    int c;

    c = skip_ws_read_char(ctx);

    switch(c) {
      case '(':
          return read_list(ctx);

      case '"':
          return read_string(ctx);

      case '#':
          c = read_char_lower(ctx);

          switch(c) {
            case 't':           /* #t */
                return S_EXP_TRUE;

            case 'f':           /* #f */
                return S_EXP_FALSE;

            case 'b':           /* #bNN */
                return read_integer(ctx, 2);

            case 'o':           /* #oNN */
                return read_integer(ctx, 8);

            case 'd':           /* #dNN */
                return read_integer(ctx, 10);

            case 'x':           /* #xNN */
                return read_integer(ctx, 16);

            default:
                return s_exp_read_error(ctx, "unknown #-syntax: '%c'.", c);
          }

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
          pushback_char(ctx, c);
          return read_integer(ctx, 10);

      case ';':
          skip_line(ctx);
          return read_s_exp(ctx);

      case EOF:
          return NULL;

      default:
          pushback_char(ctx, c);
          return read_symbol(ctx);
    }
}

s_exp_data_t *read_all_s_exp(s_exp_read_context_t *ctx)
{
    s_exp_data_t *head, *tail, *data, *d;

    head = S_EXP_NIL;
    tail = S_EXP_NIL;

    while(1) {
        data = read_s_exp(ctx);
        if(data == NULL) {
            return head;
        }

        if(S_EXP_ERROR(data)) {
            free_s_exp(head);
            return data;
        }

        d = s_exp_append(head, &tail, data);
        if(S_EXP_ERROR(d)) {
            free_s_exp(head);
            free_s_exp(data);
            return d;
        }

        head = d;
    }
}


static s_exp_data_t *read_list(s_exp_read_context_t *ctx)
{
    int c;
    s_exp_data_t *head, *tail, *data, *d;

    head = S_EXP_NIL;
    tail = S_EXP_NIL;

    while(1) {
        c = skip_ws_read_char(ctx);

        switch(c) {
          case ')':
              return head;

          case '.':
              if(head == S_EXP_NIL) {
                  return s_exp_read_error(ctx, "bad dot syntax.");
              }

              data = read_s_exp(ctx);
              if(data == NULL) {
                  free_s_exp(head);
                  return s_exp_read_error(ctx, "EOF detected inside list.");
              }

              if(S_EXP_ERROR(data)) {
                  free_s_exp(head);
                  return data;
              }

              S_EXP_CDR(tail) = data;

              c = skip_ws_read_char(ctx);
              if(c != ')') {
                  free_s_exp(head);
                  return s_exp_read_error(ctx, "bad dot syntax.");
              }

              return head;

          case EOF:
              free_s_exp(head);
              return s_exp_read_error(ctx, "EOF detected inside list.");

          default:
              pushback_char(ctx, c);

              data = read_s_exp(ctx);
              if(data == NULL) {
                  free_s_exp(head);
                  return s_exp_read_error(ctx, "EOF detected inside list.");
              }

              if(S_EXP_ERROR(data)) {
                  free_s_exp(head);
                  return data;
              }

              d = s_exp_append(head, &tail, data);
              if(S_EXP_ERROR(d)) {
                  free_s_exp(head);
                  free_s_exp(data);
                  return d;
              }

              head = d;

              break;
        }
    }
}

static s_exp_data_t *read_string(s_exp_read_context_t *ctx)
{
    int c;
    s_exp_data_t *data, *d;

    data = s_exp_empty_string();
    if(S_EXP_ERROR(data)) {
        return data;
    }

    while(1) {
        c = read_char(ctx);

        switch(c) {
          case '"':
              return data;

          case EOF:
              free_s_exp(data);
              return s_exp_read_error(ctx, "EOF detected inside string.");

          case '\\':
              c = read_char(ctx);

              switch(c) {
                case '"':
                case '\\':
                    break;

                case EOF:
                    free_s_exp(data);
                    return s_exp_read_error(ctx,
                                            "EOF detected inside string.");

                default:
                    free_s_exp(data);
                    return s_exp_read_error(ctx,
                                            "unknown escape character: '%c'.",
                                            c);
              }
              break;

          default:
              break;
        }

        d = s_exp_append_char_to_string(data, c);
        if(S_EXP_ERROR(d)) {
            free_s_exp(data);
            return d;
        }
    }
}

static s_exp_data_t *read_integer_from_word(s_exp_read_context_t *ctx,
                                            int base,
                                            s_exp_data_t *data)
{
    wchar_t *s;
    int l, i;
    long long val;
    long v;
    int sign;

    s = data->string.str;
    l = data->string.len;
    i = 0;

    val = 0;
    switch(s[i]) {
      case L'+':
          sign = +1;
          i += 1;
          break;

      case L'-':
          sign = -1;
          i += 1;
          break;

      default:
          sign = +1;
          break;
    }

    for(; i < l; i++) {
        v = char_to_number(s[i], base);
        if(v < 0) {
            s_exp_data_t *err;
            err = s_exp_read_error(ctx, "bad number syntax: \"%S\".", s);
            return err;
        }

        val = val * base + v;
        if(val > LONG_MAX) {
            val = LONG_MAX;
            break;
        }
    }

    return s_exp_integer((long)val * sign);
}

static s_exp_data_t *read_integer(s_exp_read_context_t *ctx, int base)
{
    s_exp_data_t *data, *num;

    data = read_word(ctx);

    if(data == NULL) {
        return s_exp_read_error(ctx, "EOF detected inside number.");
    }

    if(S_EXP_ERROR(data)) {
        return data;
    }

    num = read_integer_from_word(ctx, base, data);

    free_s_exp(data);
    return num;
}

static s_exp_data_t *read_symbol(s_exp_read_context_t *ctx)
{
    s_exp_data_t *data, *sym;
    wchar_t *s;
    int l, i;

    data = read_word(ctx);

    if(data == NULL) {
        return s_exp_read_error(ctx, "EOF detected inside symbol.");
    }

    if(S_EXP_ERROR(data)) {
        return data;
    }

    s = data->string.str;
    l = data->string.len;

    if((l == 1 && wcsncmp(s, L"+", l) == 0) ||
       (l == 1 && wcsncmp(s, L"-", l) == 0) ||
       (l == 3 && wcsncmp(s, L"...", l) == 0)) {
        sym = s_exp_intern_symbol(data);

        free_s_exp(data);
        return sym;
    }

    if(s[0] == L'+' || s[0] == L'-') {
        s_exp_data_t *num;

        num = read_integer_from_word(ctx, 10, data);

        free_s_exp(data);
        return num;
    }

    if(get_char_type(s[0]) != CHAR_TYPE_LETTER &&
       get_char_type(s[0]) != CHAR_TYPE_SPECIAL_INITIAL) {
        s_exp_data_t *err;
        err = s_exp_read_error(ctx, "bad symbol: \"%S\".", s);
        free_s_exp(data);
        return err;
    }

    for(i = 1; i < l; i++) {
        if(get_char_type(s[i]) != CHAR_TYPE_LETTER &&
           get_char_type(s[i]) != CHAR_TYPE_SPECIAL_INITIAL &&
           get_char_type(s[i]) != CHAR_TYPE_DIGIT &&
           get_char_type(s[i]) != CHAR_TYPE_SPECIAL_SUBSEQENT) {
            s_exp_data_t *err;
            err = s_exp_read_error(ctx, "bad symbol: \"%S\".", s);
            free_s_exp(data);
            return err;
        }
    }

    sym = s_exp_intern_symbol(data);

    free_s_exp(data);
    return sym;
}

static s_exp_data_t *read_word(s_exp_read_context_t *ctx)
{
    int c;
    s_exp_data_t *data, *d;

    c = skip_ws_read_char(ctx);

    if(c == EOF) {
        return NULL;
    }

    data = s_exp_empty_string();
    if(S_EXP_ERROR(data)) {
        return data;
    }

    while(1) {
        int type;

        type = get_char_type(c);
        if(c == EOF || type == CHAR_TYPE_SPACE || type == CHAR_TYPE_OTHER) {
            if(c != EOF) {
                pushback_char(ctx, c);
            }

            return data;
        }

        d = s_exp_append_char_to_string(data, c);
        if(S_EXP_ERROR(d)) {
            free_s_exp(data);
            return d;
        }

        c = read_char(ctx);
    }
}


static void fill_read_buf(s_exp_read_context_t *ctx)
{
    unsigned char cbuf[INBUF_SIZE];
    int ret;

    if(ctx->buf[ctx->buf_idx] != 0) {
        return;
    }

    memset(cbuf, 0, sizeof(cbuf));
    strcpy(cbuf, ctx->pending);
    ret = fread(cbuf + strlen(cbuf),
                1, INBUF_SIZE - strlen(cbuf) - 1, ctx->fp);
    if(ret <= 0) {
        return;
    }

    {
        ucs4_t *u4s;
        const unsigned char *pending;
        int i;

        u4s = u4s_dup_from_u8s(cbuf, &pending);
        if(u4s == NULL) {
            return;
        }

        for(i = 0; u4s[i] != 0; i++) {
            ctx->buf[i] = u4s[i];
        }
        ctx->buf[i] = 0;
        ctx->buf_idx = 0;

        strcpy(ctx->pending, pending);

        free(u4s);
    }
}

static int read_char(s_exp_read_context_t *ctx)
{
    ucs4_t c;
    int cc;

    if(ctx->pushback_char != (ucs4_t)-1) {
        c = ctx->pushback_char;
        ctx->pushback_char = (ucs4_t)-1;
    } else {
        fill_read_buf(ctx);

        if(ctx->buf[ctx->buf_idx] == 0) {
            return EOF;
        }

        c = ctx->buf[ctx->buf_idx];
        ctx->buf_idx = (ctx->buf_idx + 1) % INBUF_SIZE;
    }

    cc = wc_from_u4(c);

    if(cc == L'\n') {
        ctx->line_no += 1;
    }

    return c;
}

static int read_char_lower(s_exp_read_context_t *ctx)
{
    int c;

    c = read_char(ctx);
    if(c == EOF) {
        return c;
    }

    if(0 <= c && c <= 0xff) {
        return tolower(c);
    } else {
        return c;
    }
}

static int skip_ws_read_char(s_exp_read_context_t *ctx)
{
    int c;

    do {
        c = read_char(ctx);
        if(c == EOF) {
            return c;
        }
    } while(get_char_type(c) == CHAR_TYPE_SPACE);

    return c;
}

static void pushback_char(s_exp_read_context_t *ctx, int c)
{
    if(ctx->pushback_char != (ucs4_t)-1) {
        return;
    }

    if(c == L'\n') {
        ctx->line_no -= 1;
    }

    ctx->pushback_char = (ucs4_t)c;
}

static void skip_line(s_exp_read_context_t *ctx)
{
    int c;

    do {
        c = read_char(ctx);
    } while(c != EOF && c != '\n');
}


/*
 * table of char type for 0x00 - 0x7f
 *
 * 0: other
 * 1: white space
 * 2: letter
 * 3: special initial
 * 4: digit
 * 5: special subsequent
 *
 */
const static int char_type[] = {
/*  NUL SOH STX ETX EOT ENQ ACK BEL BS  HT  LF  VT  FF  CR  SO  SI  */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  0,  0,

/*  DLE DC1 DC2 DC3 DC4 NAK SYN ETB CAN EM  SUB ESC FS  GS  RS  US  */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

/*  SPC !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /   */
    1,  3,  0,  0,  3,  3,  3,  0,  0,  0,  3,  5,  0,  5,  5,  3,

/*  0   1   2   3   4   5   6   7   3   9   :   ;   <   =   >   ?   */
    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  3,  0,  3,  3,  3,  3,

/*  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   */
    5,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,

/*  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _   */
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  3,  3,

/*  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o   */
    0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,

/*  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~   DEL */
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  3,  0
};

static int get_char_type(int c)
{
    if(c >= 0 && c < sizeof(char_type)) {
        return char_type[c];
    } else {
        return CHAR_TYPE_LETTER;
    }
}

static int char_to_number(int c, int base)
{
    int n;

    if(L'0' <= c && c <= L'9') {
        n = c - L'0';
    } else if(L'a' <= c && c <= L'z') {
        n = 10 + (c - L'a');
    } else if(L'A' <= c && c <= L'Z') {
        n = 10 + (c - L'A');
    } else {
        return -1;
    }

    if(n >= base) {
        return -1;
    }

    return n;
}


static s_exp_data_t *s_exp_read_error(s_exp_read_context_t *ctx,
                                      char *fmt, ...)
{
    va_list vlist;
    static s_exp_data_t data;
    static char buf1[1024], buf2[1024];

    memset(&data, 0, sizeof(data));
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));

    data.type = S_EXP_TYPE_ERROR;
    data.error.descript = buf2;

    va_start(vlist, fmt);
    vsnprintf(buf1, sizeof(buf1) - 1, fmt, vlist);
    va_end(vlist);

    //snprintf(buf2, sizeof(buf2) - 1, "%s: %d: %s", ctx->file_name, ctx->line_no, buf1);

    return &data;
}


static s_exp_data_t *s_exp_sys_error(char *name, int en)
{
    static s_exp_data_t data;
    static char buf[1024];

    memset(&data, 0, sizeof(data));
    memset(buf, 0, sizeof(buf));

    data.type = S_EXP_TYPE_ERROR;
    data.error.descript = buf;

    //snprintf(buf, sizeof(buf) - 1, "%s: %s", name, strerror(en));

    return &data;
}

static s_exp_data_t *s_exp_append(s_exp_data_t *head, s_exp_data_t **tail,
                                  s_exp_data_t *data)
{
    s_exp_data_t *c;

    c = s_exp_alloc(S_EXP_TYPE_CONS);
    if(S_EXP_ERROR(c)) {
        return c;
    }

    S_EXP_CAR(c) = data;
    S_EXP_CDR(c) = S_EXP_NIL;

    if(head == S_EXP_NIL) {
        head = c;
        *tail = c;
    } else {
        S_EXP_CDR(*tail) = c;
        *tail = c;
    }

    return head;
}

static s_exp_data_t *s_exp_empty_string(void)
{
    s_exp_data_t *d;

    d = s_exp_alloc(S_EXP_TYPE_STRING);
    if(S_EXP_ERROR(d)) {
        return d;
    }

    d->string.str = (wchar_t *)malloc(sizeof(wchar_t));
    if(d->string.str == NULL) {
        free_s_exp(d);
        return s_exp_sys_error("malloc", errno);
    }

    d->string.len = 0;
    d->string.alloc_len = 1;

    return d;
}

static s_exp_data_t *s_exp_append_char_to_string(s_exp_data_t *data, int c)
{
    if(data->string.len + 2 >= data->string.alloc_len) {
        wchar_t *s;
        int size;

        size = data->string.alloc_len + 256;
        s = (wchar_t *)realloc(data->string.str, size);
        if(s == NULL) {
            return s_exp_sys_error("realloc", errno);
        }

        data->string.str = s;
        data->string.alloc_len = size;

        return s_exp_append_char_to_string(data, c);
    }

    data->string.str[data->string.len] = c;
    data->string.str[data->string.len + 1] = 0;
    data->string.len += 1;

    return data;
}

static s_exp_data_t *s_exp_integer(long val)
{
    s_exp_data_t *data;

    data = s_exp_alloc(S_EXP_TYPE_NUMBER);
    if(S_EXP_ERROR(data)) {
        return data;
    }

    data->number.val = val;

    return data;
}


static s_exp_data_t *s_exp_intern_symbol(s_exp_data_t *str)
{
    static int sym_alloc_cnt = 0;
    static int sym_cnt = 0;
    static s_exp_data_t **syms = NULL;

    s_exp_data_t *data;
    int i;

    for(i = 0; i < sym_cnt; i++) {
        if(wcscmp(syms[i]->symbol.name, str->string.str) != 0) {
            continue;
        }

        syms[i]->ref_cnt += 1;

        return syms[i];
    }

    {
        if(sym_cnt == sym_alloc_cnt) {
            int new_cnt;
            s_exp_data_t **new_syms;

            new_cnt = sym_alloc_cnt + 32;
            new_syms = (s_exp_data_t **)
                       realloc(syms, sizeof(s_exp_data_t *) * new_cnt);
            if(new_syms == NULL) {
                return s_exp_sys_error("realloc", errno);
            }

            syms = new_syms;
            sym_alloc_cnt = new_cnt;
        }

        data = s_exp_alloc(S_EXP_TYPE_SYMBOL);
        if(S_EXP_ERROR(data)) {
            return data;
        }

        data->symbol.name = (wchar_t *)
                            malloc((str->string.len + 1) * sizeof(wchar_t));
        if(data->symbol.name == NULL) {
            free_s_exp(data);
            return s_exp_sys_error("malloc", errno);
        }

        memcpy(data->symbol.name, str->string.str,
               (str->string.len + 1) * sizeof(wchar_t));
        data->ref_cnt += 1;

        syms[sym_cnt] = data;
        sym_cnt += 1;
    }

    return data;
}

s_exp_data_t *s_exp_intern(wchar_t *str)
{
    s_exp_data_t s;

    memset(&s, 0, sizeof(s));
    s.type = S_EXP_TYPE_STRING;
    s.string.str = str;
    s.string.len = wcslen(str);
    s.string.alloc_len = s.string.len + 1;

    return s_exp_intern_symbol(&s);
}


static s_exp_data_t *s_exp_alloc(int type)
{
    s_exp_data_t *data;

    data = (s_exp_data_t *)malloc(sizeof(s_exp_data_t));
    if(data == NULL) {
        return s_exp_sys_error("malloc", errno);
    }

    memset(data, 0, sizeof(*data));
    data->type = type;
    data->ref_cnt = 1;

    return data;
}

void free_s_exp(s_exp_data_t *data)
{
    if(data == NULL) {
        return;
    }

    if(data->ref_cnt == 0) {
        return;
    }

    data->ref_cnt -= 1;
    if(data->ref_cnt > 0) {
        return;
    }

    switch(data->type) {
      case S_EXP_TYPE_CONS:
          free_s_exp(data->cons.car);
          free_s_exp(data->cons.cdr);
          break;

      case S_EXP_TYPE_STRING:
          if(data->string.str != NULL) free(data->string.str);
          break;

      case S_EXP_TYPE_SYMBOL:
          if(data->symbol.name != NULL) free(data->symbol.name);
          break;

      default:
          break;
    }

    free(data);
}
