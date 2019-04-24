#
# Makefile
#
# $Id: Makefile,v 1.20 2004/06/13 12:59:47 hos Exp $
#

include ../Makefile.comm

TARGET = $(MAIN_TARGET_NAME)cpl.dll

TARGET_INCLUDES = -I../dinput -I../util -I../sys
TARGET_CFLAGS = -DWINVER=0x0500 -D_WIN32_IE=0x0501 -DUNICODE -D_UNICODE
LDLIBS = ../dinput/dinput_util.a ../util/util.a \
	 -lgdi32 -lcomdlg32 -luuid -ldinput

TARGET_DLL_LDFLAGS = -Wl,--kill-at

SRCS = main.c class_factory.c prop_sheet.c \
        setting.c setting_file.c setting_data.c \
       prop_page.c prop_page_main.c prop_page_main_data.c prop_page_in.c \
       prop_subpage.c \
       prop_subpage_event.c prop_subpage_action.c prop_subpage_action_data.c \
       prop_subpage_actionlist.c prop_subpage_other.c prop_subpage_top.c \
       prop_dialog_event.c prop_dialog_actionname.c prop_dialog_action.c
HEADERS = main.h resource.h
RSRC = resource.rc
RSRC_EX = arrow_up.bmp arrow_dn.bmp
DEFS = main.def
OBJS = $(SRCS:%.c=%.o) $(RSRC:%.rc=%.o)
RESS = $(RSRC:%.rc=%.res)

GENFILES = $(TARGET) $(OBJS) $(RESS)


all: $(TARGET)

$(TARGET): $(OBJS) $(DEFS)

$(OBJS): $(HEADERS)

resource.o: ../version.h

pack-bin:
	$(INSTALL) -m 755 -s $(TARGET) $(TARGET_DIR)

pack-src:
	$(INSTALL) -m 755 -d $(TARGET_DIR)
	$(INSTALL) -m 644 $(SRCS) $(TARGET_DIR)
	$(INSTALL) -m 644 $(HEADERS) $(TARGET_DIR)
	$(INSTALL) -m 644 $(RSRC) $(TARGET_DIR)
	$(INSTALL) -m 644 $(RSRC_EX) $(TARGET_DIR)
	$(INSTALL) -m 644 $(DEFS) $(TARGET_DIR)
	$(INSTALL) -m 644 Makefile $(TARGET_DIR)

clean:
	-$(RM) $(GENFILES) *.o
