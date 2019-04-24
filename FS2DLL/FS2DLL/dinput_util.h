/*
 * dinput_util.h  -- define utilities to use DirectInput
 *
 * $Id: dinput_util.h,v 1.2 2004/03/19 11:42:28 hos Exp $
 *
 */

/* open HID device of joystick from joystick ID */
#include "intsafe.h"
#include "WINDEF.H"
HANDLE open_joystick_hid_device(USHORT joystick_id, DWORD file_flag);
