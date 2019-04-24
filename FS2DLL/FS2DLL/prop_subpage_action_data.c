/*
 * prop_subpage_action_data.c  -- data of action subpage
 *
 * $Id: prop_subpage_action_data.c,v 1.9 2004/06/03 05:07:06 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"
#include "util.h"


const LPWSTR action_code_button_base = L"Button %d";


/* str name of controller_action_config.joy.flags */
const struct uint_ptr_pair action_joy_flag_map[] = {
    {CONTROLLER_ACTION_JOY_VAL_MIN, L"Minimum"},
    {CONTROLLER_ACTION_JOY_VAL_MAX, L"Maximum"},
    {CONTROLLER_ACTION_JOY_VAL_MDL, L"Center"},
    {CONTROLLER_ACTION_JOY_VAL_THROUGH, L"Pass through"},
    {CONTROLLER_ACTION_JOY_VAL_REVERSE, L"Reverse"},
    {CONTROLLER_ACTION_JOY_VAL_SPEC, L"Specified value"},
    {CONTROLLER_ACTION_JOY_VAL_RATIO, L"Specified range"},

    {0, NULL}
};

/* str name of controller_action_config.joy.code */
const struct uint_ptr_pair action_joy_code_map[] = {
    {CONTROLLER_ACTION_JOY_CODE_X, L"Axis X"},
    {CONTROLLER_ACTION_JOY_CODE_Y, L"Axis Y"},
    {CONTROLLER_ACTION_JOY_CODE_Z, L"Axis Z"},
    {CONTROLLER_ACTION_JOY_CODE_RX, L"Axis RX"},
    {CONTROLLER_ACTION_JOY_CODE_RY, L"Axis RY"},
    {CONTROLLER_ACTION_JOY_CODE_RZ, L"Axis RZ"},
    {CONTROLLER_ACTION_JOY_CODE_SLIDER, L"Axis Slider"},
    {CONTROLLER_ACTION_JOY_CODE_THROTTLE, L"Axis Throttle"},

    {CONTROLLER_ACTION_JOY_CODE_HAT1_TOP, L"POV 1 top"},
    {CONTROLLER_ACTION_JOY_CODE_HAT1_RIGHT, L"POV 1 right"},
    {CONTROLLER_ACTION_JOY_CODE_HAT1_BOTTOM, L"POV 1 bottom"},
    {CONTROLLER_ACTION_JOY_CODE_HAT1_LEFT, L"POV 1 left"},

    {CONTROLLER_ACTION_JOY_CODE_HAT2_TOP, L"POV 2 top"},
    {CONTROLLER_ACTION_JOY_CODE_HAT2_RIGHT, L"POV 2 right"},
    {CONTROLLER_ACTION_JOY_CODE_HAT2_BOTTOM, L"POV 2 bottom"},
    {CONTROLLER_ACTION_JOY_CODE_HAT2_LEFT, L"POV 2 left"},

    {0, NULL}
};


/* str name of controller_action_config.mou.flags */
const struct uint_ptr_pair action_mou_flag_map[] = {
    {CONTROLLER_ACTION_MOU_VAL_MIN, L"Minimum"},
    {CONTROLLER_ACTION_MOU_VAL_MAX, L"Maximum"},
    {CONTROLLER_ACTION_MOU_VAL_MDL, L"Center"},
    {CONTROLLER_ACTION_MOU_VAL_THROUGH, L"Pass through"},
    {CONTROLLER_ACTION_MOU_VAL_REVERSE, L"Reverse"},
    {CONTROLLER_ACTION_MOU_VAL_SPEC, L"Specified value"},
    {CONTROLLER_ACTION_MOU_VAL_RATIO, L"Specified range"},

    {0, NULL}
};

/* str name of controller_action_config.mou.code */
const struct uint_ptr_pair action_mou_code_map[] = {
    {CONTROLLER_ACTION_MOU_CODE_X, L"X"},
    {CONTROLLER_ACTION_MOU_CODE_Y, L"Y"},
    {CONTROLLER_ACTION_MOU_CODE_WHEEL, L"Wheel"},

    {0, NULL}
};


/* str name of controller_action_config.kbd.flags */
const struct uint_ptr_pair action_kbd_flag_map[] = {
    {CONTROLLER_ACTION_KBD_PRESS, L"Press"},
    {CONTROLLER_ACTION_KBD_RELEASE, L"Release"},
    {CONTROLLER_ACTION_KBD_THROUGH, L"Pass through"},
    {CONTROLLER_ACTION_KBD_REVERSE, L"Reverse"},

    {0, NULL}
};

/* str name of controller_action_config.kbd.code */
const struct uint_ptr_pair action_kbd_code_map[] = {
    {0x0004, L"a / A"},
    {0x0005, L"b / B"},
    {0x0006, L"c / C"},
    {0x0007, L"d / D"},
    {0x0008, L"e / E"},
    {0x0009, L"f / F"},
    {0x000a, L"g / G"},
    {0x000b, L"h / H"},
    {0x000c, L"i / I"},
    {0x000d, L"j / J"},
    {0x000e, L"k / K"},
    {0x000f, L"l / L"},

    {0x0010, L"m / M"},
    {0x0011, L"n / N"},
    {0x0012, L"o / O"},
    {0x0013, L"p / P"},
    {0x0014, L"q / Q"},
    {0x0015, L"r / R"},
    {0x0016, L"s / S"},
    {0x0017, L"t / T"},
    {0x0018, L"u / U"},
    {0x0019, L"v / V"},
    {0x001a, L"w / W"},
    {0x001b, L"x / X"},
    {0x001c, L"y / Y"},
    {0x001d, L"z / Z"},
    {0x001e, L"1"},
    {0x001f, L"2"},

    {0x0020, L"3"},
    {0x0021, L"4"},
    {0x0022, L"5"},
    {0x0023, L"6"},
    {0x0024, L"7"},
    {0x0025, L"8"},
    {0x0026, L"9"},
    {0x0027, L"0"},
    {0x0028, L"Enter"},
    {0x0029, L"Esc"},
    {0x002a, L"Back Space"},
    {0x002b, L"Tab"},
    {0x002c, L"Space"},
    {0x002d, L"- / _"},
    {0x002e, L"= / +"},
    {0x002f, L"[ / {"},

    {0x0030, L"] / }"},
    {0x0031, L"\\ / |"},
    {0x0032, L"Non-US # / ~"},
    {0x0033, L"; / :"},
    {0x0034, L"\x91 / \x93"},
    {0x0035, L"Grave Accent / Tilde"},
    {0x0036, L", / <"},
    {0x0037, L". / >"},
    {0x0038, L"/ / ?"},
    {0x0039, L"Caps Lock"},
    {0x003a, L"F1"},
    {0x003b, L"F2"},
    {0x003c, L"F3"},
    {0x003d, L"F4"},
    {0x003e, L"F5"},
    {0x003f, L"F6"},

    {0x0040, L"F7"},
    {0x0041, L"F8"},
    {0x0042, L"F9"},
    {0x0043, L"F10"},
    {0x0044, L"F11"},
    {0x0045, L"F12"},
    {0x0046, L"Print Screen"},
    {0x0047, L"Scroll Lock"},
    {0x0048, L"Pause"},
    {0x0049, L"Insert"},
    {0x004a, L"Home"},
    {0x004b, L"Page Up"},
    {0x004c, L"Delete"},
    {0x004d, L"End"},
    {0x004e, L"Page Down"},
    {0x004f, L"Right Arrow"},

    {0x0050, L"Left Arrow"},
    {0x0051, L"Down Arrow"},
    {0x0052, L"Up Arrow"},
    {0x0053, L"Keypad Num Lock / Clear"},
    {0x0054, L"Keypad /"},
    {0x0055, L"Keypad *"},
    {0x0056, L"Keypad -"},
    {0x0057, L"Keypad +"},
    {0x0058, L"Keypad Enter"},
    {0x0059, L"Keypad 1 / End"},
    {0x005a, L"Keypad 2 / Down Arrow"},
    {0x005b, L"Keypad 3 / Page Down"},
    {0x005c, L"Keypad 4 / Left Arrow"},
    {0x005d, L"Keypad 5"},
    {0x005e, L"Keypad 6 / Right Arrow"},
    {0x005f, L"Keypad 7 / Home"},

    {0x0060, L"Keypad 8 / Up Arrow"},
    {0x0061, L"Keypad 9 / Page Up"},
    {0x0062, L"Keypad 0 / Insert"},
    {0x0063, L"Keypad . / Delete"},
    {0x0064, L"Non-US \\ / |"},
    {0x0065, L"Application"},
#ifdef SHOW_ALL_HID_KEYCODE
    {0x0066, L"Power"},
    {0x0067, L"Keypad ="},
    {0x0068, L"F13"},
    {0x0069, L"F14"},
    {0x006a, L"F15"},
    {0x006b, L"F16"},
    {0x006c, L"F17"},
    {0x006d, L"F18"},
    {0x006e, L"F19"},
    {0x006f, L"F20"},

    {0x0070, L"F21"},
    {0x0071, L"F22"},
    {0x0072, L"F23"},
    {0x0073, L"F24"},
    {0x0074, L"Execute"},
    {0x0075, L"Help"},
    {0x0076, L"Menu"},
    {0x0077, L"Select"},
    {0x0078, L"Stop"},
    {0x0079, L"Again"},
    {0x007a, L"Undo"},
    {0x007b, L"Cut"},
    {0x007c, L"Copy"},
    {0x007d, L"Paste"},
    {0x007e, L"Find"},
    {0x007f, L"Mute"},

    {0x0080, L"Volume Up"},
    {0x0081, L"Volume Down"},
    {0x0082, L"Locking Caps Lock"},
    {0x0083, L"Locking Num Lock"},
    {0x0084, L"Locking Scroll Lock"},
    {0x0085, L"Keypad Comma"},
    {0x0086, L"Keypad Equal Sign"},
    {0x0087, L"International 1"},
    {0x0088, L"International 2"},
    {0x0089, L"International 3"},
    {0x008a, L"International 4"},
    {0x008b, L"International 5"},
    {0x008c, L"International 6"},
    {0x008d, L"International 7"},
    {0x008e, L"International 8"},
    {0x008f, L"International 9"},

    {0x0090, L"LANG 1"},
    {0x0091, L"LANG 2"},
    {0x0092, L"LANG 3"},
    {0x0093, L"LANG 4"},
    {0x0094, L"LANG 5"},
    {0x0095, L"LANG 6"},
    {0x0096, L"LANG 7"},
    {0x0097, L"LANG 8"},
    {0x0098, L"LANG 9"},
    {0x0099, L"Alternate Erase"},
    {0x009a, L"SysReq / Attenton"},
    {0x009b, L"Cancel"},
    {0x009c, L"Clear"},
    {0x009d, L"Prior"},
    {0x009e, L"Return"},
    {0x009f, L"Separator"},

    {0x00a0, L"Out"},
    {0x00a1, L"Oper"},
    {0x00a2, L"Clear / Again"},
    {0x00a3, L"CrSel / Props"},
    {0x00a4, L"ExSel"},

    {0x00b0, L"Keypad 00"},
    {0x00b1, L"Keypad 000"},
    {0x00b2, L"Thousands Separator"},
    {0x00b3, L"Decimal Separator"},
    {0x00b4, L"Currency Unit"},
    {0x00b5, L"Currency Sub-unit"},
    {0x00b6, L"Keypad ("},
    {0x00b7, L"Keypad )"},
    {0x00b8, L"Keypad {"},
    {0x00b9, L"Keypad }"},
    {0x00ba, L"Keypad Tab"},
    {0x00bb, L"Keypad Backspace"},
    {0x00bc, L"Keypad A"},
    {0x00bd, L"Keypad B"},
    {0x00be, L"Keypad C"},
    {0x00bf, L"Keypad D"},

    {0x00c0, L"Keypad E"},
    {0x00c1, L"Keypad F"},
    {0x00c2, L"Keypad XOR"},
    {0x00c3, L"Keypad ^"},
    {0x00c4, L"Keypad %"},
    {0x00c5, L"Keypad <"},
    {0x00c6, L"Keypad >"},
    {0x00c7, L"Keypad &"},
    {0x00c8, L"Keypad &&"},
    {0x00c9, L"Keypad |"},
    {0x00ca, L"Keypad ||"},
    {0x00cb, L"Keypad :"},
    {0x00cc, L"Keypad #"},
    {0x00cd, L"Keypad Space"},
    {0x00ce, L"Keypad @"},
    {0x00cf, L"Keypad !"},

    {0x00d0, L"Keypad Memory Store"},
    {0x00d1, L"Keypad Memory Recall"},
    {0x00d2, L"Keypad Memory Clear"},
    {0x00d3, L"Keypad Memory Add"},
    {0x00d4, L"Keypad Memory Subtract"},
    {0x00d5, L"Keypad Memory Multiply"},
    {0x00d6, L"Keypad Memory Divide"},
    {0x00d7, L"Keypad +/-"},
    {0x00d8, L"Keypad Clear"},
    {0x00d9, L"Keypad Clear Entry"},
    {0x00da, L"Keypad Binary"},
    {0x00db, L"Keypad Octal"},
    {0x00dc, L"Keypad Decimal"},
    {0x00dd, L"Keypad Hexadecimal"},
#endif

    {0x00e0, L"Left Control"},
    {0x00e1, L"Left Shift"},
    {0x00e2, L"Left Alt"},
    {0x00e3, L"Left GUI"},
    {0x00e4, L"Right Control"},
    {0x00e5, L"Right Shift"},
    {0x00e6, L"Right Alt"},
    {0x00e7, L"Right GUI"},

    {0, NULL}
};

/* str name of controller_action_config.kbd.code
   for japanese 106-based key-layout specific */
const struct uint_ptr_pair action_kbd_code_map_jp106[] = {
    {0x002d, L"- / ="},
    {0x002e, L"^ / \x7e"},      /* ^ and tilde */
    {0x002f, L"@ / `"},

    {0x0030, L"[ / {"},
    {0x0031, L"] / }"},
    {0x0032, L"] / }"},
    {0x0033, L"; / +"},
    {0x0034, L": / *"},
    {0x0035, L"\x534a\x89d2/\x5168\x89d2"}, /* Hankaku/Zenkaku */

    {0x0064, L""},

    {0x0087, L"\xff3c / _"},    /* International 1: reverse solidus and _ */
    {0x0088, L"\x30ab\x30bf\x30ab\x30ca/\x3072\x3089\x304c\x306a"},
                                /* International 2: Katakana/Hiragana */
    {0x0089, L"\xa5 / |"},      /* International 3: yen sign and | */
    {0x008a, L"\x5909\x63db"},  /* International 4: Henkan */
    {0x008b, L"\x7121\x5909\x63db"}, /* International 5: Muhenkan */

    {0, NULL}
};

/* str name table of keyboard layout specific */
const struct uint_ptr_pair action_kbd_code_map_map[] = {
    {MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), &action_kbd_code_map_jp106},

    {0, NULL}
};


/* todo: This function is incomplete. */
LPWSTR kbd_code_to_str(USHORT code)
{
    LPWSTR str;
    HKL kl;
    struct uint_ptr_pair *lang_spec_code_map;

    str = NULL;

    /* get current keyboard layout */
    kl = GetKeyboardLayout(0);

    /* get key-code desc from current layout specific table */
    lang_spec_code_map = (struct uint_ptr_pair *)
                         assq_pair(action_kbd_code_map_map, LOWORD(kl), NULL);
    if(lang_spec_code_map != NULL) {
        str = (LPWSTR)assq_pair(lang_spec_code_map, code, NULL);
    }

    if(str == NULL) {
        /* get key-code desc from common table, if not found from spec table */
        str = (LPWSTR)assq_pair(action_kbd_code_map, code, NULL);
    }

    if(str == NULL) {
        /* not found */
        return NULL;
    }

    if(str[0] == 0) {
        /* not used entry */
        return NULL;
    }

    return str;
}
