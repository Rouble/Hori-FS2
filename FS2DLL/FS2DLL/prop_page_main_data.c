/*
 * prop_page_main_data.c  -- data for main prop page
 *
 * $Id: prop_page_main_data.c,v 1.1 2004/06/08 09:31:45 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"

/* tree specification */
static tree_item_spec_t custom_tree_stick_sub_items[] = {
    {TEXT("left side"), NULL, ITEM_TYPE_AXIS_LEFT,
     CONTROLLER_EVT_CODE_STICK_X, 0},
    {TEXT("right side"), NULL, ITEM_TYPE_AXIS_RIGHT,
     CONTROLLER_EVT_CODE_STICK_X, 0},
    {TEXT("top side"), NULL, ITEM_TYPE_AXIS_TOP,
     CONTROLLER_EVT_CODE_STICK_Y, 0},
    {TEXT("bottom side"), NULL, ITEM_TYPE_AXIS_BTM,
     CONTROLLER_EVT_CODE_STICK_Y, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_hat_sub_items[] = {
    {TEXT("left side"), NULL, ITEM_TYPE_AXIS_LEFT,
     CONTROLLER_EVT_CODE_HAT_X, 0},
    {TEXT("right side"), NULL, ITEM_TYPE_AXIS_RIGHT,
     CONTROLLER_EVT_CODE_HAT_X, 0},
    {TEXT("top side"), NULL, ITEM_TYPE_AXIS_TOP,
     CONTROLLER_EVT_CODE_HAT_Y, 0},
    {TEXT("bottom side"), NULL, ITEM_TYPE_AXIS_BTM,
     CONTROLLER_EVT_CODE_HAT_Y, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_btn_a_items[] = {
    {TEXT("motion"), NULL, ITEM_TYPE_BTN_ANALOG_MOTION,
     CONTROLLER_EVT_CODE_BUTTON_A, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_btn_b_items[] = {
    {TEXT("motion"), NULL, ITEM_TYPE_BTN_ANALOG_MOTION,
     CONTROLLER_EVT_CODE_BUTTON_B, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_dpad1_items[] = {
    {TEXT("left"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD1_LEFT, 0},
    {TEXT("right"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD1_RIGHT, 0},
    {TEXT("top"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD1_TOP, 0},
    {TEXT("bottom"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD1_BOTTOM, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_stick_items[] = {
    {TEXT("STICK"), custom_tree_stick_sub_items, ITEM_TYPE_AXIS_HV,
     CONTROLLER_EVT_CODE_STICK_X, CONTROLLER_EVT_CODE_STICK_Y},
    {TEXT("HAT"), custom_tree_hat_sub_items, ITEM_TYPE_AXIS_HV,
     CONTROLLER_EVT_CODE_HAT_X, CONTROLLER_EVT_CODE_HAT_Y},

    {TEXT("TRIGGER"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_TRIGGER, 0},
    {TEXT("FIRE C"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_FIRE_C, 0},
    {TEXT("LAUNCH"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_LAUNCH, 0},
    {TEXT("BUTTON A"), custom_tree_btn_a_items, ITEM_TYPE_BTN_ANALOG,
     CONTROLLER_EVT_CODE_BUTTON_A, 0},
    {TEXT("BUTTON B"), custom_tree_btn_b_items, ITEM_TYPE_BTN_ANALOG,
     CONTROLLER_EVT_CODE_BUTTON_B, 0},
    {TEXT("HAT_PRESS"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_BUTTON_HAT, 0},
    {TEXT("SW 1"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_BUTTON_SW1, 0},

    {TEXT("D-PAD 1"), custom_tree_dpad1_items, 0, 0, 0, TVIS_EXPANDED},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_throttle_sub_items[] = {
    {TEXT("top side"), NULL, ITEM_TYPE_AXIS_TOP,
     CONTROLLER_EVT_CODE_THROTTLE, 0},
    {TEXT("bottom side"), NULL, ITEM_TYPE_AXIS_BTM,
     CONTROLLER_EVT_CODE_THROTTLE, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_rudder_sub_items[] = {
    {TEXT("left side"), NULL, ITEM_TYPE_AXIS_LEFT,
     CONTROLLER_EVT_CODE_RUDDER, 0},
    {TEXT("right side"), NULL, ITEM_TYPE_AXIS_RIGHT,
     CONTROLLER_EVT_CODE_RUDDER, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_dpad2_items[] = {
    {TEXT("left"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD2_LEFT, 0},
    {TEXT("right"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD2_RIGHT, 0},
    {TEXT("top"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD2_TOP, 0},
    {TEXT("bottom"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD2_BOTTOM, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_dpad3_items[] = {
    {TEXT("left"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD3_LEFT, 0},
    {TEXT("middle"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD3_MIDDLE, 0},
    {TEXT("right"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_DPAD3_RIGHT, 0},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_throttle_items[] = {
    {TEXT("THROTTLE"), custom_tree_throttle_sub_items, ITEM_TYPE_AXIS_V,
     CONTROLLER_EVT_CODE_THROTTLE, 0},
    {TEXT("RUDDER"), custom_tree_rudder_sub_items, ITEM_TYPE_AXIS_H,
     CONTROLLER_EVT_CODE_RUDDER, 0},

    {TEXT("BUTTON D"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_BUTTON_D, 0},
    {TEXT("BUTTON ST"), NULL, ITEM_TYPE_BTN_DIGITAL,
     CONTROLLER_EVT_CODE_BUTTON_ST, 0},
    {TEXT("M1"), NULL, ITEM_TYPE_BTN_DIGITAL, CONTROLLER_EVT_CODE_MODE_M1, 0},
    {TEXT("M2"), NULL, ITEM_TYPE_BTN_DIGITAL, CONTROLLER_EVT_CODE_MODE_M2, 0},
    {TEXT("M3"), NULL, ITEM_TYPE_BTN_DIGITAL, CONTROLLER_EVT_CODE_MODE_M3, 0},

    {TEXT("D-PAD 2"), custom_tree_dpad2_items, 0, 0, 0, TVIS_EXPANDED},
    {TEXT("D-PAD 3"), custom_tree_dpad3_items, 0, 0, 0, TVIS_EXPANDED},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_controller_items[] = {
    {TEXT("Stick Block"), custom_tree_stick_items,
     0, 0, 0, TVIS_EXPANDED},
    {TEXT("Throttle Block"), custom_tree_throttle_items,
     0, 0, 0, TVIS_EXPANDED},

    {NULL, NULL, 0, 0, 0}
};

static tree_item_spec_t custom_tree_items[] = {
    /* controller setting */
    {TEXT("Flight Stick 2"), custom_tree_controller_items,
     ITEM_TYPE_CONTRLLER_ROOT, 0, 0, TVIS_EXPANDED},
    {TEXT("Action List"), NULL, ITEM_TYPE_ACTION_ROOT, 0, 0, TVIS_EXPANDED},

    {NULL, NULL, 0, 0, 0}
};

tree_item_spec_t custom_tree_root = {
    /* root */
    TEXT("Customize"), custom_tree_items, ITEM_TYPE_ROOT, 0, 0, TVIS_EXPANDED
};
