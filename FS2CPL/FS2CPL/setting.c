/*
 * setting.c  -- device setting
 *
 * $Id: setting.c,v 1.30 2004/06/13 12:59:47 hos Exp $
 *
 */

#include <windows.h>
#include "main.h"


int output_to_device(prop_common_data_t *comm_ctx,
                     const struct controller_output_report *rep)
{
    DWORD cnt;
    OVERLAPPED ovlp;
    HANDLE evt;
    int ret;

    evt = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(evt == NULL) {
        return 0;
    }

    memset(&ovlp, 0, sizeof(ovlp));
    ovlp.hEvent = evt;

    WriteFile(comm_ctx->hid_device,
              rep, sizeof(struct controller_output_report),
              NULL, &ovlp);
    ret = GetOverlappedResult(comm_ctx->hid_device, &ovlp, &cnt, TRUE);

    CloseHandle(evt);

    return ret;
}


static HKEY open_registry_subkey(LPWSTR subkey)
{
    HKEY base_key, key;
    int ret;

    /* open base registry path */
    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REGISTRY_REL_PATH, 0, KEY_READ,
                        &base_key);
    if(ret != ERROR_SUCCESS) {
        return NULL;
    }

    /* open subkey */
    ret = RegOpenKeyExW(base_key, subkey, 0, KEY_READ, &key);
    if(ret != ERROR_SUCCESS) {
        key = NULL;
    }

    RegCloseKey(base_key);

    return key;
}

static int alloc_copy_data(PVOID dst, PULONG dst_size,
                           PVOID src, ULONG src_size)
{
    void **dst_ptr;

    dst_ptr = (void **)dst;

    if(src_size == 0) {
        *dst_ptr = NULL;
        *dst_size = 0;
        return TRUE;
    }

    *dst_ptr = malloc(src_size);
    if(*dst_ptr == NULL) {
        *dst_size = 0;
        return FALSE;
    }

    memcpy(*dst_ptr, src, src_size);
    *dst_size = src_size;

    return TRUE;
}

static int read_registry_data(LPWSTR subkey, LPWSTR name,
                              PVOID val, PULONG size,
                              PVOID def_val, ULONG def_size, DWORD req_type)
{
    HKEY key;
    DWORD type, rsize;
    void *rval = NULL;
    int ret;

    /* open subkey */
    key = open_registry_subkey(subkey);
    if(key == NULL) {
        alloc_copy_data(val, size, def_val, def_size);
        ret = FALSE;
        goto func_end;
    }

    /* get size of data */
    rsize = 0;
    ret = RegQueryValueExW(key, name, NULL, &type, NULL, &rsize);
    if(ret != ERROR_SUCCESS || type != req_type) {
        alloc_copy_data(val, size, def_val, def_size);
        ret = FALSE;
        goto func_end;
    }

    rval = malloc(rsize);
    if(rval == NULL) {
        alloc_copy_data(val, size, def_val, def_size);
        ret = FALSE;
        goto func_end;
    }

    /* get data */
    ret = RegQueryValueExW(key, name, NULL, NULL, rval, &rsize);
    if(ret != ERROR_SUCCESS) {
        alloc_copy_data(val, size, def_val, def_size);
        ret = FALSE;
        goto func_end;
    }

    alloc_copy_data(val, size, rval, rsize);
    ret = TRUE;

  func_end:
    if(key != NULL) RegCloseKey(key);
    if(rval != NULL) free(rval);

    return ret;
}

static int read_registry_dword(LPWSTR subkey, LPWSTR name,
                               PDWORD val, DWORD def_val)
{
    DWORD *data;
    ULONG size;
    int ret;

    ret = read_registry_data(subkey, name,
                             &data, &size, &def_val, sizeof(DWORD), REG_DWORD);
    *val = *data;
    free(data);

    return ret;
}

static int read_registry_binary(LPWSTR subkey, LPWSTR name,
                                PVOID val, PULONG size,
                                PVOID def_val, ULONG def_size)
{
    return read_registry_data(subkey, name,
                              val, size, def_val, def_size, REG_BINARY);
}

static int read_registry_string(LPWSTR subkey, LPWSTR name,
                                LPWSTR *val, PULONG size,
                                LPWSTR def_val, ULONG def_size)
{
    return read_registry_data(subkey, name,
                              val, size, def_val, def_size, REG_SZ);
}


static HKEY create_registry_subkey(LPWSTR subkey)
{
    HKEY base_key, key;
    DWORD dispos;
    int ret;

    /* open base registry path */
    ret = RegCreateKeyExW(HKEY_LOCAL_MACHINE, REGISTRY_REL_PATH, 0, NULL, 0,
                          KEY_WRITE, NULL, &base_key, &dispos);
    if(ret != ERROR_SUCCESS) {
        return NULL;
    }

    /* open subkey */
    ret = RegCreateKeyExW(base_key, subkey, 0, NULL, 0,
                          KEY_WRITE, NULL, &key, &dispos);
    if(ret != ERROR_SUCCESS) {
        key = NULL;
    }

    RegCloseKey(base_key);

    return key;
}

static int write_registry_data(LPWSTR subkey, LPWSTR name,
                               LPVOID val, ULONG size, DWORD type)
{
    HKEY key;
    int ret;

    /* open subkey */
    key = create_registry_subkey(subkey);
    if(key == NULL) {
        return FALSE;
    }

    /* set value */
    ret = RegSetValueExW(key, name, 0, type, (BYTE *)val, size);
    if(ret != ERROR_SUCCESS) {
        ret = FALSE;
    } else {
        ret = TRUE;
    }

    RegCloseKey(key);

    return ret;
}

static int write_registry_dword(LPWSTR subkey, LPWSTR name, DWORD val)
{
    return write_registry_data(subkey, name, &val, sizeof(val), REG_DWORD);
}

static int write_registry_binary(LPWSTR subkey, LPWSTR name,
                                 LPVOID val, ULONG size)
{
    return write_registry_data(subkey, name, val, size, REG_BINARY);
}

static int write_registry_string(LPWSTR subkey, LPWSTR name,
                                 LPWSTR val, ULONG size)
{
    return write_registry_data(subkey, name, val, size, REG_SZ);
}


enum {
    CONF_LOAD,
    CONF_DFLT,
    CONF_SAVE
};

static int apply_device_setting(controller_setting_t *conf, int load_flag)
{
    WCHAR subkey[256];
    DWORD map;
    struct {
        PWSTR name;
        PDWORD ptr;
        DWORD def_val;
    } name_ptr_list[] = {
        {L"button A max", &conf->btn_a_sensitivity, 0x80},
        {L"button B max", &conf->btn_b_sensitivity, 0x80},
        {L"button threshold", &conf->btn_threshold, 0x80},
        {L"lower threshold", &conf->lower_threshold, 0x10},
        {L"upper threshold", &conf->upper_threshold, 0xef},
        {L"initial modifier", &conf->initial_modifier, 0x0000},

        {NULL, NULL, 0}
    };
    int i;

    switch(load_flag) {
      case CONF_LOAD:
          read_registry_dword(L"map", L"current setting", &map, 1);
          break;

      case CONF_DFLT:
          map = 1;
          break;

      case CONF_SAVE:
          if(! read_registry_dword(L"map", L"current setting", &map, 1)) {
              /* save default map */
              write_registry_dword(L"map", L"current setting", map);
          }
          break;
    }

    wsprintfW(subkey, L"parameters\\%x", map);

    /* load/save DWORD setting from registry */
    for(i = 0; name_ptr_list[i].name != NULL; i++) {
        switch(load_flag) {
          case CONF_LOAD:
              /* load */
              read_registry_dword(subkey, name_ptr_list[i].name,
                                  name_ptr_list[i].ptr,
                                  name_ptr_list[i].def_val);
              break;

          case CONF_DFLT:
              *name_ptr_list[i].ptr = name_ptr_list[i].def_val;
              break;

          case CONF_SAVE:
              /* save */
              write_registry_dword(subkey, name_ptr_list[i].name,
                                   *name_ptr_list[i].ptr);
        }
    }

    /* load/save event config */
    {
        WCHAR name[256];
        ULONG size, def_size;
        unsigned short *act_list;
        int i, nact;

        struct {
            PWSTR name;
            struct controller_event_config *event;
            struct controller_modifier_config *def_mod;
        } *lptr, evt_name_list[] = {
            {L"motion ", conf->motion_event, default_motion_mod},
            {L"press ", conf->press_event, default_press_mod},
            {L"release ", conf->release_event, default_release_mod},
            {L"lower in ", conf->lower_in_event, NULL},
            {L"lower out ", conf->lower_out_event, NULL},
            {L"upper in ", conf->upper_in_event, NULL},
            {L"upper out ", conf->upper_out_event, NULL},

            {NULL, NULL, NULL}
        };

        /* load/save modifier/action assoc list of each event */
        {
            struct controller_modifier_config *mod, *def_mod;
            int nmod;

            for(lptr = evt_name_list; lptr->name != NULL; lptr++) {
                for(i = 0; i < CONTROLLER_NUM_EVENT; i++) {
                    /* name of value */
                    wsprintfW(name, L"%s%x", lptr->name, i);

                    switch(load_flag) {
                      case CONF_LOAD:
                      case CONF_DFLT:
                          /* load modifier list */
                          size = 0;
                          def_mod = ((lptr->def_mod != NULL &&
                                      lptr->def_mod[i].action_idx != 0) ?
                                     &lptr->def_mod[i] : NULL);
                          def_size =
                              (def_mod != NULL ?
                               sizeof(struct controller_modifier_config) : 0);

                          if(load_flag == CONF_LOAD) {
                              read_registry_binary(subkey, name,
                                                   &mod, &size,
                                                   def_mod, def_size);
                          } else {
                              alloc_copy_data(&mod, &size, def_mod, def_size);
                          }

                          nmod = size /
                                 sizeof(struct controller_modifier_config);
                          lptr->event[i].num_mod = nmod;
                          lptr->event[i].mod = mod;
                          break;

                      case CONF_SAVE:
                          /* save modifier list */
                          mod = lptr->event[i].mod;
                          size = lptr->event[i].num_mod *
                                 sizeof(struct controller_modifier_config);

                          write_registry_binary(subkey, name,
                                                mod, size);
                          break;
                    }
                }
            }
        }

        /* load/save list of action list */
        {
            wsprintfW(name, L"action list");

            nact = 0;
            switch(load_flag) {
              case CONF_LOAD:
              case CONF_DFLT:
                  /* load */
                  if(load_flag == CONF_LOAD) {
                      read_registry_binary(subkey, name,
                                           &act_list, &size,
                                           default_action_list,
                                           sizeof(default_action_list));
                  } else {
                      alloc_copy_data(&act_list, &size,
                                      default_action_list,
                                      sizeof(default_action_list));
                  }

                  nact = size / sizeof(unsigned short);
                  conf->num_action_list = nact;
                  break;

              case CONF_SAVE:
                  /* save */
                  nact = conf->num_action_list;
                  size = nact * sizeof(unsigned short);
                  act_list = (unsigned short *)malloc(size);
                  if(act_list == NULL) {
                      return FALSE;
                  }
                  for(i = 0; i < nact; i++) {
                      act_list[i] = i;
                  }

                  write_registry_binary(subkey, name,
                                        act_list, size);
                  break;
            }
        }

        /* load/save action list, action name */
        {
            struct controller_action_config *act, *def_act;
            LPWSTR act_name, def_act_name;

            if(load_flag == CONF_LOAD || load_flag == CONF_DFLT) {
                /* setup list of action list */
                conf->action_list =
                    (struct controller_action_config_list *)
                    malloc(sizeof(struct controller_action_config_list) *
                           nact);
                if(conf->action_list == NULL) {
                    free(act_list);
                    return FALSE;
                }
            }

            for(i = 0; i < nact; i++) {
                /* action list */
                wsprintfW(name, L"action list %x", act_list[i]);

                switch(load_flag) {
                  case CONF_LOAD:
                  case CONF_DFLT:
                      /* load */
                      def_act =
                          ((i < NUM_OF_DEF_ACTION &&
                            default_action[default_action_list[i]].type !=
                            CONTROLLER_ACTION_NOP) ?
                           &default_action[default_action_list[i]] :
                           NULL);
                      def_size = (def_act != NULL ?
                                  sizeof(struct controller_action_config) : 0);

                      if(load_flag == CONF_LOAD) {
                          read_registry_binary(subkey, name,
                                               &act, &size,
                                               def_act, def_size);
                      } else {
                          alloc_copy_data(&act, &size, def_act, def_size);
                      }

                      conf->action_list[i].num_action =
                          size / sizeof(struct controller_action_config);
                      conf->action_list[i].action = act;
                      break;

                  case CONF_SAVE:
                      /* save */
                      act = conf->action_list[i].action;
                      size = conf->action_list[i].num_action *
                             sizeof(struct controller_action_config);

                      write_registry_binary(subkey, name,
                                            act, size);
                      break;
                }

                /* action name */
                wsprintfW(name, L"action name %x", act_list[i]);

                switch(load_flag) {
                  case CONF_LOAD:
                  case CONF_DFLT:
                      /* load */
                      def_act_name =
                          (i < NUM_OF_DEF_ACTION ?
                           default_action_name[default_action_list[i]] : NULL);
                      def_size =
                          (def_act_name != NULL ?
                           (lstrlenW(def_act_name) + 1) * sizeof(WCHAR) : 0);

                      if(load_flag == CONF_LOAD) {
                          read_registry_string(subkey, name,
                                               &act_name, &size,
                                               def_act_name, def_size);
                      } else {
                          alloc_copy_data(&act_name, &size,
                                          def_act_name, def_size);
                      }

                      conf->action_list[i].name = act_name;
                      break;

                  case CONF_SAVE:
                      /* save */
                      act_name = conf->action_list[i].name;
                      size = (lstrlenW(act_name) + 1) * sizeof(WCHAR);

                      write_registry_string(subkey, name,
                                            act_name, size);
                      break;
                }
            }
        }

        free(act_list);
    }

    /* load/save profile name */
    {
        ULONG size, def_size;
        LPWSTR name;

        name = L"profile name";

        switch(load_flag) {
          case CONF_LOAD:
              def_size = (lstrlenW(default_profile_name) + 1) * sizeof(WCHAR);
              read_registry_string(subkey, name,
                                   &conf->profile_name, &size,
                                   default_profile_name, def_size);
              break;

          case CONF_DFLT:
              def_size = (lstrlenW(default_profile_name) + 1) * sizeof(WCHAR);
              alloc_copy_data(&conf->profile_name, &size,
                              default_profile_name, def_size);
              break;

          case CONF_SAVE:
              size = (lstrlenW(conf->profile_name) + 1) *
                     sizeof(WCHAR);
              write_registry_string(subkey, name, conf->profile_name, size);
              break;
        }
    }

    return TRUE;
}

int load_device_setting(controller_setting_t *conf)
{
    generate_default_setting();
    free_device_setting(conf);
    return ((apply_device_setting(conf, CONF_LOAD) &&
             verify_device_setting(conf)) ||
            load_default_setting(conf));
}

int load_default_setting(controller_setting_t *conf)
{
    generate_default_setting();
    free_device_setting(conf);
    return apply_device_setting(conf, CONF_DFLT);
}

int save_device_setting(controller_setting_t *conf)
{
    return apply_device_setting(conf, CONF_SAVE);
}

void free_device_setting(controller_setting_t *conf)
{
    int i;

#define FREE_IF_NON_NULL(_p_) if((_p_) != NULL) { free(_p_); (_p_) = NULL; }
#define FREE_MOD_IF_NON_NULL(_p_)               \
  FREE_IF_NON_NULL((_p_).mod);                  \
  (_p_).num_mod = 0;

    for(i = 0; i < CONTROLLER_NUM_EVENT; i++) {
        FREE_MOD_IF_NON_NULL(conf->motion_event[i]);
        FREE_MOD_IF_NON_NULL(conf->press_event[i]);
        FREE_MOD_IF_NON_NULL(conf->release_event[i]);
        FREE_MOD_IF_NON_NULL(conf->lower_in_event[i]);
        FREE_MOD_IF_NON_NULL(conf->lower_out_event[i]);
        FREE_MOD_IF_NON_NULL(conf->upper_in_event[i]);
        FREE_MOD_IF_NON_NULL(conf->upper_out_event[i]);
    }

    for(i = 0; i < conf->num_action_list; i++) {
        FREE_IF_NON_NULL(conf->action_list[i].action);
        FREE_IF_NON_NULL(conf->action_list[i].name);
        conf->action_list[i].num_action = 0;
    }
    FREE_IF_NON_NULL(conf->action_list);
    conf->num_action_list = 0;

    FREE_IF_NON_NULL(conf->profile_name);

#undef FREE_MOD_IF_NON_NULL
#undef FREE_IF_NON_NULL

    memset(conf, 0, sizeof(*conf));
}

int verify_device_setting(controller_setting_t *conf)
{
    int i, j, k;

    /* range of DWORD values */
    {
        struct {
            DWORD *val;
            DWORD min;
            DWORD max;
        } val_range[] = {
            {&conf->btn_a_sensitivity, 0x00, 0xff},
            {&conf->btn_b_sensitivity, 0x00, 0xff},
            {&conf->btn_threshold, 0x00, 0xff},
            {&conf->lower_threshold, 0x00, 0xff},
            {&conf->upper_threshold, 0x00, 0xff},
            {&conf->initial_modifier, 0x00, 0xff},

            {NULL, 0, 0}
        };

        for(i = 0; val_range[i].val != NULL; i++) {
            if(*val_range[i].val < val_range[i].min ||
               *val_range[i].val > val_range[i].max) {
                return FALSE;
            }
        }
    }

    /* events */
    {
        struct controller_event_config *event[] = {
            conf->motion_event,
            conf->press_event,
            conf->release_event,
            conf->lower_in_event,
            conf->lower_out_event,
            conf->upper_in_event,
            conf->upper_out_event,

            NULL
        };

        for(i = 0; event[i] != NULL; i++) {
            for(j = 0; j < CONTROLLER_NUM_EVENT; j++) {
                for(k = 0; k < event[i][j].num_mod; k++) {
                    /* action index of action trigger */
                    if(event[i][j].mod[k].action_idx >=
                       conf->num_action_list) {
                        return FALSE;
                    }
                }
            }
        }
    }

    /* action list */
    for(i = 0; i < conf->num_action_list; i++) {
        /* length of name */
        if(lstrlenW(conf->action_list[i].name) <= 0) {
            return FALSE;
        }

        for(j = 0; j < i; j++) {
            /* name duplication */
            if(lstrcmpW(conf->action_list[i].name,
                        conf->action_list[j].name) == 0) {
                return FALSE;
            }
        }

        for(j = 0; j < conf->action_list[i].num_action; j++) {
            /* action index of apply action */
            if(conf->action_list[i].action[j].type ==
               CONTROLLER_ACTION_APPLY &&
               conf->action_list[i].action[j].apply.action_idx >=
               conf->num_action_list) {
                return FALSE;
            }
        }
    }

    return TRUE;
}
