// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module = render.module
  module_name = render.GetModuleName()
%>\
<%def name="GenFunctionDefine(func_node)">\
<%
  identifier = func_node['identifier']
  ret_type = func_node['return_type']
  render.CacheFuncReturnNode(identifier, ret_type)
%>\
  ${render.GenerateFunctionDefine(func_node)}
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
export declare class ${struct_name} {
%for member in struct_node['members']:
${GenStructMember(member)}\
%endfor
}

</%def>\
<%def name="GenPropertyDefines(prop_node)">\
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
    params += render.GenerateParamList(func_node["params"])

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
%endif
%endfor

export class ${module_name} {
  constructor(){
    this.init_native(this.clazz_name);
  }
%for block in module['members']:
%if block['type'] == 'function':
${GenFunctionDefine(block)}\
%elif block['type'] == 'use':
${GenUse(block)}\
%elif block['type'] == 'callback':
${CacheCallback(block)}\
%elif block['type'] == 'property':
${GenPropertyDefines(block)}\
%endif
%endfor

// private:
  readonly clazz_name = "${module_name}";
  declare init_native(name: string): void;
}