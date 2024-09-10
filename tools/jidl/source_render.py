# Copyright 2023 Xiaomi, Inc. All rights reserved.

import sys
import os
import re
import json
import jsonpath
import argparse
from mako.template import Template

DESCRIPTION='''
  source_render generate the source code
'''

COPYRIGHT='''
Copyright 2023 Xiaomi, Inc. All rights reserved.
'''

cpp_type_map = {
  'int' : 'int',
  'uint' : 'unsigned int',
  'long' : 'long',
  'ulong' : 'unsigned long',
  'float' : 'float',
  'double' : 'double',
  'boolean' : 'int', # bool as int
  'string': 'const_pstr',
  'uint8' : 'uint8_t',
  'int8'  : 'int8_t',
  'uint16' : 'uint16_t',
  'int16' : 'int16_t',
  'uint32' : 'uint32_t',
  'int32' : 'int32_t',
  'uint64' : 'uint64_t',
  'int64' : 'int64_t',
  'void' : 'void',
  'object' : 'xs_value_type',
}

def copyDict(d, s, prefix = None):
  for k,v in s.items():
    if prefix:
      d['%s_%s'%(prefix, str(k))] = v
    else:
      d[k] = v

def readFile(filename):
  f = open(filename)
  content = f.read()
  f.close()
  return content


def writeFile(content, filename):
  f = open(filename, 'wt')
  f.write(content)
  f.close()


def loadJSONFile(json_file):
    with open(json_file, 'r', encoding='UTF-8') as f:
        json_module = json.load(f)
    return json_module


def parseArgs():
    parser = argparse.ArgumentParser(prog='source_render', description=DESCRIPTION, epilog=COPYRIGHT)
    parser.add_argument('-t', '--template', help='give the template to render data', required=True)
    parser.add_argument('-i', '--input', help='the input file of json', required=True)
    parser.add_argument('-o', '--output', help="set the ouput file", required=True)
    parser.add_argument('-c', '--config', help='the input config file', required=False)
    parser.add_argument('-v', '--vars', help='set the input vars [NAME]=[VALUE]', nargs='*')
    return parser.parse_args()


class Utils:
  def __init__(self, doc, args, vars):
    self.doc = doc
    self.args = args
    self.vars = vars

  def findType(self, rtype, rname):
    found = self.select(self.doc, '$.members[?(@.type=="%s" && @.name=="%s")]'%(rtype, rname));
    if found:
      return found[0]
    return None

  def findTypeMeta(self, rtype, rname, meta_name, need_extends):
    tp = self.findType(rtype, rname)
    if tp:
      if 'meta' in tp and meta_name in tp['meta']:
        return tp['meta'][meta_name]
      if need_extends and 'extends' in tp:
        for e in tp['extends']:
          v = self.findTypeMeta(rtype, e, meta_name, need_extends)
          if v: return v
    return None

  def findReferenceNativeType(self, tp):
    nt = self.findTypeMeta(tp['referred_type'], tp['referred_name'], 'native_type', True)
    if nt: return nt
    if tp['referred_type'] == 'callback':
      return self.cppTypeCallback(tp)
    if tp['referred_type'] == 'struct':
      return self.cppTypeStruct(tp)
    if tp['referred_type'] == 'enum':
      return self.cppTypeEnum(tp)
    if tp['referred_type'] == 'interface':
      return self.cppTypeInterface(tp)
    if tp['referred_type'] == 'user_type':
      tp = self.findType('user_type', tp['referred_name'])
      return self.cppType(tp['target'])
    return self.cppTypeDefault(tp)

  def cppTypeEnum(self, tp):
    return self.cppTypeDefault('int')

  def cppTypeInterface(self, tp):
    return self.cppTypeDefault(tp)

  def cppTypeStruct(self, tp):
    return self.cppTypeDefault(tp)

  def cppTypeCallback(self, tp):
    return self.cppTypeDefault(tp['referred_type'])

  def cppTypeDefault(self, tp):
    if isinstance(tp,str):
        if 'cppType' in self.vars:
          cppType = self.vars['cppType']
          if tp in cppType:
            return cppType[tp]
        new_tp = cpp_type_map[tp]
        return new_tp and new_tp or tp
    elif isinstance(tp, dict):
        if tp['type'] == 'reference' and tp['referred_type'] == 'enum':
            return self.cppTypeDefault('int')
    return str(tp)

  def isPromiseType(self, tp):
    return isinstance(tp, dict) and tp['type'] == 'promise'

  def isReferenceType(self, tp, tn):
    return isinstance(tp, dict) and 'type' in tp and tp['type'] == 'reference' and 'referred_type' in tp and tp['referred_type'] == tn

  def isCallbackType(self, tp):
    return self.isReferenceType(tp, 'callback')

  def isStructType(self, tp):
    return self.isReferenceType(tp, 'struct')

  def isInterfaceType(self, tp):
    return self.isReferenceType(tp, 'interface')

  def getPromiseType(self, tp):
    return 'xs_promise_type<%s>' % (tp['resolve_type'])

  def cppTypeArray(self, tp):
    ele_native_type = self.cppType(tp['element'])
    return '%s*' % ele_native_type

  def cppType(self, tp):
    if isinstance(tp, dict):
        if tp['type'] == 'reference':
          return self.findReferenceNativeType(tp)
        elif tp['type'] == 'array':
          return self.cppTypeArray(tp)
        elif self.isPromiseType(tp):
          return self.getPromiseType(tp)
    return self.cppTypeDefault(tp)

  def getMetaValue(self, m, key):
    if 'meta' in m and key in m['meta']:
      return m['meta'][key]
    return None

  def getValueByKey(self, m, key, defval):
    v = self.getMetaValue(m, key)
    if v: return v;
    return key in m and m[key] or defval

  def getDefaultValueType(self):
    return 'xs_value_type'

  def getValueType(self, m):
    return self.getValueByKey(m, 'value_type', self.getDefaultValueType())

  def getConstValue(self, m):
    v = self.getMetaValue(m, 'value')
    if v: return v;
    if 'value' in m:
      if 'value_type' in m and m['value_type'] == 'string':
        return '"%s"' % m['value']
      return m['value']
    return '"%s"' % m['name']

  def getConstValueType(self, m):
    v = self.getValueByKey(m, 'value_type', None)
    if v: return v
    t = type(m['value'])
    if t == int:
      return 'int'
    elif t == float:
      return 'float'
    elif t == bool:
      return 'bool'
    else:
      return 'string'

  def getConstInitFunc(self, m):
    return self.getMetaValue(m, 'init')

  def toNativeDefault(self, tp):
    return 'xs_value_to'

  def transNative(self, tp, meta_name):
    if isinstance(tp, dict):
      if tp['type'] == 'reference':
        trans_native = self.findTypeMeta(tp['referred_type'], tp['referred_name'], meta_name, True)
        if trans_native: return trans_native
        if tp['referred_type'] == 'enum':
          return self.transNative('int', meta_name)
        if tp['referred_type'] == 'user_type':
          tp = self.findType('user_type', tp['referred_name'])
          return self.transNative(tp['target'], meta_name)
      elif tp['type'] == 'user_type':
        return self.transNative(tp['target'], meta_name)
    if isinstance(tp, str):
      if meta_name in self.vars:
        trans = self.vars[meta_name]
        if tp in trans:
          return trans[tp]
    return None

  def toNative(self, tp):
    to_native = self.transNative(tp, 'to_native')
    if to_native: return to_native
    return self.toNativeDefault(tp)

  def fromNativeDefault(self, tp):
    return 'xs_value_from'

  def fromNative(self, tp):
    from_native = self.transNative(tp, 'from_native')
    if from_native: return from_native
    return self.fromNativeDefault(tp)

  def getInterfaceType(self, tp, check_meta):
    intf = None
    if isinstance(tp, dict):
      if tp['type'] == 'interface':
        intf = tp
      elif tp['type'] == 'reference':
        if tp['referred_type'] == 'interface':
          intf = self.findType('interface', tp['referred_name'])
    if intf and check_meta and check_meta != '':
      return  ('meta' in intf \
               and check_meta in intf['meta'] \
               and intf['meta'][check_meta] == 'true') \
            and intf or None
    return intf

  def freeNative(self, tp):
    free_native = self.transNative(tp, 'free_native')
    if free_native: return free_native
    return self.freeNativeDefault(tp)

  def freeNativeDefault(self, tp):
    return None

  def select(self, d, path):
    return jsonpath.jsonpath(d, path)

  def getByType(self, d, tp):
    return self.select(d, '$.members[?(@.type=="%s")]'%tp)

  def getUserType(self, tp, name):
    r = self.select(self.doc, '$.members[?((@.type=="%s")&&(@.name=="%s"))]'%(tp, name))
    if r and len(r) == 1:
      return r[0]
    return None

  def getInterfaces(self, d):
    return self.getByType(d, 'interface')

  def getMethods(self, d):
    return self.getByType(d, 'method')

  def getProperties(self, d):
    return self.getByType(d, 'property')

  def getStructs(self, d):
    return self.getByType(d, 'struct')

  def getCallbacks(self, d):
    return self.getByType(d, 'callback')

  def getConsts(self, d):
    return self.getByType(d, 'const')

  def toIdName(self, s):
    return  re.sub(r"[^0-9A-Za-z_]","_", s)

  def getModuleName(self):
    return self.toIdName(self.doc['name'])

  def getRawModuleName(self, s):
    module_name_without_version = s[0:(s.find("@"))]
    return self.toIdName(module_name_without_version)

  def getFeatureName(self, s):
    feature_name_without_version = s[0:(s.find("@"))]
    feature_name = feature_name_without_version
    last_dot_index = feature_name_without_version.rfind(".")
    if last_dot_index != -1:
      feature_name = feature_name_without_version[(last_dot_index + 1):]
    return feature_name;

  def toTypeName(self, tp):
    if isinstance(tp, str):
      return tp
    if isinstance(tp, dict):
      if tp['type'] == 'reference':
        return '%s<%s>' % (tp['referred_type'], tp['referred_name'])
      elif tp['type'] == 'array':
        return 'array<%s>' % (self.toTypeName(tp['element']))
    return str(tp)

  def buildPropertyValues(self, pname, prop):
    d = {}
    copyDict(d, prop['meta'])
    copyDict(d, prop, 'property')
    d['parent_name'] = pname
    return d

def parseVars(args):
    vars = {}
    if args.config:
        conf = loadJSONFile(args.config)
        for c,v in conf.items():
            vars[c] = v

    if args.vars:
        for v in args.vars:
            m = re.match(r'(.*)=(.*)', v)
            if m:
                name = m.group(1)
                if name in vars and isinstance(vars[name], list):
                  vars[name].append(m.group(2))
                else:
                  vars[name] = m.group(2)
    return vars


def render(args, vars, U):
    source = loadJSONFile(args.input)
    temp = Template(readFile(args.template))
    writeFile(temp.render(doc=source, vars=vars, utils = U(source, args, vars)), args.output)

def main(U):
    args = parseArgs()
    vars = parseVars(args)
    #print(args)
    #print(vars)
    render(args, vars, U)

if __name__ == '__main__':
    main(Utils)
