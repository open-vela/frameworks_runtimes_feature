// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module = render.module
  module_name = render.GetModuleName()
%>\
<%def name="GenFunctionDefine(func_node)">\
  ${render.GenerateFunctionDefine(func_node)}
</%def>\

module ${module_name};

export declare class ${module_name} {
%for block in module['members']:
%if block['type'] == 'function':
${GenFunctionDefine(block)}\
%endif
%endfor
}