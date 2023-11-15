# Copyright 2023 Xiaomi, Inc. All rights reserved.

import source_render as render

class UIUtils(render.Utils):
  def __init__(self, source, args, vars):
    render.Utils.__init__(self, source, args, vars)
    #self.array_trans_natives = {}
    #self.defineArrayNative('int')
    #self.defineArrayNative('float')
    #self.defineArrayNative('string')

  def getNativeType(self, tp):
    return self.findTypeMeta(tp['type'], tp['name'], 'native_type', True)

  def getDefaultValueType(self):
    return 'JSValue'

  def cppTypeDefault(self, tp):
    if type(tp) == str and tp == 'object':
      return 'JSValue'
    return render.Utils.cppTypeDefault(self, tp)

  def cppTypeStruct(self, tp):
    struct_type = self.findType("struct", tp['referred_name'])
    if struct_type:
      if 'meta' in struct_type and 'native_type' in struct_type['meta']:
        return struct_type['meta']['native_type']
    return self.cppTypeDefault(tp)

  def fromNativeDefault(self, tp):
    if isinstance(tp, dict):
      if tp['type'] == 'reference':
        if tp['referred_type'] == 'struct':
          return '%s_%s_from_native' % (self.toIdName(self.doc['name']), tp['referred_name'])
      elif tp['type'] == 'array':
        return self.fromArrayNative(tp)
    return 'jsvalue_to_jsvalue'

  def toNativeDefault(self, tp):
    if isinstance(tp, dict):
      if tp['type'] == 'reference':
        if tp['referred_type'] == 'struct':
          return '%s_%s_to_native' % (self.toIdName(self.doc['name']), tp['referred_name'])
      elif tp['type'] == 'array':
        return self.toArrayNative(tp)
    return 'jsvalue_to_jsvalue'

  def freeNativeDefault(self, tp):
    if isinstance(tp, dict):
      if tp['type'] == 'array':
        return self.freeArrayNative(tp)
    return None

  def getArrayNative(self, tp, native_name):
    el_type = tp['element']
    el_type_name = self.toTypeName(el_type)
    if el_type_name in self.array_trans_natives:
      return self.array_trans_natives[el_type_name][native_name]

  def getPropertyOnce(self, prop):
    if 'meta' in prop and 'once' in prop['meta']:
      return prop['meta']['once'] == 'true'

  def getPropertyFlags(self, prop):
    flags = '0'
    if self.getPropertyOnce(prop):
      flags = flags + '|JF_PROPERTY_ONCE'
    return flags

  def getMethodFlags(self, m):
    flags = '0'
    if not 'meta' in m: return flags;
    meta = m['meta']
    if 'param_pack' in meta and meta['param_pack'] == 'true':
      flags = flags + '|JF_PARAM_PACK';
    if 'need_this' in meta and meta['need_this'] == 'true':
      flags = flags + '|JF_METHOD_NEED_THIS'
    return flags

  #def defineArrayNative(self, el_type):
  #  el_type_name = self.toTypeName(el_type)
  #  el_native_type = self.cppType(el_type)
  #  name_prefix = 'array_%s_%s_' %(self.getModuleName(), self.toIdName(el_native_type))
  #  el_from_native = self.fromNative(el_type)
  #  el_to_native = self.toNative(el_type)
  #  el_free_native = self.freeNative(el_type)
  #  el_natives = {
  #    'element_native_type' : el_native_type,
  #    'element_from_native' : el_from_native,
  #    'element_to_native'   : el_to_native,
  #    'element_free_native' : el_free_native,
  #    'array_native_type'   : 'array_%s_t' % el_native_type,
  #    'from_native' : name_prefix + 'from_native',
  #    'to_native' : name_prefix + 'to_native',
  #    'free_native' : name_prefix + 'free_native',
  #  }
  #  self.array_trans_natives[el_type_name] = el_natives

  #def fromArrayNative(self, tp):
  #  return self.getArrayNative(tp, 'from_native')

  #def toArrayNative(self, tp):
  #  return self.getArrayNative(tp, 'to_native')

  #def freeArrayNative(self, tp):
  #  return self.getArrayNative(tp, 'free_native')

  #def cppTypeArray(self, tp):
  #  return self.getArrayNative(tp, 'array_native_type')

if __name__ == '__main__':
    render.main(UIUtils)
