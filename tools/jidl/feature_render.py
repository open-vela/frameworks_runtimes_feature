# Copyright 2023 Xiaomi, Inc. All rights reserved.

import source_render as render

cpp_type_map = {
  'int' : 'FtInt',
  'uint' : 'unsigned int',
  'long' : 'long',
  'ulong' : 'unsigned long',
  'float' : 'FtFloat',
  'double' : 'FtDouble',
  'boolean' : 'FtBoolean',
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
  'callback' : 'FeatureCallbackId',
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

  def fromNativeDefault(self, tp):
    return 'ft_help::from_native'

  def toNativeDefault(self, tp):
    return 'ft_help::to_native'

  def getPromiseType(self, tp):
    return 'FeaturePromiseHandle'

  def genParamsList(self, method):
    params = method['params']
    ret_type = method['return_type']
    param_list = []
    if self.isPromiseType(ret_type):
      param_list.append((self.getPromiseType(ret_type), 'promiseHandle'))
    for i,p in enumerate(params):
      p_type = p['type']
      p_native_type = self.cppType(p_type)
      if IsParamReferenceType(p_native_type):
        p_native_type += '&'
      if 'name' in p:
        pname = p['name']
      else:
        pname = '__args__%d' % i
      param_list.append((p_native_type, pname))
    return param_list

if __name__ == '__main__':
    render.main(FeatureUtils)
