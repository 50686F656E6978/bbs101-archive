#
#	Makefile	WJ109
#
#	bbs101 Copyright (c) 2009 by Walter de Jong <walter@heiho.net>
#

Q = @
OBJDIR = obj

CC = gcc

###
#
#	Debug version
#	this produces a really slow binary due to Electric Fence
#
CFLAGS = -g -DEBUG -Wall -Wstrict-prototypes `mysql_config --cflags`
LFLAGS = `mysql_config --libs` -lpthread -lcrypt -lefence

###
#
#	Production version
#
##CFLAGS = -Wall -Wstrict-prototypes `mysql_config --cflags`
##LFLAGS = `mysql_config --libs` -lpthread -lcrypt

INCLUDE = include `mysql_config --include`

RM = /bin/rm
RM_F = $(RM) -f
TOUCH = /usr/bin/touch

CFILES = $(wildcard *.c)
_OBJS = $(CFILES:.c=.o)
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

TARGET = main

all: .depend $(TARGET)

include .depend

$(OBJDIR)/%.o: %.c
	@echo '  CC       ' $@
	$(Q)$(CC) -I$(INCLUDE) $(CFLAGS) -c $< -o $@

#
#	Targets
#

main: $(OBJS)
	@echo '  BUILD     main'
	@echo "#define BUILD_TIME \"`date +'%Y%m%d-%H%M.%S %A, %B %d %Y'`\"" > include/build_time.h
	$(Q)$(CC) $(OBJS) -o main $(LFLAGS)

neat:
	@echo '  NEAT'
	$(Q)$(RM_F) ../core core *~

clean:
	@echo '  CLEAN'
	$(Q)$(RM_F) core ../core $(TARGET) $(OBJS) *~

mrproper: clean
	@echo '  MRPROPER'
	$(Q)$(RM_F) .depend config.status config.cache config.log
	$(Q)$(TOUCH) .depend

depend dep .depend:
	@echo '  DEP       .depend'
	$(Q)$(CC) -I$(INCLUDE) -M $(CFILES) > .depend

backup:
	( rm -rf dist/bbs101 ; \
		cd dist ; \
		svn export file:///home/walter/src/svnroot/trunk/bbs101 ; \
		tar czf bbs101.tar.gz bbs101 ; \
		cp bbs101.tar.gz /media/idisk/linux/backup/ ; \
		scp bbs101.tar.gz walter@newton.ka.sara.nl: ; \
	)

# EOB
