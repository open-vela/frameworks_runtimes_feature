// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module = render.module
  module_name = render.GetModuleName()
%>\
<%def name="GenFunctionDefine(func_node)">\
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
declare class ${struct_name} {
%for member in struct_node['members']:
${GenStructMember(member)}\
%endfor
}

</%def>\

module ${module_name};

%for block in module['members']:
%if block['type'] == 'struct':
${GenStructDefine(block)}\
%endif
%endfor

export declare class ${module_name} {
  constructor(){
    this.init_native(this.clazz_name);
  }
%for block in module['members']:
%if block['type'] == 'function':
${GenFunctionDefine(block)}\
%elif block['type'] == 'callback':
${CacheCallback(block)}\
%endif
%endfor

// private:
  readonly clazz_name = "${module_name}";
  declare init_native(name: string): void;
}