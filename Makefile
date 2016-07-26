#
# Licensed under the terms of the MIT license:
#
# Copyright (c) 2000,2002,2006,2012,2016 Andrew E. Mileski <andrewm@isoar.ca>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The copyright notice, and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

#
# Installation directories
#

PREFIX	:= /usr/local
INCDIR	:= $(PREFIX)/include
LIBDIR	:= $(PREFIX)/lib

#
# Executables
#

AR	:= ar
CC	:= gcc
CFLAGS	:= -O2 -W -Wall
INSTALL := install
RANLIB	:= ranlib
RM	:= rm -f
STRIP	:= strip --strip-unneeded

#
# Recipes
#

OBJS	:= math-sll.o
LIBS	:= math-sll.a

.PHONY: all clean install

all: $(LIBS)

clean:
	$(RM) $(LIBS) $(OBJS)

install: $(LIBS) math-sll.h
	$(INSTALL) -m a=rx,u+w math-sll.a $(LIBDIR)
	$(INSTALL) -m a=r,u+w math-sll.h $(INCDIR)

math-sll.o: math-sll.c math-sll.h

math-sll.a: math-sll.o
	$(STRIP) $<
	$(AR) rcs $@ $<
	$(RANLIB) $@

