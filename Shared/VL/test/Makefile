#
# Makefile for a VL test program
#

default:	vltest
NAME    	=	vltest

SRCS		=	VLTest.cpp
LIBS		=	-lcl -lvl -lm
DBG_LIBS    =   -lcl.dbg -lvl.dbg -lm

SRC_DIRS		= 	../src
INC_DIRS		=	-I$(REACTOR)/include -I$(REACTOR)/include/vl -I$(REACTOR)/include/cl
LIB_DIRS		=	-L$(REACTOR)/lib

BUILD_FLAGS	= 	-DVL_CHECKING -DVL_DEBUG 

test:	vltest
	-./vltest | \
	  sed -e 's/[0-9].[0-9]*e-1[6-9]/0/g; s/-0\([^\.0-9]\)/0\1/g' > out
	@-echo \
	  "--- Differences from reference run ------------------------------"
	@-diff out out.ref
# the sed command above clips numbers like 6.12345e-16 to zero, and converts
# -0 to 0. 

vl.lib:
	cd $(REACTOR) && $(MAKE) vl.lib
include $(REACTOR)/makefiles/prog.mf
