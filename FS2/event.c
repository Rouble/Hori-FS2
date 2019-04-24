/*
 * event.c  -- event processing
 *
 * $Id: event.c,v 1.22 2004/03/25 13:53:38 hos Exp $
 *
 */


#include "main.h"
#include <math.h>


static unsigned char fix_abutton_value(unsigned char val,
                                       unsigned char max)
{
    int v;

    DBG_ENTER2(("%02x, %02x", val, max));

    /* clamp to [0x26 - 0xe8], then scale to [0x00 - 0xff] */
    v = (((int)0xff - val) - 0x26) * 0xff / 0xc2;

    if(max != 0) {
        /* clamp to [0x00 - max], then scale to [0x00 - 0xff] */
        v = v * 0xff / max;
    }

    if(v < 0x00) {
        v = 0;
    } else if(v > 0xff) {
        v = 0xff;
    }

    DBG_LEAVE2(("void: %x", v));
    return v;
}

static int conv_digi_val(PDEVICE_OBJECT dev_obj, int val)
{
    device_extension_t *dev_ext;

    DBG_ENTER2(("%p, %d", dev_obj, val));

    dev_ext = GET_DEV_EXT(dev_obj);

    val = (val < dev_ext->conf.button_threshold ? 0 : 1);

    DBG_LEAVE2(("%d", val));
    return val;
}


static CHAR conv_schar_val(UCHAR val)
{
    int v;

    DBG_ENTER2(("%d", val));

    v = (int)val - (val <= 0x7f ? 0x7f : 0x80);

    DBG_LEAVE2(("%d", v));
    return (CHAR)v;
}


static UCHAR calc_ratio(UCHAR val, SHORT numer, SHORT denom, SHORT offset)
{
    int v;
    KFLOATING_SAVE save;
    NTSTATUS nt_ret;

    DBG_ENTER2(("%d, %d, %d, %d", val, numer, denom, offset));

    if(denom == 0) {
        DBG_LEAVE2(("0: denom is zero"));
        return 0;
    }

    nt_ret = KeSaveFloatingPointState(&save);
    if(! NT_SUCCESS(nt_ret)) {
        v = (int)val * numer / denom + offset;
    } else {
        v = (double)val * numer / denom + offset;

        KeRestoreFloatingPointState(&save);
    }

    val = (v < 0x00 ? 0x00 : (v > 0xff ? 0xff : v));

    DBG_LEAVE2(("%d", val));
    return val;
}



static void preprocess_input(IN PDEVICE_OBJECT dev_obj,
                             IN struct controller_raw_input *raw_in,
                             OUT struct controller_pp_input *pp_in)
{
    device_extension_t *dev_ext;

    DBG_ENTER2(("%p, %p, %p", dev_obj, raw_in, pp_in));

    dev_ext = GET_DEV_EXT(dev_obj);

    RtlZeroMemory(pp_in, sizeof(struct controller_pp_input));

#define SET_AVAL(_c_, _v_) pp_in->val[CONTROLLER_EVT_CODE_ ## _c_] = (_v_)
#define SET_DVAL(_c_, _v_) SET_AVAL(_c_, ~((_v_) * 0xff))

    /* axis */
    SET_AVAL(STICK_X,  raw_in->ir.stick_x);
    SET_AVAL(STICK_Y,  raw_in->ir.stick_y);
    SET_AVAL(HAT_X,    raw_in->ir.hat_x);
    SET_AVAL(HAT_Y,    raw_in->ir.hat_y);
    SET_AVAL(RUDDER,   raw_in->ir.rudder);
    SET_AVAL(THROTTLE, 0xff - raw_in->ir.throttle);

    /* d-pad 1/2 (default: POVs) */
    SET_DVAL(DPAD1_TOP,    raw_in->vr_00.dpad1_top);
    SET_DVAL(DPAD1_RIGHT,  raw_in->vr_00.dpad1_right);
    SET_DVAL(DPAD1_BOTTOM, raw_in->vr_00.dpad1_bottom);
    SET_DVAL(DPAD1_LEFT,   raw_in->vr_00.dpad1_left);

    SET_DVAL(DPAD2_TOP,    raw_in->vr_01.dpad2_top);
    SET_DVAL(DPAD2_RIGHT,  raw_in->vr_01.dpad2_right);
    SET_DVAL(DPAD2_BOTTOM, raw_in->vr_01.dpad2_bottom);
    SET_DVAL(DPAD2_LEFT,   raw_in->vr_01.dpad2_left);

    /* button and d-pad 3 */
    SET_DVAL(TRIGGER,    raw_in->vr_00.trigger);
    SET_DVAL(FIRE_C,     raw_in->vr_00.fire_c);
    SET_DVAL(LAUNCH,     raw_in->vr_00.launch);
    SET_AVAL(BUTTON_A,   fix_abutton_value(raw_in->ir.button_a,
                                           dev_ext->conf.button_a_max));
    SET_AVAL(BUTTON_B,   fix_abutton_value(raw_in->ir.button_b,
                                           dev_ext->conf.button_b_max));
    SET_DVAL(BUTTON_HAT, raw_in->vr_00.hat);
    SET_DVAL(BUTTON_SW1, raw_in->vr_01.button_sw1);

    SET_DVAL(BUTTON_D,   raw_in->vr_00.button_d);
    SET_DVAL(BUTTON_ST,  raw_in->vr_00.button_st);

    SET_AVAL(MODE_M1, (raw_in->vr_01.mode_select == 0x2 ? 0xff : 0));
    SET_AVAL(MODE_M2, (raw_in->vr_01.mode_select == 0x1 ? 0xff : 0));
    SET_AVAL(MODE_M3, (raw_in->vr_01.mode_select == 0x3 ? 0xff : 0));

    SET_DVAL(DPAD3_LEFT,   raw_in->vr_01.dpad3_left);
    SET_DVAL(DPAD3_MIDDLE, raw_in->vr_01.dpad3_middle);
    SET_DVAL(DPAD3_RIGHT,  raw_in->vr_01.dpad3_right);

#undef SET_DVAL
#undef SET_AVAL

    DBG_LEAVE2(("void"));
}



static int count_mod_match(IN struct controller_modifier_config *mod,
                           IN USHORT cur_mod)
{
    int nmatch, i;

    DBG_ENTER2(("%p, %04x", mod, cur_mod));

    if((cur_mod & mod->mod_mask) != (mod->mod & mod->mod_mask)) {
        /* not match */
        return -1;
    }

    nmatch = 0;
    for(i = 0; i < 16; i++) {
        if(mod->mod_mask & (1 << i)) {
            nmatch += 1;
        }
    }

    DBG_LEAVE2(("%d", nmatch));
    return nmatch;
}


static int select_action(IN struct controller_event_config *event,
                         IN USHORT mod)
{
    int i, n, nmatch, idx;
    int ret;

    DBG_ENTER2(("%p, %04x", event, mod));

    nmatch = -1;
    idx = -1;
    for(i = 0; i < event->num_mod; i++) {
        n = count_mod_match(&event->mod[i], mod);
        if(n > nmatch) {
            nmatch = n;
            idx = i;
        }
    }

    if(idx < 0) {
        /* not found */
        ret = -1;
    } else {
        /* found */
        ret = event->mod[idx].action_idx;
    }

    DBG_LEAVE2(("%x", ret));
    return ret;
}



static void add_pending_action(IN PDEVICE_OBJECT dev_obj,
                               IN struct controller_action_config *action,
                               IN int num_action,
                               IN UCHAR val,
                               IN ULONG msec)
{
    device_extension_t *dev_ext;
    struct controller_pending_action_list *new_pending_action, *list, *prev;
    ULONGLONG cur_time, time;

    DBG_ENTER(("%p, %p, %d, %u, %u", dev_obj, action, num_action, val, msec));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* 100-nanosec units */
    cur_time = KeQueryInterruptTime();
    time = cur_time + (ULONGLONG)msec * 10 * 1000;

    for(prev = &dev_ext->pending_action, list = prev->next;
        list != NULL;
        prev = list, list = prev->next) {
        if(list->time > time) {
            /* insert before this */
            break;
        }
    }

    /* build new pending action */
    new_pending_action =
        (struct controller_pending_action_list *)
        ExAllocatePool(NonPagedPool,
                       sizeof(struct controller_pending_action_list));
    if(new_pending_action == NULL) {
        DBG_LEAVE(("void: ExAllocatePool failed"));
        return;
    }

    DBG_OUT(("new pending action: %08x%08x, %08x%08x, %d, %p, %u, %p",
             ((ULONG *)&cur_time)[1], ((ULONG *)&cur_time)[0],
             ((ULONG *)&time)[1], ((ULONG *)&time)[0],
             num_action, action, val, list));
    new_pending_action->time = time;
    new_pending_action->num_action = num_action;
    new_pending_action->action = action;
    new_pending_action->val = val;
    new_pending_action->next = list;

    /* add to list */
    prev->next = new_pending_action;

    DBG_LEAVE(("void"));
}

VOID free_pending_action(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    struct controller_pending_action_list *list, *next;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    list = dev_ext->pending_action.next;
    dev_ext->pending_action.next = NULL;

    while(list != NULL) {
        next = list->next;

        ExFreePool(list);

        list = next;
    }

    DBG_LEAVE(("void"));
}



static void process_action(IN PDEVICE_OBJECT dev_obj,
                           IN struct controller_action_config *action,
                           IN int num_action,
                           IN UCHAR in_val,
                           IN USHORT *modifier,
                           OUT struct controller_pr_input *pr_in)
{
    device_extension_t *dev_ext;
    UCHAR val;
    int i;

    DBG_ENTER2(("%p, %p, %d, %x, %04x, %p",
                dev_obj, action, num_action, val, *modifier, pr_in));

    dev_ext = GET_DEV_EXT(dev_obj);

    for(i = 0; i < num_action; i++) {
        val = in_val;

        switch(action[i].type) {
          case CONTROLLER_ACTION_NOP:
              /* no operation */
              DBG_OUT2(("action: nop"));
              break;

          case CONTROLLER_ACTION_MOD:
              /* modifier action */
              DBG_OUT2(("action: mod: %02x, %02x",
                        action[i].mod.mod, action[i].mod.mod_mask));
              *modifier =
                  (action[i].mod.mod & action[i].mod.mod_mask) |
                  (*modifier & ~action[i].mod.mod_mask);
              break;

          case CONTROLLER_ACTION_DELAY:
              /* execute later followed action */
              DBG_OUT2(("action: delay: %u", action[i].delay.msec));
              if(num_action - i > 1) {
                  add_pending_action(dev_obj,
                                     &action[i + 1], num_action - i - 1,
                                     val,
                                     action[i].delay.msec);
              }

              /* skip followed action, for now */
              i = num_action - 1;
              break;

          case CONTROLLER_ACTION_JOY:
              DBG_OUT2(("action: joy: %x, %x, %u, %d, %d, %d",
                        action[i].joy.flags, action[i].joy.code,
                        action[i].joy.val,
                        action[i].joy.ratio_numerator,
                        action[i].joy.ratio_denominator,
                        action[i].joy.offset));
              switch(action[i].joy.flags) {
                case CONTROLLER_ACTION_JOY_VAL_MIN:
                    /* minimum value */
                    val = 0x00;
                    break;

                case CONTROLLER_ACTION_JOY_VAL_MAX:
                    /* maximum value */
                    val = 0xff;
                    break;

                case CONTROLLER_ACTION_JOY_VAL_MDL:
                    /* middle value */
                    val = 0x7f;
                    break;

                case CONTROLLER_ACTION_JOY_VAL_THROUGH:
                    /* use original val */
                    break;

                case CONTROLLER_ACTION_JOY_VAL_REVERSE:
                    /* use reverse of original val */
                    val = 0xff - val;
                    break;

                case CONTROLLER_ACTION_JOY_VAL_SPEC:
                    /* use specified value */
                    val = action[i].joy.val;
                    break;

                case CONTROLLER_ACTION_JOY_VAL_RATIO:
                    /* multiple of specified ratio */
                    val = calc_ratio(val,
                                     action[i].joy.ratio_numerator,
                                     action[i].joy.ratio_denominator,
                                     action[i].joy.offset);
                    break;

                default:
                    /* unknown: use original value */
                    DBG_OUT(("unknown joy action: %x", action[i].joy.flags));
                    break;
              }

              if(action[i].joy.code > CONTROLLER_ACTION_JOY_CODE_MAX) {
                  DBG_OUT(("unknown joy code: %x", action[i].joy.code));
                  break;
              }

              pr_in->joy.val[action[i].joy.code] = val;

              break;

          case CONTROLLER_ACTION_APPLY:
              /* apply other action */
              DBG_OUT2(("action: apply: %u, %04x, %04x",
                        action[i].apply.action_idx,
                        action[i].apply.mod, action[i].apply.mod_mask));
              if((*modifier & action[i].apply.mod_mask) ==
                 (action[i].apply.mod & action[i].apply.mod_mask) &&
                 action[i].apply.action_idx < dev_ext->conf.num_action_list) {
                  /* apply if modifier match */
                  struct controller_action_config_list *act;

                  act = &dev_ext->conf.action_list[action[i].apply.action_idx];
                  add_pending_action(dev_obj,
                                     act->action, act->num_action,
                                     val,
                                     0); /* next time */
              }
              break;

          case CONTROLLER_ACTION_KBD:
              DBG_OUT2(("action: kbd: %x, %x",
                        action[i].kbd.flags, action[i].kbd.code));
              switch(action[i].kbd.flags) {
                case CONTROLLER_ACTION_KBD_PRESS:
                    /* press */
                    val = 0xff;
                    break;

                case CONTROLLER_ACTION_KBD_RELEASE:
                    /* release */
                    val = 0x00;
                    break;

                case CONTROLLER_ACTION_KBD_THROUGH:
                    /* original value */
                    val = val;
                    break;

                case CONTROLLER_ACTION_KBD_REVERSE:
                    /* reverse value */
                    val = 0xff - val;
                    break;

                default:
                    /* unknown: use original value */
                    DBG_OUT(("unknown keyboard action: %d",
                             action[i].kbd.flags));
                    break;
              }

              if(action[i].kbd.code > CONTROLLER_ACTION_KBD_CODE_MAX) {
                  DBG_OUT(("unknown keyboard code: %x", action[i].kbd.code));
                  break;
              }

              val = conv_digi_val(dev_obj, val);

              {
                  int idx, shft;
                  UCHAR mask;

                  idx = action[i].kbd.code / 8;
                  shft = action[i].kbd.code % 8;
                  mask = ~(1 << shft);

                  pr_in->kbd.val[idx] = (pr_in->kbd.val[idx] & mask) |
                                        (val << shft);
              }

              break;

          case CONTROLLER_ACTION_MOU:
              DBG_OUT2(("action: mou: %x, %x, %u, %d, %d, %d",
                        action[i].mou.flags, action[i].mou.code,
                        action[i].mou.val,
                        action[i].mou.ratio_numerator,
                        action[i].mou.ratio_denominator,
                        action[i].mou.offset));
              switch(action[i].joy.flags) {
                case CONTROLLER_ACTION_MOU_VAL_MIN:
                    /* minimum value */
                    val = 0x00;
                    break;

                case CONTROLLER_ACTION_MOU_VAL_MAX:
                    /* maximum value */
                    val = 0xff;
                    break;

                case CONTROLLER_ACTION_MOU_VAL_MDL:
                    /* middle value */
                    val = 0x7f;
                    break;

                case CONTROLLER_ACTION_MOU_VAL_THROUGH:
                    /* use original val */
                    break;

                case CONTROLLER_ACTION_MOU_VAL_REVERSE:
                    /* use reverse of original val */
                    val = 0xff - val;
                    break;

                case CONTROLLER_ACTION_MOU_VAL_SPEC:
                    /* use specified value */
                    val = action[i].mou.val;
                    break;

                case CONTROLLER_ACTION_MOU_VAL_RATIO:
                    /* multiple of specified ratio */
                    val = calc_ratio(val,
                                     action[i].mou.ratio_numerator,
                                     action[i].mou.ratio_denominator,
                                     action[i].mou.offset);
                    break;

                default:
                    /* unknown: use original value */
                    DBG_OUT(("unknown mou action: %x", action[i].mou.flags));
                    break;
              }

              if(action[i].mou.code > CONTROLLER_ACTION_MOU_CODE_MAX) {
                  DBG_OUT(("unknown mou code: %x", action[i].mou.code));
                  break;
              }

              pr_in->mou.val[action[i].mou.code] = val;
              break;

          default:
              /* ignore unknown action */
              DBG_OUT(("unknown action: %x", action[i].type));
              break;
        }
    }

    DBG_LEAVE2(("void"));
}


static void process_event(IN PDEVICE_OBJECT dev_obj,
                          IN struct controller_event_config *event,
                          IN UCHAR val,
                          IN USHORT *modifier,
                          OUT struct controller_pr_input *pr_in)
{
    device_extension_t *dev_ext;
    int action_idx;

    DBG_ENTER2(("%p, %p, %x, %p, %p",
                dev_obj, event, val, modifier, pr_in));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* select action from current modifier */
    action_idx = select_action(event, *modifier);
    if(action_idx < 0 || action_idx >= dev_ext->conf.num_action_list) {
        DBG_LEAVE2(("void: no event match"));
        return;
    }

    /* do action */
    process_action(dev_obj,
                   dev_ext->conf.action_list[action_idx].action,
                   dev_ext->conf.action_list[action_idx].num_action,
                   val,
                   modifier,
                   pr_in);

    DBG_LEAVE2(("void"));
}



static void process_input(IN PDEVICE_OBJECT dev_obj,
                          IN struct controller_pp_input *pp_in,
                          IN USHORT *modifier,
                          OUT struct controller_pr_input *pr_in)
{
    device_extension_t *dev_ext;
    int i;

    DBG_ENTER2(("%p, %p, %p", dev_obj, pp_in, pr_in));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* process pending action before main action */
    {
        struct controller_pending_action_list *list, *prev, *next;
        ULONGLONG time;

        time = KeQueryInterruptTime();

        prev = &dev_ext->pending_action;
        list = prev->next;
        while(list != NULL) {
            /* list is always sorted by time in ascending */
            if(list->time > time) {
                break;
            }

            /* process pending action */
            DBG_OUT(("processing: pending action, %d, %p, %u",
                     list->num_action, list->action, list->val));
            process_action(dev_obj,
                           list->action, list->num_action, list->val,
                           modifier,
                           pr_in);

            /* remove current pending action from list:
               it is able to insert other action between `prev' and `list'
               by another pending action. */
            while(prev->next != list) {
                prev = prev->next;
            }
            next = list->next;
            prev->next = next;

            /* free current pending action */
            ExFreePool(list);

            /* for next loop */
            list = next;
        }
    }

    /* for each axis/button/d-pad */
    for(i = 0; i < CONTROLLER_NUM_EVENT; i++) {
        DBG_OUT2(("processing: val[%02x], % 3d, prev=% 3d",
                  i, pp_in->val[i], dev_ext->pp_input.val[i]));
        if(pp_in->val[i] != dev_ext->pp_input.val[i]) {
            /* motion event */
            DBG_OUT2(("processing: motion event"));
            process_event(dev_obj,
                          &dev_ext->conf.motion_event[i], pp_in->val[i],
                          modifier,
                          pr_in);

            if(dev_ext->pp_input.val[i] < dev_ext->conf.button_threshold &&
               pp_in->val[i] >= dev_ext->conf.button_threshold) {
                /* press event */
                DBG_OUT2(("processing: press event"));
                process_event(dev_obj,
                              &dev_ext->conf.press_event[i], pp_in->val[i],
                              modifier,
                              pr_in);
            }

            if(dev_ext->pp_input.val[i] >= dev_ext->conf.button_threshold &&
               pp_in->val[i] < dev_ext->conf.button_threshold) {
                /* release event */
                DBG_OUT2(("processing: release event"));
                process_event(dev_obj,
                              &dev_ext->conf.release_event[i], pp_in->val[i],
                              modifier,
                              pr_in);
            }

            if(dev_ext->pp_input.val[i] >= dev_ext->conf.lower_threshold &&
               pp_in->val[i] < dev_ext->conf.lower_threshold) {
                /* lower in event */
                DBG_OUT2(("processing: lower in event"));
                process_event(dev_obj,
                              &dev_ext->conf.lower_in_event[i], pp_in->val[i],
                              modifier,
                              pr_in);
            }

            if(dev_ext->pp_input.val[i] < dev_ext->conf.lower_threshold &&
               pp_in->val[i] >= dev_ext->conf.lower_threshold) {
                /* lower out event */
                DBG_OUT2(("processing: lower out event"));
                process_event(dev_obj,
                              &dev_ext->conf.lower_out_event[i], pp_in->val[i],
                              modifier,
                              pr_in);
            }

            if(dev_ext->pp_input.val[i] <= dev_ext->conf.upper_threshold &&
               pp_in->val[i] > dev_ext->conf.upper_threshold) {
                /* upper in event */
                DBG_OUT2(("processing: upper in event"));
                process_event(dev_obj,
                              &dev_ext->conf.upper_in_event[i], pp_in->val[i],
                              modifier,
                              pr_in);
            }

            if(dev_ext->pp_input.val[i] > dev_ext->conf.upper_threshold &&
               pp_in->val[i] <= dev_ext->conf.upper_threshold) {
                /* upper out event */
                DBG_OUT2(("processing: upper out event"));
                process_event(dev_obj,
                              &dev_ext->conf.upper_out_event[i], pp_in->val[i],
                              modifier,
                              pr_in);
            }
        }
    }

    DBG_LEAVE2(("void"));
}



static UCHAR map_hat_value(PDEVICE_OBJECT dev_obj,
                           UCHAR top, UCHAR right, UCHAR bottom, UCHAR left)
{
    UCHAR val;

    const static UCHAR hat_map[] = {
        /* 0     0     1     1  right*/
        /* 0     1     0     1  top */
        0x0f, 0x00, 0x02, 0x01, /* 0 0 */
        0x04, 0x0f, 0x03, 0x0f, /* 0 1 */
        0x06, 0x07, 0x0f, 0x0f, /* 1 0 */
        0x05, 0x0f, 0x0f, 0x0f  /* 1 1 */
                                /*   bottom  */
                                /* left */
    };

    DBG_ENTER2(("%p, %x, %x, %x, %x", dev_obj, top, right, bottom, left));

    val = hat_map[(conv_digi_val(dev_obj, left) << 3) |
                  (conv_digi_val(dev_obj, bottom) << 2) |
                  (conv_digi_val(dev_obj, right) << 1) |
                  (conv_digi_val(dev_obj, top) << 0)];

    DBG_LEAVE2(("%x", val));
    return val;
}


static void postprocess_input(IN PDEVICE_OBJECT dev_obj,
                              IN struct controller_pr_input *pr_in,
                              IN USHORT *modifier,
                              OUT struct controller_input_report *in_state)

{
    UCHAR val;
    int i;

    DBG_ENTER2(("%p, %p, %04x, %p", dev_obj, pr_in, *modifier, in_state));

    /* joystck: axis */
    in_state->joy.x = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_X];
    in_state->joy.y = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_Y];
    in_state->joy.z = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_Z];
    in_state->joy.rx = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_RX];
    in_state->joy.ry = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_RY];
    in_state->joy.rz = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_RZ];
    in_state->joy.slider = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_SLIDER];
    in_state->joy.throttle = pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_THROTTLE];

    /* joystick: pov */
    in_state->joy.hat1 =
        map_hat_value(dev_obj,
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT1_TOP],
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT1_RIGHT],
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT1_BOTTOM],
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT1_LEFT]);
    in_state->joy.hat2 =
        map_hat_value(dev_obj,
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT2_TOP],
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT2_RIGHT],
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT2_BOTTOM],
                      pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_HAT2_LEFT]);

    /* joystick: button */
    for(i = 0; i < INPUT_REPORT_NUM_JOY_BUTTON; i++) {
        val = conv_digi_val(dev_obj,
                            pr_in->joy.val[CONTROLLER_ACTION_JOY_CODE_BTN(i)]);

        in_state->joy.button[i / 8] =
            (in_state->joy.button[i / 8] & ~(1 << (i % 8))) | (val << (i % 8));
    }

    /* joystick: modifier */
    in_state->joy.modifier = *modifier;

    /* keyboard */
    RtlCopyMemory(in_state->kbd.keys, pr_in->kbd.val,
                  sizeof(in_state->kbd.keys));

    /* mouse: x, y, wheel */
    in_state->mou.x =
        conv_schar_val(pr_in->mou.val[CONTROLLER_ACTION_MOU_CODE_X]);
    in_state->mou.y =
        conv_schar_val(pr_in->mou.val[CONTROLLER_ACTION_MOU_CODE_Y]);
    in_state->mou.wheel =
        conv_schar_val(pr_in->mou.val[CONTROLLER_ACTION_MOU_CODE_WHEEL]);

    /* mouse: button */
    for(i = 0; i < INPUT_REPORT_NUM_MOU_BUTTON; i++) {
        val = conv_digi_val(dev_obj,
                            pr_in->mou.val[CONTROLLER_ACTION_MOU_CODE_BTN(i)]);

        in_state->mou.button[i / 8] =
            (in_state->mou.button[i / 8] & ~(1 << (i % 8))) | (val << (i % 8));
    }

    DBG_LEAVE2(("void"));
}


VOID process_raw_input(IN PDEVICE_OBJECT dev_obj,
                       IN struct controller_raw_input *raw_in,
                       OUT struct controller_input_report *in_state)
{
    device_extension_t *dev_ext;
    struct controller_pp_input pp_in;
    struct controller_pr_input pr_in;
    USHORT modifier;

    DBG_ENTER2(("%p, %p, %p", dev_obj, raw_in, in_state));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* pre process input data */
    preprocess_input(dev_obj, raw_in, &pp_in);

    /* process input data */
    pr_in = dev_ext->pr_input;
    modifier = in_state->joy.modifier;
    process_input(dev_obj, &pp_in, &modifier, &pr_in);

    /* post process input data */
    postprocess_input(dev_obj, &pr_in, &modifier, in_state);

    /* save current state */
    dev_ext->pp_input = pp_in;
    dev_ext->pr_input = pr_in;

    DBG_LEAVE2(("void"));
}
