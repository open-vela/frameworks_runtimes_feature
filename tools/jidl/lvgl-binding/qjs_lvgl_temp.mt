// Copyright 2023 Xiaomi, Inc. All rights reserved.

%for include in vars['includes']:
#include "${include}"
%endfor

<% module_name = utils.toIdName(doc['name']) %>

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
  if 'meta' in m:
    meta = m['meta']
  getter = 'NULL'
  setter = 'NULL'
  if 'readable' in m and  m['readable']:
    getter = '_%s_get_%s' % (pname, m['name'])
  if 'writeable' in m and m['writeable']:
    setter = '_%s_set_%s' % (pname, m['name'])

%>

%if 'type' in meta:
  %if meta['type'] == 'event':
  ${gen_property_event(p, native_type, m, meta, setter)}
  %elif meta['type'] == 'id':
  %endif

%else:

<%
  value_type = utils.getValueType(m)
%>

%if 'readable' in m and  m['readable']:
static JSValue ${getter}(JSContext* ctx, NativeHandle self) {
  return ${utils.fromNative(value_type)}(ctx, ${meta['get']}((${native_type})self));
}
%endif

%if 'writeable' in m and m['writeable']:
static void ${setter}(JSContext* ctx, NativeHandle self, JSValueConst val) {
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
}

%endif
%endif

<% prop_name = '_%s_%s_property' % (pname, m['name']) %>
static JSMetaProperty ${prop_name} = {
  "${m['name']}",
  JM_PROPERTY,
  ${('readable' in m and m['readable']) and '1' or '0'},
  ${('writeable' in m and m['writeable']) and '1' or '0'},
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
  if 'meta' in m:
    meta = m['meta']
  func_name = '_%s_%s_method' % (pname, m['identifier'])
  call_name = '_%s_%s' %(pname, m['identifier'])
%>

%if meta and 'func' in meta:
<%
  func_impl = meta['func']
  return_type = m['return_type']
  return_from_native = utils.fromNative(return_type)
  if meta and 'return_from_native' in meta:
    return_from_native = meta['return_from_native']
  param_list = []
%>
static JSValue ${call_name}(JSContext* ctx, NativeHandle self, int argc, JSValueConst* argv) {
%for p in m['params']:
<%
  p_meta = ('meta' in p and p['meta'] or None)
  p_type = p['type']
  p_to_native = (p_meta and 'to_native' in p_meta) and p_meta['to_native'] or utils.toNative(p_type)
  p_free_native = (p_meta and 'free_native' in p_meta) and p_meta['free_native'] or utils.freeNative(p_type)
  idx = len(param_list)
  arg_name = '__native_arg%d__' % idx
  param_list.append((arg_name, idx, p_to_native, p_free_native))
%>
  ${utils.cppType(p_type)} ${arg_name};
%endfor
%if return_type != 'void':
  JSValue __ret__ = JS_UNDEFINED;
%endif

%for p in param_list:
  if (${p[2]}(ctx, &${p[0]}, argv[${p[1]}]) != 0) {
    goto failed_args${p[1]};
  }
%endfor
%if return_type != 'void':
  ${utils.cppType(return_type)} __native_ret__ =
%endif
    ${func_impl}((${native_type})self, ${','.join([p[0] for p in param_list])});

%if return_type != 'void':
  __ret__ = ${return_from_native}(ctx, __native_ret__);
%endif

%if len(param_list) > 0:
%for p in param_list[::-1]:
failed_args${p[1]}:
%if len(p) > 3 and p[3]:
  ${p[3]}(ctx, ${p[0]});
%endif
%endfor
%if return_type != 'void':
  return __ret__;
%else:
  return JS_UNDEFINED;
%endif
%endif
}
%endif

static JSMetaMethod ${func_name} = {
  "${m['identifier']}",
  JM_METHOD,
  ${call_name}
};
<%
if func_name:
  member_defines.append(func_name)
%>
</%def>

<%def name='gen_const_content(m)'>
<%
  const_value = utils.getConstValue(m)
  value_type = utils.getConstValueType(m)
%>
  "${m['name']}",
  JM_CONST,
%if value_type == 'int':
  JV_INT,
  {.ival = ${const_value}}
%elif value_type == 'float':
  JV_FLOAT,
  {.fval = ${const_value}}
%elif value_type == 'bool':
  JV_BOOLEAN,
  {.ival = ${const_value}}
%elif value_type == 'string':
  JV_STRING,
  {.sval = ${const_value}}
%endif
</%def>

<%def name='gen_const(pname, m, member_defines)'>
<%
  const_name = '_%s_%s_const' % (pname,  m['name'])
  member_defines.append(const_name)
%>
static JSMetaConst ${const_name} = {
${gen_const_content(m)}
};
</%def>

<%def name='gen_enum(pname, m, member_defines)'>
<%
  enum_name = '_%s_%s_enum' % (pname, m['name'])
  enum_member_name = '%s_members' % enum_name
  member_defines.append(enum_name)
%>
static JSMetaConst ${enum_member_name}[] = {
%for mb in m['members']:
{${gen_const_content(mb)}},
%endfor
};
static JSMetaEnum ${enum_name} = {
  "${m['name']}",
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
  native_type = utils.getNativeType(inf)
  if not native_type:
    native_type = 'NativeHandle'

  finalizer = utils.findTypeMeta('interface', inf['name'], 'finalizer', True)

  if meta:
    if 'tag' in meta:
      tag = meta['tag']
    if 'create' in meta:
      creator = meta['create']
    need_parent = utils.findTypeMeta('interface', inf['name'], 'need_parent', True) == 'true'

  members = inf['members']

  extends = 'NULL'

  if not is_mod and 'extends' in inf and len(inf['extends']) > 0:
    extends = '&_%s_%s_class' % (pname, utils.toIdName(inf['extends'][0]))

  create_param = ''
  if need_parent:
    create_param = 'NativeHandle parent'

  class_name = '_%s_class' % name

  if not is_mod:
    member_defines.append(class_name)

  inf_members = []
%>

%if creator:
static NativeHandle _${name}_create(${create_param}) {
  return (NativeHandle)(${creator}(${need_parent and '(%s)parent' % native_type or ''}));
}
%endif

%if finalizer:
static void _${name}_finalizer(NativeHandle self) {
  ${finalizer}((${native_type})self);
}
%endif

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

${not is_mod and 'static ' or ''}JSMetaInterface ${class_name} = {
  "${inf_name}",
  ${is_mod and 'JM_MODULE' or 'JM_INTERFACE'},
  0,
  ${len(inf_members)},
  "${tag}",
  ${extends},
  ${not creator and 'NULL' or '_%s_create' % name},
  ${not finalizer and 'NULL' or '_%s_finalizer' % name},
  ${class_name}_members
};
</%def>

${gen_interface('', doc, [], True)}
