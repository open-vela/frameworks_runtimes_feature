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

module ${module_name};

export declare class ${module_name} {
%for block in module['members']:
%if block['type'] == 'function':
${GenFunctionDefine(block)}\
%elif block['type'] == 'callback':
${CacheCallback(block)}\
%endif
%endfor
}