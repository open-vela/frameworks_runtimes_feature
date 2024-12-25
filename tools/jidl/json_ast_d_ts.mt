// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module = render.module
  raw_module_name = render.GetRawModuleName()
  module_name = raw_module_name.replace('.', '_')
%>\
<%def name="GenInterfaceCtorFunction(func_node, ctor_info)">\
<%
  identifier = func_node['identifier']
  func_def = render.GenerateFunctionDefine(func_node)

  params = ''
  param_names = ''
  if 'params' in func_node:
    params = render.GenerateParamsStr(func_node["params"])
    param_names = render.GenerateParamNameList(func_node["params"])
  func_head = f"{identifier}({params})"

  ctor_target = ctor_info['target']
  ctor_interface = ctor_info['interface']
%>\
  ${func_def} {
    let instance = this._${identifier}(${param_names});
    let ${ctor_target} = new ${ctor_interface}();
    ${ctor_target}.instance = instance;
    return ${ctor_target};
  }
  declare _${func_head}: number;
</%def>\
<%def name="GenFunction(func_node)">\
<%
  identifier = func_node['identifier']
  ret_type = func_node['return_type']
  render.CacheFuncReturnNode(identifier, ret_type)
  # for interface constructor function
  ctor_info = render.GetInterfaceCtorInfo(func_node)
%>\
%if ctor_info:
  /****** for JIDL Interface constructor function '${identifier}' ******/
${GenInterfaceCtorFunction(func_node, ctor_info)}
%else:
  declare ${render.GenerateFunctionDefine(func_node)};
%endif
</%def>\
<%def name="CacheCallback(cb_node)">\
<%
  render.TryCacheCallback(cb_node)
%>\
</%def>\
<%def name="GenStructMember(member_node)">\
<%
  member_name = member_node['name']
  member_type = member_node['type']
  ts_type = render.GenerateTsType(member_type)
  member_def = f"{member_name}: {ts_type}"
%>\
  ${member_def};
</%def>\
<%def name="GenStructDefine(struct_node)">\
<%
  struct_name = struct_node['name']
%>\
export class ${struct_name} {
%for member in struct_node['members']:
${GenStructMember(member)}\
%endfor
}

</%def>\
<%def name="GenInterfaceParentMember(iname, member)">\
<%
  type = member['type'] # 0 for Method, 1 for property
  if type == 0:
    method_def = member['method_def']
  elif type == 1:
    prop_name = member['prop_name']
    prop_type = member['prop_type']
    has_getter = member['has_getter']
    has_setter = member['has_setter']
%>\
%if type == 0:
  ${method_def}
%elif type == 1:
%if has_getter:
  get ${prop_name}(): ${prop_type} {
    return this.get_${prop_name}_0();
  }
  declare get_${prop_name}_0(): ${prop_type};
%endif
%if has_setter:
  set ${prop_name}(v: ${prop_type}) {
    this.set_${prop_name}_0(v);
  }
  declare set_${prop_name}_0(v: ${prop_type}): void;
%endif
%endif

</%def>\
<%def name="GenInterfaceClassMember(parent_name, member_node)">\
<%
  member_type = member_node['type']
  if member_type == 'function':
    identifier = member_node['identifier']
    method_def = render.GenerateFunctionDefine(member_node)
    method_def = f"declare {method_def};"
    member_info = {
      'type': 0,
      'method_def': method_def
    }
    render.CacheInterfaceMember(parent_name, member_info)
  elif member_type == 'property':
    prop_name = member_node["name"]
    prop_type = render.GenerateTsType(member_node["value_type"])
    has_getter = render.PropertyHasGetter(member_node)
    has_setter = render.PropertyHasSetter(member_node)
    member_info = {
      'type': 1,
      'prop_name': prop_name,
      'prop_type': prop_type,
      'has_getter': has_getter,
      'has_setter': has_setter
    }
    render.CacheInterfaceMember(parent_name, member_info)
%>\
%if member_type == 'function':
  ${method_def}
%elif member_type == 'property':
%if has_getter:
  get ${prop_name}(): ${prop_type} {
    return this.get_${prop_name}_0();
  }
  declare get_${prop_name}_0(): ${prop_type};
%endif
%if has_setter:
  set ${prop_name}(v: ${prop_type}) {
    this.set_${prop_name}_0(v);
  }
  declare set_${prop_name}_0(v: ${prop_type}): void;
%endif
%endif

</%def>\
<%def name="GenInterfaceClass(i_node)">\
<%
  i_name = i_node['name']
  implements = f"implements _{i_name}"
  parent_members = render.GetFinalInterfaceMembers(i_name)
%>\
export class ${i_name} {
  private instance: number;
  constructor() {
    this.init_native(${i_name}.clazz_name);
  }

  // parent member defines
%for p_member in parent_members:
${GenInterfaceParentMember(i_name, p_member)}\
%endfor
  // self member defines
%for member in i_node['members']:
${GenInterfaceClassMember(i_name, member)}\
%endfor

  static readonly clazz_name = "${i_name}";
  declare init_native(i_name: string): void;
}

</%def>\
<%def name="GenInterface(i_node)">\
<%
  iname = i_node['name']
  for extend in i_node['extends']:
    render.CacheInterfaceExtend(iname, extend)
%>\
${GenInterfaceClass(i_node)}\
</%def>\
<%def name="GenProperty(prop_node)">\
<%
  prop_name = prop_node["name"]
  prop_type = prop_node["value_type"]
  ts_type = render.GenerateTsType(prop_type)
  has_getter = render.PropertyHasGetter(prop_node)
  has_setter = render.PropertyHasSetter(prop_node)
%>\
%if has_getter:
  get ${prop_name}(): ${ts_type} {
    return this.get_${prop_name}_0();
  }
  declare get_${prop_name}_0(): ${ts_type};
%endif
%if has_setter:
  set ${prop_name}(v: ${ts_type}) {
    this.set_${prop_name}_0(v);
  }
  declare set_${prop_name}_0(v: ${ts_type}): void;
%endif
</%def>\
<%def name="GenConst(const_node)">\
<%
  const_name = const_node["name"]
  const_val = const_node["value"]
  const_type = const_node["value_type"]
  ts_type = render.GenerateTsType(const_type)
%>\
  static const ${const_name}: ${ts_type} = ${const_val};
</%def>\
<%def name="GenUse(use_node)">\
<%
  func_node = use_node['function']
  identifier = func_node['identifier']
  func_call_node = use_node['function_call']
  func_call_id = func_call_node['identifier']
  ret_node = render.GetUseReturnNode(func_call_id)
  ret_type = render.GenerateTsType(ret_node)
  params = ''
  if 'params' in func_node:
    params += render.GenerateParamsStr(func_node["params"])

  func_call = ''
  if ret_type != 'void':
    func_call += 'return '
  func_call += f"this.{func_call_id} ("

  if 'param_calls' in func_call_node:
    params_calls = render.GenerateParamCallList(func_call_node["param_calls"])
    func_call += f"{params_calls}"
  func_call += ")"
%>\
  ${identifier} (${params}): ${ret_type} {
    ${func_call};
  }
</%def>\

%for block in module['members']:
%if block['type'] == 'struct':
${GenStructDefine(block)}\
%elif block['type'] == 'interface':
${GenInterface(block)}\
%endif
%endfor

export class ${module_name} {
  private instance: number;
  constructor(){
    this.init_native(${module_name}.clazz_name);
  }
%for block in module['members']:
%if block['type'] == 'function':
${GenFunction(block)}\
%elif block['type'] == 'use':
${GenUse(block)}\
%elif block['type'] == 'callback':
${CacheCallback(block)}\
%elif block['type'] == 'property':
${GenProperty(block)}\
%elif block['type'] == 'const':
${GenConst(block)}\
%endif
%endfor

// private:
  static readonly clazz_name = "${raw_module_name}";
  declare init_native(name: string): void;
}