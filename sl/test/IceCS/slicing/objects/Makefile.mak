# **********************************************************************
#
# Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************

top_srcdir	= ..\..\..\..

TARGETS		= client.exe

C_SRCS		= AllTests.cs Client.cs

GEN_SRCS	= $(GDIR)\Test.cs \
		  $(GDIR)\Forward.cs
CGEN_SRCS	= $(GDIR)\ClientPrivate.cs

SDIR		= .

GDIR		= generated

!include $(top_srcdir)\config\Make.rules.mak.cs

MCSFLAGS	= $(MCSFLAGS) -target:exe

SLICE2SLFLAGS	= $(SLICE2SLFLAGS) -I.

client.exe: $(C_SRCS) $(GEN_SRCS) $(CGEN_SRCS)
	$(MCS) $(MCSFLAGS) -out:$@ -r:$(bindir)\icesl.dll $(C_SRCS) $(GEN_SRCS) $(CGEN_SRCS)

!include .depend