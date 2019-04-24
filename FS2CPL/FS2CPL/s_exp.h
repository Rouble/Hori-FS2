/*
 * s_exp.h  -- s-expression
 *
 * $Id: s_exp.h,v 1.8 2004/06/08 06:36:23 hos Exp $
 *
 */

#include <wchar.h>


typedef enum {
    S_EXP_TYPE_TRUE,
    S_EXP_TYPE_FALSE,
    S_EXP_TYPE_NIL,

    S_EXP_TYPE_CONS,
    S_EXP_TYPE_NUMBER,
    S_EXP_TYPE_STRING,
    S_EXP_TYPE_SYMBOL,

    S_EXP_TYPE_ERROR
} s_exp_type_t;

typedef struct s_exp_data {
    s_exp_type_t type;

    unsigned int ref_cnt;

    union {
        struct {
            struct s_exp_data *car;
            struct s_exp_data *cdr;
        } cons;

        struct {
            int val;
        } number;

        struct {
            wchar_t *str;
            int len;
            int alloc_len;
        } string;

        struct {
            wchar_t *name;
        } symbol;

        struct {
            char *descript;
        } error;
    };
} s_exp_data_t;

typedef struct s_exp_read_context s_exp_read_context_t;

extern s_exp_data_t s_exp_true, s_exp_false, s_exp_nil;

#define S_EXP_TRUE (&s_exp_true)
#define S_EXP_FALSE (&s_exp_false)
#define S_EXP_NIL (&s_exp_nil)

#define S_EXP_ERROR(sexp) ((sexp)->type == S_EXP_TYPE_ERROR)

#define S_EXP_CAR(cell) ((cell)->cons.car)
#define S_EXP_CDR(cell) ((cell)->cons.cdr)

#define S_EXP_CAAR(cell) S_EXP_CAR(S_EXP_CAR(cell))
#define S_EXP_CADR(cell) S_EXP_CAR(S_EXP_CDR(cell))
#define S_EXP_CDAR(cell) S_EXP_CDR(S_EXP_CAR(cell))
#define S_EXP_CDDR(cell) S_EXP_CDR(S_EXP_CDR(cell))

#define S_EXP_CAAAR(cell) S_EXP_CAR(S_EXP_CAAR(cell))
#define S_EXP_CAADR(cell) S_EXP_CAR(S_EXP_CADR(cell))
#define S_EXP_CADAR(cell) S_EXP_CAR(S_EXP_CDAR(cell))
#define S_EXP_CADDR(cell) S_EXP_CAR(S_EXP_CDDR(cell))
#define S_EXP_CDAAR(cell) S_EXP_CDR(S_EXP_CAAR(cell))
#define S_EXP_CDADR(cell) S_EXP_CDR(S_EXP_CADR(cell))
#define S_EXP_CDDAR(cell) S_EXP_CDR(S_EXP_CDAR(cell))
#define S_EXP_CDDDR(cell) S_EXP_CDR(S_EXP_CDDR(cell))

#define S_EXP_CAAAAR(cell) S_EXP_CAR(S_EXP_CAAAR(cell))
#define S_EXP_CAAADR(cell) S_EXP_CAR(S_EXP_CAADR(cell))
#define S_EXP_CAADAR(cell) S_EXP_CAR(S_EXP_CADAR(cell))
#define S_EXP_CAADDR(cell) S_EXP_CAR(S_EXP_CADDR(cell))
#define S_EXP_CADAAR(cell) S_EXP_CAR(S_EXP_CDAAR(cell))
#define S_EXP_CADADR(cell) S_EXP_CAR(S_EXP_CDADR(cell))
#define S_EXP_CADDAR(cell) S_EXP_CAR(S_EXP_CDDAR(cell))
#define S_EXP_CADDDR(cell) S_EXP_CAR(S_EXP_CDDDR(cell))
#define S_EXP_CDAAAR(cell) S_EXP_CDR(S_EXP_CAAAR(cell))
#define S_EXP_CDAADR(cell) S_EXP_CDR(S_EXP_CAADR(cell))
#define S_EXP_CDADAR(cell) S_EXP_CDR(S_EXP_CADAR(cell))
#define S_EXP_CDADDR(cell) S_EXP_CDR(S_EXP_CADDR(cell))
#define S_EXP_CDDAAR(cell) S_EXP_CDR(S_EXP_CDAAR(cell))
#define S_EXP_CDDADR(cell) S_EXP_CDR(S_EXP_CDADR(cell))
#define S_EXP_CDDDAR(cell) S_EXP_CDR(S_EXP_CDDAR(cell))
#define S_EXP_CDDDDR(cell) S_EXP_CDR(S_EXP_CDDDR(cell))

#define S_EXP_FOR_EACH(list, p) \
  for((p) = (list); (p)->type == S_EXP_TYPE_CONS; (p) = S_EXP_CDR(p))


struct s_exp_read_context *open_s_exp_read_context(const char *file);
struct s_exp_read_context *open_s_exp_read_context_f(FILE *fp,
                                                     const char *name);
int close_s_exp_read_context(struct s_exp_read_context *ctx);

s_exp_data_t *read_s_exp(struct s_exp_read_context *ctx);
s_exp_data_t *read_all_s_exp(s_exp_read_context_t *ctx);
int write_s_exp(FILE *fp, s_exp_data_t *data);

s_exp_data_t *s_exp_intern(wchar_t *str);

s_exp_data_t *s_exp_assq(s_exp_data_t *alist, wchar_t *sym);
s_exp_data_t *s_exp_massq(s_exp_data_t *alist, int type, ...);
int s_exp_length(s_exp_data_t *list);

void free_s_exp(s_exp_data_t *data);
