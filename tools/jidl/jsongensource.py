# Copyright 2023 Xiaomi, Inc. All rights reserved.
# jidl AST Json file to cpp generator

import sys
import os
import re
import json
import jidl_error
from mako.template import Template
import shutil
from cpp_keywords import CPP_KEYWORDS

g_debug = False

script_dir = os.path.dirname(os.path.realpath(__file__))


def Debug(message):
  if g_debug: print(message)

def ReadFile(filename):
  f = open(filename)
  content = f.read()
  f.close()
  return content

def GetFile(filename):
  return os.path.join(script_dir, filename)

def GetTemplate(filename):
  return Template(ReadFile(GetFile(filename)))

def WriteFile(content, filename):
  #print('write to %s' % filename)
  #print(content)
  Debug("================ show out file:%s==================" % filename)
  Debug(content)
  Debug("================ end %s =======================" % filename)
  f = open(filename, 'wt')
  f.write(content)
  f.close()

def GetFileName(filepath):
  return os.path.basename(filepath)

def runProtobuf(pb_file, out_dir):
  ## copy to tmp file
  ## if  pb_file is  samples/test.proto, outdir is /my/out
  ##     pb file while set to /my/out/samples/test.pb.h
  ## but we want to set /my/out/test.pb.h
  ## so, out put a tmp_out first, and the cp to /my/out
  out_dir = os.path.abspath(out_dir)
  pb_path = os.path.dirname(pb_file)
  cmd_list = ['protoc-c', pb_file, '--c_out', out_dir, '-I', pb_path]
  cmds = ' '.join(cmd_list)
  print('protobuf: %s' % cmds)
  os.system(cmds)
  real_out = os.path.abspath(os.path.join(out_dir, os.path.dirname(pb_file)))
  if real_out == out_dir:
    return
  try:
    file = os.path.basename(pb_file)
    file = os.path.splitext(file)[0]
    shutil.copy(os.path.join(real_out, '%s.pb-c.h'%file), out_dir)
    shutil.copy(os.path.join(real_out, '%s.pb-c.c'%file), out_dir)
  except:
    print("copy protobuf file filed: from %s => %s" % (real_out, out_dir))

def CamelToLowerStr(s):
    new_s = ''
    was_upper = True
    for ch in s:
       is_upper = ch.isupper()
       if is_upper:
         if not was_upper:
           new_s = new_s + '_'
         new_s = new_s + ch.lower()
       else:
         new_s = new_s + ch
       was_upper = is_upper
    return new_s

def CamelToLower(names):
    # see protobuf https://github.com/protobuf-c/protobuf-c/blob/master/protoc-c/c_helpers.cc:201
    # in function FullNameToLower
    #
    i = 0
    while i < len(names):
        name = names[i]
        names[i] = CamelToLowerStr(name)
        i = i + 1


class Render:
  def __init__(self, json_file, configs):
    self.json_file = json_file
    self.configs = configs
    self.outdir = script_dir
    if 'out-dir' in configs:
      self.outdir = configs['out-dir']
    self.module = self.LoadJSON()
    self.interface_extends_map = {}

  def LoadJSON(self):
    with open(self.json_file, 'r', encoding='UTF-8') as f:
      # load JSON data as Python direction
      json_module = json.load(f)
    # print("module json ast 对象：",json_module)
    return json_module

  def GetModuleName(self):
    name = self.GetRawModuleName()
    return name.replace('.', '_')

  def ToClassName(self, name):
    name = name.split('@')[0]
    name = name.replace('.', '_')
    return name[0].upper() + name[1:]

  def GetRawModuleName(self):
    return self.module['name'].split("@")[0]

  def MakeOutPath(self, filename):
    if self.outdir:
      return os.path.join(self.outdir, filename)
    return os.path.join(script_dir, filename)

  def _MapType(self, ast_type, type_map):
    if not isinstance(ast_type, str):
      raise Exception('invalid str type: {}'.format(ast_type))
    if ast_type not in type_map:
      raise Exception('can not map type: {}'.format(ast_type))
    return type_map[ast_type]

  def IsStruct(self, ast_node):
    if not isinstance(ast_node, dict):
      return False
    if ast_node['type'] == 'struct':
      return True
    return False

  def PropertyHasGetter(self, ast_type):
    if not (isinstance(ast_type, dict) \
        and 'type' in ast_type and ast_type['type'] == 'property'):
      raise Exception('invalid property type: {}'.format(ast_type))
    if 'readable' in ast_type or 'const' in ast_type:
      return True
    return False

  def PropertyHasSetter(self, ast_type):
    if not (isinstance(ast_type, dict) \
        and 'type' in ast_type and ast_type['type'] == 'property'):
      raise Exception('invalid property type: {}'.format(ast_type))
    if ('writeable' in ast_type) and ('const' not in ast_type):
      return True
    return False

  def CacheInterfaceExtend(self, name, extend):
    if name in self.interface_extends_map:
      extend_list = self.interface_extends_map[name]
    else:
      extend_list = []
      self.interface_extends_map[name] = extend_list
    extend_list.append(extend)

  def GetInterfaceExtends(self, name):
    if not name in self.interface_extends_map:
      return []
    return self.interface_extends_map[name]

  def GetInterfaceCtorInfo(self, ast_node):
    ctor_info = {}
    if isinstance(ast_node, dict) \
        and ast_node['type'] == 'function' \
        and 'meta' in ast_node \
        and 'ctor' in ast_node['meta'] \
        and 'target' in ast_node['meta'] \
        and ast_node['meta']['ctor'] == 'true' \
        and isinstance(ast_node['return_type'], dict) \
        and 'referred_type' in ast_node['return_type'] \
        and ast_node['return_type']['referred_type'] == 'interface':
      ctor_info['target'] = ast_node['meta']['target']
      ctor_info['interface'] = ast_node['return_type']['referred_name']
    return ctor_info

### CPP Render
class CPPRender(Render):
  cpp_type_map = {
    'int' : 'FtInt',
    'uint' : 'unsigned int',
    'long' : 'FtInt64',
    'ulong' : 'FtUint64',
    'float' : 'FtFloat',
    'double' : 'FtDouble',
    'boolean' : 'FtBool',
    'string': 'FtString',
    'uint8' : 'FtUint8',
    'int8'  : 'FtInt8',
    'uint16' : 'FtUint16',
    'int16' : 'FtInt16',
    'uint32' : 'FtUint32',
    'int32' : 'FtInt32',
    'uint64' : 'FtUint64',
    'int64' : 'FtInt64',
    'void' : 'void',
    'ellipse' : 'FtVariParams',
    'callback' : 'FtCallbackId',
    'object' : 'FtAny',
    'jsonobject' : 'FtJsonObject',
    'array'  : 'FtArray*',
    'Int8Array' : 'FtArray*',
    'Uint8Array' : 'FtArray*',
    'Int16Array' : 'FtArray*',
    'Uint16Array' : 'FtArray*',
    'Int32Array' : 'FtArray*',
    'Uint32Array' : 'FtArray*',
    'Int64Array' : 'FtArray*',
    'Uint64Array' : 'FtArray*',
    'IntArray' : 'FtArray*',
    'UintArray' : 'FtArray*',
    'LongArray' : 'FtArray*',
    'UlongArray' : 'FtArray*',
    'FloatArray' : 'FtArray*',
    'DoubleArray' : 'FtArray*',
  }

  ref_types = [
    'string',
    'object',
    'array',
    'Int8Array',
    'Uint8Array',
    'Int16Array',
    'Uint16Array',
    'Int32Array',
    'Uint32Array',
    'Int64Array',
    'Uint64Array',
    'IntArray',
    'UintArray',
    'LongArray',
    'UlongArray',
    'FloatArray',
    'DoubleArray',
  ]

  array_cpp_type_map = {
    'Int8Array' : 'FtInt8',
    'Uint8Array' : 'FtUint8',
    'Int16Array' : 'FtInt16',
    'Uint16Array' : 'FtUint16',
    'Int32Array' : 'FtInt32',
    'Uint32Array' : 'FtUint32',
    'Int64Array' : 'FtInt64',
    'Uint64Array' : 'FtUint64',
    'IntArray' : 'FtInt',
    'UintArray' : 'unsigned int',
    'LongArray' : 'long',
    'UlongArray' : 'unsigned long',
    'FloatArray' : 'FtFloat',
    'DoubleArray' : 'FtDouble',
  }

  base_feature_type_map = {
    'int' : 'FT_INT',
    'uint' : 'FT_UINT32',
    'long' : 'FT_INT64',
    'ulong' : 'FT_UINT64',
    'float' : 'FT_FLOAT',
    'double' : 'FT_DOUBLE',
    'boolean' : 'FT_BOOLEAN',
    'string': 'FT_STRING',
    'uint8' : 'FT_UINT8',
    'int8'  : 'FT_INT8',
    'uint16' : 'FT_UINT16',
    'int16' : 'FT_INT16',
    'uint32' : 'FT_UINT32',
    'int32' : 'FT_INT', # to be fixed
    'uint64' : 'FT_UINT64',
    'int64' : 'FT_INT64',
    'void' : 'FT_VOID',
    'ellipse' : 'FT_PARAM_REST_END',
    'object' : 'FT_ANY_REF',
    'jsonobject' : 'FT_JSON_OBJ',
    'array' : 'FT_ARRAY',
    'Int8Array' : 'FT_ARRAY',
    'Uint8Array' : 'FT_ARRAY',
    'Int16Array' : 'FT_ARRAY',
    'Uint16Array' : 'FT_ARRAY',
    'Int32Array' : 'FT_ARRAY',
    'Uint32Array' : 'FT_ARRAY',
    'Int64Array' : 'FT_ARRAY',
    'Uint64Array' : 'FT_ARRAY',
    'IntArray' : 'FT_ARRAY',
    'UintArray' : 'FT_ARRAY',
    'LongArray' : 'FT_ARRAY',
    'UlongArray' : 'FT_ARRAY',
    'FloatArray' : 'FT_ARRAY',
    'DoubleArray' : 'FT_ARRAY',
  }

  array_feature_type_map = {
    'Int8Array' : 'FT_INT8',
    'Uint8Array' : 'FT_UINT8',
    'Int16Array' : 'FT_INT16',
    'Uint16Array' : 'FT_UINT16',
    'Int32Array' : 'FT_INT',
    'Uint32Array' : 'FT_UINT16',
    'Int64Array' : 'FT_INT64',
    'Uint64Array' : 'FT_UINT64',
    'IntArray' : 'FT_INT',
    'UintArray' : 'FT_UINT',
    'LongArray' : 'FT_LONG',
    'UlongArray' : 'FT_ULONG',
    'FloatArray' : 'FT_FLOAT',
    'DoubleArray' : 'FT_DOUBLE',
  }

  opt_val_name_map = {
    'FT_INT' : 'ival',
    'FT_BOOLEAN' : 'ival',
    'FT_INT64' : 'lval',
    'FT_FLOAT' : 'fval',
    'FT_DOUBLE' : 'fval',
    'FT_STRING' : 'str',
    'FT_UINT32' : 'uval',
    'FT_UINT64' : 'ulval',
  }

  append_data_name_map = {
    'FT_INT' : 'i32',
    'FT_UINT' : 'u32',
    'FT_INT32' : 'i32',
    'FT_UINT32' : 'u32',
    'FT_INT64' : 'i64',
    'FT_UINT64' : 'u64',
    'FT_FLOAT' : 'f32',
    'FT_DOUBLE' : 'f64',
    'FT_STRING' : 'str',
    'FT_BOOLEAN' : 'i32',
  }

  low_perms_map = {
    'hapjs.permission.INTERNET' : 0, # HAPJS_PERMISSION_INTERNET
    'hapjs.permission.LOCATION' : 1, # HAPJS_PERMISSION_LOCATION
    'hapjs.permission.RECORD' : 2, # HAPJS_PERMISSION_RECORD
    'hapjs.permission.DEVICE_INFO' : 3, # HAPJS_PERMISSION_DEVICE_INFO
    'hapjs.permission.READ_HEALTH_DATA' : 4, # HAPJS_PERMISSION_READ_HEALTH_DATA
  }

  high_perms_map = {
    'hapjs.permission.MAX' : 64, # HAPJS_PERMISSION_MAX
  }

  def __init__(self, json_file, header_file, source_file, configs):
    lang = configs['lang']
    self.isCPP = lang == 'c++'
    self.header_tmpl = GetTemplate(self.isCPP and 'json_ast_cppheader.mt'  or 'json_ast_header.mt')
    self.source_tmpl = GetTemplate(self.isCPP and 'json_ast_cppsource.mt'  or 'json_ast_source.mt')
    self.header_file = header_file
    self.source_file = source_file
    self.func_ret_node_map = {}
    self.callback_id_set = set()
    self.event_id_map = {}
    self.interface_events_map = {}
    self.event_idx = 0
    self.promise_type_set = set()
    self.struct_name_set = set()
    self.interface_name_set = set()
    self.vtable_map = {}
    self.interface_members_map = {}
    self.feature_type_set = set()
    self.array_malloc_func_set = set()
    Render.__init__(self, json_file, configs)
    # cache types
    self._cacheTypes()

    # cache the all user defiend type
    self.user_types_map = {}
    self._collectUserTypes(self.module)

  def _collectUserTypes(self, intf):
    for m in intf['members']:
      if m['type'] == 'function' or m['type'] == 'callback':
         self._collectUserTypesFromFunc(m)
      elif m['type'] == 'property':
         self._tryCollectUserType(m['value_type'])
      elif m['type'] == 'struct':
         self._colloectUserTypesFromStruct(m)
      elif m['type'] == 'interface':
         self._collectUserTypes(m)
      self._tryCollectUserType(m)

  def _tryCollectUserType(self, m):
    if not isinstance(m, dict):
       return

    if m['type'] in ['interface', 'struct', 'callback']:
      self.user_types_map['%s_%s_type' % (m['name'], m['type'])] = m
    elif m['type'] == 'array':
      name = self._getArrayElementTypeName(m['element'])
      self.user_types_map['%s_array' % name] = m
    elif m['type'] == 'promise':
      self._tryCollectUserType(m['resolve_type'])

  def _colloectUserTypesFromStruct(self, s):
    for m in s['members']:
       self._tryCollectUserType(m['type'])


  def _getArrayElementTypeName(self, tp):
     if isinstance(tp, str):
         return tp
     if isinstance(tp, dict):
         if tp['type'] == 'reference':
             ref_type = tp['referred_type']
             ref_name = tp['referred_name']
             if ref_type == 'id':
               if ref_name in self.struct_name_set:
                  ref_type = 'struct'
               elif ref_name in self.interface_name_set:
                  ref_type = 'interface'

             if ref_type == 'interface' or ref_type == 'struct':
               return '%s_%s_type' % (ref_name, ref_type)

         elif tp['type'] == 'array':
             return '%s_array' % self._getArrayElementTypeName(tp['element'])
     return str(tp)

  def _collectUserTypesFromFunc(self, f):
    if 'return_type' in f:
      self._tryCollectUserType(f['return_type'])
    if 'params' in f:
      for p in f['params']:
        self._tryCollectUserType(p['type'])

  def _cacheTypes(self):
    for child in self.module["members"]:
      if not "type" in child: continue
      t = child["type"]
      if t == "interface":
        name = child["name"]
        self.interface_name_set.add(name)
      elif t == "struct":
        name = child["name"]
        self.struct_name_set.add(name)

  def isCPPKeyword(self, word):
    return word in CPP_KEYWORDS

  def toMemberName(self, name):
    if self.isCPPKeyword(name):
      return '_'+name
    else:
      return name

  def Generate(self):
    self._GenerateFromTemplate(self.source_tmpl, self.GetCppFilePath())
    self._GenerateFromTemplate(self.header_tmpl, self.GetHeaderFilePath())
    self._GenerateProtobufs()

  def _GenerateFromTemplate(self, tmpl, out):
    WriteFile(tmpl.render(render=self), out)

  def _GenerateProtobufs(self):
    if not 'imports' in self.module:
      return None
    import_list = []
    for imp in self.module['imports']:
        if 'type' in imp and imp['type'] == 'import_message':
            self._GeneratorProtobuf(imp['protobuf'])

  def _GeneratorProtobuf(self, pb_path):
    dir_name = os.path.dirname(self.json_file)
    pb_file = os.path.join(dir_name, pb_path)
    runProtobuf(pb_file, self.outdir)


  def GetImportsHeadList(self):
    if not 'imports' in self.module:
      return None
    import_list = []
    for imp in self.module['imports']:
       import_header = self._TryGetImportHeader(imp)
       if import_header:
         import_list.append(import_header)
    return import_list

  def _TryGetImportHeader(self, imp):
    if not 'protobuf' in imp:
      return None
    pb_path = imp['protobuf']
    pb_name = os.path.basename(pb_path)
    pb_name = os.path.splitext(pb_name)[0]
    return '%s.pb-c.h' % pb_name

  def GetPbTypeName(self, name):
    names = name.split('.')
    return ('__'.join([n[0].upper() + n[1:] for n in names]))

  def GetPbVarName(self, name):
    names = name.split('.')
    CamelToLower(names)
    return '__'.join(names)

  def GenHeaderDefine(self):
    return 'JSON_AST_GEN_MODULE_%s_H_' % (self.GetModuleName().upper())

  def GetHeaderFileName(self):
    if self.header_file:
      return GetFileName(self.header_file)
    return '%s.h' % (self.GetModuleName())

  def GetHeaderFilePath(self):
    if self.header_file:
      return self.MakeOutPath(self.header_file)
    file_name = '%s.h' % (self.GetModuleName())
    return self.MakeOutPath(file_name)

  def GetCppFilePath(self):
    if self.source_file:
      return self.MakeOutPath(self.source_file)
    file_name = '%s.cpp' % (self.GetModuleName())
    return self.MakeOutPath(file_name)

  def IsRefType(self, ast_type):
    if isinstance(ast_type, str):
        return ast_type in self.ref_types

    if isinstance(ast_type, dict):
       if 'element' in ast_type:
           return True
       elif 'referred_type' in ast_type:
           referred_type = ast_type['referred_type']
           return referred_type in ['struct', 'interface']
       return ast_type['type'] in ['struct', 'interface', 'message']
    return True

  def GenerateCppType(self, ast_type):
    if isinstance(ast_type, str):
      if self.isCPP and ast_type == "string":
        return "ft_utils::FtStringPtr"
      else:
        return self._MapType(ast_type, self.cpp_type_map)

    if not isinstance(ast_type, dict):
      raise Exception('invalid complex type: {}'.format(ast_type))

    module_name = self.GetModuleName()
    if 'element' in ast_type:
      if self.isCPP:
        return "ft_utils::RefPtr<FtArray>"
      else:
        return 'FtArray*'
    elif 'referred_type' in ast_type:
      referred_type = ast_type['referred_type']
      if referred_type == 'callback':
        return self.GenerateCppType(referred_type)
      elif referred_type == 'struct':
        referred_name = ast_type['referred_name']
        if self.isCPP:
          return "ft_utils::RefPtr<" + self.ToClassName(referred_name) + ">"
        else:
          return f"{module_name}_{referred_name} *"
      elif referred_type == 'interface':
        if self.isCPP:
          referred_name = ast_type['referred_name']
          return "I" + referred_name + "*"
        else:
          return "FeatureInterfaceHandle"
    elif ast_type['type'] == 'struct':
        struct_name = ast_type['name']
        if self.isCPP:
          return "ft_utils::RefPtr<" + self.ToClassName(struct_name) + ">"
        else:
          return f"{module_name}_{struct_name} *"
    elif ast_type['type'] == 'message':
        if self.isCPP:
          return self.ToClassName(ast_type['message_name']) + "_p"
        else:
          return f"{module_name}_{ast_type['message_name']}_p"
    else:
      raise Exception('invalid complex type: {}'.format(ast_type))

  def GenerateArrayCppType(self, ast_type):
    if isinstance(ast_type, str):
      return self._MapType(ast_type, self.array_cpp_type_map)
    if not isinstance(ast_type, dict) or ('element' not in ast_type):
      raise Exception('invalid array type: {}'.format(ast_type))
    return self._MapType(ast_type['element'], self.cpp_type_map)

  def GetOptValName(self, feature_type):
    if feature_type not in self.opt_val_name_map:
      # print('got a complex opt value, type: {}'.format(feature_type))
      return 'ptr'
    return self._MapType(feature_type, self.opt_val_name_map)

  def GetOptVal(self, feature_type, default_value):
    if feature_type not in self.opt_val_name_map:
      return default_value
    if feature_type == 'FT_UINT64':
      default_value = '(uint64_t)' + default_value
    elif feature_type == 'FT_UINT32':
      default_value = '(uint32_t)' + default_value
    return default_value

  def GetAppendDataName(self, feature_type):
    append_data_type = self._MapType(feature_type, self.append_data_name_map)
    if append_data_type:
      return append_data_type
    return 'ptr'

  def _TryCacheFeatureType(self, feature_type):
    if not feature_type in self.feature_type_set:
      self.feature_type_set.add(feature_type)
      return True
    return False

  def _TryCacheArrayMallocFunc(self, array_malloc_func):
    if not array_malloc_func in self.array_malloc_func_set:
      self.array_malloc_func_set.add(array_malloc_func)
      return True
    return False

  def GetArrayMallocFuncDefines(self):
    return self.array_malloc_func_set

  def ToBaseFeatureType(self, ast_type):
    feature_type = self._MapType(ast_type, self.base_feature_type_map)
    if feature_type == 'FT_ARRAY':
      feature_type = self._MapType(ast_type, self.array_feature_type_map)
    return feature_type

  def SetArrayTypeGenerator(self, ArrayTypeGenerator):
    self.ArrayTypeGenerator = ArrayTypeGenerator

  def _MakeComplexFeatureInfo(self, type, suffix):
    ft_info = {'type': f"{type}_{suffix}", 'is_complex': True}
    return ft_info

  def _GenArrayTypeDefine(self, elem_type, is_complex):
    module_name = self.GetModuleName()
    array_type = f"{elem_type}_array"
    if self._TryCacheFeatureType(array_type):
      self.ArrayTypeGenerator.Generate(elem_type, is_complex)
      array_malloc_func_str = f"FtArray* {module_name}_malloc_{elem_type}_array(void)"
      self._TryCacheArrayMallocFunc(array_malloc_func_str)

  def _GenBaseFeatureInfo(self, ast_type):
    if not isinstance(ast_type, str):
      raise Exception('invalid base ast type: {}'.format(ast_type))
    feature_type = self._MapType(ast_type, self.base_feature_type_map)
    if feature_type == 'FT_ARRAY':
      # void bar(array arr); // same as object[]
      ft_info = self._MakeComplexFeatureInfo("object", 'array')
      self._GenArrayTypeDefine("object", True)
    else:
      ft_info = {'is_complex': False, 'type': feature_type}
    return ft_info

  def _GenArrayFeatureInfo(self, ast_type):
    if not 'element' in ast_type:
      raise Exception('invalid array ast type: {}'.format(ast_type))
    elem_ast_type = ast_type['element']
    elem_ft_info = self.GenerateFeatureInfo(elem_ast_type)
    if elem_ft_info['is_complex']:
      elem_ft_type = elem_ft_info['type']
      ft_info = self._MakeComplexFeatureInfo(elem_ft_type, 'array')
      self._GenArrayTypeDefine(elem_ft_type, True)
    else:
      ft_info = self._MakeComplexFeatureInfo(elem_ast_type, 'array')
      self._GenArrayTypeDefine(elem_ast_type, False)
    return ft_info

  def _GenReferredFeatureInfo(self, ast_type):
    if 'referred_type' not in ast_type:
      raise Exception('invalid referred ast type: {}'.format(ast_type))
    ft_info = {'is_complex': True}
    referred_type = ast_type['referred_type']
    referred_name = ast_type['referred_name']
    if referred_type == 'callback':
      if not referred_name in self.callback_id_set:
        raise Exception('undefined callback: {}'.format(referred_name))
      ft_info['type'] = f"{referred_name}_callback_type"
    elif referred_type == 'struct':
      if not referred_name in self.struct_name_set:
        raise Exception('undefined struct: {}'.format(referred_name))
      ft_info = self._MakeComplexFeatureInfo(referred_name, 'struct_type')
    elif referred_type == 'interface':
      if not referred_name in self.interface_name_set:
        raise Exception('undefined interface: {}'.format(referred_name))
      if self.isCPP: # special
         referred_name = 'I' + self.ToClassName(referred_name)
      ft_info = self._MakeComplexFeatureInfo(referred_name, 'interface_type')
    elif referred_type == 'id':
      if not (referred_name in self.struct_name_set \
          or referred_name in self.callback_id_set \
          or referred_name in self.interface_name_set):
        raise Exception('undefined id: {}'.format(referred_name))
      if referred_name in self.callback_id_set:
        ft_info['type'] = f"{referred_name}_callback_type"
      elif referred_name in self.struct_name_set:
        ft_info = self._MakeComplexFeatureInfo(referred_name, 'struct_type')
      elif referred_name in self.interface_name_set:
        ft_info = self._MakeComplexFeatureInfo(referred_name, 'interface_type')
    elif referred_type == 'enum':
      ft_info['type'] = 'FT_INT';
    return ft_info

  def GenerateFeatureInfo(self, ast_type):
    if isinstance(ast_type, str):
      return self._GenBaseFeatureInfo(ast_type)
    if not isinstance(ast_type, dict):
      raise Exception('invalid complex type: {}'.format(ast_type))
    if 'element' in ast_type:
      return self._GenArrayFeatureInfo(ast_type)
    elif 'referred_type' in ast_type:
      return self._GenReferredFeatureInfo(ast_type)
    elif ast_type['type'] == 'struct':
      return self._MakeComplexFeatureInfo(ast_type['name'], 'struct_type')
    elif ast_type['type'] == 'message':
      return self._MakeComplexFeatureInfo(ast_type['message_name'], 'message_type')
    elif ast_type['type'] == 'promise':
      raise Exception('promise is not supported now, type: {}'.format(ast_type))
    else:
      raise Exception('invalid complex type: {}'.format(ast_type))

  def GenerateFtExpression(self, info):
    ft_expr = info['type']
    if info['is_complex']:
      module_name = self.GetModuleName()
      if self.isCPP:
        ft_expr = '%s' % (ft_expr)
      else:
        ft_expr = f"{module_name}_{ft_expr}"
      ft_expr = f"FT_MK_COMPLEX(&{ft_expr})"
    return ft_expr

  def GenerateReturnFtInfo(self, ret_node):
    if isinstance(ret_node, dict) and ret_node['type'] == 'promise':
      resolve_info = self.GenerateFeatureInfo(ret_node['resolve_type'])
      resolve_ft = resolve_info['type']
      promise_ft = f"promise_{resolve_ft}_type"
      return {'type': promise_ft, 'is_complex': True}
    return render.GenerateFeatureInfo(ret_node)

  def GenerateReturnType(self, ret_node):
    if isinstance(ret_node, str):
      if self.isCPP and ret_node == "string":
        return "ft_utils::FtStringPtr"
      else:
        return self._MapType(ret_node, self.cpp_type_map)
    elif isinstance(ret_node, dict):
      if ret_node['type'] == 'promise':
        return 'FtPromiseId'
      else:
        return self.GenerateCppType(ret_node)
    return 'void'

  def GenerateParamList(self, params):
    param_list = []
    param_count = len(params)
    for index, param in enumerate(params):
      param_type = param["type"]
      if index < param_count -1 and param_type == 'ellipse':
        raise Exception('wrong ellipse param position: {}'.format(params))
      param_str = self.GenerateCppType(param_type)
      if self.isCPP:
        if self.IsRefType(param_type):
          param_str = "const " + param_str
          param_str += "&"
      if 'name' in param:
        p_name = param["name"]
        param_str += f" {p_name}"
      elif param_type == 'ellipse':
        param_str += f" vari_params"
      param_list.append(param_str)
    return param_list

  def GenerateParamsStr(self, params):
    return ", ".join(self.GenerateParamList(params))

  def GenerateParamTypeList(self, params):
    type_list = []
    param_count = len(params)
    for index, param in enumerate(params):
      param_type = param["type"]
      if index < param_count -1 and param_type == 'ellipse':
        raise Exception('wrong ellipse param position: {}'.format(params))
      param_type = self.GenerateCppType(param_type)
      type_list.append(param_type)
    return type_list

  def GenerateParamValuesStr(self, params):
    value_list = []
    for param in params:
      if param["type"] == 'ellipse':
        raise Exception('ellipse param not allowed : {}'.format(params))
      value_list.append(param["name"])
    return ", ".join(value_list)

  def _GenerateParamsDefine(self, node, ret_type):
    if 'identifier' not in node:
      raise Exception('not a function or use node: {}'.format(node))
    params_def = 'FeatureInstanceHandle feature, AppendData append_data'
    if ret_type == 'FtPromiseId':
      params_def += ', FtPromiseId pid'
    if 'params' in node:
      params_str = self.GenerateParamsStr(node["params"])
      params_def += f", {params_str}"
    return params_def

  def GenerateReturnParamsDefine(self, node, ret_node):
    if 'identifier' not in node:
      raise Exception('not a function or use node: {}'.format(node))
    ret_type = self.GenerateReturnType(ret_node)
    params_def = self._GenerateParamsDefine(node, ret_type)
    return {'ret_type': ret_type, 'params_def': params_def}

  def GenerateFunctionDefine(self, node):
    if node['type'] != 'function':
      raise Exception('not a function node: {}'.format(node))
    identifier = node["identifier"]
    ret_params = self.GenerateReturnParamsDefine(node, node["return_type"])
    ret_type = ret_params['ret_type']
    if ret_type == 'FtPromiseId':
      ret_type = 'void'
    params_def = ret_params['params_def']
    module_name = self.GetModuleName()
    func_define = f"{ret_type} {module_name}_wrap_{identifier}({params_def})"
    return func_define

  def GenerateFuncStubDefine(self, node):
    if node['type'] != 'function':
      raise Exception('not a function node: {}'.format(node))
    module_name = self.GetModuleName()
    identifier = node["identifier"]
    stub_define = f"void {module_name}_{identifier}_stub(NativeFunc wrap_func, void** argv, void* ret)"
    return stub_define

  def GenerateWrapFuncCallArgs(self, node, ret_node):
    if 'identifier' not in node:
      raise Exception('not a function or use node: {}'.format(node))
    has_promise = isinstance(ret_node, dict) and ret_node['type'] == 'promise'
    extra_argc = 0
    arg_list = []
    if has_promise:
      extra_argc += 1
      promis_arg = "*(FtPromiseId*)(argv[0])"
      arg_list.append(promis_arg)
    if 'params' in node:
      argc = 0
      for param in node['params']:
        ptype = param['type']
        argc += 1
        if isinstance(ptype, str) and ptype == 'ellipse':
          break
      param_types = self.GenerateParamTypeList(node['params'])
      for i in range(extra_argc, argc + extra_argc):
        arg_name = f"argv[{i}]"
        arg_type = param_types[i - extra_argc]
        if self.isCPP:
          arg_type = "const " + arg_type
        arg = f"*({arg_type}*)({arg_name})"
        arg_list.append(arg)
    return arg_list

  def GenerateInterfaceCtorDefine(self, node):
    if node['type'] != 'function':
      return None

    module_name = self.GetModuleName()
    identifier = node["identifier"]
    ret_type = self.GenerateReturnType(node["return_type"])
    ctor_name = f"{module_name}_{identifier}_instance"
    return f"{ret_type} {ctor_name}(FeatureInstanceHandle feature)"

  def HasEllipseParam(self, params):
    if not isinstance(params, list):
      raise Exception('params node is not a list: {}'.format(params))
    param_count = len(params)
    if param_count <= 0:
      return False
    last_param = params[param_count - 1]
    if last_param['type'] == 'ellipse':
      return True
    return False

  def GetUseReturnTypeNode(self, node):
    if node['type'] != 'use':
      raise Exception('not a use node: {}'.format(node))

    func_call = node['function_call']
    identifier = func_call['identifier']
    if identifier in self.func_ret_node_map:
      return self.func_ret_node_map[identifier]
    raise Exception('cannot find the called function for the use node: {}'.format(node))

  def GenerateParamCallList(self, param_calls):
    call_list = ''
    is_first_param = True
    for param_call in param_calls:
      call_type = param_call["type"]
      call_value = param_call["value"]
      if not is_first_param:
        call_list += ", "
      else:
        is_first_param = False

      if call_type == 'ellipse':
        call_list += "vari_params"
      else:
        call_list += f"{call_value}"
    return call_list

  def TryCacheCallbackId(self, id):
    if not id in self.callback_id_set:
      self.callback_id_set.add(id)
      return True
    return False

  def HasCallbackId(self, id):
    if id in self.callback_id_set:
      return True
    return False

  def TryCacheEventId(self, id):
    if not id in self.event_id_map:
      self.event_idx += 1
      self.event_id_map[id] = self.event_idx
      return self.event_idx
    print('warning: event id already exists: {}'.format(id))
    return 0

  def CheckInterfaceEventId(self, iname, id):
    extends = self.GetInterfaceExtends(iname)
    for extend in extends:
      if not self.CheckInterfaceEventId(extend, id):
        return False

    if iname in self.interface_events_map:
      event_set = self.interface_events_map[iname]
      if id in event_set:
        print('warning: event {} already exists in ancestor interface: {}'.format(id, iname))
        return False
    return True

  def CacheInterfaceEventId(self, iname, id):
    if iname in self.interface_events_map:
      event_set = self.interface_events_map[iname]
    else:
      event_set = set()
      self.interface_events_map[iname] = event_set

    if id in event_set:
      raise Exception('event {} already exists in interface: {}'.format(id, iname))
    event_set.add(id)
    return len(event_set)

  def GetInterfaceFinalEventSize(self, iname):
    final_size = 0
    extends = self.GetInterfaceExtends(iname)
    for extend in extends:
      final_size += self.GetInterfaceFinalEventSize(extend)

    if iname in self.interface_events_map:
      event_set = self.interface_events_map[iname]
      final_size += len(event_set)
    return final_size

  def TryCachePromiseType(self, type):
    if not type in self.promise_type_set:
      self.promise_type_set.add(type)
      return True
    return False

  def CacheFuncReturnNode(self, id, node):
    self.func_ret_node_map[id] = node

  def CacheStructName(self, name):
    if not name in self.struct_name_set:
      self.struct_name_set.add(name)

  def TryCacheInterface(self, name):
    if not name in self.interface_name_set:
      self.interface_name_set.add(name)
      return True
    return False

  def CacheInterfaceMember(self, name, member_info):
    if name in self.interface_members_map:
      member_list = self.interface_members_map[name]
    else:
      member_list = []
      self.interface_members_map[name] = member_list
    # print('cache interface: {}, member_info: {}'.format(name, member_info))
    member_list.append(member_info)

  def GetFinalInterfaceMembers(self, name):
    member_list = []
    extends = self.GetInterfaceExtends(name)
    for extend in extends:
      # print('get parent member, parent: {}'.format(extend))
      member_list.extend(self.GetFinalInterfaceMembers(extend))

    if name in self.interface_members_map:
      member_list.extend(self.interface_members_map[name])
    return member_list

  def GetInterface(self, name):
    for m in self.module['members']:
      if m['type'] == 'interface' and m['name'] == name:
        return m
    return None

  def CacheVTableItem(self, iname, node, item_type):
    if iname in self.vtable_map:
      item_list = self.vtable_map[iname]
    else:
      item_list = []
      self.vtable_map[iname] = item_list

    params = ''
    ret_type = 'void'
    if node['type'] == 'function':
      if item_type != 0:
        raise Exception('interface member function with wrong type: {}'.format(item_type))
      name = node['identifier']
      ret_type = self.GenerateReturnType(node["return_type"])
      has_params = 'params' in node
      if ret_type == 'FtPromiseId':
        ret_type = 'void'
        params += 'FtPromiseId pid'
        if has_params:
          params += ', '
      if has_params:
        params += self.GenerateParamsStr(node["params"])
    elif node['type'] == 'property':
      name = node['name']
      prop_type = node["value_type"]
      cpp_type = self.GenerateCppType(prop_type)
      if item_type == 1:
        ret_type = cpp_type
      elif item_type == 2:
        params = f"{cpp_type} {name}"
      else:
        raise Exception('interface member property with wrong type: {}'.format(item_type))
    else:
      raise Exception('invalid interface member type: {}'.format(node))

    func_item = {
       'index': 0,
       'name': name,
       'params': params,
       'return_type': ret_type,
       'type': item_type # 0 for method, 1 for getter, 2 for setter
    }
    item_list.append(func_item)
    index = item_list.index(func_item)
    func_item['index'] = index
    return index

  def GetFinalVTable(self, iname):
    final_vtable = []
    extends = self.GetInterfaceExtends(iname)
    for extend in extends:
      final_vtable.extend(self.GetFinalVTable(extend))

    if not iname in self.vtable_map:
      raise Exception('cannot find vtable for name: {}'.format(iname))
    final_vtable.extend(self.vtable_map[iname])
    return final_vtable

  def GetFinalVTableSize(self, iname):
    final_size = 0
    extends = self.GetInterfaceExtends(iname)
    for extend in extends:
      final_size += self.GetFinalVTableSize(extend)

    if not iname in self.vtable_map:
      raise Exception('cannot find vtable for name: {}'.format(iname))
    final_size += len(self.vtable_map[iname])
    return final_size

  def IsValidMemberType(self, member):
    return member['type'] == 'function' \
        or member['type'] == 'use' \
        or member['type'] == 'property' \
        or member['type'] == 'const' \
        or member['type'] == 'const_object' \
        or member['type'] == 'event'

  def GetMemberInfo(self, member):
    member_info = {}
    if member['type'] == 'function' or member['type'] == 'use':
      member_info['type'] = 'MEMBER_METHOD'
      member_info['suffix'] = '_member_method'
      member_info['val_type'] = 'method'
      if member['type'] == 'function':
        member_info['name'] = member['identifier']
      elif member['type'] == 'use':
        member_info['name'] = member['function']['identifier']
    elif member['type'] == 'property':
      member_info['type'] = 'MEMBER_ACCESSOR'
      member_info['suffix'] = '_member_accessor'
      member_info['val_type'] = 'accessor'
      member_info['name'] = member['name']
    elif member['type'] == 'const':
      member_info['type'] = 'MEMBER_CONST'
      member_info['suffix'] = '_member_const'
      member_info['val_type'] = 'value'
      member_info['name'] = member['name']
    elif member['type'] == 'const_object':
      member_info['type'] = 'MEMBER_CONST'
      member_info['suffix'] = '_member_const_object'
      member_info['val_type'] = 'value'
      member_info['name'] = member['name']
    elif member['type'] == 'event':
      member_info['type'] = 'MEMBER_EVENT'
      member_info['suffix'] = '_member_event'
      member_info['val_type'] = 'event'
      member_info['name'] = member['identifier']
    else:
      raise Exception('invalid member type: {}'.format(member))
    return member_info

  def mark_permission(self, bits, perm_id):
    bits |= (1 << (perm_id % 64))
    return bits

  def GetPermissionIds(self, permissions):
    perms = permissions.replace(" ", "").split(',')
    low_bits = 0
    high_bits = 0
    for perm in perms:
      if perm in self.low_perms_map:
        low_bits = self.mark_permission(low_bits, self.low_perms_map[perm])
      elif perm in self.high_perms_map:
        high_bits = self.mark_permission(high_bits, self.high_perms_map[perm])

    perm_bits = {
        'low_bits': f"{low_bits}",
        'high_bits': f"{high_bits}"
    }
    return perm_bits


### TS Render
class TSRender(Render):

  ts_type_map = {
    'int' : 'number',
    'int' : 'number',
    'long' : 'number',
    'ulong' : 'number',
    'float' : 'number',
    'double' : 'number',
    'boolean' : 'boolean',
    'string': 'string',
    'uint8' : 'number',
    'int8'  : 'number',
    'uint16' : 'number',
    'int16' : 'number',
    'uint32' : 'number',
    'int32' : 'number',
    'uint64' : 'number',
    'int64' : 'number',
    'void' : 'void',
    'object' : 'any',
    'ellipse' : '...rest: any[]',
    'callback' : 'callback',
    'Int8Array' : 'array',
    'Uint8Array' : 'array',
    'Int16Array' : 'array',
    'Uint16Array' : 'array',
    'Int32Array' : 'array',
    'Uint32Array' : 'array',
    'Int64Array' : 'array',
    'Uint64Array' : 'array',
    'IntArray' : 'array',
    'UintArray' : 'array',
    'LongArray' : 'array',
    'UlongArray' : 'array',
    'FloatArray' : 'array',
    'DoubleArray' : 'array',
  }

  def __init__(self, json_file, d_ts_file, configs):
    self.d_ts_tmpl = GetTemplate('json_ast_d_ts.mt')
    self.d_ts_file = d_ts_file
    self.callback_map = {}
    self.func_ret_node_map = {}
    self.interface_member_map = {}
    Render.__init__(self, json_file, configs)
    # cache types
    self._cacheTypes()

  def _cacheTypes(self):
    for child in self.module["members"]:
      if not "type" in child: continue
      t = child["type"]
      if t == "callback":
        self.TryCacheCallback(child)

  def Generate(self):
    self._GenerateFromTemplate(self.d_ts_tmpl, self.GetDTSFilePath())

  def _GenerateFromTemplate(self, tmpl, out):
    WriteFile(tmpl.render(render=self), out)

  def GenHeaderDefine(self):
    return 'JSON_AST_GEN_MODULE_%s_H_' % (self.GetModuleName().upper())

  def GetDTSFileName(self):
    if self.d_ts_file:
      return GetFileName(self.d_ts_file)
    return '%s.d.ts' % (self.GetModuleName())

  def GetDTSFilePath(self):
    if self.d_ts_file:
      return self.MakeOutPath(self.d_ts_file)
    file_name = '%s.d.ts' % (self.GetModuleName())
    return self.MakeOutPath(file_name)

  def GenerateTsType(self, ast_type):
    if isinstance(ast_type, str):
      return self._MapType(ast_type, self.ts_type_map)

    if not (isinstance(ast_type, dict) and 'type' in ast_type):
      raise Exception('invalid complex type: {}'.format(ast_type))

    # print("ast_type: {}".format(ast_type))
    if 'element' in ast_type:
      ts_type = self.GenerateTsType(ast_type['element'])
      return ts_type + '[]'
    elif 'referred_type' in ast_type:
      referred_type = ast_type['referred_type']
      if referred_type == 'callback':
        referred_name = ast_type['referred_name']
        if referred_name not in self.callback_map:
          raise Exception('invalid callback type: {}'.format(referred_name))
        return self.callback_map[referred_name]
      elif referred_type == 'struct' or referred_type == 'interface':
        return ast_type['referred_name']
    elif ast_type['type'] == 'promise':
      return 'any'
    else:
      raise Exception('invalid complex type: {}'.format(ast_type))

  def GenerateParamsStr(self, params):
    param_list = []
    param_count = len(params)
    if param_count == 0:
      return ''
    for index, param in enumerate(params):
      param_type = param["type"]
      if index < param_count -1 and param_type == 'ellipse':
        raise Exception('wrong ellipse param position: {}'.format(params))
      param_str = self.GenerateTsType(param_type)
      if 'name' in param:
        p_name = param["name"]
        param_str = f"{p_name}: {param_str}"
      param_list.append(param_str)
    return ", ".join(param_list)

  def GenerateFunctionDefine(self, node):
    if node['type'] != 'function':
      return None

    identifier = node["identifier"]
    ret_type = self.GenerateTsType(node["return_type"])
    params = ''
    if 'params' in node:
      params = self.GenerateParamsStr(node["params"])
    func_define = f"{identifier}({params}): {ret_type}"
    return func_define

  def TryCacheCallback(self, ast_type):
    if not (isinstance(ast_type, dict) and \
        'type' in ast_type and ast_type['type'] == 'callback'):
      raise Exception('invalid callback node: {}'.format(ast_type))
    id = ast_type['identifier']
    if id in self.callback_map:
      return False
    params = ''
    if 'params' in ast_type:
      params = self.GenerateParamsStr(ast_type['params'])
    cb_def = f"({params}) => void"
    self.callback_map[id] = cb_def
    return True

  def CacheFuncReturnNode(self, id, node):
    self.func_ret_node_map[id] = node

  def GetUseReturnNode(self, identifier):
    if identifier in self.func_ret_node_map:
      return self.func_ret_node_map[identifier]
    raise Exception('cannot find the called function for the use node: {}'.format(node))

  def GenerateParamNameList(self, params):
    param_count = len(params)
    if param_count == 0:
      return ''
    name_list = []
    for index, param in enumerate(params):
      param_type = param["type"]
      if index < param_count -1 and param_type == 'ellipse':
        raise Exception('wrong ellipse param position: {}'.format(params))
      if param_type == 'ellipse':
        name_list.append('rest')
      elif 'name' in param:
        name_list.append(param["name"])
    return ", ".join(name_list)

  def GenerateParamCallList(self, param_calls):
    call_list = ''
    is_first_param = True
    for param_call in param_calls:
      call_type = param_call["type"]
      call_value = ''
      if 'value' not in param_call:
        if call_type != 'ellipse':
          raise Exception('invalid param_call param: {}'.format(param_call))
      else:
        call_value = param_call["value"]

      if not is_first_param:
        call_list += ", "
      else:
        is_first_param = False

      if call_type == 'ellipse':
        call_list += 'rest'
      else:
        call_list += f"{call_value}"
    return call_list

  def _CacheToListMap(self, list_map, list_key, value):
    if list_key in list_map:
      list = list_map[list_key]
    else:
      list = []
      list_map[list_key] = list
    list.append(value)

  def CacheInterfaceMember(self, iname, method_def):
    # print("cache Interface({}) member: {}".format(iname, method_def))
    self._CacheToListMap(self.interface_member_map, iname, method_def)

  def GetFinalInterfaceMembers(self, iname):
    # print("get Interface({}) members".format(iname))
    method_list = []
    extends = self.GetInterfaceExtends(iname)
    for extend in extends:
      method_list.extend(self.GetFinalInterfaceMembers(extend))

    if not iname in self.interface_member_map:
      return method_list
      # raise Exception('cannot find methods for interface: {}'.format(iname))
    method_list.extend(self.interface_member_map[iname])
    return method_list

### Usage and main entry point
def Usage():
   print("usage %s <jidl-file|json-ast-file> -out-dir <outdir> [-options]" % sys.argv[0])

   print("options:")
   print("\t-lang [c++|ts]")
   print("\toptions when -lang c++")
   print("\t\t-header <header-file-name>, header file in <outdir>")
   print("\t\t-source <source-file-name>, source file in <outdir>")
   print("\t\t-namespace <name-space>, namespace of c++, default is 'feature_wrap'")

lang_keys = {
  'c': ['header', 'source'],
  'c++': ['header', 'source'],
  'ts': ['dts']
}

def CheckArgs(configs):
  lang = configs['lang']
  if not lang in lang_keys:
    print("unkown lang type: %s" % lang)
  keys = lang_keys[lang]
  for k in keys:
    if not k in configs:
      print("need the option: '%s' of lang '%s'" % (k, lang))
      Usage()
      sys.exit(0)

def ParseArgs():
  if len(sys.argv) <= 1:
    Usage()
    sys.exit(0)

  configs = {'lang': 'c', 'debug': False}
  configs['input'] = sys.argv[1]
  options = {}
  i = 2
  while i < len(sys.argv):
    if sys.argv[i].startswith("-option-"):
      options[sys.argv[i][len("-option-"):]] = True
    elif sys.argv[i] == '-debug':
      configs['debug'] = True
      global g_debug
      g_debug = True
    elif sys.argv[i][0] == '-':
      configs[sys.argv[i][1:]] = sys.argv[i+1]
      i = i + 1
    i = i + 1

  CheckArgs(configs)

  configs['options'] = options
  return configs

if __name__ == '__main__':
  from jidl import JIDL
  import jidlast
  configs = ParseArgs()
  input_file = configs['input']
  file_ext = os.path.splitext(input_file)[1]
  if file_ext != '.json' and file_ext != '.jidl':
    print("error! invalid input file extension: '%s'" % (file_ext))
    Usage()
    sys.exit(0)

  file_path_name = os.path.splitext(input_file)[0]
  json_file = file_path_name + ".json"
  if file_ext == '.jidl':
    # print("generating json ast file: '%s' ..." % (json_file))
    jidl_file = open(input_file)
    jidl_file_name = os.path.splitext(os.path.basename(input_file))[0]
    jidl = JIDL(errorReporter = jidl_error.Reporter(input_file), jidlFileName = jidl_file_name, outDir = configs['out-dir'])
    jidl.parse(jidl_file.read())
    jidl.clean()
    jidl_file.close()
    module = jidl.module
    if module:
       dump_out = jidlast.DumpOut()
       module.Dump(dump_out)

       # resolve the types
       context = jidlast.Context()
       module.Resolve(context)
       context.ResetTable()
       module.Check(context)
       context.ShowError(dump_out)

       # to json
       ast_json = {}
       module.ToJson(ast_json)
       json_out = json.dumps(ast_json, indent=4, separators=(',',':'))
       WriteFile(json_out, json_file)
    else:
       print("parse file %s failed" % jidl_file)
       sys.exit(-1)

  if configs['lang'] == 'c' or configs['lang'] == 'c++':
    render = CPPRender(json_file, configs['header'], configs['source'], configs)
  elif configs['lang'] == 'ts':
    render = TSRender(json_file, configs['dts'], configs)

  render.Generate()

