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
    to_key = ''
    if 'meta' in p:
      meta = p['meta']
      if 'msg_key' in meta:
        p_name = meta['msg_key']
      if 'to_msg' in meta:
        to_key = meta['to_msg']
    return (p_name, to_key)
    

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
    key_name, to_key = self.getMsgKey(param)

    # char* only be used in protobuf message, all message will be change to json message, no changes for now.
    if to_key == "char*":
      out[key_name] = '(%s)(%s%s)' %(to_key, prefix, p_name)
    elif to_key == "int_to_str":
      out[key_name] = 'MicoFeatureUtils::num_to_str<int>(szbuf, buf_len, %s%s)' %(prefix, p_name)
    elif to_key == "double_to_str":
      out[key_name] = 'MicoFeatureUtils::num_to_str<double>(szbuf, buf_len, %s%s)' %(prefix, p_name)
    elif to_key == "bool_to_str":
      out[key_name] = 'MicoFeatureUtils::num_to_str<double>(szbuf, buf_len, %s%s)' %(prefix, p_name)
    elif to_key == "any_to_str":
      out[key_name] = 'MicoFeatureUtils::any_to_str(szbuf, buf_len, %s%s, conn)' %(prefix, p_name)
    elif to_key == "int_arr_to_str":
      out[key_name] = 'MicoFeatureUtils::arr_to_str<int>(szbuf, buf_len, %s%s, ArrayType::INT)' %(prefix, p_name)
    elif to_key == "double_arr_to_str":
      out[key_name] = 'MicoFeatureUtils::arr_to_str<double>(szbuf, buf_len, %s%s, ArrayType::DOUBLE)' %(prefix, p_name)
    elif to_key == "str_arr_to_str":
      out[key_name] = 'MicoFeatureUtils::arr_to_str<char*>(szbuf, buf_len, %s%s, ArrayType::STRING)' %(prefix, p_name)
    elif to_key == "bool_arr_to_str":
      out[key_name] = 'MicoFeatureUtils::arr_to_str<bool>(szbuf, buf_len, %s%s, ArrayType::BOOL)' %(prefix, p_name)
    elif to_key == "str_to_json_str":
      out[key_name] = 'MicoFeatureUtils::str_to_json_str(szbuf, buf_len, %s%s)' %(prefix, p_name)
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
