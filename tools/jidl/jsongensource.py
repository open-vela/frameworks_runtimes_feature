# Copyright 2023 Xiaomi, Inc. All rights reserved.
# jidl AST Json file to cpp generator

import sys
import os
import re
import json
from mako.template import Template

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

class Render:
  def __init__(self, json_file, configs):
    self.json_file = json_file
    self.configs = configs
    self.outdir = None
    if 'out-dir' in configs:
      self.outdir = configs['out-dir']
    self.module = self.LoadJSON()

  def LoadJSON(self):
    with open(self.json_file, 'r', encoding='UTF-8') as f:
      # load JSON data as Python direction
      json_module = json.load(f)
    # print("module json ast 对象：",json_module)
    return json_module

  def GetModuleName(self):
    name = self.GetRawModuleName()
    return name.replace('.', '_')

  def GetRawModuleName(self):
    return self.module['name'].split("@")[0]

  def MakeOutPath(self, filename):
    if self.outdir:
      return os.path.join(self.outdir, filename)
    return os.path.join(script_dir, filename)

### CPP Render
class CPPRender(Render):
  param_ref_types = (
    'FtArray',
  )

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
    'array'  : 'FtArray',
    'Int8Array' : 'FtArray',
    'Uint8Array' : 'FtArray',
    'Int16Array' : 'FtArray',
    'Uint16Array' : 'FtArray',
    'Int32Array' : 'FtArray',
    'Uint32Array' : 'FtArray',
    'Int64Array' : 'FtArray',
    'Uint64Array' : 'FtArray',
    'IntArray' : 'FtArray',
    'UintArray' : 'FtArray',
    'LongArray' : 'FtArray',
    'UlongArray' : 'FtArray',
    'FloatArray' : 'FtArray',
    'DoubleArray' : 'FtArray',
  }

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
  }

  def __init__(self, json_file, header_file, source_file, configs):
    self.header_tmpl = GetTemplate('json_ast_header.mt')
    self.source_tmpl = GetTemplate('json_ast_source.mt')
    self.header_file = header_file
    self.source_file = source_file
    self.func_ret_node_map = {}
    self.callback_id_set = set()
    self.promise_type_set = set()
    self.struct_name_set = set()
    self.interface_name_set = set()
    self.vtable_map = {}
    self.interface_extends_map = {}
    self.interface_members_map = {}
    self.feature_type_set = set()
    self.array_malloc_func_set = set()
    Render.__init__(self, json_file, configs)
    # cache types
    self._cacheTypes()

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

  def Generate(self):
    self._GenerateFromTemplate(self.source_tmpl, self.GetCppFilePath())
    self._GenerateFromTemplate(self.header_tmpl, self.GetHeaderFilePath())

  def _GenerateFromTemplate(self, tmpl, out):
    WriteFile(tmpl.render(render=self), out)

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

  def _MapType(self, ast_type, type_map):
    if not isinstance(ast_type, str):
      raise Exception('not a valid str type: {}'.format(ast_type))
    if ast_type not in type_map:
      raise Exception('can not map type: {}'.format(ast_type))
    return type_map[ast_type]

  def GenerateCppType(self, ast_type):
    if isinstance(ast_type, str):
      return self._MapType(ast_type, self.cpp_type_map)

    if not isinstance(ast_type, dict):
      raise Exception('not a valid complex type: {}'.format(ast_type))

    module_name = self.GetModuleName()
    if 'element' in ast_type:
      return 'FtArray'
    elif 'referred_type' in ast_type:
      referred_type = ast_type['referred_type']
      if referred_type == 'callback':
        return self.GenerateCppType(referred_type)
      elif referred_type == 'struct':
        referred_name = ast_type['referred_name']
        return f"{module_name}_{referred_name} *"
      elif referred_type == 'interface':
        return "FeatureInterfaceHandle"
    elif ast_type['type'] == 'struct':
        struct_name = ast_type['name']
        return f"{module_name}_{struct_name} *"
    else:
      raise Exception('not a valid complex type: {}'.format(ast_type))

  def IsParamRefType(self, cpp_type):
    return cpp_type in self.param_ref_types

  def GenerateArrayCppType(self, ast_type):
    if isinstance(ast_type, str):
      return self._MapType(ast_type, self.array_cpp_type_map)
    if not isinstance(ast_type, dict) or ('element' not in ast_type):
      raise Exception('not a valid array type: {}'.format(ast_type))
    return self._MapType(ast_type['element'], self.cpp_type_map)

  def GetOptValName(self, feature_type):
    val_type = self._MapType(feature_type, self.opt_val_name_map)
    if val_type:
      return val_type
    return 'ptr'

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

  def SetArrayTypeGenerator(self, ArrayTypeGenerator):
    self.ArrayTypeGenerator = ArrayTypeGenerator

  def _GenComplexRefFeatureInfo(self, type, suffix):
    ft_info = {}
    ft_info['type'] = f"{type}_{suffix}"
    ft_info['is_complex'] = True
    ft_info['is_complex_ref'] = True
    return ft_info

  def _GenArrayFeatureInfo(self, array_type, is_complex):
    ft_info = self._GenComplexRefFeatureInfo(array_type, 'array')
    if self._TryCacheFeatureType(ft_info['type']):
      self.ArrayTypeGenerator.Generate(array_type, is_complex)
      module_name = self.GetModuleName()
      array_malloc_func_str = f"FtArray* {module_name}_malloc_{array_type}_array()"
      self._TryCacheArrayMallocFunc(array_malloc_func_str)
    return ft_info

  def GetArrayMallocFuncDefines(self):
    return self.array_malloc_func_set

  def ToBaseFeatureType(self, ast_type):
    feature_type = self._MapType(ast_type, self.base_feature_type_map)
    if feature_type == 'FT_ARRAY':
      feature_type = self._MapType(ast_type, self.array_feature_type_map)
    return feature_type

  def GenerateFeatureInfo(self, ast_type):
    ft_info = {}
    ft_info['is_complex'] = False
    ft_info['is_complex_ref'] = False

    if isinstance(ast_type, str):
      feature_type = self._MapType(ast_type, self.base_feature_type_map)
      if feature_type == 'FT_ARRAY':
        # void bar(array arr); // same as object[]
        ft_info = self._GenArrayFeatureInfo("object", True)
      else:
        ft_info['type'] = feature_type
      return ft_info

    if not isinstance(ast_type, dict):
      raise Exception('not a valid complex type: {}'.format(ast_type))

    if 'element' in ast_type:
      elem_ast_type = ast_type['element']
      elem_ft_info = self.GenerateFeatureInfo(elem_ast_type)
      if elem_ft_info['is_complex']:
        elem_ft_type = elem_ft_info['type']
        ft_info = self._GenArrayFeatureInfo(elem_ft_type, True)
      else:
        ft_info = self._GenArrayFeatureInfo(elem_ast_type, False)
    elif 'referred_type' in ast_type:
      if ast_type['referred_type'] == 'callback':
        callback_name = ast_type['referred_name']
        if not callback_name in self.callback_id_set:
          raise Exception('undefined callback: {}'.format(callback_name))
        ft_info['type'] = f"{callback_name}_callback_type"
        ft_info['is_complex'] = True
      elif ast_type['referred_type'] == 'struct':
        struct_name = ast_type['referred_name']
        if not struct_name in self.struct_name_set:
          raise Exception('undefined struct: {}'.format(struct_name))
        ft_info = self._GenComplexRefFeatureInfo(struct_name, 'struct_type')
      elif ast_type['referred_type'] == 'interface':
        interface_name = ast_type['referred_name']
        if not interface_name in self.interface_name_set:
          raise Exception('undefined interface: {}'.format(interface_name))
        ft_info = self._GenComplexRefFeatureInfo(interface_name, 'interface_type')
      elif ast_type['referred_type'] == 'enum':
        ft_info['type'] = 'FT_INT';
    elif ast_type['type'] == 'struct':
      ft_info = self._GenComplexRefFeatureInfo(ast_type['name'], 'struct_type')
    elif ast_type['type'] == 'promise':
      raise Exception('promise is not supported now, type: {}'.format(ast_type))
    else:
      raise Exception('not a valid complex type: {}'.format(ast_type))
    return ft_info

  def GenerateFtExpression(self, info):
    ft_expr = info['type']
    if info['is_complex_ref'] or info['is_complex']:
      module_name = self.GetModuleName()
      ft_expr = f"{module_name}_{ft_expr}"

    if info['is_complex_ref']:
      ft_expr = f"FT_MK_COMPLEX_REF(&{ft_expr})"
    elif info['is_complex']:
      ft_expr = f"FT_MK_COMPLEX(&{ft_expr})"

    return ft_expr

  def GeneratePromiseType(self, ret_type):
    if not isinstance(ret_type, dict) \
      or ret_type['type'] != 'promise':
        raise Exception('not a valid promise type: {}'.format(ret_type))

    if not ('resolve_type' in ret_type and 'reject_type' in ret_type):
      raise Exception('not a complete promise type: {}'.format(ret_type))

    resolve_type = self.GenerateCppType(ret_type['resolve_type'])
    reject_type = self.GenerateCppType(ret_type['reject_type'])
    return f"promise<{resolve_type}, {reject_type}>"

  def _GenerateReturnType(self, ret_type):
    if isinstance(ret_type, str):
      return self._MapType(ret_type, self.cpp_type_map)
    elif isinstance(ret_type, dict):
      if ret_type['type'] == 'promise':
        return 'FtPromiseId'
      else:
        return self.GenerateCppType(ret_type)
    return 'void'

  def GenerateReturnType(self, ret_type):
    ret_type = self._GenerateReturnType(ret_type)
    if ret_type == 'FtArray':
      ret_type += '*'
    return ret_type

  def GenerateParamList(self, params):
    param_list = []
    param_count = len(params)
    for index, param in enumerate(params):
      param_type = param["type"]
      if index < param_count -1 and param_type == 'ellipse':
        raise Exception('wrong ellipse param position: {}'.format(params))
      param_str = self.GenerateCppType(param_type)
      if param_str in self.param_ref_types:
        param_str += '&'
      if 'name' in param:
        p_name = param["name"]
        param_str += f" {p_name}"
      elif param_type == 'ellipse':
        param_str += f" vari_params"
      param_list.append(param_str)
    return ", ".join(param_list)

  def GenerateFunctionDefine(self, node):
    if node['type'] != 'function':
      return None

    identifier = node["identifier"]
    ret_type_node = node["return_type"]
    ret_type = self.GenerateReturnType(ret_type_node)
    prefix_params = 'FeatureInstanceHandle feature, AppendData append_data'
    if ret_type == 'FtPromiseId':
      ret_type = 'void'
      prefix_params += ', FtPromiseId pid'
    module_name = self.GetModuleName()
    func_define = f"{ret_type} {module_name}_wrap_{identifier}({prefix_params}"
    if 'params' in node:
      params_str = self.GenerateParamList(node["params"])
      func_define += f", {params_str}"
    func_define += ")"
    return func_define

  def GenerateInterfaceCtorDefine(self, node):
    if node['type'] != 'function':
      return None

    module_name = self.GetModuleName()
    identifier = node["identifier"]
    ret_type = self.GenerateReturnType(node["return_type"])
    ctor_name = f"{module_name}_{identifier}_instance"
    return f"{ret_type} {ctor_name}(FeatureInstanceHandle feature)"

  def HasEllipseParam(self, node):
    if not 'params' in node:
      return False

    params = node['params']
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
    p_call_str = ''
    is_first_param = True
    for param_call in param_calls:
      p_call_type = param_call["type"]
      p_call_value = param_call["value"]
      if not is_first_param:
        p_call_str += ", "
      else:
        is_first_param = False

      if p_call_type == 'ellipse':
        p_call_str += "vari_params"
      else:
        p_call_str += f"{p_call_value}"
    return p_call_str

  def TryCacheCallbackId(self, id):                                                                                                                                                                                                             
    if not id in self.callback_id_set:
      self.callback_id_set.add(id)
      return True
    return False

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

  def CacheInterfaceMember(self, name, member_info):
    if name in self.interface_members_map:
      member_list = self.interface_members_map[name]
    else:
      member_list = []
      self.interface_members_map[name] = member_list
    # print('cache interface: {}, member_info: {}'.format(name, member_info))
    member_list.append(member_info)

  def GetFinalInterfaceMembers(self, name):
    parent_members = []
    extends = self.GetInterfaceExtends(name)
    for extend in extends:
      # print('get parent member, parent: {}'.format(extend))
      parent_members.extend(self.GetFinalInterfaceMembers(extend))

    if name in self.interface_members_map:
      parent_members.extend(self.interface_members_map[name])
    return parent_members

  def CacheVTableItem(self, interface_name, node, func_type):
    if interface_name in self.vtable_map:
      item_list = self.vtable_map[interface_name]
    else:
      item_list = []
      self.vtable_map[interface_name] = item_list

    params = ''
    ret_type = 'void'
    if node['type'] == 'function':
      if func_type != 0:
        raise Exception('interface member function with wrong type: {}'.format(func_type))
      name = node['identifier']
      ret_type = self.GenerateReturnType(node["return_type"])
      has_params = 'params' in node
      if ret_type == 'FtPromiseId':
        ret_type = 'void'
        params += 'FtPromiseId pid'
        if has_params:
          params += ', '
      if has_params:
        params += self.GenerateParamList(node["params"])
    elif node['type'] == 'property':
      name = node['name']
      prop_type = node["value_type"]
      cpp_type = self.GenerateCppType(prop_type)
      if func_type == 1:
        if cpp_type == 'FtArray':
          cpp_type += '*'
        ret_type = cpp_type
      elif func_type == 2:
        if self.IsParamRefType(cpp_type):
          cpp_type += '&'
        params = f"{cpp_type} {name}"
      else:
        raise Exception('interface member property with wrong type: {}'.format(func_type))
    else:
      raise Exception('not a valid interface member type: {}'.format(node))

    func_item = {
       'index': 0,
       'name': name,
       'params': params,
       'return_type': ret_type,
       'type': func_type # 0 for method, 1 for getter, 2 for setter
    }
    item_list.append(func_item)
    index = item_list.index(func_item)
    func_item['index'] = index
    return index

  def GetFinalVTable(self, interface_name):
    final_vtable = []
    extends = self.GetInterfaceExtends(interface_name)
    for extend in extends:
      final_vtable.extend(self.GetFinalVTable(extend))

    if not interface_name in self.vtable_map:
      raise Exception('cannot find vtable for name: {}'.format(interface_name))
    final_vtable.extend(self.vtable_map[interface_name])
    return final_vtable

  def GetFinalVTableSize(self, interface_name):
    final_size = 0
    extends = self.GetInterfaceExtends(interface_name)
    for extend in extends:
      final_size += self.GetFinalVTableSize(extend)

    if not interface_name in self.vtable_map:
      raise Exception('cannot find vtable for name: {}'.format(interface_name))
    final_size += len(self.vtable_map[interface_name])
    return final_size

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

  def IsStruct(self, ast_node):
    if not isinstance(ast_node, dict):
      return False
    if ast_node['type'] == 'struct':
      return True
    return False

  def PropertyHasGetter(self, node):
    if 'readable' in node or 'const' in node:
      return True
    return False

  def PropertyHasSetter(self, node):
    if ('writeable' in node) and ('const' not in node):
      return True
    return False

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
    else:
      member_info['type'] = 'MEMBER_NULL'
      member_info['suffix'] = ''
      member_info['val_type'] = ''
      member_info['name'] = ''
    return member_info

def Usage():
   print("usage %s <jidl-file|json-ast-file> -out-dir <outdir> [-options]" % sys.argv[0])

lang_keys = {
  'c++': ['header', 'source']
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
  configs = {'lang': 'c++', 'debug': False}
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
    print("error! not a valid input file extension: '%s'" % (file_ext))
    Usage()
    sys.exit(0)
  
  file_path_name = os.path.splitext(input_file)[0]
  json_file = file_path_name + ".json"
  if file_ext == '.jidl':
    print("generating json ast file: '%s' ..." % (json_file))
    jidl_file = open(input_file)
    jidl = JIDL()
    jidl.parse(jidl_file.read())
    jidl_file.close()
    module = jidl.module
    dump_out = jidlast.DumpOut()
    module.Dump(dump_out)
    context = jidlast.Context()
    module.Resolve(context)
    context.ResetTable()
    module.Check(context)
    context.ShowError(dump_out)
    ast_json = {}
    module.ToJson(ast_json)
    json_out = json.dumps(ast_json)
    WriteFile(json_out, json_file)

  if configs['lang'] == 'c++':
    print("generating c/c++ glue files from: '%s' ..." % (json_file))
    render = CPPRender(json_file, configs['header'], configs['source'], configs)
    render.Generate()

