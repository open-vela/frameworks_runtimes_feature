// Copyright 2023 Xiaomi, Inc. All rights reserved.

%for include in vars['includes']:
#include "${include}"
%endfor

<% module_name = utils.getModuleName() %>

<%
def gen_from_native_interface(tp):
  intf_tp = utils.getInterfaceType(tp, 'need_interface')
  return intf_tp and ',%s_member_index_%s'%(module_name, utils.toIdName(intf_tp['name'])) or ''
%>

<%def name='gen_struct_define(s)'>
<%
  meta = 'meta' in s and s['meta'] or None
%>
%if meta:
<%
  native_type = meta['native_type']
  members = []
  for m in s['members']:
    m_meta = ('meta' in m and m['meta'] or None)
    m_name = m['name']
    m_native_name = m_name
    m_type = m['type']
    m_to_native = None
    m_from_native = None
    m_from_native_intf = ''
    m_native_type = None
    if m_meta:
      if 'name' in m_meta: m_native_name = m_meta['name']
      if 'native_type' in m_meta: m_native_type = m_meta['native_type']
      if 'to_native' in m_meta:   m_to_native = m_meta['to_native']
      if 'from_native' in m_meta: m_from_native = m_meta['from_native']

    if not m_native_type:
      m_native_type = utils.cppType(m_type)
    if not m_to_native:
      m_to_native = utils.toNative(m_type)
    if not m_from_native:
      m_from_native = utils.fromNative(m_type)
      m_from_native_intf = gen_from_native_interface(m_type)
    members.append({
      "name": m_name,
      "native_name": m_native_name,
      "native_type" : m_native_type,
      "to_native" : m_to_native,
      "from_native" : m_from_native,
      "from_native_intf" : m_from_native_intf
    })
%>
JSValue ${module_name}_${s['name']}_from_native(JSContext* ctx, const ${native_type}* native_value) {
  JSValue js_value = JS_NewObject(ctx);
%for m in members:
  JS_SetPropertyStr(ctx, js_value, "${m['name']}", ${m['from_native']}(ctx, native_value->${m['native_name']}${m['from_native_intf']}));
%endfor
  return js_value;
}
%endif

int  ${module_name}_${s['name']}_to_native(JSContext* ctx, ${native_type}* native_value, JSValueConst value) {
%for m in members:
  ${m['to_native']}(ctx, &(native_value->${m['native_name']}), JS_GetPropertyStr(ctx, value, "${m['name']}"));
%endfor
  return 0;
}
</%def>

<%def name="gen_property_event(pname, native_type, m, meta, setter)">
<%
  source_gen = meta['source_gen']
  source_format = vars[source_gen]
  if not source_format:
    source_format = "ERROR: unkown format \"%s\" in %s.%s" % (source_gen, pname, m['name'])
  values = utils.buildPropertyValues(pname, m)
  values['context'] = 'ctx'
  values['native_self'] = 'self'
  values['param_value'] = 'value'
  values['module_name'] = module_name
  values['native_type'] = native_type
  source = source_format % values
%>
static void ${setter}(JSContext* ctx, NativeHandle self, JSValueConst value) {
  ${source};
}
</%def>

<%def name="gen_property(pname, native_type, m, member_defines)">
<%
  meta = None
  prop_name = None
  native_value_type = None
  if 'meta' in m:
    meta = m['meta']
    if 'native_value_type' in meta: native_value_type = meta['native_value_type']
  getter = 'NULL'
  setter = 'NULL'
  if 'readable' in m and  m['readable']:
    if meta and 'rawget' in meta:
      getter = meta['rawget']
    else:
      getter = '_%s_get_%s' % (pname, m['name'])
  if 'writeable' in m and m['writeable']:
    if meta and 'rawset' in meta:
      setter = meta['rawset']
    else:
      setter = '_%s_set_%s' % (pname, m['name'])
%>

%if meta:
%if 'type' in meta:
  %if meta['type'] == 'event':
  ${gen_property_event(p, native_type, m, meta, setter)}
  %elif meta['type'] == 'id':
  %endif
%else:
<%
  value_type = utils.getValueType(m)
%>
%if 'readable' in m and  m['readable'] and meta and (not 'rawget' in meta):
static JSValue ${getter}(JSContext* ctx, NativeHandle self) {
%if 'value' in meta:
  return ${utils.fromNative(value_type)}(ctx, ${meta['value']}${gen_from_native_interface(value_type)});
%elif 'get' in meta:
  %if utils.isStructType(value_type):
  ${utils.cppType(value_type)} to_val;
  ${meta['get']}((${native_type})self, &to_val);
  return ${utils.fromNative(value_type)}(ctx, &to_val${gen_from_native_interface(value_type)});
  %else:
    %if native_value_type and native_value_type == 'string_buffer':
  <%
    buffer_length = 256
    if 'buffer_length' in meta: buffer_length = meta['buffer_length']
  %>
  char __szbuf__[${buffer_length}];
  ${meta['get']}((${native_type})self, __szbuf__, sizeof(__szbuf__));
  return ${utils.fromNative(value_type)}(ctx, __szbuf__${gen_from_native_interface(value_type)});
    %else:
  return ${utils.fromNative(value_type)}(ctx, ${meta['get']}((${native_type})self)${gen_from_native_interface(value_type)});
    %endif
  %endif
%elif 'field' in meta:
  return ${utils.fromNative(value_type)}(ctx, ((${native_type})(self))->${meta['field']}${gen_from_native_interface(value_type)});
%endif
}
%endif

%if 'writeable' in m and m['writeable'] and meta and (not 'rawset' in meta):
static void ${setter}(JSContext* ctx, NativeHandle self, JSValueConst val) {
%if 'value' in meta:
  ${utils.toNative(value_type)}(ctx, &${meta['value']}, val);
%elif 'set' in meta:
  ${utils.cppType(value_type)} to_val;
  if (${utils.toNative(value_type)}(ctx, &to_val, val) == 0) {
    ${meta['set']}((${native_type})self, to_val);
  } else {
    // TODO exception
  }
  <% free_value = utils.freeNative(value_type) %>
  %if free_value:
  ${free_value}(ctx, to_val);
  %endif
%elif 'field' in meta:
  ${utils.toNative(native_type)}(ctx, &(((${native_type})self)->${meta['field']}), val);
%endif
}
%endif

%endif
%endif

<% prop_name = '_%s_%s_property' % (pname, m['name']) %>
static JSMetaProperty ${prop_name} = {
  "${m['name']}",
  NULL, /* aliase_name */
  ${pname}_member_index_${m['name']},
  JM_PROPERTY,
  ${('readable' in m and m['readable']) and '1' or '0'},
  ${('writeable' in m and m['writeable']) and '1' or '0'},
  ${utils.getPropertyFlags(m)},
  ${setter},
  ${getter}
};
<%
if prop_name:
  member_defines.append(prop_name)
%>
</%def>

<%def name="gen_method(pname, native_type, m, member_defines)">
<%
  meta = None
  func_name = None
  need_self = True
  param_pack = False
  aliase_name = 'NULL'
  call_name = '_%s_%s' %(pname, m['identifier'])
  func_name = '_%s_%s_method' % (pname, m['identifier'])
  if 'meta' in m:
    meta = m['meta']
    if 'need_self' in meta:
      need_self = (meta['need_self'] == "true" and True or False)
    if 'param_pack' in meta:
      param_pack = (meta['param_pack'] == "true" and True or False)
    if 'name' in meta:
      aliase_name = '"%s"' %  meta['name']
    if 'rawfunc' in meta:
      call_name = meta['rawfunc']
%>

%if meta and 'func' in meta:
<%
  func_impl = meta['func']
  return_type = m['return_type']
  return_from_native = utils.fromNative(return_type)
  return_from_intf = gen_from_native_interface(return_type)
  if meta and 'return_from_native' in meta:
    return_from_native = meta['return_from_native']
    return_from_intf = ''
  param_list = []
%>
static JSValue ${call_name}(JSContext* ctx, NativeHandle self, int argc, JSValueConst* argv) {
%if 'params' in m:
%for p in m['params']:
<%
  p_meta = ('meta' in p and p['meta'] or None)
  p_type = p['type']
  p_to_native = (p_meta and 'to_native' in p_meta) and p_meta['to_native'] or utils.toNative(p_type)
  p_free_native = (p_meta and 'free_native' in p_meta) and p_meta['free_native'] or utils.freeNative(p_type)
  idx = len(param_list)
  arg_name = '__native_arg%d__' % idx
  param_list.append((arg_name, idx, p_to_native, p_free_native, p['name']))
%>
  ${utils.cppType(p_type)} ${arg_name};
%endfor
%endif
%if return_type != 'void':
  JSValue __ret__ = JS_UNDEFINED;
%endif

%if param_pack:
  int __need_unpack = (argc == 1 && JS_IsObject(argv[0]));
%endif
%for p in param_list:
  <%
    if param_pack:
      param_value = '__need_unpack ? JS_GetPropertyStr(ctx, argv[0], "%s") : argv[%d]' % (p[4], p[1])
    else:
      param_value = 'argv[%d]' % p[1]
  %>
  if (${p[2]}(ctx, &${p[0]}, ${param_value}) != 0) {
    goto failed_args${p[1]};
  }
%endfor
%if return_type != 'void':
  ${utils.cppType(return_type)} __native_ret__ =
%endif
  <%
    self_str = need_self and "(%s)self" % native_type or ""
    need_comma = (need_self and len(param_list) > 0) and "," or ""
    params_str = ",".join([p[0] for p in param_list])
  %>  
  ${func_impl}(${self_str} ${need_comma} ${params_str});

%if return_type != 'void':
%if utils.isStructType(return_type):
  __ret__ = ${return_from_native}(ctx, &__native_ret__${return_from_intf});
%else:
  __ret__ = ${return_from_native}(ctx, __native_ret__${return_from_intf});
%endif
%endif

%if len(param_list) > 0:
%for p in param_list[::-1]:
failed_args${p[1]}:
%if len(p) > 3 and p[3]:
  ${p[3]}(ctx, ${p[0]});
%endif
%endfor
%endif
%if return_type != 'void':
  return __ret__;
%else:
  return JS_UNDEFINED;
%endif
}
%endif

static JSMetaMethod ${func_name} = {
  "${m['identifier']}",
  ${aliase_name},
  ${pname}_member_index_${m['identifier']},
  JM_METHOD,
  ${utils.getMethodFlags(m)},
  ${call_name}
};
<%
if func_name:
  member_defines.append(func_name)
%>
</%def>

<%def name='gen_const_content(pname, m, idx)'>
<%
  const_value = utils.getConstValue(m)
  value_type = utils.getConstValueType(m)
  init_func = utils.getConstInitFunc(m)
  js_flags = '0'
  js_type = 'JV_INT'
  val_name = '.ival'
  if init_func:
    const_value = '%s_%s_init' % (pname, utils.toIdName(m['name']))
    js_flags = 'JF_USE_INIT'
    val_name = '.ptr'
  else:
    if value_type == 'init':
      val_name = '.ival'
      js_type = 'JV_INT'
    elif value_type == 'float':
      js_type = 'JV_FLOAT'
      val_name = '.fval'
    elif value_type == 'bool':
      js_type = 'JV_BOOLEAN'
      val_name = '.ival'
    elif value_type == 'string':
      js_type = 'JV_STRING'
      val_name = '.sval'
%>
  "${m['name']}",
  NULL, /* aliase_name */
  ${idx},
  JM_CONST,
  ${js_flags},
  ${js_type},
  {${val_name} = ${const_value}}
</%def>

<%def name='gen_const_init(pname, m)'>
<%
  value_type_map = {
    'float': 'double',
    'bool' : 'int',
    'string' : 'const char*'
  }
  init_func = utils.getConstInitFunc(m)
  if init_func:
    value_type = utils.getConstValueType(m)
    if value_type in value_type_map:
      value_type = value_type_map[value_type]
%>
%if init_func:
static ${value_type} ${pname}_${utils.toIdName(m['name'])}_init() {
  return (${value_type})${init_func};
}
%endif
</%def>

<%def name='gen_const(pname, m, member_defines)'>
<%
  const_name = '_%s_%s_const' % (pname,  m['name'])
  member_defines.append(const_name)
%>
${gen_const_init(pname, m)}
static JSMetaConst ${const_name} = {
${gen_const_content(pname, m, "%s_member_index_%s" % (pname, m['name']))}
};
</%def>

<%def name='gen_enum(pname, m, member_defines)'>
<%
  enum_name = '_%s_%s_enum' % (pname, m['name'])
  enum_member_name = '%s_members' % enum_name
  member_defines.append(enum_name)
  prefix_name = '%s_%s' % (pname, m['name'])
%>
%for mb in m['members']:
${gen_const_init(prefix_name, mb)}
%endfor
static JSMetaConst ${enum_member_name}[] = {
%for mb in m['members']:
{${gen_const_content(prefix_name, mb, 0)}},
%endfor
};
static JSMetaEnum ${enum_name} = {
  "${m['name']}",
  NULL, /* aliase_name */
  ${pname}_member_index_${m['name']},
  JM_ENUM,
  0,
  sizeof(${enum_member_name}) / sizeof(${enum_member_name}[0]),
  ${enum_member_name}
};
</%def>

<%def name='gen_interface(pname, inf, member_defines, is_mod)'>
<%
  meta = None
  if 'meta' in inf:
    meta = inf['meta']

  inf_name = utils.toIdName(inf['name'])
  tag = inf_name
  name = is_mod and inf_name or '%s_%s' % (pname, inf_name)
  creator = None
  need_parent = False
  need_context = False
  native_type = utils.getNativeType(inf)
  if not native_type:
    native_type = 'NativeHandle'

  finalizer = utils.findTypeMeta('interface', inf['name'], 'finalizer', True)
  native_class = None

  if meta:
    if 'tag' in meta:
      tag = meta['tag']
    if 'create' in meta:
      creator = meta['create']
    need_parent = utils.findTypeMeta('interface', inf['name'], 'need_parent', True) == 'true'
    need_context = utils.findTypeMeta('interface', inf['name'], 'need_context', False) == 'true'
    if 'native_class' in meta:
      native_class = meta['native_class']

  members = inf['members']

  extends = 'NULL'

  if not is_mod and 'extends' in inf and len(inf['extends']) > 0:
    extends = '&_%s_%s_class' % (pname, utils.toIdName(inf['extends'][0]))

  call_params = []
  if need_context:
    call_params.append('ctx')
  if need_parent:
    call_params.append('(%s)parent' % native_type)

  class_name = '_%s_class' % name

  if not is_mod:
    member_defines.append(class_name)

  inf_members = []
%>

%if creator:
static NativeHandle _${name}_create(JSContext* ctx, NativeHandle parent) {
  return (NativeHandle)(${creator}(${','.join(call_params)}));
}
%endif

%if finalizer:
static void _${name}_finalizer(NativeHandle self) {
  ${finalizer}((${native_type})self);
}
%endif

enum {
%for m in members:
  ${name}_member_index_${'name' in m and m['name'] or m['identifier'] },
%endfor
  ${name}_member_max,
};

%for m in members:
%if m['type'] == 'property':
${gen_property(name, native_type, m, inf_members)}
%elif m['type'] == 'function':
${gen_method(name, native_type, m, inf_members)}
%elif m['type'] == 'enum':
${gen_enum(name, m, inf_members)}
%elif m['type'] == 'const':
${gen_const(name, m, inf_members)}
%elif m['type'] == 'interface':
${gen_interface(name, m, inf_members, False)}
%endif
%endfor

static JSMetaMember* ${class_name}_members[] = {
%for m in inf_members:
  (JSMetaMember*)&${m},
%endfor
};

%if native_class:
extern const lv_obj_class_t ${native_class};
%endif

${not is_mod and 'static ' or ''}JSMetaInterface ${class_name} = {
  "${inf_name}",
  NULL, /* aliase_name */
  ${pname}_member_index_${inf_name},
  ${is_mod and 'JM_MODULE' or 'JM_INTERFACE'},
  0,
  ${len(inf_members)},
  "${tag}",
  ${extends},
  ${native_class and '&' + native_class or 'NULL'},
  ${not creator and 'NULL' or '_%s_create' % name},
  ${not finalizer and 'NULL' or '_%s_finalizer' % name},
  ${class_name}_members
};
</%def>

enum {
  _member_index_${module_name}
};

%for m in doc['members']:
%if m['type'] == 'struct':
${gen_struct_define(m)}
%endif
%endfor

${gen_interface('', doc, [], True)}
