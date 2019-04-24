/*
 * write_s_exp.c  -- write s-expression
 *
 * $Id: write_s_exp.c,v 1.1 2004/06/02 10:21:42 hos Exp $
 *
 */


#include <stdlib.h>
#include <stdio.h>

#include "s_exp.h"
#include "util.h"


static int write_list(FILE *fp, s_exp_data_t *data)
{
    if(! write_s_exp(fp, S_EXP_CAR(data))) {
        return 0;
    }

    if(S_EXP_CDR(data)->type == S_EXP_TYPE_CONS) {
        fprintf(fp, " ");
        return write_list(fp, S_EXP_CDR(data));
    } else if(S_EXP_CDR(data) == S_EXP_NIL) {
        return 1;
    } else {
        fprintf(fp, " . ");
        return write_s_exp(fp, S_EXP_CDR(data));
    }
}

int write_s_exp(FILE *fp, s_exp_data_t *data)
{
    switch(data->type) {
      case S_EXP_TYPE_TRUE:
          fprintf(fp, "#t");
          break;

      case S_EXP_TYPE_FALSE:
          fprintf(fp, "#f");
          break;

      case S_EXP_TYPE_NIL:
          fprintf(fp, "()");
          break;

      case S_EXP_TYPE_CONS:
          fprintf(fp, "(");
          if(! write_list(fp, data)) {
              return 0;
          }
          fprintf(fp, ")");
          break;

      case S_EXP_TYPE_NUMBER:
          fprintf(fp, "%d", data->number.val);
          break;

      case S_EXP_TYPE_STRING:
      {
          unsigned char *u8s;

          u8s = u8s_dup_from_wcs(data->string.str);
          if(u8s == NULL) {
              return 0;
          }

          fprintf(fp, "\"%s\"", u8s);

          free(u8s);
          break;
      }

      case S_EXP_TYPE_SYMBOL:
      {
          unsigned char *u8s;

          u8s = u8s_dup_from_wcs(data->symbol.name);
          if(u8s == NULL) {
              return 0;
          }

          fprintf(fp, "%s", u8s);

          free(u8s);
          break;
      }

      case S_EXP_TYPE_ERROR:
          fprintf(fp, "#<error %s>", data->error.descript);
          break;

      default:
          return 0;
    }

    return 1;
}
