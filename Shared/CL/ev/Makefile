#
# Makefile for expression evaluator
#

NAME    	=	ev

SRCS		=	EV.cpp

LIBS		=	-lcl
DBG_LIBS	=	-lcl.dbg

SRC_DIRS		=	../src 
INC_DIRS		=	-I$(REACTOR)/include -I$(REACTOR)/include/cl
LIB_DIRS		=	-L$(REACTOR)/lib 
   
include $(REACTOR)/makefiles/prog.mf

cl.lib:
	cd $(REACTOR); $(MAKE) cl.lib
cl.dlib:
	cd $(REACTOR); $(MAKE) cl.dlib
