# Copyright 2024 Xiaomi, Inc. All rights reserved.

import os
import sys


class Reporter:
  def __init__(self, jidl_file):
    self.jidl_file = jidl_file

  def reportLexError(self, t):
    print("Illegal character '%s' in line %d" % (t.value[0], t.lexer.lineno))
    self.showLine(t.lexer.lineno, t.lexer.lexpos)

  def reportYaccError(self, p):
     print("Syntax error at '%s' in line: %d" % (p.value, p.lineno))
     self.showLine(p.lineno, p.lexpos)

  def showLine(self, lineno, lexpos):
     f = open(self.jidl_file, "rt")
     i = 0
     pos = 0
     found_line = None
     for line in f.readlines():
        pos += len(line)
        i = i + 1
        if i >= lineno and pos >= lexpos:
           found_line = line
           break

     pos = pos - lexpos
     pos = len(found_line) - pos

     print("file: %s:%d,%d"%(self.jidl_file, lineno, pos))
     print("%04d %s"%(lineno, found_line[:-1]))

     print("     %s^"%(' '*pos))

     f.close()
