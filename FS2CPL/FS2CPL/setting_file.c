/*
 * setting_file.c  -- device setting write to/read from file
 *
 * $Id: setting_file.c,v 1.11 2004/06/10 07:07:33 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"
#include "s_exp.h"


static struct uint_ptr_pair act_type_map[] = {
    {CONTROLLER_ACTION_MOD, L"modifier"},
    {CONTROLLER_ACTION_DELAY, L"delay"},
    {CONTROLLER_ACTION_JOY, L"joystick"},
    {CONTROLLER_ACTION_APPLY, L"apply"},
    {CONTROLLER_ACTION_MOU, L"mouse"},
    {CONTROLLER_ACTION_KBD, L"keyboard"},

    {0, NULL}
};

static struct uint_ptr_pair act_joy_flag_map[] = {
    {CONTROLLER_ACTION_JOY_VAL_MIN, L"minimum"},
    {CONTROLLER_ACTION_JOY_VAL_MAX, L"maximum"},
    {CONTROLLER_ACTION_JOY_VAL_MDL, L"middle"},
    {CONTROLLER_ACTION_JOY_VAL_THROUGH, L"through"},
    {CONTROLLER_ACTION_JOY_VAL_REVERSE, L"reverse"},
    {CONTROLLER_ACTION_JOY_VAL_SPEC, L"specify"},
    {CONTROLLER_ACTION_JOY_VAL_RATIO, L"ratio"},

    {0, NULL}
};

static struct uint_ptr_pair act_joy_code_map[] = {
    {CONTROLLER_ACTION_JOY_CODE_X, L"x"},
    {CONTROLLER_ACTION_JOY_CODE_Y, L"y"},
    {CONTROLLER_ACTION_JOY_CODE_Z, L"z"},
    {CONTROLLER_ACTION_JOY_CODE_RX, L"rx"},
    {CONTROLLER_ACTION_JOY_CODE_RY, L"ry"},
    {CONTROLLER_ACTION_JOY_CODE_RZ, L"rz"},
    {CONTROLLER_ACTION_JOY_CODE_SLIDER, L"slider"},
    {CONTROLLER_ACTION_JOY_CODE_THROTTLE, L"throttle"},
    {CONTROLLER_ACTION_JOY_CODE_HAT1_TOP, L"hat1-top"},
    {CONTROLLER_ACTION_JOY_CODE_HAT1_RIGHT, L"hat1-right"},
    {CONTROLLER_ACTION_JOY_CODE_HAT1_BOTTOM, L"hat1-bottom"},
    {CONTROLLER_ACTION_JOY_CODE_HAT1_LEFT, L"hat1-left"},
    {CONTROLLER_ACTION_JOY_CODE_HAT2_TOP, L"hat2-top"},
    {CONTROLLER_ACTION_JOY_CODE_HAT2_RIGHT, L"hat2-right"},
    {CONTROLLER_ACTION_JOY_CODE_HAT2_BOTTOM, L"hat2-bottom"},
    {CONTROLLER_ACTION_JOY_CODE_HAT2_LEFT, L"hat2-left"},

    {0, NULL}
};

static struct uint_ptr_pair act_mou_flag_map[] = {
    {CONTROLLER_ACTION_MOU_VAL_MIN, L"minimum"},
    {CONTROLLER_ACTION_MOU_VAL_MAX, L"maximum"},
    {CONTROLLER_ACTION_MOU_VAL_MDL, L"middle"},
    {CONTROLLER_ACTION_MOU_VAL_THROUGH, L"through"},
    {CONTROLLER_ACTION_MOU_VAL_REVERSE, L"reverse"},
    {CONTROLLER_ACTION_MOU_VAL_SPEC, L"specify"},
    {CONTROLLER_ACTION_MOU_VAL_RATIO, L"ratio"},

    {0, NULL}
};

static struct uint_ptr_pair act_mou_code_map[] = {
    {CONTROLLER_ACTION_MOU_CODE_X, L"x"},
    {CONTROLLER_ACTION_MOU_CODE_Y, L"y"},
    {CONTROLLER_ACTION_MOU_CODE_WHEEL, L"wheel"},
    {CONTROLLER_ACTION_MOU_CODE_BTN(0), L"button-1"},
    {CONTROLLER_ACTION_MOU_CODE_BTN(1), L"button-2"},
    {CONTROLLER_ACTION_MOU_CODE_BTN(2), L"button-3"},
    {CONTROLLER_ACTION_MOU_CODE_BTN(3), L"button-4"},
    {CONTROLLER_ACTION_MOU_CODE_BTN(4), L"button-5"},

    {0, NULL}
};

static struct uint_ptr_pair act_kbd_flag_map[] = {
    {CONTROLLER_ACTION_KBD_PRESS, L"press"},
    {CONTROLLER_ACTION_KBD_RELEASE, L"release"},
    {CONTROLLER_ACTION_KBD_THROUGH, L"through"},
    {CONTROLLER_ACTION_KBD_REVERSE, L"reverse"},

    {0, NULL}
};

static struct uint_ptr_pair evt_code_map[] = {
    {CONTROLLER_EVT_CODE_STICK_X, L"stick-x"},
    {CONTROLLER_EVT_CODE_STICK_Y, L"stick-y"},
    {CONTROLLER_EVT_CODE_THROTTLE, L"throttle"},
    {CONTROLLER_EVT_CODE_HAT_X, L"hat-x"},
    {CONTROLLER_EVT_CODE_HAT_Y, L"hat-y"},
    {CONTROLLER_EVT_CODE_RUDDER, L"rudder"},

    {CONTROLLER_EVT_CODE_DPAD1_TOP, L"dpad1-top"},
    {CONTROLLER_EVT_CODE_DPAD1_RIGHT, L"dpad1-right"},
    {CONTROLLER_EVT_CODE_DPAD1_BOTTOM, L"dpad1-bottom"},
    {CONTROLLER_EVT_CODE_DPAD1_LEFT, L"dpad1-left"},

    {CONTROLLER_EVT_CODE_DPAD2_TOP, L"dpad2-top"},
    {CONTROLLER_EVT_CODE_DPAD2_RIGHT, L"dpad2-right"},
    {CONTROLLER_EVT_CODE_DPAD2_BOTTOM, L"dpad2-bottom"},
    {CONTROLLER_EVT_CODE_DPAD2_LEFT, L"dpad2-left"},

    {CONTROLLER_EVT_CODE_TRIGGER, L"trigger"},
    {CONTROLLER_EVT_CODE_FIRE_C, L"fire-c"},
    {CONTROLLER_EVT_CODE_LAUNCH, L"launch"},
    {CONTROLLER_EVT_CODE_BUTTON_A, L"button-a"},
    {CONTROLLER_EVT_CODE_BUTTON_B, L"button-b"},
    {CONTROLLER_EVT_CODE_BUTTON_HAT, L"button-hat"},
    {CONTROLLER_EVT_CODE_BUTTON_SW1, L"button-sw1"},
    {CONTROLLER_EVT_CODE_BUTTON_D, L"button-d"},
    {CONTROLLER_EVT_CODE_BUTTON_ST, L"button-st"},

    {CONTROLLER_EVT_CODE_MODE_M1, L"mode-m1"},
    {CONTROLLER_EVT_CODE_MODE_M2, L"mode-m2"},
    {CONTROLLER_EVT_CODE_MODE_M3, L"mode-m3"},

    {CONTROLLER_EVT_CODE_DPAD3_LEFT, L"dpad3-left"},
    {CONTROLLER_EVT_CODE_DPAD3_MIDDLE, L"dpad3-middle"},
    {CONTROLLER_EVT_CODE_DPAD3_RIGHT, L"dpad3-right"},

    {0, NULL}
};


static int get_action_index_of(controller_setting_t *conf,
                               LPCWSTR name, USHORT *idx)
{
    int i;

    for(i = 0; i < conf->num_action_list; i++) {
        if(wcscmp(conf->action_list[i].name, name) == 0) {
            *idx = i;
            return 1;
        }
    }

    return 0;
}

static unsigned char *escaped_u8s_dup_from_wcs(wchar_t *wcs)
{
    wchar_t *escaped_wcs;
    unsigned char *u8s;
    int i, ii, j;

    wchar_t ec[] = L"\\\"";

    for(i = ii = 0; wcs[i] != 0; i++) {
        for(j = 0; ec[j] != 0; j++) {
            if(wcs[i] == ec[j]) {
                ii++;
                break;
            }
        }

        ii++;
    }

    escaped_wcs = (wchar_t *)malloc(sizeof(wchar_t) * (ii + 1));
    if(escaped_wcs == NULL) {
        return NULL;
    }

    for(i = ii = 0; wcs[i] != 0; i++) {
        for(j = 0; ec[j] != 0; j++) {
            if(wcs[i] == ec[j]) {
                escaped_wcs[ii++] = L'\\';
                break;
            }
        }

        escaped_wcs[ii++] = wcs[i];
    }
    escaped_wcs[ii] = 0;

    u8s = u8s_dup_from_wcs(escaped_wcs);

    free(escaped_wcs);

    return u8s;
}


int load_file_setting(controller_setting_t *conf, LPCWSTR fname)
{
    s_exp_data_t *data = NULL, *d;
    int ret = 1;
    int i, j, k;

#define NUMOF(var) (sizeof(var) / sizeof(struct uint_ptr_pair))
    struct uint_ptr_pair act_type_rmap[NUMOF(act_type_map)];
    struct uint_ptr_pair act_joy_flag_rmap[NUMOF(act_joy_flag_map)];
    struct uint_ptr_pair act_joy_code_rmap[NUMOF(act_joy_code_map)];
    struct uint_ptr_pair act_mou_flag_rmap[NUMOF(act_mou_flag_map)];
    struct uint_ptr_pair act_mou_code_rmap[NUMOF(act_mou_code_map)];
    struct uint_ptr_pair act_kbd_flag_rmap[NUMOF(act_kbd_flag_map)];
#undef NUMOF

    struct {
        struct uint_ptr_pair *map;
        struct uint_ptr_pair *rmap;
    } rmaps[] = {
        {act_type_map, act_type_rmap},
        {act_joy_flag_map, act_joy_flag_rmap},
        {act_joy_code_map, act_joy_code_rmap},
        {act_mou_flag_map, act_mou_flag_rmap},
        {act_mou_code_map, act_mou_code_rmap},
        {act_kbd_flag_map, act_kbd_flag_rmap},

        {NULL, NULL}
    };

    for(i = 0; rmaps[i].map != NULL; i++) {
        rmaps[i].rmap[0].ptr = NULL;
    }

    memset(conf, 0, sizeof(*conf));

    /* build rmap */
    for(i = 0; rmaps[i].map != NULL; i++) {
        for(j = 0; rmaps[i].map[j].ptr != NULL; j++) {
            rmaps[i].rmap[j + 1].ptr = NULL;

            rmaps[i].rmap[j].key = rmaps[i].map[j].key;
            rmaps[i].rmap[j].ptr =
                s_exp_intern((wchar_t *)rmaps[i].map[j].ptr);
            if(S_EXP_ERROR((s_exp_data_t *)rmaps[i].rmap[j].ptr)) {
                ret = 0;
                goto func_end;
            }
        }
    }

    /* open/read file */
    {
        struct s_exp_read_context *rp;

        {
            FILE *fp;
            char *fn;

            fp = _wfopen(fname, L"r");
            if(fp == NULL) {
                ret = 0;
                goto func_end;
            }

            fn = u8s_dup_from_wcs(fname);
            if(fn == NULL) {
                fclose(fp);
                ret = 0;
                goto func_end;
            }

            rp = open_s_exp_read_context_f(fp, fn);
            free(fn);
            if(rp == NULL) {
                fclose(fp);
                ret = 0;
                goto func_end;
            }
        }

        data = read_all_s_exp(rp);
        close_s_exp_read_context(rp);
    }

    if(S_EXP_ERROR(data)) {
        ret = 0;
        goto func_end;
    }

    /* profile name */
    d = s_exp_massq(data, S_EXP_TYPE_STRING, L"profile", L"name", NULL);
    if(d == NULL) {
        ret = 0;
        goto func_end;
    }

    conf->profile_name = wcsdup(d->string.str);
    if(conf->profile_name == NULL) {
        ret = 0;
        goto func_end;
    }

    /* parameter */
    {
        struct {
            wchar_t *cat;
            wchar_t *name;
            DWORD *val;
        } parms[] = {
            {L"button", L"a-sensitivity", &conf->btn_a_sensitivity},
            {L"button", L"b-sensitivity", &conf->btn_b_sensitivity},
            {L"button", L"threshold", &conf->btn_threshold},
            {L"axis", L"lower-threshold", &conf->lower_threshold},
            {L"axis", L"upper-threshold", &conf->upper_threshold},
            {L"modifier", L"initial", &conf->initial_modifier},

            {NULL, NULL, NULL}
        };

        for(i = 0; parms[i].val != NULL; i++) {
            d = s_exp_massq(data, S_EXP_TYPE_NUMBER,
                            L"parameter", parms[i].cat, parms[i].name, NULL);
            if(d == NULL) {
                ret = 0;
                goto func_end;
            }

            *parms[i].val = d->number.val;
        }
    }

    /* action list */
    {
        struct controller_action_config_list *acts;
        s_exp_data_t *al, *a;
        int n;

        al = s_exp_assq(data, L"action-list");
        if(al == NULL) {
            ret = 0;
            goto func_end;
        }

        n = s_exp_length(al);
        if(n < 0) {
            ret = 0;
            goto func_end;
        }

        acts = (struct controller_action_config_list *)
               malloc(sizeof(struct controller_action_config_list) * n);
        if(acts == NULL) {
            ret = 0;
            goto func_end;
        }

        memset(acts, 0, sizeof(struct controller_action_config_list) * n);

        conf->num_action_list = n;
        conf->action_list = acts;

        /* alloc */
        i = 0;
        S_EXP_FOR_EACH(al, a) {
            s_exp_data_t *act;
            int na;

            act = S_EXP_CAR(a);
            if(act->type != S_EXP_TYPE_CONS) {
                ret = 0;
                goto func_end;
            }

            if(S_EXP_CAR(act)->type != S_EXP_TYPE_STRING) {
                ret = 0;
                goto func_end;
            }

            na = s_exp_length(S_EXP_CDR(act));
            if(na < 0) {
                ret = 0;
                goto func_end;
            }

            acts[i].num_action = na;
            acts[i].action =
                (struct controller_action_config *)
                malloc(sizeof(struct controller_action_config) * na);
            acts[i].name = wcsdup(S_EXP_CAR(act)->string.str);

            if(acts[i].action == NULL || acts[i].name == NULL) {
                ret = 0;
                goto func_end;
            }

            memset(acts[i].action, 0,
                   sizeof(struct controller_action_config) * na);

            i++;
        }

        /* build */
        i = 0;
        S_EXP_FOR_EACH(al, a) {
            s_exp_data_t *act;

            act = S_EXP_CAR(a);

            j = 0;
            S_EXP_FOR_EACH(S_EXP_CDR(act), act) {
                s_exp_data_t *acte;
                unsigned long type;

                acte = S_EXP_CAR(act);
                type = rassq_pair(act_type_rmap, S_EXP_CAR(acte),
                                  (unsigned long)-1);
                switch(type) {
                  case CONTROLLER_ACTION_MOD:
                      if(s_exp_length(S_EXP_CDR(acte)) != 2 ||
                         S_EXP_CADR(acte)->type != S_EXP_TYPE_NUMBER ||
                         S_EXP_CADDR(acte)->type != S_EXP_TYPE_NUMBER) {
                          ret = 0;
                          goto func_end;
                      }

                      acts[i].action[j].type = type;
                      acts[i].action[j].mod.mod = S_EXP_CADR(acte)->number.val;
                      acts[i].action[j].mod.mod_mask =
                          S_EXP_CADDR(acte)->number.val;
                      break;

                  case CONTROLLER_ACTION_DELAY:
                      if(s_exp_length(S_EXP_CDR(acte)) != 1 ||
                         S_EXP_CADR(acte)->type != S_EXP_TYPE_NUMBER) {
                          ret = 0;
                          goto func_end;
                      }

                      acts[i].action[j].type = type;
                      acts[i].action[j].delay.msec =
                          S_EXP_CADR(acte)->number.val;
                      break;

                  case CONTROLLER_ACTION_JOY:
                  {
                      int len;
                      unsigned long flags, code;
                      s_exp_data_t *rest;

                      len = s_exp_length(S_EXP_CDR(acte));
                      if(len < 2 || len > 5) {
                          ret = 0;
                          goto func_end;
                      }

                      flags = rassq_pair(act_joy_flag_rmap, S_EXP_CADR(acte),
                                        (unsigned long)-1);
                      if(flags == (unsigned long)-1) {
                          ret = 0;
                          goto func_end;
                      }

                      if(S_EXP_CADDR(acte)->type == S_EXP_TYPE_CONS) {
                          s_exp_data_t *code_s = S_EXP_CADDR(acte);

                          if(S_EXP_CAR(code_s)->type != S_EXP_TYPE_SYMBOL ||
                             wcscmp(S_EXP_CAR(code_s)->symbol.name,
                                    L"button") != 0 ||
                             S_EXP_CDR(code_s)->type != S_EXP_TYPE_CONS ||
                             S_EXP_CADR(code_s)->type != S_EXP_TYPE_NUMBER) {
                              ret = 0;
                              goto func_end;
                          }

                          code = CONTROLLER_ACTION_JOY_CODE_BTN(
                              S_EXP_CADR(code_s)->number.val - 1);
                      } else {
                          code = rassq_pair(act_joy_code_rmap,
                                            S_EXP_CADDR(acte),
                                            (unsigned long)-1);
                          if(code == (unsigned long)-1) {
                              ret = 0;
                              goto func_end;
                          }
                      }

                      acts[i].action[j].type = type;
                      acts[i].action[j].joy.flags = flags;
                      acts[i].action[j].joy.code = code;

                      rest = S_EXP_CDDDR(acte);

                      switch(flags) {
                        case CONTROLLER_ACTION_JOY_VAL_MIN:
                        case CONTROLLER_ACTION_JOY_VAL_MAX:
                        case CONTROLLER_ACTION_JOY_VAL_MDL:
                        case CONTROLLER_ACTION_JOY_VAL_THROUGH:
                        case CONTROLLER_ACTION_JOY_VAL_REVERSE:
                            if(len != 2) {
                                ret = 0;
                                goto func_end;
                            }
                            break;

                        case CONTROLLER_ACTION_JOY_VAL_SPEC:
                            if(len != 3 ||
                               S_EXP_CAR(rest)->type != S_EXP_TYPE_NUMBER) {
                                ret = 0;
                                goto func_end;
                            }

                            acts[i].action[j].joy.val =
                                S_EXP_CAR(rest)->number.val;
                            break;

                        case CONTROLLER_ACTION_JOY_VAL_RATIO:
                            if(len != 5 ||
                               S_EXP_CAR(rest)->type != S_EXP_TYPE_NUMBER ||
                               S_EXP_CADR(rest)->type != S_EXP_TYPE_NUMBER ||
                               S_EXP_CADDR(rest)->type != S_EXP_TYPE_NUMBER) {
                                ret = 0;
                                goto func_end;
                            }

                            acts[i].action[j].joy.ratio_numerator =
                                S_EXP_CAR(rest)->number.val;
                            acts[i].action[j].joy.ratio_denominator =
                                S_EXP_CADR(rest)->number.val;
                            acts[i].action[j].joy.offset =
                                S_EXP_CADDR(rest)->number.val;
                            break;
                      }

                      break;
                  }

                  case CONTROLLER_ACTION_APPLY:
                      if(s_exp_length(S_EXP_CDR(acte)) != 3 ||
                         S_EXP_CADR(acte)->type != S_EXP_TYPE_NUMBER ||
                         S_EXP_CADDR(acte)->type != S_EXP_TYPE_NUMBER ||
                         S_EXP_CADDDR(acte)->type != S_EXP_TYPE_STRING) {
                          ret = 0;
                          goto func_end;
                      }

                      acts[i].action[j].type = type;
                      acts[i].action[j].apply.mod =
                          S_EXP_CADR(acte)->number.val;
                      acts[i].action[j].apply.mod_mask = 
                          S_EXP_CADDR(acte)->number.val;
                      if(! get_action_index_of(
                             conf,
                             S_EXP_CADDDR(acte)->string.str,
                             &acts[i].action[j].apply.action_idx)) {
                          ret = 0;
                          goto func_end;
                      }

                      break;

                  case CONTROLLER_ACTION_MOU:
                  {
                      int len;
                      unsigned long flags, code;
                      s_exp_data_t *rest;

                      len = s_exp_length(S_EXP_CDR(acte));
                      if(len < 2 || len > 5) {
                          ret = 0;
                          goto func_end;
                      }

                      flags = rassq_pair(act_mou_flag_rmap, S_EXP_CADR(acte),
                                        (unsigned long)-1);
                      code = rassq_pair(act_mou_code_rmap, S_EXP_CADDR(acte),
                                        (unsigned long)-1);
                      if(flags == (unsigned long)-1 ||
                         code == (unsigned long)-1) {
                          ret = 0;
                          goto func_end;
                      }

                      acts[i].action[j].type = type;
                      acts[i].action[j].mou.flags = flags;
                      acts[i].action[j].mou.code = code;

                      rest = S_EXP_CDDDR(acte);

                      switch(flags) {
                        case CONTROLLER_ACTION_MOU_VAL_MIN:
                        case CONTROLLER_ACTION_MOU_VAL_MAX:
                        case CONTROLLER_ACTION_MOU_VAL_MDL:
                        case CONTROLLER_ACTION_MOU_VAL_THROUGH:
                        case CONTROLLER_ACTION_MOU_VAL_REVERSE:
                            if(len != 2) {
                                ret = 0;
                                goto func_end;
                            }
                            break;

                        case CONTROLLER_ACTION_MOU_VAL_SPEC:
                            if(len != 3 ||
                               S_EXP_CAR(rest)->type != S_EXP_TYPE_NUMBER) {
                                ret = 0;
                                goto func_end;
                            }

                            acts[i].action[j].mou.val =
                                S_EXP_CAR(rest)->number.val;
                            break;

                        case CONTROLLER_ACTION_MOU_VAL_RATIO:
                            if(len != 5 ||
                               S_EXP_CAR(rest)->type != S_EXP_TYPE_NUMBER ||
                               S_EXP_CADR(rest)->type != S_EXP_TYPE_NUMBER ||
                               S_EXP_CADDR(rest)->type != S_EXP_TYPE_NUMBER) {
                                ret = 0;
                                goto func_end;
                            }

                            acts[i].action[j].mou.ratio_numerator =
                                S_EXP_CAR(rest)->number.val;
                            acts[i].action[j].mou.ratio_denominator =
                                S_EXP_CADR(rest)->number.val;
                            acts[i].action[j].mou.offset =
                                S_EXP_CADDR(rest)->number.val;
                            break;
                      }

                      break;
                  }

                  case CONTROLLER_ACTION_KBD:
                  {
                      unsigned long flags;

                      if(s_exp_length(S_EXP_CDR(acte)) != 2 ||
                         S_EXP_CADDR(acte)->type != S_EXP_TYPE_NUMBER) {
                          ret = 0;
                          goto func_end;
                      }

                      flags = rassq_pair(act_kbd_flag_rmap, S_EXP_CADR(acte),
                                         (unsigned long)-1);
                      if(flags == (unsigned long)-1) {
                          ret = 0;
                          goto func_end;
                      }

                      acts[i].action[j].type = type;
                      acts[i].action[j].kbd.flags = flags;
                      acts[i].action[j].kbd.code =
                          S_EXP_CADDR(acte)->number.val;

                      break;
                  }

                  default:
                      ret = 0;
                      goto func_end;
                }

                j++;
            }

            i++;
        }
    }

    /* event */
    {
        struct {
            wchar_t *name;
            struct controller_event_config *evts;
        } evt_map[] = {
            {L"motion", conf->motion_event},
            {L"press", conf->press_event},
            {L"release", conf->release_event},
            {L"lower-in", conf->lower_in_event},
            {L"lower-out", conf->lower_out_event},
            {L"upper-in", conf->upper_in_event},
            {L"upper-out", conf->upper_out_event},

            {NULL, NULL}
        };

        for(i = 0; evt_map[i].name != NULL; i++) {
            for(j = 0; evt_code_map[j].ptr != NULL; j++) {
                s_exp_data_t *e;
                struct controller_modifier_config *mod;
                int n;

                e = s_exp_massq(data, S_EXP_TYPE_CONS,
                                L"event", evt_map[i].name,
                                (wchar_t *)evt_code_map[j].ptr, NULL);
                if(e == NULL) {
                    continue;
                }

                n = s_exp_length(e);
                if(n < 0) {
                    ret = 0;
                    goto func_end;
                }

                mod = (struct controller_modifier_config *)
                      malloc(sizeof(struct controller_modifier_config) * n);
                if(mod == NULL) {
                    ret = 0;
                    goto func_end;
                }

                memset(mod, 0, sizeof(struct controller_modifier_config) * n);

                evt_map[i].evts[evt_code_map[j].key].num_mod = n;
                evt_map[i].evts[evt_code_map[j].key].mod = mod;

                k = 0;
                S_EXP_FOR_EACH(e, e) {
                    if(s_exp_length(S_EXP_CAR(e)) != 3 ||
                       S_EXP_CAAR(e)->type != S_EXP_TYPE_NUMBER ||
                       S_EXP_CADAR(e)->type != S_EXP_TYPE_NUMBER ||
                       S_EXP_CADDAR(e)->type != S_EXP_TYPE_STRING) {
                        ret = 0;
                        goto func_end;
                    }

                    if(! get_action_index_of(conf, S_EXP_CADDAR(e)->string.str,
                                             &mod[k].action_idx)) {
                        ret = 0;
                        goto func_end;
                    }

                    mod[k].mod = S_EXP_CAAR(e)->number.val;
                    mod[k].mod_mask = S_EXP_CADAR(e)->number.val;

                    k++;
                }
            }
        }
    }

    ret = 1;

  func_end:
    free_s_exp(data);

    for(i = 0; rmaps[i].rmap != NULL; i++) {
        for(j = 0; rmaps[i].rmap[j].ptr != NULL; j++) {
            free_s_exp((s_exp_data_t *)rmaps[i].rmap[j].ptr);
        }
    }

    if(ret == 0) {
        free_device_setting(conf);
    }

    return ret;
}

int save_file_setting(controller_setting_t *conf, LPCWSTR fname)
{
    FILE *fp;
    char *str;
    int i, j, k;
    int ret = 1;

    fp = _wfopen(fname, L"w");
    if(fp == NULL) {
        return 0;
    }

    /* header */
    fprintf(fp,
            ";;;; -*- coding: utf-8; -*-\n"
            ";;;; S-expression Device Profile\n"
            "\n");

    /* profile name */
    str = escaped_u8s_dup_from_wcs(conf->profile_name);
    if(str == NULL) {
        ret = 0;
        goto func_end;
    }

    fprintf(fp,
            "(profile\n"
            " (name . \"%s\")\n"
            " )\n\n",
            str);

    free(str);

    /* parameter */
    fprintf(fp,
            "(parameter\n"
            " (button\n"
            "  (a-sensitivity . #x%02x)\n"
            "  (b-sensitivity . #x%02x)\n"
            "  (threshold . #x%02x)\n"
            "  )\n"
            "\n"
            " (axis\n"
            "  (lower-threshold . #x%02x)\n"
            "  (upper-threshold . #x%02x)\n"
            "  )\n"
            "\n"
            " (modifier\n"
            "  (initial . #x%04x)\n"
            "  )\n"
            " )\n\n",
            (unsigned int)conf->btn_a_sensitivity,
            (unsigned int)conf->btn_b_sensitivity,
            (unsigned int)conf->btn_threshold,
            (unsigned int)conf->lower_threshold,
            (unsigned int)conf->upper_threshold,
            (unsigned int)conf->initial_modifier);

    /* action list */
    fprintf(fp, "(action-list\n");

    for(i = 0; i < conf->num_action_list; i++) {
        str = escaped_u8s_dup_from_wcs(conf->action_list[i].name);
        if(str == NULL) {
            ret = 0;
            goto func_end;
        }

        fprintf(fp, " (\"%s\"\n", str);

        free(str);

        for(j = 0; j < conf->action_list[i].num_action; j++) {
            struct controller_action_config *act;

            act = &conf->action_list[i].action[j];

            fprintf(fp, "  (%S ",
                    (wchar_t *)assq_pair(act_type_map, act->type, L"nop"));

            switch(conf->action_list[i].action[j].type) {
              case CONTROLLER_ACTION_MOD:
                  fprintf(fp, "#x%04x #x%04x",
                          act->mod.mod, act->mod.mod_mask);
                  break;

              case CONTROLLER_ACTION_DELAY:
                  fprintf(fp, "%lu", act->delay.msec);
                  break;

              case CONTROLLER_ACTION_JOY:
                  fprintf(fp, "%S ",
                          (wchar_t *)assq_pair(act_joy_flag_map,
                                               act->joy.flags, L"#f"));

                  if(act->joy.code < CONTROLLER_ACTION_JOY_CODE_BTN_MIN) {
                      fprintf(fp, "%S",
                              (wchar_t *)assq_pair(act_joy_code_map,
                                                   act->joy.code, L"#f"));
                  } else {
                      fprintf(fp, "(button %d)",
                              act->joy.code -
                              CONTROLLER_ACTION_JOY_CODE_BTN_MIN + 1);
                  }

                  switch(act->joy.flags) {
                    case CONTROLLER_ACTION_JOY_VAL_SPEC:
                        fprintf(fp, " %d", act->joy.val);
                        break;

                    case CONTROLLER_ACTION_JOY_VAL_RATIO:
                        fprintf(fp, " %d %d %d",
                                act->joy.ratio_numerator,
                                act->joy.ratio_denominator,
                                act->joy.offset);
                        break;
                  }
                  break;

              case CONTROLLER_ACTION_APPLY:
                  str = escaped_u8s_dup_from_wcs(
                      conf->action_list[act->apply.action_idx].name);
                  if(str == NULL) {
                      ret = 0;
                      goto func_end;
                  }

                  fprintf(fp, "#x%04x #x%04x \"%s\"",
                          act->apply.mod, act->apply.mod_mask, str);

                  free(str);
                  break;

              case CONTROLLER_ACTION_MOU:
                  fprintf(fp, "%S ",
                          (wchar_t *)assq_pair(act_mou_flag_map,
                                               act->mou.flags, L"#f"));

                  fprintf(fp, "%S",
                          (wchar_t *)assq_pair(act_mou_code_map,
                                               act->mou.code, L"#f"));

                  switch(act->mou.flags) {
                    case CONTROLLER_ACTION_MOU_VAL_SPEC:
                        fprintf(fp, " %d", act->mou.val);
                        break;

                    case CONTROLLER_ACTION_MOU_VAL_RATIO:
                        fprintf(fp, " %d %d %d",
                                act->mou.ratio_numerator,
                                act->mou.ratio_denominator,
                                act->mou.offset);
                        break;
                  }
                  break;

              case CONTROLLER_ACTION_KBD:
                  fprintf(fp, "%S #x%04x",
                          (wchar_t *)assq_pair(act_kbd_flag_map,
                                               act->kbd.flags, L"#f"),
                          act->kbd.code);
                  break;
            }

            fprintf(fp, ")\n");
        }

        fprintf(fp, "  )\n\n");
    }

    fprintf(fp, " )\n\n");

    /* event */
    fprintf(fp, "(event\n");

    {
        struct {
            char *name;
            struct controller_event_config *evts;
        } evt_map[] = {
            {"motion", conf->motion_event},
            {"press", conf->press_event},
            {"release", conf->release_event},
            {"lower-in", conf->lower_in_event},
            {"lower-out", conf->lower_out_event},
            {"upper-in", conf->upper_in_event},
            {"upper-out", conf->upper_out_event},

            {NULL, NULL}
        };

        for(i = 0; evt_map[i].name != NULL; i++) {
            fprintf(fp, " (%s\n", evt_map[i].name);

            for(j = 0; j < CONTROLLER_NUM_EVENT; j++) {
                if(evt_map[i].evts[j].num_mod > 0) {
                    fprintf(fp, "  (%S\n   ",
                            (wchar_t *)assq_pair(evt_code_map, j, L"#f"));

                    for(k = 0; k < evt_map[i].evts[j].num_mod; k++) {
                        struct controller_modifier_config *mod;

                        mod = &evt_map[i].evts[j].mod[k];
                        str = escaped_u8s_dup_from_wcs(
                            conf->action_list[mod->action_idx].name);
                        if(str == NULL) {
                            ret = 0;
                            goto func_end;
                        }

                        fprintf(fp, "(#x%04x #x%04x \"%s\")\n   ",
                                mod->mod, mod->mod_mask, str);

                        free(str);
                    }

                    fprintf(fp, ")\n");
                }
            }

            fprintf(fp, "  )\n\n");
        }
    }

    fprintf(fp, " )\n");

  func_end:
    fclose(fp);

    return ret;
}
