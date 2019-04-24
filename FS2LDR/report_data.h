/*
 * report_data.h  -- define report data format 
 *
 * $Id: report_data.h,v 1.23 2004/06/11 18:16:22 hos Exp $
 *
 */


/* read report */

#define INPUT_REPORT_NUM_JOY_BUTTON 32
#define INPUT_REPORT_NUM_KBD_CODE   232
#define INPUT_REPORT_NUM_MOU_BUTTON 5

#pragma pack(push, 1)
struct controller_joy_input_report {
    UCHAR report_id;            /* Report ID: 1 */

    union {
        struct {
            UCHAR x;            /* X:  0x00 - 0xff */
            UCHAR y;            /* Y:  0x00 - 0xff */
            UCHAR z;            /* Z:  0x00 - 0xff */
            UCHAR rx;           /* RX: 0x00 - 0xff */
            UCHAR ry;           /* RY: 0x00 - 0xff */
            UCHAR rz;           /* RZ: 0x00 - 0xff */
            UCHAR slider;       /* SL: 0x00 - 0xff */
            UCHAR throttle;     /* TH: 0x00 - 0xff */
        };

        struct {
            UCHAR axis[8];      /* x, y, z, rx, ry, yz, slider, throttle */
        };
    };

    UCHAR hat1 : 4;             /* H1: 0x00 - 0x07 */
    UCHAR hat2 : 4;             /* H2: 0x00 - 0x07 */

    UCHAR button[(INPUT_REPORT_NUM_JOY_BUTTON + 7) / 8]; /* BN: 0x00/0x01 */

    USHORT modifier;            /* modifier state */
};

struct controller_kbd_input_report {
    UCHAR report_id;            /* Report ID: 2 */

    UCHAR keys[(INPUT_REPORT_NUM_KBD_CODE + 7) / 8]; /* 232 keys */
};

struct controller_mou_input_report {
    UCHAR report_id;            /* Report ID: 3 */

    CHAR x;                     /* x:     0x81 - 0x7f */
    CHAR y;                     /* y:     0x81 - 0x7f */
    CHAR wheel;                 /* wheel: 0x81 - 0x7f */

    UCHAR button[(INPUT_REPORT_NUM_MOU_BUTTON + 7) / 8]; /* 5 buttons */
};
#pragma pack(pop)


/* write report */

#define WRITE_REPORT_CMD_VIBLATION 0x01
#define WRITE_REPORT_CMD_RELOAD_SETTING 0x02

struct controller_output_report {
    UCHAR report_id;            /* Report ID: 1 */

    UCHAR command;              /* command: 0x00 - 0xff */
    UCHAR value;                /* value: 0x00 - 0xff */
};


/* setting stored in registroy */
#define REGISTRY_REL_PATH L"Software\\noname\\hfsd"
#define REGISTRY_BASE_PATH L"\\Registry\\Machine\\" REGISTRY_REL_PATH


/* configuration of value processor */

/* 6 axis, two 4-way hat sw, 9 buttons, one 3-way mode sw, one 3-way hat sw */
enum {
    CONTROLLER_EVT_CODE_STICK_X = 0x0000,
    CONTROLLER_EVT_CODE_STICK_Y,
    CONTROLLER_EVT_CODE_THROTTLE,
    CONTROLLER_EVT_CODE_HAT_X,
    CONTROLLER_EVT_CODE_HAT_Y,
    CONTROLLER_EVT_CODE_RUDDER,

    CONTROLLER_EVT_CODE_AXIS_MAX = CONTROLLER_EVT_CODE_RUDDER,

    CONTROLLER_EVT_CODE_DPAD1_TOP,
    CONTROLLER_EVT_CODE_DPAD1_RIGHT,
    CONTROLLER_EVT_CODE_DPAD1_BOTTOM,
    CONTROLLER_EVT_CODE_DPAD1_LEFT,

    CONTROLLER_EVT_CODE_DPAD2_TOP,
    CONTROLLER_EVT_CODE_DPAD2_RIGHT,
    CONTROLLER_EVT_CODE_DPAD2_BOTTOM,
    CONTROLLER_EVT_CODE_DPAD2_LEFT,

    CONTROLLER_EVT_CODE_TRIGGER,
    CONTROLLER_EVT_CODE_FIRE_C,
    CONTROLLER_EVT_CODE_LAUNCH,
    CONTROLLER_EVT_CODE_BUTTON_A,
    CONTROLLER_EVT_CODE_BUTTON_B,
    CONTROLLER_EVT_CODE_BUTTON_HAT,
    CONTROLLER_EVT_CODE_BUTTON_SW1,
    CONTROLLER_EVT_CODE_BUTTON_D,
    CONTROLLER_EVT_CODE_BUTTON_ST,

    CONTROLLER_EVT_CODE_MODE_M1,
    CONTROLLER_EVT_CODE_MODE_M2,
    CONTROLLER_EVT_CODE_MODE_M3,

    CONTROLLER_EVT_CODE_DPAD3_LEFT,
    CONTROLLER_EVT_CODE_DPAD3_MIDDLE,
    CONTROLLER_EVT_CODE_DPAD3_RIGHT,

    CONTROLLER_NUM_EVENT        /* = 29 */
};

/* 16 modifiers */
#define CONTROLLER_MODEFIER(n) ((USHORT)(1 << ((n) - 1)))

/* sizeof(struct controller_modifier_config) == 8 */
struct controller_modifier_config {
    USHORT mod;                 /* modifier */
    USHORT mod_mask;            /* modifier mask */

    USHORT action_idx;          /* index of action list */

    char reserved[2];
};


/* controller_action_config.type */
#define CONTROLLER_ACTION_NOP   0x00
#define CONTROLLER_ACTION_MOD   0x01
#define CONTROLLER_ACTION_DELAY 0x02
#define CONTROLLER_ACTION_JOY   0x03
#define CONTROLLER_ACTION_APPLY 0x04
#define CONTROLLER_ACTION_MOU   0x10
#define CONTROLLER_ACTION_KBD   0x11

/* controller_action_config.joy.flags */
#define CONTROLLER_ACTION_JOY_VAL_MIN     0x0001
#define CONTROLLER_ACTION_JOY_VAL_MAX     0x0002
#define CONTROLLER_ACTION_JOY_VAL_MDL     0x0003
#define CONTROLLER_ACTION_JOY_VAL_THROUGH 0x0004
#define CONTROLLER_ACTION_JOY_VAL_REVERSE 0x0005
#define CONTROLLER_ACTION_JOY_VAL_SPEC    0x0006
#define CONTROLLER_ACTION_JOY_VAL_RATIO   0x0007

/* controller_action_config.joy.code */
enum {
    CONTROLLER_ACTION_JOY_CODE_X = 0x0000,
    CONTROLLER_ACTION_JOY_CODE_Y,
    CONTROLLER_ACTION_JOY_CODE_Z,
    CONTROLLER_ACTION_JOY_CODE_RX,
    CONTROLLER_ACTION_JOY_CODE_RY,
    CONTROLLER_ACTION_JOY_CODE_RZ,
    CONTROLLER_ACTION_JOY_CODE_SLIDER,
    CONTROLLER_ACTION_JOY_CODE_THROTTLE,

    CONTROLLER_ACTION_JOY_CODE_HAT1_TOP,
    CONTROLLER_ACTION_JOY_CODE_HAT1_RIGHT,
    CONTROLLER_ACTION_JOY_CODE_HAT1_BOTTOM,
    CONTROLLER_ACTION_JOY_CODE_HAT1_LEFT,

    CONTROLLER_ACTION_JOY_CODE_HAT2_TOP,
    CONTROLLER_ACTION_JOY_CODE_HAT2_RIGHT,
    CONTROLLER_ACTION_JOY_CODE_HAT2_BOTTOM,
    CONTROLLER_ACTION_JOY_CODE_HAT2_LEFT,

    CONTROLLER_ACTION_JOY_CODE_BTN_MIN
};

#define CONTROLLER_ACTION_JOY_CODE_BTN(n) \
  (CONTROLLER_ACTION_JOY_CODE_BTN_MIN + (n))

#define CONTROLLER_ACTION_JOY_CODE_MAX \
  CONTROLLER_ACTION_JOY_CODE_BTN(INPUT_REPORT_NUM_JOY_BUTTON - 1)

/* controller_action_config.kbd.flags */
#define CONTROLLER_ACTION_KBD_PRESS   0x0001
#define CONTROLLER_ACTION_KBD_RELEASE 0x0002
#define CONTROLLER_ACTION_KBD_THROUGH 0x0003
#define CONTROLLER_ACTION_KBD_REVERSE 0x0004

/* controller_action_config.kbd.code */
/*   see; Universal Serial Bus HID Usage Tables
          Chapter 10 Keyboard/Keypad Page (0x07) */
#define CONTROLLER_ACTION_KBD_CODE_MIN 0x0000
#define CONTROLLER_ACTION_KBD_CODE_MAX \
  (CONTROLLER_ACTION_KBD_CODE_MIN + INPUT_REPORT_NUM_KBD_CODE - 1)

/* controller_action_config.mou.flags */
#define CONTROLLER_ACTION_MOU_VAL_MIN     0x0001
#define CONTROLLER_ACTION_MOU_VAL_MAX     0x0002
#define CONTROLLER_ACTION_MOU_VAL_MDL     0x0003
#define CONTROLLER_ACTION_MOU_VAL_THROUGH 0x0004
#define CONTROLLER_ACTION_MOU_VAL_REVERSE 0x0005
#define CONTROLLER_ACTION_MOU_VAL_SPEC    0x0006
#define CONTROLLER_ACTION_MOU_VAL_RATIO   0x0007

/* controller_action_config.mou.code */
#define CONTROLLER_ACTION_MOU_CODE_X      0x00
#define CONTROLLER_ACTION_MOU_CODE_Y      0x01
#define CONTROLLER_ACTION_MOU_CODE_WHEEL  0x02
#define CONTROLLER_ACTION_MOU_CODE_BTN(n) (0x03 + (n))

#define CONTROLLER_ACTION_MOU_CODE_MAX \
  CONTROLLER_ACTION_MOU_CODE_BTN(INPUT_REPORT_NUM_MOU_BUTTON - 1)

/* sizeof(struct controller_action_config) == 32 */
struct controller_action_config {
    UCHAR type;                 /* type of action */

    char reserved[3];

    union {
        /* modifier */
        struct {
            USHORT mod;         /* modifier */
            USHORT mod_mask;    /* modifier mask */
        } mod;

        /* delay */
        struct {
            ULONG msec;         /* millisecond */
        } delay;

        /* joystick */
        struct {
            USHORT flags;       /* flags */
            USHORT code;        /* code */
            UCHAR val;          /* optional value */
            char reserved[1];
            SHORT ratio_numerator; /* numerator of ratio */
            SHORT ratio_denominator; /* denominator of ratio */
            SHORT offset;       /* offset */
        } joy;

        /* apply other action */
        struct {
            USHORT mod;         /* modifier: apply if match */
            USHORT mod_mask;    /* modifier mask */
            USHORT action_idx;  /* action index */
        } apply;

        /* keyboard */
        struct {
            USHORT flags;       /* flags */
            USHORT code;        /* code */
        } kbd;

        /* mouse */
        struct {
            USHORT flags;       /* flags */
            USHORT code;        /* code */
            UCHAR val;          /* optional value */
            char reserved[1];
            SHORT ratio_numerator; /* numerator of ratio */
            SHORT ratio_denominator; /* denominator of ratio */
            SHORT offset;       /* offset */
        } mou;

        char place_holder[28];
    };
};
