/*
 * setting.c   -- load/save device setting
 *
 * $Id: setting.c,v 1.29 2004/06/09 13:39:30 hos Exp $
 *
 */


#include "main.h"


/* default settings */
#define NUM_OF_DEF_ACTION (1 + 6 + (4 * 2 + 3 + 9 + 3) * 2)

static
struct controller_modifier_config default_motion_mod[CONTROLLER_NUM_EVENT];
static
struct controller_modifier_config default_press_mod[CONTROLLER_NUM_EVENT];
static
struct controller_modifier_config default_release_mod[CONTROLLER_NUM_EVENT];
static unsigned short default_action_list[NUM_OF_DEF_ACTION];
static struct controller_action_config default_action[NUM_OF_DEF_ACTION];



static HANDLE open_registry_subkey(IN PWSTR subkey)
{
    WCHAR buf[256];
    UNICODE_STRING path;
    OBJECT_ATTRIBUTES obj_attr;
    HANDLE key;
    NTSTATUS nt_ret;

    DBG_ENTER2(("\"%ws\"", subkey));

    RtlZeroMemory(buf, sizeof(buf));
    path.Length = 0;
    path.MaximumLength = sizeof(buf);
    path.Buffer = buf;

    RtlAppendUnicodeToString(&path, REGISTRY_BASE_PATH);
    RtlAppendUnicodeToString(&path, L"\\");
    RtlAppendUnicodeToString(&path, subkey);

    InitializeObjectAttributes(&obj_attr, &path,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    nt_ret = ZwOpenKey(&key, KEY_QUERY_VALUE, &obj_attr);
    if(! NT_SUCCESS(nt_ret)) {
        DBG_LEAVE(("NULL: ZwOpenKey failed: %x", nt_ret));
        return NULL;
    }

    DBG_LEAVE2(("%p", key));
    return key;
}

static NTSTATUS get_registry_data(IN PWSTR subkey, IN PWSTR name,
                                  OUT PKEY_VALUE_PARTIAL_INFORMATION *val)
{
    HANDLE key;
    UNICODE_STRING uname;
    PKEY_VALUE_PARTIAL_INFORMATION val_info;
    ULONG size;
    NTSTATUS nt_ret;

    DBG_ENTER2(("\"%ws\", \"%ws\", %p", subkey, name, val));

    /* open subkey */
    key = open_registry_subkey(subkey);
    if(key == NULL) {
        *val = NULL;
        DBG_LEAVE(("STATUS_OBJECT_NAME_NOT_FOUND"));
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    RtlInitUnicodeString(&uname, name);

    /* query size of data */
    nt_ret = ZwQueryValueKey(key, &uname,
                             KeyValuePartialInformation, NULL, 0,
                             &size);
    if(! NT_SUCCESS(nt_ret) && nt_ret != STATUS_BUFFER_TOO_SMALL) {
        ZwClose(key);
        *val = NULL;
        DBG_LEAVE2(("%x", nt_ret));
        return nt_ret;
    }

    /* allocate for query */
    val_info =
        (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(NonPagedPool, size);
    if(val_info == NULL) {
        ZwClose(key);
        *val = NULL;
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* read data */
    nt_ret = ZwQueryValueKey(key, &uname,
                             KeyValuePartialInformation, val_info, size,
                             &size);
    ZwClose(key);

    if(! NT_SUCCESS(nt_ret)) {
        ExFreePool(val_info);
        *val = NULL;
        DBG_LEAVE(("%x", nt_ret));
        return nt_ret;
    }

    *val = val_info;
    DBG_LEAVE2(("STATUS_SUCCESS"));
    return STATUS_SUCCESS;
}

static NTSTATUS get_registry_dword(IN PWSTR subkey, IN PWSTR name,
                                   OUT PDWORD val, IN unsigned int def_val)
{
    PKEY_VALUE_PARTIAL_INFORMATION val_info;
    NTSTATUS nt_ret;

    DBG_ENTER2(("\"%ws\", \"%ws\", %p, %u", subkey, name, val, def_val));

    nt_ret = get_registry_data(subkey, name, &val_info);
    if(! NT_SUCCESS(nt_ret)) {
        *val = def_val;
        DBG_LEAVE2(("%x, %u", nt_ret, *val));
        return nt_ret;
    }

    if(val_info->Type != REG_DWORD || val_info->DataLength != sizeof(unsigned int)) {
        ExFreePool(val_info);
        *val = def_val;
        DBG_LEAVE(("STATUS_UNSUCCESSFUL, %u", *val));
        return STATUS_UNSUCCESSFUL;
    }

    *val = *((unsigned int *)val_info->Data);

    ExFreePool(val_info);

    DBG_LEAVE2(("STATUS_SUCCESS: %u", *val));
    return STATUS_SUCCESS;
}

static NTSTATUS alloc_copy_data(OUT PVOID dst, OUT PULONG dst_size,
                                IN PVOID src, IN ULONG src_size)
{
    void **dst_ptr;

    DBG_ENTER2(("%p, %p, %p, %u", dst, dst_size, src, src_size));

    dst_ptr = (void **)dst;

    if(src_size == 0) {
        *dst_ptr = NULL;
        *dst_size = 0;
        DBG_LEAVE2(("STATUS_SUCCESS: %p, %u", *dst_ptr, *dst_size));
        return STATUS_SUCCESS;
    }

    *dst_ptr = ExAllocatePool(NonPagedPool, src_size);
    if(*dst_ptr == NULL) {
        *dst_size = 0;
        DBG_LEAVE(("STATUS_INSUFFICIENT_RESOURCES: %p, %u",
                   *dst_ptr, *dst_size));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(*dst_ptr, src, src_size);
    *dst_size = src_size;

    DBG_LEAVE2(("STATUS_SUCCESS: %p, %u", *dst_ptr, *dst_size));
    return STATUS_SUCCESS;
}

static NTSTATUS get_registry_binary(IN PWSTR subkey, IN PWSTR name,
                                    OUT PVOID val, OUT PULONG size,
                                    IN PVOID def_val, IN ULONG def_size)
{
    PKEY_VALUE_PARTIAL_INFORMATION val_info;
    NTSTATUS nt_ret;

    DBG_ENTER2(("\"%ws\", \"%ws\", %p, %p, %u",
               subkey, name, val, def_val, def_size));

    nt_ret = get_registry_data(subkey, name, &val_info);
    if(! NT_SUCCESS(nt_ret)) {
        nt_ret = alloc_copy_data(val, size, def_val, def_size);
        DBG_LEAVE2(("%x, %u", nt_ret, *size));
        return nt_ret;
    }

    if(val_info->Type != REG_BINARY) {
        ExFreePool(val_info);
        nt_ret = alloc_copy_data(val, size, def_val, def_size);
        DBG_LEAVE(("type not REG_BINARY: %x, %u, %u",
                   nt_ret, *size, val_info->Type));
        return nt_ret;
    }

    nt_ret = alloc_copy_data(val, size, val_info->Data, val_info->DataLength);

    ExFreePool(val_info);

    DBG_LEAVE2(("%x, %u", nt_ret, *size));
    return nt_ret;
}


static VOID load_settings_from_subkey(IN PDEVICE_OBJECT dev_obj,
                                      IN PWSTR subkey)
{
    device_extension_t *dev_ext;

    DBG_ENTER(("%p, \"%ws\"", dev_obj, subkey));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* load DWORD setting */
    {
        struct {
            PWSTR name;
            PDWORD ptr;
            unsigned int def_val;
        } *lptr, name_ptr_list[] = {
            {L"button A max", &dev_ext->conf.button_a_max, 0x80},
            {L"button B max", &dev_ext->conf.button_b_max, 0x80},
            {L"button threshold", &dev_ext->conf.button_threshold, 0x80},
            {L"lower threshold", &dev_ext->conf.lower_threshold, 0x10},
            {L"upper threshold", &dev_ext->conf.upper_threshold, 0xef},
            {L"initial modifier", &dev_ext->conf.initial_modifier, 0x0000},

            {NULL, NULL, 0}
        };

        for(lptr = name_ptr_list; lptr->name != NULL; lptr++) {
            get_registry_dword(subkey, lptr->name, lptr->ptr, lptr->def_val);
        }
    }

    /* load event config */
    {
        struct controller_modifier_config *mod, *def_mod;
        unsigned short *act_list;
        WCHAR buf[64], name[256];
        UNICODE_STRING ubuf, uname;
        ULONG size, def_size;
        int i, nmod, nact;

        struct {
            PWSTR name;
            struct controller_event_config *event;
            struct controller_modifier_config *def_mod;
        } *lptr, evt_name_list[] = {
            {L"motion ", dev_ext->conf.motion_event, default_motion_mod},
            {L"press ", dev_ext->conf.press_event, default_press_mod},
            {L"release ", dev_ext->conf.release_event, default_release_mod},
            {L"lower in ", dev_ext->conf.lower_in_event, NULL},
            {L"lower out ", dev_ext->conf.lower_out_event, NULL},
            {L"upper in ", dev_ext->conf.upper_in_event, NULL},
            {L"upper out ", dev_ext->conf.upper_out_event, NULL},

            {NULL, NULL, NULL}
        };

        /* load modifier/action assoc list of each event */
        for(lptr = evt_name_list; lptr->name != NULL; lptr++) {
            for(i = 0; i < CONTROLLER_NUM_EVENT; i++) {
                /* value ID */
                RtlZeroMemory(buf, sizeof(buf));
                ubuf.Length = 0;
                ubuf.MaximumLength = sizeof(buf);
                ubuf.Buffer = buf;

                RtlIntegerToUnicodeString(i, 16, &ubuf);

                /* prepend event name */
                RtlZeroMemory(name, sizeof(name));
                uname.Length = 0;
                uname.MaximumLength = sizeof(name);
                uname.Buffer = name;

                RtlAppendUnicodeToString(&uname, lptr->name);
                RtlAppendUnicodeStringToString(&uname, &ubuf);

                /* load modifier list */
                size = 0;
                def_mod = ((lptr->def_mod != NULL &&
                            lptr->def_mod[i].action_idx != 0) ?
                           &lptr->def_mod[i] : NULL);
                def_size = (def_mod != NULL ?
                            sizeof(struct controller_modifier_config) : 0);
                get_registry_binary(subkey, name,
                                    &mod, &size,
                                    def_mod, def_size);
                nmod = size / sizeof(struct controller_modifier_config);

                lptr->event[i].num_mod = nmod;
                lptr->event[i].mod = mod;

                DBG_OUT2(("modifier load: \"%ws\", %u",
                          name, lptr->event[i].num_mod));
            }
        }

        /* load list of action list */
        {
            get_registry_binary(subkey, L"action list",
                                &act_list, &size,
                                default_action_list,
                                sizeof(default_action_list));
            nact = size / sizeof(unsigned short);
            dev_ext->conf.num_action_list = nact;

            DBG_OUT2(("list of action list load: \"%ws\", %d",
                      L"action list", nact));
        }

        /* load action list */
        if(nact == 0) {
            dev_ext->conf.action_list = NULL;
        } else {
            struct controller_action_config *act, *def_act;

            /* setup list of action list */
            dev_ext->conf.action_list =
                (struct controller_action_config_list *)
                ExAllocatePool(NonPagedPool,
                               sizeof(struct controller_action_config_list) *
                               nact);
            if(dev_ext->conf.action_list == NULL) {
                DBG_LEAVE(("void: ExAllocatePool failed"));
                ExFreePool(act_list);
                return;
            }

            for(i = 0; i < nact; i++) {
                /* action ID */
                RtlZeroMemory(buf, sizeof(buf));
                ubuf.Length = 0;
                ubuf.MaximumLength = sizeof(buf);
                ubuf.Buffer = buf;

                RtlIntegerToUnicodeString(act_list[i], 16, &ubuf);

                /* prepend "action list " */
                RtlZeroMemory(name, sizeof(name));
                uname.Length = 0;
                uname.MaximumLength = sizeof(name);
                uname.Buffer = name;

                RtlAppendUnicodeToString(&uname, L"action list ");
                RtlAppendUnicodeStringToString(&uname, &ubuf);

                /* load action list */
                size = 0;
                def_act = (i < NUM_OF_DEF_ACTION ?
                           &default_action[default_action_list[i]] : NULL);
                def_size = (def_act != NULL ?
                            sizeof(struct controller_action_config) : 0);
                get_registry_binary(subkey, name,
                                    &act, &size,
                                    def_act, def_size);

                dev_ext->conf.action_list[i].num_action =
                    size / sizeof(struct controller_action_config);
                dev_ext->conf.action_list[i].action = act;

                DBG_OUT2(("action list load: \"%ws\", %u",
                          name, dev_ext->conf.action_list[i].num_action));
            }

            ExFreePool(act_list);
        }
    }

    DBG_LEAVE(("void"));
}

static VOID load_settings_by_name(IN PDEVICE_OBJECT dev_obj, IN PWSTR name)
{
    DWORD val;
    UNICODE_STRING id_ubuf, subkey_ubuf;
    WCHAR id_buf[64], subkey_buf[256];
    NTSTATUS nt_ret;

    DBG_ENTER(("%p, \"%ws\"", dev_obj, name));

    /* get name -> ID mapping */
    nt_ret = get_registry_dword(L"map", name, &val, 0);
    DBG_OUT_IF(! NT_SUCCESS(nt_ret), ("no mapping: %x", nt_ret));

    /* convert ID to UNICODE_STRING */
    RtlZeroMemory(id_buf, sizeof(id_buf));
    id_ubuf.Length = 0;
    id_ubuf.MaximumLength = sizeof(id_buf);
    id_ubuf.Buffer = id_buf;

    RtlIntegerToUnicodeString(val, 16, &id_ubuf);

    /* prepend "parameters\" */
    RtlZeroMemory(subkey_buf, sizeof(subkey_buf));
    subkey_ubuf.Length = 0;
    subkey_ubuf.MaximumLength = sizeof(subkey_buf);
    subkey_ubuf.Buffer = subkey_buf;

    RtlAppendUnicodeToString(&subkey_ubuf, L"parameters\\");
    RtlAppendUnicodeStringToString(&subkey_ubuf, &id_ubuf);

    /* load parameters */
    load_settings_from_subkey(dev_obj, subkey_ubuf.Buffer);

    DBG_LEAVE(("void"));
}

static void generate_default_setting(void)
{
    int i, ai;
    USHORT code;

    DBG_ENTER(("void"));

    RtlZeroMemory(default_motion_mod, sizeof(default_motion_mod));
    RtlZeroMemory(default_press_mod, sizeof(default_press_mod));
    RtlZeroMemory(default_release_mod, sizeof(default_release_mod));
    RtlZeroMemory(default_action_list, sizeof(default_action_list));
    RtlZeroMemory(default_action, sizeof(default_action));

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

    DBG_LEAVE(("void: %d, %d", i, ai));
}

VOID free_settings(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    int i;

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    /* free pending action before free settings:
       pending action references action list inside settings */
    free_pending_action(dev_obj);

#define FREE_IF_NON_NULL(_p_)                           \
  if((_p_) != NULL) { ExFreePool(_p_); (_p_) = NULL; }
#define FREE_MOD_IF_NON_NULL(_p_)               \
  FREE_IF_NON_NULL((_p_).mod);                  \
  (_p_).num_mod = 0;

    /* for each value config */
    for(i = 0; i < CONTROLLER_NUM_EVENT; i++) {
        FREE_MOD_IF_NON_NULL(dev_ext->conf.motion_event[i]);
        FREE_MOD_IF_NON_NULL(dev_ext->conf.press_event[i]);
        FREE_MOD_IF_NON_NULL(dev_ext->conf.release_event[i]);
        FREE_MOD_IF_NON_NULL(dev_ext->conf.lower_in_event[i]);
        FREE_MOD_IF_NON_NULL(dev_ext->conf.lower_out_event[i]);
        FREE_MOD_IF_NON_NULL(dev_ext->conf.upper_in_event[i]);
        FREE_MOD_IF_NON_NULL(dev_ext->conf.upper_out_event[i]);
    }

    for(i = 0; i < dev_ext->conf.num_action_list; i++) {
        FREE_IF_NON_NULL(dev_ext->conf.action_list[i].action);
        dev_ext->conf.action_list[i].num_action = 0;
    }
    FREE_IF_NON_NULL(dev_ext->conf.action_list);
    dev_ext->conf.num_action_list = 0;

#undef FREE_MOD_IF_NON_NULL
#undef FREE_IF_NON_NULL

    RtlZeroMemory(&dev_ext->conf, sizeof(dev_ext->conf));

    DBG_LEAVE(("void"));
}

VOID load_settings(IN PDEVICE_OBJECT dev_obj)
{
    device_extension_t *dev_ext;
    PWSTR ptr;
    static PWSTR name = L"current setting";

    DBG_ENTER(("%p", dev_obj));

    dev_ext = GET_DEV_EXT(dev_obj);

    ptr = name;

    /* free memory of current setting */
    free_settings(dev_obj);

    /* generate default setting */
    generate_default_setting();

    /* load parameters */
    load_settings_by_name(dev_obj, ptr);

    DBG_LEAVE(("void"));
}
