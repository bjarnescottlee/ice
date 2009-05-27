#!/usr/bin/env python
# **********************************************************************
#
# Copyright (c) 2003-2009 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************

import os, sys, re

path = [ ".", "..", "../..", "../../..", "../../../.." ]
head = os.path.dirname(sys.argv[0])
if len(head) > 0:
    path = [os.path.join(head, p) for p in path]
path = [os.path.abspath(p) for p in path if os.path.exists(os.path.join(p, "scripts", "TestUtil.py")) ]
if len(path) == 0:
    raise "can't find toplevel os.getcwd()!"
sys.path.append(os.path.join(path[0]))
from scripts import *

print "testing list-generated...",

slice2java = os.path.join(TestUtil.getCppBinDir(), "slice2java")
if not os.path.exists("classes"):
    os.mkdir("classes")

command = slice2java + " --list-generated --output-dir classes File1.ice File2.ice"
stdin, stdout, stderr = os.popen3(command)
lines1 = stdout.readlines()
lines2 = open(os.path.join(os.getcwd(), "list-generated.out"), "r").readlines()
if len(lines1) != len(lines2):
    print "failed!"
    sys.exit(1)
    
i = 0
while i < len(lines1):
    line1 = lines1[i].strip()
    line2 = lines2[i].strip()
    if line1 != line2:
        print "failed!"
        sys.exit(1)
    i = i + 1
else:
    print "ok"

TestUtil.cleanup()
sys.exit(0)