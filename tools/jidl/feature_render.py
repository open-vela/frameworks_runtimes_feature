# Copyright 2023 Xiaomi, Inc. All rights reserved.

import source_render as render

cpp_type_map = {
  'int' : 'FtInt',
  'uint' : 'unsigned int',
  'long' : 'long',
  'ulong' : 'unsigned long',
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
  'object' : 'FtAny',
  'void' : 'void',
  'ellipse' : '...',
  'callback' : 'FtCallbackId',
  'Int8Array' : 'FTArray',
  'Uint8Array' : 'FTArray',
  'Int16Array' : 'FTArray',
  'Uint16Array' : 'FTArray',
  'Int32Array' : 'FTArray',
  'Uint32Array' : 'FTArray',
  'Int64Array' : 'FTArray',
  'Uint64Array' : 'FTArray',
  'IntArray' : 'FTArray',
  'UintArray' : 'FTArray',
  'LongArray' : 'FTArray',
  'UlongArray' : 'FTArray',
  'FloatArray' : 'FTArray',
  'DoubleArray' : 'FTArray',
}

maybe_reference_types = (
  'FTArray'
)

def IsParamReferenceType(p):
  return p in maybe_reference_types

class FeatureUtils(render.Utils):
  def cppTypeDefault(self, tp):
    if isinstance(tp, str):
        if tp in cpp_type_map:
            return cpp_type_map[tp]
    return str(tp)

  def getModuleName(self):
    name = self.doc['name'].split('@')[0]
    return self.toIdName(name)
     

  def cppTypeStruct(self, tp):
    return '%s_%s' % (self.getModuleName(), tp['referred_name'])

  def fromNativeDefault(self, tp):
    return 'ft_help::from_native'

  def toNativeDefault(self, tp):
    return 'ft_help::to_native'

  def getPromiseType(self, tp):
    return 'FeaturePromiseHandle'

  def getMsgKey(self, p):
    p_name = p['name']
    if 'meta' in p:
      meta = p['meta']
      if 'msg_key' in meta:
        p_name = meta['msg_key']
    return p_name

  def getParamStructFromMethod(self, method):
    out = {}
    params = method['params']
    for p in params:
      p_type = 'value_type' in p and p['value_type'] or p['type']
      p_name = p['name']
      s = self.getUserType('struct', p_type['referred_name'])
      out[p_name] = s['members']
    return out

  def genParamsList(self, method):
    if not 'params' in method:
      return []
    params = method['params']
    ret_type = method['return_type']
    param_list = []
    if self.isPromiseType(ret_type):
      param_list.append((self.getPromiseType(ret_type), 'promiseHandle'))
    for i,p in enumerate(params):
      p_type = p['type']
      p_native_type = self.cppType(p_type)
      if self.isStructType(p_type):
        p_native_type += '*'
      elif IsParamReferenceType(p_native_type):
        p_native_type += '&'
      if 'name' in p:
        pname = p['name']
      else:
        pname = '__args__%d' % i
      param_list.append((p_native_type, pname))
    return param_list

  def needGenerator(self, m):
    return not ('meta' in m and 'external' in m['meta'] and m['meta']['external'] == 'true')

  def genToMsg(self, out, param, prefix):
    p_name = param['name']
    p_type = param['type']
    if ('type' in p_type and 'element' in p_type):
      p_type = p_type['element'] + "_" + p_type['type']
    key_name = self.getMsgKey(param)

    if p_type == 'int' or p_type == 'uint' or p_type == 'long' or p_type == 'ulong' or \
      p_type == 'uint8' or p_type == 'uint16' or p_type == 'uint32' or p_type == 'uint64' or \
      p_type == 'int8' or p_type == 'int16' or p_type == 'int32' or p_type == 'int64' or \
      p_type == 'float' or p_type == 'double':
      out[key_name] = 'MicoFeatureUtils::num_to_json_str<%s>(%s%s)' %(p_type, prefix, p_name)
    elif p_type == 'int_array' or p_type == 'uint_array' or p_type == 'long_array' or \
      p_type == 'ulong_array' or p_type == 'double_array' or p_type == 'float_array' or \
      p_type == 'string_array' or p_type == 'boolean_array' or \
      p_type == 'boolean' or p_type == 'string':
      out[key_name] = 'MicoFeatureUtils::%s_to_json_str(%s%s)' %(p_type, prefix, p_name)
    elif p_type == 'object':
      out[key_name] = 'MicoFeatureUtils::%s_to_json_str(%s%s, conn, arg_%s_c_str)' %(p_type, prefix, p_name, key_name)
    else:
      out[key_name] = '%s%s' %(prefix, p_name)

  def genParam(self, out, param, prefix):
    p_type = 'value_type' in param and param['value_type'] or param['type']
    p_name = param['name']
    if self.isStructType(p_type):
      s = self.getUserType('struct', p_type['referred_name'])
      if s:
        for s_mb in s['members']:
          self.genParam(out, s_mb, '%s->' % p_name)
    elif self.isCallbackType(p_type):
      if p_name in ['success', 'fail', 'complete']:
        if not 'callbacks' in out: out['callbacks'] = {}
        out['callbacks'][p_name] = '%s%s' % (prefix, p_name)
    else:
      self.genToMsg(out, param, prefix)

  def genParams(self, method):
    out = {}
    if not 'params' in method:
      return out
    params = method['params']
    for p in params:
      self.genParam(out, p, '')
    return out

if __name__ == '__main__':
    render.main(FeatureUtils)
