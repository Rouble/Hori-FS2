/*
 * setting_data.c  -- device setting data
 *
 * $Id: setting_data.c,v 1.2 2004/06/13 13:02:39 hos Exp $
 *
 */

#include <windows.h>
#include "util.h"
#include "main.h"


struct controller_modifier_config default_motion_mod[CONTROLLER_NUM_EVENT];
struct controller_modifier_config default_press_mod[CONTROLLER_NUM_EVENT];
struct controller_modifier_config default_release_mod[CONTROLLER_NUM_EVENT];
unsigned short default_action_list[NUM_OF_DEF_ACTION];
struct controller_action_config default_action[NUM_OF_DEF_ACTION];
LPWSTR default_profile_name = L"default profile";
LPWSTR default_action_name[] = {
    L"null action",
    L"axis X motion",
    L"axis Y motion",
    L"axis Z motion",
    L"axis RX motion",
    L"axis RY motion",
    L"axis RZ motion",
    L"POV 1 top press",
    L"POV 1 top release",
    L"POV 1 right press",
    L"POV 1 right release",
    L"POV 1 bottom press",
    L"POV 1 bottom release",
    L"POV 1 left press",
    L"POV 1 left release",
    L"POV 2 top press",
    L"POV 2 top release",
    L"POV 2 right press",
    L"POV 2 right release",
    L"POV 2 bottom press",
    L"POV 2 bottom release",
    L"POV 2 left press",
    L"POV 2 left release",
    L"button 1 press",
    L"button 1 release",
    L"button 2 press",
    L"button 2 release",
    L"button 3 press",
    L"button 3 release",
    L"button 4 press",
    L"button 4 release",
    L"button 5 press",
    L"button 5 release",
    L"button 6 press",
    L"button 6 release",
    L"button 7 press",
    L"button 7 release",
    L"button 8 press",
    L"button 8 release",
    L"button 9 press",
    L"button 9 release",
    L"button 10 press",
    L"button 10 release",
    L"button 11 press",
    L"button 11 release",
    L"button 12 press",
    L"button 12 release",
    L"button 13 press",
    L"button 13 release",
    L"button 14 press",
    L"button 14 release",
    L"button 15 press",
    L"button 15 release",
};

void generate_default_setting(void)
{
    int i, ai;
    USHORT code;

    memset(default_motion_mod, 0, sizeof(default_motion_mod));
    memset(default_press_mod, 0, sizeof(default_press_mod));
    memset(default_release_mod, 0, sizeof(default_release_mod));
    memset(default_action_list, 0, sizeof(default_action_list));
    memset(default_action, 0, sizeof(default_action));

    ai = 1;
    for(i = 0; i < CONTROLLER_NUM_EVENT; i++) {
        if(i <= CONTROLLER_EVT_CODE_AXIS_MAX) {
            /* motion action for axis: pass through value */

            switch(i) {
              case CONTROLLER_EVT_CODE_STICK_X:
                  code = CONTROLLER_ACTION_JOY_CODE_X;
                  break;

              case CONTROLLER_EVT_CODE_STICK_Y:
                  code = CONTROLLER_ACTION_JOY_CODE_Y;
                  break;

              case CONTROLLER_EVT_CODE_THROTTLE:
                  code = CONTROLLER_ACTION_JOY_CODE_Z;
                  break;

              case CONTROLLER_EVT_CODE_HAT_X:
                  code = CONTROLLER_ACTION_JOY_CODE_RX;
                  break;

              case CONTROLLER_EVT_CODE_HAT_Y:
                  code = CONTROLLER_ACTION_JOY_CODE_RY;
                  break;

              case CONTROLLER_EVT_CODE_RUDDER:
                  code = CONTROLLER_ACTION_JOY_CODE_RZ;
                  break;

              default:
                  /* not reach */
                  code = 0;
                  break;
            }

            default_motion_mod[i].action_idx = ai;
            default_action_list[ai] = ai;
            default_action[ai].type = CONTROLLER_ACTION_JOY;
            default_action[ai].joy.flags = CONTROLLER_ACTION_JOY_VAL_THROUGH;
            default_action[ai].joy.code = code;
            ai += 1;
        } else {
            /* press/release action for d-pad/button */

            code = CONTROLLER_ACTION_JOY_CODE_HAT1_TOP +
                   (i - CONTROLLER_EVT_CODE_AXIS_MAX - 1);

            default_press_mod[i].action_idx = ai;
            default_action_list[ai] = ai;
            default_action[ai].type = CONTROLLER_ACTION_JOY;
            default_action[ai].joy.flags = CONTROLLER_ACTION_JOY_VAL_MAX;
            default_action[ai].joy.code = code;
            ai += 1;

            default_release_mod[i].action_idx = ai;
            default_action_list[ai] = ai;
            default_action[ai].type = CONTROLLER_ACTION_JOY;
            default_action[ai].joy.flags = CONTROLLER_ACTION_JOY_VAL_MIN;
            default_action[ai].joy.code = code;
            ai += 1;
        }
    }
}

