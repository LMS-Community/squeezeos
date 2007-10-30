#----------------------------------------------------------------------
# L I N K I N G   N O T E S :
#----------------------------------------------------------------------
#
# If  you  have  trouble  linking xltwavplay, then chances are you
# don't  have  your  shared libraries  installed correctly.  If it
# links, but  cannot find a shared library when it runs, this
# indicates that ldconfig needs to be configured and/or re-invoked.
# 
# WON'T LINK XLTWAVPLAY:
# 
# On  SLAKWARE  LINUX,  there  is  a directory /usr/i486-linux/lib
# which  lists  a bunch  of  symbolic links  to libraries that may
# reside in other places (most notably X libraries).  If ld cannot
# find the library for -lXm then make sure you have a symlink:
# 
#   /usr/i486-linux/lib/libXmu.so -> /usr/X11R6/lib/libXmu.so
# 
# (your pathnames may vary)
# 
# WON'T RESOLVE SHARED LIBRARIES AT RUNTIME:
# 
# First review your configuration file: /etc/ld.so.conf and review
# the  man  pages  for  ldconfig.  Adjust  your  configuration  if
# necessary, and then run (as root) ldconfig:
# 
#   # /sbin/ldconfig -v | less
# 
# and try xltwavplay again.
#
#----------------------------------------------------------------------
# C O M P I L A T I O N :
#----------------------------------------------------------------------
#
# To compile it all, just type:
#
#   $ make
#
# If you don't have X server/libraries, compile as follows:
#
#   $ make no_x
#
#----------------------------------------------------------------------
# I N S T A L L A T I O N :
#----------------------------------------------------------------------
# 
# To install everything (wavplay, wavrec and xltwavplay):
# 
# # make install
# 
# To install wavplay and wavrec only (no X based programs):
# 
# # make install_no_x
# 
#----------------------------------------------------------------------
# C O N F I G U R A T I O N   S E C T I O N :
#----------------------------------------------------------------------

#----------------------------------------------------------------------
# If you are running a 386 you should comment this line out.
#
CPU ?= -m486

#----------------------------------------------------------------------
# If you are experiencing troubles (like core dumps!), then uncomment
# the DEBUG= line and comment out the OPT= line, and recompile. When
# turning off debug, you may want to uncomment the OPT= line.
#
#DEBUG=-g
OPT ?= -O2

#----------------------------------------------------------------------
# This parameter indicates where the program binaries should be
# placed (wavplay, wavrec, and xltwavplay)
#
INSTDIR ?= /usr/local/bin

#----------------------------------------------------------------------
# Uncomment this line if you want to install the xltwavplay.res resource
# file into your resource file(s) directory, instead of relying on the
# builtin fallback resources in the program itself. If you don't know
# what this is all about, then leave the next line commented out.
#
#USERES=-DINSTALLED_RESOURCE_FILE

#----------------------------------------------------------------------
# If the resource file is to be installed, indicate where. If the above
# parameter is commented out, then this parameter is ignored.
#
RESDIR ?= /usr/X11R6/lib/X11/app-defaults

#----------------------------------------------------------------------
# If you don't have libXmu.so (which you should have) or you simply
# don't want EDITRES capability in xltwavplay, then uncomment this
# to disable the EDITRES facility. If you're not compiling xltwavplay
# then this setting can be left asis.
#
#NOEDITRES=-DNO_EDITRES

#----------------------------------------------------------------------
# If you COMMENTED OUT the NOEDITRES line, then UNCOMMENT this line.
# If you UNCOMMENTED the NOEDITRES line, then COMMENT out this line.
# (ie. do the opposite of NOEDITRES)
#
LIBXMU=-lXmu

#----------------------------------------------------------------------
# These are the non X load options (for wavplay/wavrec)
#
LDOPTS=

#----------------------------------------------------------------------
# These are the MOTIF/LessTif + X load options (for xltwavplay)
# Make sure that -lXm is always placed before -lXt
#
XLDOPTS=-lXm $(LIBXMU) -lXt -lX11 

#----------------------------------------------------------------------
# When UNcommented, this causes the wavplay/wavrec programs to be compiled to use
# only the standard getopt() routine. When commented out, the long options like --help
# are recognized, with the help of the getopt_long() routine.
#
#STDGETOPTS= -DUSE_GETOPT_STD

#----------------------------------------------------------------------
# The next option specifies the pathname that the xltwavplay X client should exec
# when starting the wavplay server program. This should be a full pathname.
#
PLAYPATH= -DWAVPLAYPATH=\"$(INSTDIR)/wavplay\"

#----------------------------------------------------------------------
# Minimum DSP sampling rate (default is 4000)
#
#DSPMIN= -DDSP_MIN=4000

#----------------------------------------------------------------------
# Maximum DSP sampling rate (default is 48000)
#
#DSPMAX= -DDSP_MAX=48000

#----------------------------------------------------------------------
# Pathname of the audio device (default is /dev/dsp)
#
#DSPPATH= -DAUDIODEV=\"/dev/dsp\"

#----------------------------------------------------------------------
# Semaphore IPC Key for the Play & Record locks (Default is 0x33333333)
#
#DSPLOCKS= -DAUDIOLCK=0x33333333

#----------------------------------------------------------------------
# Only for unusual sites would you change this: the name of the
# install program. (has no impact if you choose to manually install)
#
INSTALL=install

#----------------------------------------------------------------------
# If I did my job well, you won't need to change anything here for
# LINUX.
#----------------------------------------------------------------------
.SUFFIXES: .c .cpp .h .o
CUSTMOPTS=$(STDGETOPTS) $(PLAYPATH) $(DSPMIN) $(DSPMAX) $(DSPPATH) $(DSPLOCKS) $(USERES) $(NOEDITRES)
ELF=	-fPIC
INCL=	-I.
STRIP=echo # strip	# use echo when debugging
CPOPTS=	-c $(DEBUG)$(OPT) $(CPU) $(ELF) -fhandle-exceptions $(CUSTMOPTS) -Wall -Wstrict-prototypes
CCOPTS= -c $(DEBUG)$(OPT) $(CPU) $(ELF) -Wall -Wstrict-prototypes $(CUSTMOPTS)
#CC=	gcc

SVROBJ=	main.o file.o recplay.o wavfile.o locks.o msg.o server.o
CLNOBJ=	client.o msg.o xmsprint.o xltwavplay.o menu.o procterm.o save_as.o

.cpp.o:
	$(CP) $(CPOPTS) $(INCL) $<

.c.o:
	$(CC) $(CCOPTS) $(INCL) $<

all:	wavplay wavrec xltwavplay

no_x:	wavplay wavrec

wavplay: $(SVROBJ)
	$(CC) $(SVROBJ) $(LDOPTS) $(DEBUG) -o wavplay
	ls -l wavplay

wavrec:	wavplay
	rm -f wavrec
	ln wavplay wavrec

xltwavplay: $(CLNOBJ) xltwavplay.o
	$(CC) $(CLNOBJ) $(XLDOPTS) $(DEBUG) -o xltwavplay
	$(STRIP) xltwavplay
	ls -l xltwavplay

install: install_wavplay install_xltwavplay

install_no_x: install_wavplay

install_wavplay: wavplay wavrec
	$(INSTALL) -m 555 wavplay $(INSTDIR)
	rm -f $(INSTDIR)/wavrec
	ln $(INSTDIR)/wavplay $(INSTDIR)/wavrec

install_xltwavplay: xltwavplay
	$(INSTALL) -o root -g root -m 555 xltwavplay $(INSTDIR)
	if [ "$(USERES)" != "" ] ; then $(INSTALL) -o root -g root -m 555 xltwavplay.res $(RESDIR)/xltwavplay; fi

clean:
	rm -f *.o a.out core t.t errs.t

real_clean: clean
	rm -f xltwavplay wavplay wavrec

archive: real_clean
	rm -f ../xltwavplay.tar ../xltwavplay.tar.gz
	cd .. && tar cvf xltwavplay.tar wavplay.RCS && gzip xltwavplay.tar

dist:	real_clean archive
	cd .. && rm -fr wavplay-1.0 wavplay-1.0.tar wavplay-1.0.tar.gz && mkdir wavplay-1.0
	find . -print | cpio -pmdv ../wavplay-1.0
	cd ../wavplay-1.0 && rm -fr RCS && chmod 444 * && chmod u+w Makefile
	cd .. && tar cvf wavplay-1.0.tar wavplay-1.0 && gzip wavplay-1.0.tar && rm -fr wavplay-1.0
	ls -l ../wavplay-1.0.tar.gz
