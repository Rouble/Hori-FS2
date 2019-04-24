#
# Makefile
#
# $Id: Makefile,v 1.8 2004/06/07 10:45:20 hos Exp $
#

include ../Makefile.comm

TARGET = util.a

SRCS = assq_pair.c rassq_pair.c ucs.c \
       read_s_exp.c write_s_exp.c s_exp_assq.c s_exp_massq.c s_exp_length.c
HEADERS = util.h s_exp.h
OBJS = $(SRCS:%.c=%.o)
GENFILES = $(TARGET) $(OBJS)

all: $(TARGET)

$(TARGET): $(OBJS)

$(OBJS): $(HEADERS)

clean:
	-$(RM) $(GENFILES) *.o

pack-bin:

pack-src:
	$(INSTALL) -m 755 -d $(TARGET_DIR)
	$(INSTALL) -m 644 $(SRCS) $(TARGET_DIR)
	$(INSTALL) -m 644 $(HEADERS) $(TARGET_DIR)
	$(INSTALL) -m 644 Makefile $(TARGET_DIR)
