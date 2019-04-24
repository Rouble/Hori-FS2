/*
 * vidpid.h  -- Vendor ID and Product ID of device
 *
 * $Id: vidpid.h,v 1.1 2004/06/08 09:31:43 hos Exp $
 *
 */

static struct {
    unsigned short vid;
    unsigned short pid;
} device_vid_pid[] = {
    {0x06d3, 0x0f10}
};

#define DEVICE_VID_PID_CNT (sizeof(device_vid_pid) / sizeof(device_vid_pid[0]))
