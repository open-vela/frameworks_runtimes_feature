// Copyright 2023 Xiaomi, Inc. All rights reserved.

<%
  module = render.module
  module_name = render.GetModuleName()
  header_name = render.GetHeaderFileName()
%>\
<%def name="GenOptionalType(name, feature_type, value)">\
<% val_name = render.GetOptValName(feature_type) %>\
  static OptionalType ${module_name}_${name}_opt_type = {
    .header = { .type = COMPLEX_OPTIONAL, .size = sizeof(OptionalType) },
    .type = ${feature_type},
    .${val_name} = ${value}
  };

</%def>\
<%def name="GenStructMemberItems(members, struct_name, member_items)">\
<%
  for member in members:
    member_item = {}
    member_item['name'] = member['name']
    member_type = member['type']
    cpp_type = render.GenerateCppType(member_type)
    if cpp_type == 'FtArray':
      cpp_type += '*'
    member_item['cpp_type'] = cpp_type

    m_info = render.GenerateFeatureInfo(member_type)
    ft_expr = render.GenerateFtExpression(m_info)
    if 'default' in member:
      if m_info['is_complex'] or m_info['is_complex']:
        raise Exception('wrong default struct member type: {}'.format(m_info['type']))
      m_name = struct_name + '_member_' + member['name']
      m_default = member['default']
      GenOptionalType(m_name, ft_expr, m_default)
      member_item['feature_type'] = f"FT_MK_OPTIONAL(&{module_name}_{m_name}_opt_type)"
    else:
      member_item['feature_type'] = ft_expr
    member_items.append(member_item)
%>\
</%def>\
<%def name="GenStruct(struct_node)">\
<%
  struct_name = struct_node['name']
  render.CacheStructName(struct_name)
  for member in struct_node['members']:
    if render.IsStruct(member['type']):
      GenStruct(member['type'])
  member_items = []
  GenStructMemberItems(struct_node['members'], struct_name, member_items)
%>\
  /****** for JIDL struct '${struct_name}' ******/
  static ObjectMember ${module_name}_${struct_name}_struct_members[] = {
%for member_item in member_items:
<%
  member_name = member_item['name']
  member_ft = member_item['feature_type']
  cpp_type = member_item['cpp_type']
%>\
    { "${member_name}", ${member_ft}, offsetof(${module_name}_${struct_name}, _${member_name}), sizeof(${cpp_type}) },
%endfor
    { nullptr },
  };

  // complex defination
  static const ObjectMapType ${module_name}_${struct_name}_struct_type {
    .header = { .type = COMPLEX_STRUCT_MAP, .size = sizeof(${module_name}_${struct_name}) },
    .members = ${module_name}_${struct_name}_struct_members
  };

  ${module_name}_${struct_name}* malloc${struct_name} () {
    return (${module_name}_${struct_name}*)FeatureMalloc(
      sizeof(${module_name}_${struct_name}), FT_MK_COMPLEX(&${module_name}_${struct_name}_struct_type));
  }

</%def>\

<%def name="GenInterface(interface_node)">\
<%
  iname = interface_node['name']
  render.TryCacheInterface(iname)
  members = interface_node['members']
  parent_prefix = f"{iname}_interface_"
%>\
  /****** JIDL interface '${iname}' glue code begin ******/
extern const InterfaceType ${module_name}_${parent_prefix}type;
<%
  for member in members:
    if member['type'] == 'function':
      GenFunction(member, parent_prefix)
    elif member['type'] == 'property':
      GenProperty(member, parent_prefix)
    else:
      raise Exception('wrong interface member type: {}'.format(member))
  endfor
%>\
  // Interface members
  static const Member ${module_name}_${parent_prefix}members[] = {
${GenMembers(members, parent_prefix)}\
  };

  // Interface description
  static const FeatureDescription ${module_name}_${parent_prefix}desc = {
    .version = 1,
    .name = "${iname}",
    .description = "${iname} description",
    { .dynamic = true },
    nullptr,
    countof(${module_name}_${parent_prefix}members),
    ${module_name}_${parent_prefix}members,
  };

  // InterfaceType
  const InterfaceType ${module_name}_${parent_prefix}type {
    .header = { .type = COMPLEX_INTERFACE, .size = 0 },
    .desc = &${module_name}_${parent_prefix}desc
  };
  /****** JIDL interface '${iname}' glue code end ******/
</%def>\

<%def name="GenerateArrayType(array_type, is_complex)">\
  static const ArrayType ${module_name}_${array_type}_array = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
%if is_complex:
    .element_type = FT_MK_COMPLEX(&${array_type})
%else:
<% ft_type = render.ToBaseFeatureType(array_type) %>\
    .element_type = ${ft_type}
%endif
  };

  FtArray* ${module_name}_malloc_${array_type}_array() {
    return (FtArray*)FeatureMalloc(
      sizeof(FtArray), FT_MK_COMPLEX(&${module_name}_${array_type}_array));
  }

</%def>\
<%!
  class ArrayTypeGenerator:
    def __init__(self, gen_func):
      self.gen_func = gen_func

    def Generate(self, array_type, is_complex):
      self.gen_func(array_type, is_complex)
%>\
<%
  render.SetArrayTypeGenerator(ArrayTypeGenerator(GenerateArrayType))
%>\
<%def name="GenParamsFeatureType(node, parent_prefix = '')">\
<%
  identifier = node['identifier']
  has_ellipse_param = render.HasEllipseParam(node)
  param_infos = []
  if 'params' in node:
    for param in node['params']:
      p_info = render.GenerateFeatureInfo(param["type"])
      ft_expr = render.GenerateFtExpression(p_info)
      if 'name' in param and 'default' in param:
        if p_info['is_complex_ref'] or p_info['is_complex']:
          raise Exception('wrong default param type: {}'.format(p_info['type']))
        p_name = identifier + '_param_' + param["name"]
        p_default = param["default"]
        GenOptionalType(p_name, ft_expr, p_default)
        p_info['type'] = f"FT_MK_OPTIONAL(&{module_name}_{p_name}_opt_type)"
      else:
        p_info['type'] = ft_expr

      param_infos.append(p_info)
%>\
  static const FeatureType ${module_name}_${parent_prefix}${identifier}_parameters[] = {
%for param_info in param_infos:
    ${param_info['type']},
%endfor
%if not has_ellipse_param:
    FT_PARAM_END
%endif
  };
</%def>\
<%def name="GenPromiseType(promise_type, promise_ft)">\
<%
  resolve_type = promise_type['resolve_type']
  reject_type = promise_type['reject_type']
  resolve_info = render.GenerateFeatureInfo(resolve_type)
  resolve_ft_expr = render.GenerateFtExpression(resolve_info)
  reject_info = render.GenerateFeatureInfo(reject_type)
  reject_ft_expr = render.GenerateFtExpression(reject_info)
  resolve_ft = resolve_info['type']
  reject_ft = reject_info['type']
  p_feature_type = f"{module_name}_promise_{resolve_ft}_{reject_ft}_type"
  promise_ft['feature_type'] = p_feature_type
  success = render.TryCachePromiseType(p_feature_type)
%>\
%if success:
  static const PromiseType ${p_feature_type} = {
    .header = { .type = COMPLEX_PROMISE, .size = sizeof(FtPromiseId) },
    .resolveTypes = { ${resolve_ft_expr}, ${reject_ft_expr} }
  };

%endif
</%def>\
<%def name="GenMemberMethod(identifier, ret_type, parent_prefix = '', index = -1)">\
<%
  if isinstance(ret_type, dict) and ret_type['type'] == 'promise':
    p_ft = {}
    GenPromiseType(ret_type, p_ft)
    ret_ft = p_ft['feature_type']
    ret_ft = f"FT_MK_COMPLEX_REF(&{ret_ft})"
  else:
    ret_info = render.GenerateFeatureInfo(ret_type)
    ret_ft = render.GenerateFtExpression(ret_info)
  if index >= 0:
    func_info = f".vtable_idx = {index}"
  else:
    func_info = f".callback = FFI_FN({module_name}_wrap_{identifier})"
%>\
  static const MemberMethod ${module_name}_${parent_prefix}${identifier}_member_method = {
    .func = { ${func_info} },
    .parameters = ${module_name}_${parent_prefix}${identifier}_parameters,
    .return_type = ${ret_ft},
  };
</%def>\
<%def name="GenInterfaceCtorFunction(func_node, ctor_info)">\
<%
  identifier = func_node['identifier']
  func_def = render.GenerateFunctionDefine(func_node)
  ctor_target = ctor_info['target']
  ctor_interface = ctor_info['interface']
  parent_prefix = f"{ctor_interface}_interface_"
  vtable = render.GetVTable(parent_prefix)
%>\
  /****** for JIDL Interface constructor function '${identifier}' ******/
static ${func_def} {
    static NativeFunc ${ctor_target}_vtable[] = {
        nullptr,
%for vtable_item in vtable:
<%
  item_name = vtable_item['name']
  item_type = vtable_item['type']
  item_content = f"{module_name}_{parent_prefix}{ctor_target}"
  if item_type == 0:
    item_content = f"{item_content}_{item_name}"
  elif item_type == 1:
    item_content = f"{item_content}_get_{item_name}"
  elif item_type == 2:
    item_content = f"{item_content}_set_{item_name}"
  if item_content != '':
    item_content = f"NativeFunc({item_content})"
%>\
        ${item_content},
%endfor
    };
    return FeatureCreateInterface(feature, ${ctor_target}_vtable, countof(${ctor_target}_vtable));
}
</%def>\
<%def name="GenFunction(func_node, parent_prefix = '')">\
<%
  identifier = func_node['identifier']
  ret_type = func_node['return_type']
  ctor_info = {}
  index = -1
  if parent_prefix != '':
    # for interface member function
    index = render.CacheVTableItem(parent_prefix, func_node, 0) + 1
  else:
    render.CacheFuncReturnNode(identifier, ret_type)
    # for interface constructor function
    ctor_info = render.GetInterfaceCtorInfo(func_node)
%>\
  /****** for JIDL function '${parent_prefix}${identifier}' ******/
%if ctor_info:
${GenInterfaceCtorFunction(func_node, ctor_info)}
%endif
${GenParamsFeatureType(func_node, parent_prefix)}
${GenMemberMethod(identifier, ret_type, parent_prefix, index)}
</%def>\
<%def name="GenUse(use_node)">\
<%
  func_node = use_node['function']
  identifier = func_node['identifier']
  func_call_node = use_node['function_call']
  func_call_id = func_call_node['identifier']
  ret_type_node = render.GetUseReturnTypeNode(use_node)
  ret_type = render.GenerateReturnType(ret_type_node)
  params = ''
  if 'params' in func_node:
    param_list = render.GenerateParamList(func_node["params"])
    params += f", {param_list}"

  is_promise = (ret_type == 'FtPromiseId')
  if is_promise:
    ret_type = 'void'

  func_call = ''
  if ret_type != 'void':
    func_call += 'return '
  func_call += f"{module_name}_wrap_{func_call_id} (feature, data"
  if is_promise:
    func_call += ', pid'

  if 'param_calls' in func_call_node:
    params_call_list = render.GenerateParamCallList(func_call_node["param_calls"])
    func_call += f", {params_call_list}"
  func_call += ")"

  prefix_params = 'FeatureInstanceHandle feature, AppendData data'
  if is_promise:
    prefix_params += ', FtPromiseId pid'
%>\
  /****** for JIDL use '${identifier}' ******/
  static ${ret_type} ${module_name}_wrap_${identifier} (${prefix_params}${params}) {
    ${func_call};
  }

${GenParamsFeatureType(func_node)}
${GenMemberMethod(identifier, ret_type_node)}
</%def>\
<%def name="GenCallback(cb_node)">\
<%
  identifier = cb_node['identifier']
  success = render.TryCacheCallbackId(identifier)
%>\
%if success:
  /****** for JIDL callback '${identifier}' ******/
${GenParamsFeatureType(cb_node)}
  static const CallbackType ${module_name}_${identifier}_callback_type {
    .header = { .type = COMPLEX_CALLBACK, .size = sizeof(FtCallbackId) },
    .parameters = ${module_name}_${identifier}_parameters,
    .return_type = FT_VOID
  };

%endif
</%def>\

<%def name="GenProperty(prop_node, parent_prefix = '')">\
<%
  prop_name = prop_node['name']
  value_type = prop_node['value_type']
  prop_info = render.GenerateFeatureInfo(value_type)
  prop_ft = render.GenerateFtExpression(prop_info)
  has_getter = render.PropertyHasGetter(prop_node)
  has_setter = render.PropertyHasSetter(prop_node)
  getter_info = ''
  if has_getter:
    if parent_prefix != '':
      index = render.CacheVTableItem(parent_prefix, prop_node, 1) + 1
      getter_info = f".vtable_idx = {index}"
    else:
      getter_info = f".callback = FFI_FN({module_name}_get_{prop_name})"
  setter_info = ''
  if has_setter:
    if parent_prefix != '':
      index = render.CacheVTableItem(parent_prefix, prop_node, 2) + 1
      setter_info = f".vtable_idx = {index}"
    else:
      setter_info = f".callback = FFI_FN({module_name}_set_{prop_name})"
%>\
  /****** for JIDL property '${parent_prefix}${prop_name}' ******/
  static const MemberAccessor ${module_name}_${parent_prefix}${prop_name}_member_accessor = {
%if has_getter:
    .getter = { ${getter_info} },
%endif
%if has_setter:
    .setter = { ${setter_info} },
%endif
    .type = ${prop_ft},
  };

</%def>\
<%def name="GenConst(const_node)">\
<%
  const_name = const_node['name']
  const_value = const_node['value']
  const_type = const_node['value_type']
  const_info = render.GenerateFeatureInfo(const_type)
  if const_info['is_complex'] or const_info['is_complex_ref']:
    raise Exception('wrong const type: {}'.format(const_info['type']))
  val_name = render.GetAppendDataName(const_info['type'])
  cpp_type = render.GenerateCppType(const_type)
  is_array = cpp_type == 'FtArray'
  if is_array:
    cpp_type = render.GenerateArrayCppType(const_type)
  if cpp_type != 'FtString':
    cpp_type = 'const ' + cpp_type
  const_def = f"{cpp_type} {module_name}_g_const_{const_name}"
  if is_array:
    const_def += f"[] = {const_value}"
    const_func_def = f"{cpp_type}[] "
  else:
    const_def += f" = {const_value}"
    const_func_def = f"{cpp_type} "
  const_func_def += f"{module_name}_init_const_{const_name}(FeatureInstanceHandle feature, AppendData data) {{ return {module_name}_g_const_{const_name}; }}"
%>\
  /****** for JIDL const '${const_name}' ******/
  ${const_def};
  ${const_func_def};

  static const MemberConst ${module_name}_${const_name}_member_const = {
    .type = ${const_info['type']},
    //.func = { .callback = FFI_FN(${module_name}_init_const_${const_name}) },
    .func = { .callback = nullptr },
    .data = { .${val_name} = ${module_name}_g_const_${const_name} }
  };
</%def>\
<%def name="GenMembers(members, parent_prefix)">\
%for member in members:
<%
  member_info = render.GetMemberInfo(member)
  member_type = member_info['type']
  member_name = member_info['name']
  member_suffix = member_info['suffix']
  member_val_type = member_info['val_type']
  is_valid_member = True
  if member_type == 'MEMBER_NULL':
    is_valid_member = False
%>\
%if is_valid_member:
    {
      .type = ${member_type},
      .name = "${member_name}",
      .${member_val_type} = ${module_name}_${parent_prefix}${member_name}${member_suffix},
    },
%endif
%endfor
</%def>\
#include "${header_name}"
#include "ajs_features_init.h"
#include "feature_description.h"

#define countof(x) (sizeof(x) / sizeof(x[0]))

%for block in module['members']:
%if block['type'] == 'function':
${GenFunction(block)}
%elif block['type'] == 'use':
${GenUse(block)}
%elif block['type'] == 'callback':
${GenCallback(block)}
%elif block['type'] == 'property':
${GenProperty(block)}
%elif block['type'] == 'const':
${GenConst(block)}
%elif block['type'] == 'struct':
${GenStruct(block)}
%elif block['type'] == 'interface':
${GenInterface(block)}
%endif
%endfor
  // members
  static const Member ${module_name}_members[] = {
${GenMembers(module['members'], '')}\
  };

  // callbacks
  static const struct FeatureCallbacks ${module_name}_callbacks {
    ${module_name}_onRegister,
    ${module_name}_onCreate,
    ${module_name}_onRequired,
    ${module_name}_onDetached,
    ${module_name}_onDestroy,
    ${module_name}_onUnregister
  };

  static const FeatureDescription ${module_name}_desc = {
    .version = 1,
    .name = "${module_name}",
    .description = "${module_name}",
    { .dynamic = false },
    .native_callbacks = &${module_name}_callbacks,
    .member_count = countof(${module_name}_members),
    .members = ${module_name}_members,
  };

QAPPFEATURE_INIT(${module_name})
{
    return mgr->registerFeature(features, &${module_name}_desc);
}