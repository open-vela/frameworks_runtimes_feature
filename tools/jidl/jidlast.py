# Copyright 2023 Xiaomi, Inc. All rights reserved.

import json
import jidl_error

LITERVAL = 1
PRIMARY_TYPE = 2
CALLBACK_DEFINE = 3
FUNCTION_DEFINE = 4
STRUCT_DEFINE = 5
CLASS_DEFINE = 6
MODULE_DEFINE = 7
PRIMARY_ARRAY_TYPE =  8
ID = 9
PARAM_DEFINE = 10
PARAM_LIST = 11
PARAM_CALL_LIST = 12
PARAM_ELLIPSE = 13
FUNCTION_CALL = 14
USE_DEFINE = 15
ASYNC_INFO = 16
CONST_DEFINE = 17
CONSTRUCTOR_DEFINE = 18
PROPERTY_DEFINE = 19
BLOCK_LIST = 20
IMPORT_DEFINE = 21
IMPORT_LIST = 22
EVENT_DEFINE = 23
PROMISE_TYPE = 24
TYPED_ARRAY_TYPE = 25
UNIQUE_BUFFER_TYPE = 26
STRUCT_MEMBER_BASE = 27
STRUCT_MEMBER_STRUCT = 28
STRUCT_MEMBER_CALLBACK = 29
INTERFACE_DEFINE = 30
META_ATTRIBUTE = 31
ARRAY_LITERAL = 32
ENUM_DEFINE = 33
USER_TYPE_DEFINE = 34
ID_ARRAY_TYPE = 35
IMPORT_MESSAGE = 36
MESSAGE_DECLARE = 37
MESSAGE_TYPE = 38
CONST_OBJECT_DEFINE = 39

type_names = {
  LITERVAL : 'literval',
  PRIMARY_TYPE : 'primary',
  PROMISE_TYPE : 'promise',
  CALLBACK_DEFINE : 'callback',
  EVENT_DEFINE : 'event',
  FUNCTION_DEFINE : 'function',
  STRUCT_DEFINE : 'struct',
  CLASS_DEFINE : 'class',
  INTERFACE_DEFINE : 'interface',
  MODULE_DEFINE : 'module',
  PRIMARY_ARRAY_TYPE : 'primary array',
  ID : 'id',
  PARAM_DEFINE : 'param',
  PARAM_LIST : 'param list',
  PARAM_CALL_LIST : ' param call list',
  PARAM_ELLIPSE : 'param ellipse',
  FUNCTION_CALL : 'function call',
  USE_DEFINE : 'use',
  ASYNC_INFO : 'async',
  CONST_DEFINE : 'const define',
  CONSTRUCTOR_DEFINE : 'constructor',
  PROPERTY_DEFINE : 'property',
  BLOCK_LIST : 'block list',
  IMPORT_DEFINE : 'import',
  IMPORT_LIST : 'import list',
  EVENT_DEFINE : 'event',
  PROMISE_TYPE : 'promise',
  TYPED_ARRAY_TYPE : 'typed array',
  UNIQUE_BUFFER_TYPE : 'unique_type',
  STRUCT_MEMBER_BASE: 'struct_member_base',
  STRUCT_MEMBER_STRUCT: 'struct_member_struct',
  STRUCT_MEMBER_CALLBACK: 'struct_member_callback',
  META_ATTRIBUTE : 'meta_attribute',
  ARRAY_LITERAL : 'array_literal',
  ENUM_DEFINE : 'enum_define',
  USER_TYPE_DEFINE : 'type_define',
  ID_ARRAY_TYPE : 'id array',
  IMPORT_MESSAGE : 'import_message',
  MESSAGE_DECLARE : 'message_declare',
  MESSAGE_TYPE : 'message_type',
  CONST_OBJECT_DEFINE : 'const_object'
}

def TypeName(tp):
  return type_names[tp]

def AddJson(t, out):
  if isinstance(out, dict):
    for k,v in t.items(): out[k] = v
  else:
    out.append(t)

class Node:
  def __init__(self, tp):
    self.__type = tp
    self.lineno = 0
    self.lexpos = 0

  def GetType(self):
    return self.__type

  def SetType(self, tp):
    self.__type = tp

  def Is(self, tp):
    return self.__type == tp

  def IsReferenceType(self):
    return self.__type == INTERFACE_DEFINE or \
           self.__type == CLASS_DEFINE or \
           self.__type == STRUCT_DEFINE or \
           self.__type == ENUM_DEFINE or \
           self.__type == CALLBACK_DEFINE or \
           self.__type == USER_TYPE_DEFINE

  def Check(self, context):
    pass

  def Resolve(self, context):
    pass

  def __str__(self):
    return '[type:%s]'% TypeName(self.__type)

  def Dump(self, out):
    out.Write(str(self))

  def ToJson(self, out):
    pass

class LiteralValue(Node):
  def __init__(self, value, value_type):
    Node.__init__(self, LITERVAL)
    self.value = value
    self.type = GetPrimaryType(value_type)

  def __str__(self):
    return str(self.value)

  def isNull(self):
    return self.type.Is(PRIMARY_TYPE) and \
      self.type.name == 'null'

class LiteralArrayValue(Node):
  def __init__(self, value):
    Node.__init__(self, ARRAY_LITERAL)
    self.value = value

class Type(Node):
  def __init__(self, name, tp):
    Node.__init__(self, tp)
    self.name = name
    self.meta_attributes = []

  def __str__(self):
    return self.name

  def IsNullable(self):
    return self.Is(INTERFACE_DEFINE) or \
           self.Is(CLASS_DEFINE) or \
           self.Is(STRUCT_DEFINE) or \
           self.Is(CALLBACK_DEFINE) or \
           self.Is(PRIMARY_ARRAY_TYPE) or \
           self.Is(ID_ARRAY_TYPE) or \
           (self.Is(PRIMARY_TYPE) and self.name == "object") or \
           (self.Is(PRIMARY_TYPE) and self.name == "jsonobject")

  def SetMetaAttributes(self, meta_attrs):
    self.meta_attributes = meta_attrs

  def Check(self, context):
    context.AddId(self.name, self)

  def ToJson(self, out):
    if len(self.meta_attributes) > 0:
      meta_attrs = {}
      for a in self.meta_attributes:
        a.ToJson(meta_attrs)
      out['meta'] = meta_attrs

  def GetJson(self):
    out = {}
    self.ToJson(out)
    return out

class PrimaryType(Type):
  def __init__(self, name):
    Type.__init__(self, name, PRIMARY_TYPE)

  def ToJson(self, out):
    if not self.name in primary_types:
      return
    out['type'] = self.name

class MessageType(Type):
   def __init__(self, name):
      Type.__init__(self, name, MESSAGE_TYPE)

   def ToJson(self, out):
     out['type'] = 'message'
     out['name'] = self.name


class UniqueBufferType(Type):
  def __init__(self, name):
    Type.__init__(self, name, UNIQUE_BUFFER_TYPE)

  def ToJson(self, out):
    out['type'] = 'unique_buffer'

class IDType(Type):
  def __init__(self, name):
    Type.__init__(self, name, ID)

class PrimaryArrayType(Type):
  def __init__(self, base_type):
    Node.__init__(self, PRIMARY_ARRAY_TYPE)
    if type(base_type) == str:
      self.base_type = GetPrimaryType(base_type)
    else:
      self.base_type = base_type

  def __str__(self):
    return str(self.base_type) + '[]'

  def ToJson(self, out):
    out['type'] = 'array'
    out['element'] = self.base_type.name

class IDArrayType(Type):
  def __init__(self, name):
    Type.__init__(self, name, ID_ARRAY_TYPE)
    self.name = name

  def __str__(self):
    return str(self.name) + '[]'
  def ToJson(self, out):
    out['type'] = 'array'
    out['element'] = {'type': 'reference',
            'referred_type' : 'id',
            'referred_name': str(self.name)
    }

class TypedArrayType(Type):
  def __init__(self, base_type):
    Node.__init__(self, TYPED_ARRAY_TYPE)
    if type(base_type) == str:
      self.base_type = GetTypedArrayElementType(base_type)
    else:
      self.base_type = base_type

  def __str__(self):
    return 'typed_array<' + str(self.base_type) + '>'

  def ToJson(self, out):
    out['type'] = 'typed_array'
    out['element'] = self.base_type.name

class PromiseType(Node):
  def __init__(self, resolve_type):
    Node.__init__(self, PROMISE_TYPE)
    self.resolve_type = resolve_type

  def Resolve(self, context):
    self.resolve_type = ResolvePromiseType(context, self.resolve_type, self);

  def __str__(self):
    return "promise<%s>" % (str(self.resolve_type))

  def GetJson(self):
    return {'type': 'promise',
            'resolve_type' : GetTypeJson(self.resolve_type)
    }

  def ToJson(self, out):
    r = self.GetJson()
    for k,v in r.items(): out[k] = v

class EllipseType(Node):
  def __init__(self, primary_type = None):
    Node.__init__(self, PARAM_ELLIPSE)
    if primary_type:
      self.primary = GetPrimaryType(primary_type)

  def __str__(self):
    if hasattr(self, 'primary'):
      return str(self.primary) + '...'
    return '...'

  def ToJson(self, out):
    ellipse_def = {}
    ellipse_def['type'] = 'ellipse'
    if hasattr(self, 'primary'):
      ellipse_def['primary'] = str(self.primary)
    out.append(ellipse_def)


def GetTypeJson(tp):
  if tp.IsReferenceType():
    return tp.GetReferenceJson()
  elif tp.Is(PRIMARY_TYPE):
    return tp.name
  else:
    return tp.GetJson()

def MakeReferenctJson(rtype, rname):
  return { 'type' : 'reference',
           'referred_type' : rtype,
           'referred_name': rname }

class ParamDefine(Node):
  def __init__(self, param_type, param_name):
    Node.__init__(self, PARAM_DEFINE)
    self.type = param_type
    self.name = param_name
    self.meta_attributes = []

  def SetMetaAttributes(self, meta_attrs):
    self.meta_attributes = meta_attrs

  def SetDefault(self, def_val):
    self.default = def_val

  def __str__(self):
    s = str(self.type) + ' ' + str(self.name)
    if hasattr(self, 'default'):
      s = s + '=' + str(self.default)
    return s

  def _checkDefault(self):
    if not hasattr(self, 'default'):
      return True
    if type(self.default) != LiteralValue:
      return False
    if (self.type.IsNullable() and (not self.default.isNull())) or \
        ((not self.type.IsNullable()) and self.default.isNull()):
      return False
    return True

  def Resolve(self, context):
    self.type = ResolveParamType(context, self.type, InterfaceDefine, self)
    #print("param resolve: ", self.type, str(self.type), str(self))
    if not self._checkDefault():
      context.AddError("[%d:%d]Resolve Type '%s' failed in '%s'" % (self.lineno, self.lexpos, self.type.name, str(self)))
    context.AddId(self.name, self.type)

  def Check(self, context):
    context.AddId(self.name, self.type)

  def GetJson(self):
    param_def = {}
    param_def['type'] = GetTypeJson(self.type)

    if self.name:
      param_def['name'] = self.name
    if hasattr(self, 'default'):
      param_def['default'] = str(self.default)

    if len(self.meta_attributes) > 0:
      meta_attrs = {}
      for a in self.meta_attributes:
        a.ToJson(meta_attrs)
      param_def['meta'] = meta_attrs

    return param_def

  def ToJson(self, out):
    out.append(self.GetJson())

class ListNode(Node):
  def __init__(self, tp):
    Node.__init__(self, tp)
    self.content = []

  def foreach(self, func):
    content = []
    for c in self.content:
      content.append(func(c))
    return content

  def Append(self, n):
    self.content.append(n)
    return self

  def toString(self, sp = ' '):
    return sp.join(self.foreach(lambda c: str(c)))

  def Check(self, context):
    for c in self.content:
      c.Check(context)

  def Resolve(self, context):
    for c in self.content:
      c.Resolve(context)

  def Count(self):
    return len(self.content)

  def Get(self, i):
    return self.content[i]

  def ToJson(self, out):
    for c in self.content:
      c.ToJson(out)

class ParamCallList(ListNode):
  def __init__(self, node):
    ListNode.__init__(self, PARAM_CALL_LIST)
    if node:
      self.Append(node)

  def __str__(self):
    return self.toString(',')

  def Check(self, context):
    for c in self.content:
      if c.Is(ID) and \
         not context.CheckIdExist(c.name, param_accepted_types, FunctionDefine):
           context.AddError("[%d:%d] Function Call param id '%s' is not exist '%s'" % \
            (self.lineno, self.lexpos, c.name, str(self)))

  def ToJson(self, out):
    for c in self.content:
      param_call = {}
      if c.Is(ID):
        param_call['type'] = 'ref'
        param_call['value'] = c.name
      elif c.Is(LITERVAL):
        param_call['type'] = 'literal'
        param_call['value'] = c.value
      elif c.Is(PARAM_ELLIPSE):
        param_call['type'] = 'ellipse'
      out.append(param_call)

class ParamList(ListNode):
  def __init__(self, node = None):
    ListNode.__init__(self, PARAM_LIST)
    if node:
      self.Append(node)

  def __str__(self):
    return self.toString(',')

class FunctionCall(Node):
  def __init__(self, name, call_params):
    Node.__init__(self, FUNCTION_CALL)
    self.name = name
    self.params = call_params

  def __str__(self):
    return '%s(%s)' % (str(self.name), str(self.params))

  def Check(self, context):
    if not context.CheckIdExist(self.name, [FunctionDefine], InterfaceDefine):
      context.AddError("[%d:%d] Function '%s' is not exits: %s" % (self.lineno, self.lexpos, self.name, str(self)))
    self.params.Check(context)

  def ToJson(self, out):
    out['identifier'] = self.name # 仅作兼容使用
    out['name'] = self.name
    out['param_calls'] = []
    self.params.ToJson(out['param_calls'])

class UseDefine(Type):
  def __init__(self, base_function, function_call):
    Type.__init__(self, base_function.name, USE_DEFINE)
    self.base_function = base_function
    self.function_call = function_call

  def __str__(self):
    return '%s=%s' % (str(self.base_function), str(self.function_call))

  def Check(self, context):
    context.PushTable(self.base_function)
    self.base_function.AddIds(context)
    self.function_call.Check(context)
    if self.function_call.params.Count() > 0 and isinstance(self.function_call.params.Get(-1), EllipseType):
      if not (self.base_function.params.Count() > 0 and \
          isinstance(self.base_function.params.Get(-1), EllipseType)):
        context.AddError("[%d:%d] Use last param ... is not defined '%s'" % (self.lineno, self.lexpos, str(self)))
    context.PopTable()

  def Resolve(self, context):
    callee = context.GetIdExist(self.function_call.name, [FunctionDefine], InterfaceDefine)
    if not callee:
      context.AddError("[%d:%d] function '%s' is not exits : %s" % (self.lineno, self.lexpos, self.function_call.name, str(self)))

    self.callee = callee
    self.base_function.Resolve(context)

  def ToJson(self, out):
    use_def = {}
    Type.ToJson(self, use_def)
    use_def['type'] = 'use'
    use_def['function'] = {}
    self.base_function.GetBaseJson(use_def['function'])
    use_def['function_call'] = {}
    self.function_call.ToJson(use_def['function_call'])
    out.append(use_def)

def RemoveLiteralStringQuotes(s):
  if isinstance(s, LiteralValue):
    return RemoveLiteralStringQuotes(s.value)
  elif type(s) == str:
    if len(s) >= 2 and s[0] == '"' and s[-1] == '"':
      return s[1:-1]
    return s
  elif type(s) == list:
    return [RemoveLiteralStringQuotes(e) for e in s]
  return s

class MetaAttribute(Node):
  def __init__(self, name, value):
    Node.__init__(self, META_ATTRIBUTE)
    self.name = name
    self.value = value

  def ToJson(self, out):
    out[self.name] = RemoveLiteralStringQuotes(self.value)


class CallbackDefine(Type):
  def __init__(self, name, params):
    Type.__init__(self, name, CALLBACK_DEFINE)
    self.params = params

  def __str__(self):
    return 'callback %s(%s)' % (self.name, str(self.params))

  def Check(self, context):
    context.AddId(self.name, self)
    context.PushTable(self)
    self.params.Check(context)
    context.PopTable()

  def Resolve(self, context):
    context.PushTable(self)
    self.params.Resolve(context)
    context.PopTable()

  def GetJson(self):
    callback_def = {}
    Type.ToJson(self, callback_def)
    callback_def['type'] = 'callback'
    callback_def['identifier'] = self.name  # 仅做兼容
    callback_def['name'] = self.name
    if self.params.Count() > 0:
      callback_def['params'] = []
      self.params.ToJson(callback_def['params'])
    return callback_def

  def ToJson(self, out):
    AddJson(self.GetJson(), out)

  def GetReferenceJson(self):
    return MakeReferenctJson('callback', self.name)

class EventDefine(Type):
  def __init__(self, name, params):
    Type.__init__(self, name, EVENT_DEFINE)
    self.params = params
    self.force_async = False

  def __str__(self):
    return 'event %s(%s)' % (self.name, str(self.params))

  def SetForceAsync(self):
    self.force_async = True

  def Check(self, context):
    context.AddId(self.name, self)
    context.PushTable(self)
    self.params.Check(context)
    context.PopTable()

  def Resolve(self, context):
    context.PushTable(self)
    self.params.Resolve(context)
    context.PopTable()

  def ToJson(self, out):
    event_def = {}
    Type.ToJson(self, event_def)
    event_def['type'] = 'event'
    event_def['identifier'] = self.name  #仅作兼容
    event_def['name'] = self.name
    if self.params.Count() > 0:
      event_def['params'] = []
      self.params.ToJson(event_def['params'])
    out.append(event_def)

class UserTypeDefine(Type):
  def __init__(self, name, target_tp):
    Type.__init__(self, name, USER_TYPE_DEFINE)
    self.target = target_tp

  def __str__(self):
    return 'type %s' % (self.name)

  def Check(self, context):
    context.AddId(self.name, self)

  def GetJson(self):
    type_def = {}
    Type.ToJson(self, type_def)
    type_def['type'] = 'user_type'
    type_def['name'] = self.name
    target = {}
    self.target.ToJson(target)
    type_def['target'] = target['type']
    return type_def

  def ToJson(self, out):
    AddJson(self.GetJson(), out)

  def GetReferenceJson(self):
    return MakeReferenctJson('user_type', self.name)

class AsyncInfo(Node):
  def __init__(self, async_type):
    Node.__init__(self, ASYNC_INFO)
    if async_type:
      self.type = async_type
    else:
      self.type = 'self'

  def IsAsync(self):
    return self.type != 'self'

  def __str__(self):
    return 'async(%s)' %(self.type)

  def ToJson(self, out):
    out['type'] = 'async'
    out['value'] = self.type

SyncInfo = AsyncInfo('self')

class ConstDefine(Type):
  def __init__(self, name, literal):
    Type.__init__(self, name, CONST_DEFINE)
    self.value = literal

  def __str__(self):
    return 'const %s = %s' % (self.name, str(self.value))

  def ToJson(self, out):
    const_def = {}
    Type.ToJson(self, const_def)
    const_def['type'] = 'const'
    const_def['name'] = self.name
    const_def['value'] = str(self.value)
    const_def['value_type'] = {}
    if self.value.type.Is(PRIMARY_TYPE):
      const_def['value_type'] = self.value.type.name
    else:
      self.value.type.ToJson(const_def['value_type'])
    out.append(const_def)

class ConstObjectDefine(Type):
  def __init__(self, name, block_list):
    Type.__init__(self, name, CONST_OBJECT_DEFINE)
    self.content = block_list

  def __str__(self):
    return 'const %s = { ... }' % (self.name)

  def Check(self, context):
    context.AddId(self.name, self)
    context.PushTable(self)
    self.content.Check(context)
    context.PopTable()

  def Resolve(self, context):
    context.PushTable(self)
    self.content.Resolve(context)
    context.PopTable()

  def GetJson(self):
    t = {
       'type': 'const_object',
       'name': self.name,
    }

    Type.ToJson(self, t)
    if hasattr(self, 'content'):
       members = []
       self.content.ToJson(members)
       t['members'] = members
    return t

  def ToJson(self, out):
    AddJson(self.GetJson(), out)

  def Dump(self, out):
    out.Write('const %s = {' % (self.name))
    out.Shift()
    if hasattr(self, 'content'):
      self.content.Dump(out)
    out.Reduce()
    out.Write('}')

class FunctionDefine(Type):
  def __init__(self, name, params):
    Type.__init__(self, name, FUNCTION_DEFINE)
    self.params = params
    self.return_type = GetPrimaryType('void')
    self.async_info = SyncInfo
    self.private = False

  def SetAsync(self, async_info):
    self.async_info = async_info

  def SetReturn(self, ret_type):
    self.return_type = ret_type
    return self

  def SetPrivate(self, p):
    self.private = p

  def __str__(self):
    private = self.private and 'private ' or ''
    return '%s%s %s %s (%s)' % (private, str(self.async_info), str(self.return_type), self.name, str(self.params))

  def Check(self, context):
    Type.Check(self, context)
    #self.return_type.Check(context) # uneed check
    context.PushTable(self)
    self.params.Check(context)
    context.PopTable()

  def Resolve(self, context):
    self.return_type = ResolveReturnType(context, self.return_type, None, self)
    context.PushTable(self)
    self.params.Resolve(context)
    context.PopTable()

  def AddIds(self, context):
    self.params.Check(context)

  def GetBaseJson(self, func_def):
    func_def['identifier'] = self.name # 仅作兼容
    func_def['name'] = self.name
    if self.params.Count() > 0:
      func_def['params'] = []
      self.params.ToJson(func_def['params'])

  def GetJson(self):
    func_def = {}
    Type.ToJson(self, func_def)
    func_def['type'] = 'function'
    if self.private or self.async_info.IsAsync():
      func_def['qualifiers'] = []
      qualifiers = func_def['qualifiers']
      if self.private:
        access_def = {}
        access_def['type'] = 'access'
        access_def['value'] = 'private'
        qualifiers.append(access_def)

      if self.async_info.IsAsync():
        async_def = {}
        self.async_info.ToJson(async_def)
        qualifiers.append(async_def)

    func_def['return_type'] = GetTypeJson(self.return_type)

    self.GetBaseJson(func_def)
    return func_def

  def ToJson(self, out):
    out.append(self.GetJson())

class ConstructorDefine(FunctionDefine):
  def __init__(self, params):
    FunctionDefine.__init__(self, '<init>', params)
    self.SetType(CONSTRUCTOR_DEFINE)

  def ToJson(self, out):
    ctor_def = {}
    ctor_def['type'] = 'constructor'

    if self.params.Count() > 0:
      ctor_def['params'] = []
      self.params.ToJson(ctor_def['params'])
    out.append(ctor_def)

class PropertyDefine(Type):
  def __init__(self, name, tp):
    Type.__init__(self, name, PROPERTY_DEFINE)
    self.type = tp
    self.readable = True
    self.writeable = True
    self.const = False

  def SetReadOnly(self):
    self.readable = True
    self.writeable = False

  def SetWriteOnly(self):
    self.readable = False
    self.writeable = True

  def SetConst(self):
    self.const = True

  def Resolve(self, context):
    self.type = ResolvePropertyType(context, self.type, None, self)

  def __str__(self):
    readable = self.readable and 'readable' or ''
    writeable = self.writeable and 'writeable' or ''
    const = self.const and 'const' or ''
    return '%s property %s %s %s %s' % (const, str(self.type), self.name, readable, writeable)

  def ToJson(self, out):
    prop_def = {}
    Type.ToJson(self, prop_def)
    prop_def['type'] = 'property'
    prop_def['name'] = self.name
    if self.readable:
      prop_def['readable'] = self.readable
    if self.writeable:
      prop_def['writeable'] = self.writeable
    if self.const:
      prop_def['const'] = self.const
    prop_def['value_type'] = GetTypeJson(self.type)
    out.append(prop_def)

class BlockList(ListNode):
  def __init__(self):
    ListNode.__init__(self, BLOCK_LIST)

  def __str__(self):
    return self.toString('\n')

  def Check(self, context):
    for block in self.content:
      block.Check(context)

  def Resolve(self, context):
    # first, add types
    for block in self.content:
      if isinstance(block, Type):
        context.AddId(block.name, block)

    # second, resolve the alldata
    for block in self.content:
      block.Resolve(context)

  def Dump(self, out):
    for block in self.content:
      block.Dump(out)

class EnumDefine(Type):
  def __init__(self, name, block_list):
    Type.__init__(self, name, ENUM_DEFINE)
    self.members = block_list
    idx = 0
    for m in self.members:
      if m.value:
        if isinstance(m.value, LiteralValue) and m.value.type.name == 'int':
          idx = self.value.value + 1
      else:
        v = LiteralValue(idx + 1, 'int')
        m.value = v
        idx = idx + 1

  def Dump(self, out):
    out.Write('enum %s {' % (self.name))
    out.Shift()
    for m in self.members:
      out.Write('%s = %s,' % (m.name, str(m.value)))
    out.Reduce()
    out.Write('}')

  def GetReferenceJson(self):
    return MakeReferenctJson('enum', self.name)

  def GetJson(self):
    members = []
    for m in self.members:
      m.ToJson(members)

    t = { 'type': 'enum',
          'name': self.name,
          'members': members
    }
    Type.ToJson(self, t)
    return t

  def ToJson(self, out):
    AddJson(self.GetJson(), out)

class InterfaceDefine(Type):
  def __init__(self, name, tp, block_list = None):
    Type.__init__(self, name, tp)
    self.extends = []
    if block_list:
      self.content = block_list

  def SetExtends(self, extends):
    self.extends = extends

  def SetContent(self, content):
    self.content = content

  def toString(self, type_name):
    return '%s %s { ... }' % (type_name, self.name)

  def __str__(self):
    return self.toString('interface')

  def Check(self, context):
    context.AddId(self.name, self)
    context.PushTable(self)
    self.content.Check(context)
    context.PopTable()

  def Resolve(self, context):
    context.PushTable(self)
    self.content.Resolve(context)
    context.PopTable()

  def GetClassType(self):
    return 'interface'

  def GetDumpName(self):
    return self.name

  def GetJson(self):
    t = {
       'type': self.GetClassType(),
       'name': self.name,
       'extends': self.extends,
    }

    Type.ToJson(self, t)

    if hasattr(self, 'content'):
       members = []
       self.content.ToJson(members)
       t['members'] = members
    return t

  def ToJson(self, out):
    AddJson(self.GetJson(), out)

  def GetReferenceJson(self):
    return MakeReferenctJson('interface', self.name)

  def Dump(self, out):
    out.Write('%s %s {' % (self.GetClassType(), self.GetDumpName()))
    out.Shift()
    if hasattr(self, 'content'):
      self.content.Dump(out)
    out.Reduce()
    out.Write('}')

class StructDefine(InterfaceDefine):
  def __init__(self, name, block_list = None):
    InterfaceDefine.__init__(self, name, STRUCT_DEFINE, block_list)

  def __str__(self):
    return self.toString('struct')

  def GetClassType(self):
    return 'struct'

  def ToJson(self, out):
    struct_type = self.GetJson()
    out.append(struct_type)

  def GetReferenceJson(self):
    return MakeReferenctJson('struct', self.name)

class StructMemberBase(Type):
  def __init__(self, member_type, member_name):
    Type.__init__(self, member_name, STRUCT_MEMBER_BASE)
    self.type = member_type

  def SetDefault(self, def_val):
    self.default = def_val

  def __str__(self):
    s = str(self.type) + ' ' + str(self.name)
    if hasattr(self, 'default'):
      s = s + '=' + str(self.default)
    return s

  def _checkDefault(self):
    if not hasattr(self, 'default'):
      return True
    if type(self.default) != LiteralValue:
      return False
    if (self.type.IsNullable() and (not self.default.isNull())) or \
        ((not self.type.IsNullable()) and self.default.isNull()):
      return False
    return True

  def Resolve(self, context):
    self.type = ResolveStructMemberType(context, self.type, None, self)
    if not self._checkDefault():
      context.AddError("[%d:%d]Resolve Type '%s' failed in '%s'" % (self.lineno, self.lexpos, str(self.type), str(self)))
    #print("param resolve: ", self.type, str(self.type), str(self))
    #context.AddId(self.name, self.type)

  #def Check(self, context):
  #  context.AddId(self.name, self.type)

  def ToJson(self, out):
    member_type = {}
    Type.ToJson(self, member_type)
    member_type['type'] = GetTypeJson(self.type)

    if self.name:
      member_type['name'] = self.name
    if hasattr(self, 'default'):
      member_type['default'] = str(self.default)
    out.append(member_type)

class StructMemberStruct(Type):
  def __init__(self, struct_type, struct_name):
    Type.__init__(self, struct_name, STRUCT_MEMBER_STRUCT)
    self.type = struct_type

  def SetDefaultNull(self, null_val):
    self.default_null = null_val

  def __str__(self):
    s = 'member struct: ' + str(self.type) + ' ' + str(self.name)
    if hasattr(self, 'default_null'):
      s = s + '=' + str(self.default_null)
    return s

  def Resolve(self, context):
    self.type = ResolveStructMemberType(context, self.type, None, self)
    #print("param resolve: ", self.type, str(self.type), str(self))
    #context.AddId(self.name, self.type)

  #def Check(self, context):
  #  context.AddId(self.name, self.type)

  def ToJson(self, out):
    member_type = {}
    if not self.type.Is(STRUCT_DEFINE):
      raise Exception('not an struct member struct: {}'.format(self.type))

    Type.ToJson(self, member_type)
    member_type['type'] = GetTypeJson(self.type)
    if self.name:
      member_type['name'] = self.name
    if hasattr(self, 'default_null'):
      member_type['default'] = str(self.default_null)
    out.append(member_type)

class StructMemberCallback(Type):
  def __init__(self, callback_type, callback_name):
    Type.__init__(self, callback_name, STRUCT_MEMBER_CALLBACK)
    #print("=== callback_type:", callback_type, type(callback_type))
    self.type = callback_type

  def SetDefaultNull(self, null_val):
    self.default_null = null_val

  def __str__(self):
    s = 'member callback: ' + str(self.type) + ' ' + str(self.name)
    if hasattr(self, 'default_null'):
      s = s + '=' + str(self.default_null)
    return s

  def Resolve(self, context):
    #print("param resolve: ", self.type)
    self.type = ResolveStructMemberType(context, self.type, None, self)
    #context.AddId(self.name, self.type)

  #def Check(self, context):
  #  context.AddId(self.name, self.type)

  def GetJson(self):
    member_type = {}
    if not self.type.Is(CALLBACK_DEFINE):
      raise Exception('not an struct member callback: {}'.format(self.type))

    Type.ToJson(self, member_type)
    member_type['type'] = GetTypeJson(self.type)
    if self.name:
      member_type['name'] = self.name
    if hasattr(self, 'default_null'):
      member_type['default'] = str(self.default_null)
    return member_type

  def ToJson(self, out):
    out.append(self.GetJson())

class ClassDefine(InterfaceDefine):
  def __init__(self, name, block_list = None):
    InterfaceDefine.__init__(self, name, CLASS_DEFINE, block_list)

  def __str__(self):
    return self.toString('class')

  def GetClassType(self):
    return 'class'

  def ToJson(self, out):
    class_def = InterfaceDefine.GetJson(self)
    class_def['type'] = 'class'
    out.append(class_def)

  def GetReferenceJson(self):
    return MakeReferenctJson('class', self.name)

class ImportDefine(Node):
  def __init__(self, name, version):
    Node.__init__(self, IMPORT_DEFINE)
    self.module = name
    self.version = version

  def __str__(self):
    return 'import %s@%s' % (self.module, self.version)

  def Resolve(self, context):
    pass

  def ToJson(self, out):
    d = {}
    d['name'] = '%s@%s' % (self.module, self.version)
    out.append(d)

class ImportMessage(Node):
  def __init__(self, message_list, pb_path):
    Node.__init__(self, IMPORT_MESSAGE)
    self.message_list = message_list
    self.pb_path = pb_path.value[1:-1]

  def __str__(self):
    return 'import_message {%s} from "%s"' % (str(self.message_list), self.pb_path)

  def ToJson(self, out):
    import_message = {"type": "import_message"}
    message_list = []
    for msg in self.message_list:
      message_list.append(msg.GetJson())
    import_message['message_list'] = message_list
    import_message['protobuf'] = self.pb_path
    out.append(import_message)

  def Resolve(self, context):
    for msg in self.message_list:
      context.AddId(msg.message_name, msg)

class MessageDeclare(Node):
  def __init__(self, pb_name, message_name):
    Node.__init__(self, MESSAGE_DECLARE)
    self.pb_name = pb_name
    self.message_name = message_name

  def __str__(self):
    return self.pb_name == self.message_name and \
                self.message_name or             \
                '%s as %s' % (self.pb_name, self.message_name)

  def ToJson(self, message):
    message['type'] = 'message'
    message['protobuf_name'] = self.pb_name
    message['message_name'] = self.message_name

  def GetJson(self):
    out = {}
    self.ToJson(out)
    return out

class ImportList(ListNode):
  def __init__(self, node = None):
    ListNode.__init__(self, IMPORT_LIST)
    if node:
      self.Append(node)

    def __str__(self):
      return self.toString('\n')

    def Resolve(self, context):
      for el in self.content:
          el.Resolve(context)

    def Dump(self, out):
      for c in self.content:
        c.Dump(out)

class ModuleDefine(InterfaceDefine):
  def __init__(self, name, version, block_list = None):
    InterfaceDefine.__init__(self, name, MODULE_DEFINE, block_list)
    self.version = version
    self.imports = None

  def SetImports(self, imports):
    self.imports = imports

  def __str__(self):
    return 'module %s@%s ...' % (self.name, self.version)

  def Check(self, context):
    if self.imports:
      self.imports.Check(context)
    InterfaceDefine.Check(self, context)

  def Resolve(self, context):
    context.AddId(self.name, self)
    if self.imports:
      self.imports.Resolve(context)
    context.PushTable(self)
    InterfaceDefine.Resolve(self, context)
    context.PopTable()

  def GetClassType(self):
    return 'Module'

  def GetDumpName(self):
    return '%s@%s' % (self.name, self.version)

  def ToJson(self, out):
    out['type'] = 'module'
    out['name'] = '%s@%s' % (self.name, self.version)
    if self.imports:
      out['imports'] = []
      self.imports.ToJson(out['imports'])

    if hasattr(self, 'content'):
      out['members'] = []
      self.content.ToJson(out['members'])

primary_types = {}

def GetPrimaryType(tp_name):
  if type(tp_name) == str:
    if tp_name in primary_types:
      return primary_types[tp_name]
    else:
      return None
  return tp_name

def InitPrimaryTypes():
  types = ["int", "float", "double", "string", "boolean",
      "long", "uint", "ulong", "null",
      "array", "object", "jsonobject", "void"]
  for t in types:
    primary_types[t] = PrimaryType(t)

InitPrimaryTypes()

typed_array_elements ={}
def InitTypedArrayElement():
  types = ['uint8', 'int8', 'uint16', 'int16', 'uint32', 'int32', 'uint', 'int',
          'uint64', 'int64', 'ulong', 'long', 'float', 'double', 'void']
  for t in types:
    typed_array_elements[t] = PrimaryType(t)

InitTypedArrayElement()

def GetTypedArrayElementType(tp_name):
  if type(tp_name) == str:
    if tp_name in typed_array_elements:
      return typed_array_elements[tp_name]
  return None

class IDTable:
  def __init__(self, owner):
    self.owner = owner
    self.table = {}

  def AddId(self, id_name, obj, context):
    if id_name in self.table:
      v = self.table[id_name]
      #print(id_name, v)
      context.AddError("[%d:%d] id:'%s' has defined in %s" % (v.lineno, v.lexpos, id_name, str(v)))
      return
    self.table[id_name] = obj

  def GetIdExist(self, id_name, accetable_types):
    if not id_name in self.table:
      return None
    v = self.table[id_name]
    for t in accetable_types:
      if isinstance(v, t):
        return v
    return None

  def IsOwner(self, owner_type):
    return self.owner and isinstance(self.owner, owner_type)

struct_member_accepted_types = (
  PrimaryType,
  PrimaryArrayType,
  IDArrayType,
  TypedArrayType,
  StructDefine,
  InterfaceDefine,
  CallbackDefine,
  EnumDefine,
  UserTypeDefine,
  MessageDeclare,
)

param_accepted_types = (
  PrimaryType,
  PrimaryArrayType,
  IDArrayType,
  CallbackDefine,
  StructDefine,
  InterfaceDefine,
  EnumDefine,
  EllipseType,
  UserTypeDefine,
  MessageDeclare,
)

return_accepted_type = (
  PrimaryType,
  PrimaryArrayType,
  IDArrayType,
  InterfaceDefine,
  StructDefine,
  EnumDefine,
  PromiseType,
  UserTypeDefine,
  MessageDeclare,
)

value_accepted_type = (
  PrimaryType,
  PrimaryArrayType,
  IDArrayType,
  InterfaceDefine,
  EnumDefine,
  StructDefine,
  MessageType,
)

direct_resolve_types = (
  PrimaryType,
  PrimaryArrayType,
  IDArrayType,
  PromiseType,
  TypedArrayType,
  UniqueBufferType
)

def IsDirectResolveType(tp):
  for t in direct_resolve_types:
    if isinstance(tp, t): return True
  return False

def ResolveType(context, tp, accepted, owner, holder):
  if IsDirectResolveType(tp):
    return tp
  #print("==== tp: ", tp, "--", type(tp));
  new_tp = context.GetIdExist(tp.name, accepted, owner)
  #print("==== new tp: ", new_tp, type(new_tp));
  if not new_tp:
    context.AddError("[%d:%d]Resolve Type '%s' failed in '%s'" % (holder.lineno, holder.lexpos, tp.name, str(holder)))
    return tp
  return new_tp

def ResolveValueType(context, tp, owner, holder):
  return ResolveType(context, tp, value_accepted_type, owner, holder)

def ResolveParamType(context, tp, owner, holder):
  return ResolveType(context, tp, param_accepted_types, None, holder)

def ResolveReturnType(context, tp, owner, holder):
  ret_type = ResolveType(context, tp, return_accepted_type, owner, holder)
  if isinstance(ret_type, PromiseType):
     ret_type.Resolve(context)
  return ret_type

def ResolveStructMemberType(context, tp, owner, holder):
  return ResolveType(context, tp, struct_member_accepted_types, owner, holder)

def ResolvePropertyType(context, tp, owner, holder):
  return ResolveType(context, tp, struct_member_accepted_types, owner, holder)

def ResolvePromiseType(context, tp, holder):
  return ResolveType(context, tp, value_accepted_type, None, holder)

class Context:
  def __init__(self):
    self.errors = []
    self.ResetTable()

  def ResetTable(self):
    self.stack_table = [IDTable(None)]
    self.current_table = self.stack_table[-1]

  def HasError(self):
    return len(self.errors) > 0

  def AddId(self, id_name, obj):
    if id_name in primary_types:
      return
    self.current_table.AddId(id_name, obj, self)

  def PushTable(self, owner):
    self.current_table = IDTable(owner)
    self.stack_table.append(self.current_table)

  def PopTable(self):
    self.stack_table.pop()
    self.current_table = self.stack_table[-1]

  def AddError(self, error):
    self.errors.append(error)

  def GetIdExist(self, id_name, accepts, owner_type):
    for i in range(1, len(self.stack_table) + 1):
      table = self.stack_table[-i]
      if owner_type:
        if table.IsOwner(owner_type):
          #print(table.table, table.owner, owner_type, accepts)
          return table.GetIdExist(id_name, accepts)

      else:
        t = table.GetIdExist(id_name, accepts)
        if t: return t
    return None

  def CheckIdExist(self, id_name, accepts, owner_type):
    return self.GetIdExist(id_name, accepts, owner_type) != None

  def ShowError(self, out):
    out.Write('\n'.join(self.errors))
    if len(self.errors) > 0:
      raise Exception("ERROR:%s" % (';\n'.join(self.errors)))

class DumpOut:
  def __init__(self):
    self.prefix = ''

  def Write(self, s):
    # print('%s%s' % (self.prefix, s))
    pass

  def Shift(self):
    self.prefix = self.prefix + '\t'

  def Reduce(self):
    self.prefix = self.prefix[0:-1]

def WriteFile(content, filename):
  print('write json ast to %s' % filename)
  f = open(filename, 'wt')
  f.write(content)
  f.close()

if __name__ == '__main__':
  import sys
  import os
  from jidl import JIDL
  jidl_file = sys.argv[1]
  f = open(jidl_file)
  jidl = JIDL(errorReporter = jidl_error.Reporter(jidl_file))
  jidl.parse(f.read())
  f.close()
  module = jidl.module
  if module:
    dump_out = DumpOut()
    module.Dump(dump_out)

    # resolve
    context = Context()
    module.Resolve(context)
    context.ResetTable()
    module.Check(context)

    context.ShowError(dump_out)

    # to json
    ast_json = {}
    module.ToJson(ast_json)
    json_out = json.dumps(ast_json, indent=4, separators=(',',':'))
    print("ast json: ", json_out)
    out_file = os.path.splitext(jidl_file)[0]
    WriteFile(json_out, out_file + ".json")
  else:
    print("parse %s failed!" % jidl_file)

