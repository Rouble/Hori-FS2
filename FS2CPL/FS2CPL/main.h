/*
 * main.h  -- definition of this program
 *
 * $Id: main.h,v 1.42 2004/06/13 12:59:47 hos Exp $
 *
 */

#undef __cplusplus
#include <stdio.h>
#include "report_data.h"
#include "resource.h"

#include <commctrl.h>

/* main.c: internal APIs */
int dll_inc_ref(void);
int dll_dec_ref(void);
int display_message_box(UINT text_id, UINT caption_id, HWND hwnd, DWORD style);

extern HINSTANCE instance;

/* setting.c: device setting */
#define ITEM_TYPE_CAT_NONE            0x0000
#define ITEM_TYPE_CAT_VERTICAL        0x0001
#define ITEM_TYPE_CAT_HORIZONTAL      0x0002
#define ITEM_TYPE_CAT_MOTION          0x0004
#define ITEM_TYPE_CAT_PRESS           0x0008
#define ITEM_TYPE_CAT_RELEASE         0x0010
#define ITEM_TYPE_CAT_LOWER_IN        0x0020
#define ITEM_TYPE_CAT_LOWER_OUT       0x0040
#define ITEM_TYPE_CAT_UPPER_IN        0x0080
#define ITEM_TYPE_CAT_UPPER_OUT       0x0100
#define ITEM_TYPE_CAT_CONTROLLER      0x0200
#define ITEM_TYPE_CAT_ACTION_ROOT     0x0400
#define ITEM_TYPE_CAT_ACTION          0x0800
#define ITEM_TYPE_CAT_ROOT            0x1000

#define ITEM_TYPE(up, dn) \
  ((ITEM_TYPE_CAT_ ##up << 16)  | ITEM_TYPE_CAT_ ##dn)
#define ITEM_TYPE_UP(type) (((type) >> 16) & 0xffff)
#define ITEM_TYPE_DN(type) ((type) & 0xffff)

#define ITEM_TYPE_AXIS_H              ITEM_TYPE(HORIZONTAL, NONE)
#define ITEM_TYPE_AXIS_V              ITEM_TYPE(VERTICAL, NONE)
#define ITEM_TYPE_AXIS_HV             ITEM_TYPE(HORIZONTAL, VERTICAL)
#define ITEM_TYPE_AXIS_TOP            ITEM_TYPE(LOWER_IN, LOWER_OUT)
#define ITEM_TYPE_AXIS_BTM            ITEM_TYPE(UPPER_IN, UPPER_OUT)
#define ITEM_TYPE_AXIS_LEFT           ITEM_TYPE(LOWER_IN, LOWER_OUT)
#define ITEM_TYPE_AXIS_RIGHT          ITEM_TYPE(UPPER_IN, UPPER_OUT)
#define ITEM_TYPE_BTN_DIGITAL         ITEM_TYPE(PRESS, RELEASE)
#define ITEM_TYPE_BTN_ANALOG          ITEM_TYPE(PRESS, RELEASE)
#define ITEM_TYPE_BTN_ANALOG_MOTION   ITEM_TYPE(MOTION, NONE)
#define ITEM_TYPE_CONTRLLER_ROOT      ITEM_TYPE(NONE, CONTROLLER)
#define ITEM_TYPE_ACTION_ROOT         ITEM_TYPE(NONE, ACTION_ROOT)
#define ITEM_TYPE_ACTION              ITEM_TYPE(NONE, ACTION)
#define ITEM_TYPE_ROOT                ITEM_TYPE(NONE, ROOT)

typedef struct _tree_item_spec {
    LPTSTR name;
    struct _tree_item_spec *sub_item;
    unsigned int type;
    int val1;
    int val2;

    UINT initial_state;
    HTREEITEM handle;
} tree_item_spec_t;

struct controller_event_config {
    int num_mod;
    struct controller_modifier_config *mod;
};

struct controller_action_config_list {
    int num_action;
    struct controller_action_config *action;
    LPWSTR name;

    tree_item_spec_t tree_item;
};

typedef struct {
    DWORD btn_a_sensitivity;
    DWORD btn_b_sensitivity;
    DWORD btn_threshold;

    DWORD lower_threshold;
    DWORD upper_threshold;

    DWORD initial_modifier;

    struct controller_event_config motion_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config press_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config release_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config lower_in_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config lower_out_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config upper_in_event[CONTROLLER_NUM_EVENT];
    struct controller_event_config upper_out_event[CONTROLLER_NUM_EVENT];

    int num_action_list;
    struct controller_action_config_list *action_list;

    LPWSTR profile_name;
} controller_setting_t;

typedef struct {
    USHORT id;                  /* joystick ID */
    HANDLE hid_device;          /* device handle */
    HDEVNOTIFY hid_device_notify; /* device notification handle */

    HWND sheet_hwnd;            /* sheet window handle */

    controller_setting_t conf;  /* settings */
} prop_common_data_t;

typedef struct _prop_page_context {
    struct uint_ptr_pair *page_proc; /* page proc alist */
    struct uint_ptr_pair *command_proc; /* page proc alist */
    struct uint_ptr_pair *notify_proc; /* page proc alist */

    prop_common_data_t *data;   /* pointer to common data */

    HWND page_hwnd;             /* page window handle */
    HWND subpage_hwnd;          /* subpage window handle */

    union {
        struct {
            HWND tree;
            HWND def_subpage;

            tree_item_spec_t *action_root;

            struct _prop_page_context *event_upper;
            struct _prop_page_context *event_lower;
            struct _prop_page_context *action;
            struct _prop_page_context *action_list;
            struct _prop_page_context *controller;
            struct _prop_page_context *top;
        } main;

        struct {
            HWND group;

            HWND list;

            HWND add;
            HWND del;
            HWND edit;

            HWND up;
            HWND down;

            HBITMAP up_bmp;
            HBITMAP dn_bmp;

            unsigned int type;
            int val;
            tree_item_spec_t *item;
            struct controller_event_config *event;
        } event;

        struct {
            HWND mod[16];
            HWND action;

            struct controller_modifier_config mod_conf;
        } event_dlg;

        struct {
            HWND name;

            HWND list;

            HWND add;
            HWND del;
            HWND edit;

            HWND up;
            HWND down;

            HBITMAP up_bmp;
            HBITMAP dn_bmp;

            tree_item_spec_t *item;
            struct controller_action_config_list *action;
        } action;

        struct {
            union {
                struct {
                    HWND type;
                    HWND page;

                    int num_page;
                    struct _prop_page_context *page_ctx;
                } main;

                struct {
                    HWND mod[16];
                } mod;

                struct {
                    HWND msec;
                    HWND spin;
                } delay;

                struct {
                    HWND target;
                    HWND type;

                    HWND sep;

                    HWND min;
                    HWND min_spin;
                    HWND max;
                    HWND max_spin;
                } joy;

                struct {
                    HWND mod[16];
                    HWND action;
                } apply;

                struct {
                    HWND target;
                    HWND type;
                } kbd;

                struct {
                    HWND target;
                    HWND type;

                    HWND sep;

                    HWND min;
                    HWND min_spin;
                    HWND max;
                    HWND max_spin;
                } mou;
            };

            HWND self;

            struct controller_action_config action_conf;
        } action_dlg;

        struct {
            HWND list;

            HWND add;
            HWND del;
            HWND edit;

            HWND up;
            HWND down;

            HBITMAP up_bmp;
            HBITMAP dn_bmp;
        } action_list;

        struct {
            HWND name;

            LPTSTR str_name;
        } action_name;

        struct {
            HWND btn_sensitivity; /* button sensitivity slider */
            HWND btn_threshold; /* button threshold slider */

            HWND axis_threshold; /* axis threshold */

            HWND viblation;     /* viblation slider */
            HWND viblation_check; /* viblation checkbox */
        } other;

        struct {
            HWND name;
        } top;

        struct {
            struct controller_joy_input_report in_rep; /* input report data */

            HANDLE read_thread;
            HANDLE read_thread_run;
            HANDLE read_thread_wakeup;
            int read_thread_stop;
        } in;
    };
} prop_page_context_t;

/* setting_data.c */
#define NUM_OF_DEF_ACTION (1 + 6 + (4 * 2 + 3 + 9 + 3) * 2)

extern
struct controller_modifier_config default_motion_mod[CONTROLLER_NUM_EVENT];
extern
struct controller_modifier_config default_press_mod[CONTROLLER_NUM_EVENT];
extern
struct controller_modifier_config default_release_mod[CONTROLLER_NUM_EVENT];
extern unsigned short default_action_list[NUM_OF_DEF_ACTION];
extern struct controller_action_config default_action[NUM_OF_DEF_ACTION];
extern LPWSTR default_profile_name;
extern LPWSTR default_action_name[];

void generate_default_setting(void);

/* class_factory.c: implementation of IClassFactory */
HRESULT class_factory_alloc(REFIID iid, LPVOID *obj);

/* prop_sheet.c: implementation of IDIGameCntrlPropSheet */
HRESULT prop_sheet_alloc(REFIID iid, LPVOID *obj);

/* prop_page.c: implementation of each property sheet */
INT_PTR CALLBACK prop_page_proc(HWND hwnd, UINT msg,
                                WPARAM wparam, LPARAM lparam);

typedef INT_PTR (*command_proc_t)(HWND, WORD, WORD, HWND,
                                  prop_page_context_t *);
INT_PTR window_command_proc(HWND hwnd, UINT msg,
                            WPARAM wparam, LPARAM lparam,
                            prop_page_context_t *ctx);

typedef INT_PTR (*notify_proc_t)(HWND, NMHDR *,
                                 prop_page_context_t *);
INT_PTR window_notify_proc(HWND hwnd, UINT msg,
                           WPARAM wparam, LPARAM lparam,
                           prop_page_context_t *ctx);

void get_dialog_item(HWND hwnd, struct uint_ptr_pair *map);

void set_modifier_bits_to_checkbox(HWND *chk, USHORT mod, USHORT mod_mask);
void get_modifier_bits_from_checkbox(HWND *chk, USHORT *mod, USHORT *mod_mask);

extern USHORT num_of_prop_pages;
extern LPWSTR prop_page_template[];
extern struct uint_ptr_pair *page_proc_list[];

/* prop_page_main.c: implementation of main setting page */
#define WM_SUBPAGE_CHANGE (WM_USER + 1)

#define WM_MAIN_TREE_CHANGE (WM_USER + 2)
#define MAIN_TREE_SELECT 1
#define MAIN_TREE_DELETE 2
#define MAIN_TREE_REBUILD 3
#define MAIN_TREE_REBUILD_ACTION 4
#define MAIN_TREE_ROOTNAME 5
#define MAIN_TREE_ADD_ITEM 6

#define WM_ACTION_DLG_QUERY_VALID (WM_USER + 3)

extern struct uint_ptr_pair main_setting_page[];
void modifier_to_string(LPWSTR str,
                        unsigned short mod, unsigned short mod_mask);

/* prop_page_in.c: implementation of input test page */
extern struct uint_ptr_pair input_test_page[];

/* prop_subpage.c: subpage */
INT_PTR CALLBACK prop_subpage_proc(HWND hwnd, UINT msg,
                                   WPARAM wparam, LPARAM lparam);

/* prop_subpage_event.c */
extern struct uint_ptr_pair event_subpage[];

/* prop_subpage_action.c */
extern struct uint_ptr_pair action_subpage[];

/* prop_subpage_action_data.c */
extern const LPWSTR action_code_button_base;
extern const struct uint_ptr_pair action_joy_flag_map[];
extern const struct uint_ptr_pair action_joy_code_map[];
extern const struct uint_ptr_pair action_mou_flag_map[];
extern const struct uint_ptr_pair action_mou_code_map[];
extern const struct uint_ptr_pair action_kbd_flag_map[];
extern const struct uint_ptr_pair action_kbd_code_map[];

LPWSTR kbd_code_to_str(USHORT code);

/* prop_subpage_actionlist.c */
extern struct uint_ptr_pair action_list_subpage[];

/* prop_subpage_other.c */
extern struct uint_ptr_pair other_setting_subpage[];

/* prop_subpage_top.c */
extern struct uint_ptr_pair top_setting_subpage[];

/* prop_dialog_event.c */
extern struct uint_ptr_pair event_dlg[];

/* prop_dialog_action.c */
extern struct uint_ptr_pair action_dlg[];

/* prop_dialog_actionname.c */
extern struct uint_ptr_pair action_name_dlg[];

/* setting.c: implementation to load/save settings */
int output_to_device(prop_common_data_t *comm_ctx,
                     const struct controller_output_report *rep);
int load_device_setting(controller_setting_t *conf);
int load_default_setting(controller_setting_t *conf);
int save_device_setting(controller_setting_t *conf);
void free_device_setting(controller_setting_t *conf);
int verify_device_setting(controller_setting_t *conf);

/* setting_file.c */
int load_file_setting(controller_setting_t *conf, LPCWSTR fname);
int save_file_setting(controller_setting_t *conf, LPCWSTR fname);

extern tree_item_spec_t custom_tree_root;

/* for debug */
#if DBG
#include <stdio.h>
#define DBG_OUT(str, a, b)                                      \
{                                                               \
    FILE *fp;                                                   \
    fp=fopen("d:\\debug.log", "a");                             \
    fprintf(fp, "%s: %d: " str "\n", __FILE__, __LINE__, a, b); \
    fclose(fp);                                                 \
}
#else
#define DBG_OUT(str, a, b)
#endif
