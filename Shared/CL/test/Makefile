#
# Makefile for cltest program
#

NAME    	=	cl-test
DEST		=	$(REACTOR)

SRCS		=	CLTest.cpp

LIBS		=	-lcl
DBG_LIBS	=	-lcl.dbg

SRC_DIRS		=	../src 
INC_DIRS		=	-I$(REACTOR)/include -I../h
LIB_DIRS		=	-L$(REACTOR)/lib 

BUILD_FLAGS     =
BUILD_DBG_FLAGS = -DCL_CHECKING

   
include $(REACTOR)/makefiles/prog.mf

cl.lib:
	cd $(REACTOR); $(MAKE) cl.lib
cl.dlib:
	cd $(REACTOR); $(MAKE) cl.dlib

test: cl-test
	touch a.a.Z a.b.gz a.e b.e.gz
	./cl-test > out
	diff out out.ref
