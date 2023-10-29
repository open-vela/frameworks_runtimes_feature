// Copyright 2023 Xiaomi, Inc. All rights reserved.
<%
  module_name = utils.getModuleName()
%>

<%def name='gen_param(param, prefix)'>
<%
  p_type = 'value_type' in param and param['value_type'] or param['type']
  p_name = param['name']
  key_name, to_key = utils.getMsgKey(param)
  if len(to_key) == 0:
    to_key = 'string_to_%s' % utils.cppType(p_type)
%>
%if utils.isStructType(p_type):
<%
  s = utils.getUserType('struct', p_type['referred_name'])
%>
  %if s:
    %for s_mb in s['members']:
     ${gen_param(s_mb, '')}
    %endfor
   %endif
%elif utils.isCallbackType(p_type):
  // ignore ${prefix}${p_name}
%else:
  sub_arg->${key_name} = ${to_key}(find_key_arg("${key_name}", argc, argv));
%endif
</%def>


<%def name='gen_method(inf, method)'>
<%
  meta = None
  func_name = None
  if 'meta' in method:
    meta = method['meta']
    msg_arg_id = 'MIOTMSG__TEST_ONEOF__NOT_SET'
    msg_sub_type = None
    cb_data_parser = 'NULL'
    if 'msg_sub_type' in meta:
      msg_sub_type = meta['msg_sub_type']
    if 'msg_arg_id' in meta:
      msg_arg_id = meta['msg_arg_id']
    if 'cb_data_parser' in meta:
      cb_data_parser = meta['cb_data_parser']
    msg_sub_type_id = meta['msg_sub_id']

  ret_type = method['return_type']
  is_promise = utils.isPromiseType(ret_type)

  return_type = utils.cppType(method['return_type'])
%>
%if meta:
int ${module_name}_${method['identifier']}_method_to_miotmsg(Miotmsg* msg, size_t msg_size, int argc, const char* argv[]) {
  %if msg_sub_type:
  ${msg_sub_type}* sub_arg = (${msg_sub_type}*)(msg->subarg);
  CHECK_SIZE(msg_size, sizeof(*msg) + sizeof(*sub_arg), "The ${module_name}_${method['identifier']} message size is too small");
  %else:
  CHECK_SIZE(msg_size, sizeof(*msg), "The ${module_name}_${method['identifier']} message size is too small");
  %endif
  miotmsg__init(msg);
  msg->type = MIOTMSG__MSGTYPE__INVOKE;
  msg->subtype = ${msg_sub_type_id};
  msg->test_oneof_case = ${msg_arg_id};

  %if msg_sub_type:
  ${msg_sub_type.lower()}__init(sub_arg);
  %endif
  %for param in method['params']:
  ${gen_param(param, '')}
  %endfor
  return 0;
}
%endif
</%def>

<%def name="gen_property(inf, prop)">
%if 'meta' in prop:
<%
  meta = prop['meta']
  msg_sub_type = meta['msg_sub_type']
  msg_sub_id = meta['msg_sub_id']
  msg_arg_id = meta['msg_arg_id']
  prop_name = prop['name']
  prop_type = prop['value_type']
  prop_native_type = utils.cppType(prop_type)
  is_callback = utils.isCallbackType(prop_type)
  return_type = is_callback and 'const char*' or 'int'
%>
%if 'writeable' in prop and prop['writeable']:
${return_type} ${module_name}_${prop_name}_${is_callback and 'event' or 'set'}_to_miotmsg(Miotmsg* msg, size_t msg_size, int argc, const char* argv[]) {
  ${msg_sub_type}* sub_arg = (${msg_sub_type}*)(msg->sub_arg);

  CHECK_SIZE(msg_size, sizeof(*msg) + sizeof(*sub_arg), "The property set ${module_name}_${prop_name} message size is too small");

  miotmsg__init(msg);
  msg->type = MIOTMSG__MSGTYPE__INVOKE;
  msg->subtype = ${msg_sub_id};
  msg->test_oneof_case = ${msg_arg_id};

  ${msg_sub_type.lower()}__init(sub_arg);

%if utils.isCallbackType(prop_type):
<%
  cb_key = 'key'
  if 'cb_key' in meta: cb_key = meta['cb_key']
  cb_name = prop_name
  if 'cb_name' in meta: cb_name = meta['cb_name']
%>
  static char _cb_name[] = "${cb_name}";
  sub_arg->${cb_key} = _cb_name;
  return _cb_name;
%else:
  ${gen_param(prop, '')}
  return 0;
%endif
}
%endif
%endif
</%def>


%for include in vars['includes']:
#include "${include}"
%endfor

#include "miot_service_agent_event.h"

%for m in doc['members']:
%if utils.needGenerator(m):
%if m['type'] == 'function':
${gen_method(doc, m)}
%elif m['type'] == 'property':
${gen_property(doc, m)}
%endif
%endif
%endfor


static EventActionEntryInfo _${module_name}_actions [] = {
%for m in doc['members']:
%if utils.needGenerator(m):
%if m['type'] == 'function':
  {
    "${m['identifier']}",
    _${module_name}_${m['identifier']}_method_to_miotmsg,
  },
%elif m['type'] == 'property' and 'writeable' in m and m['writeable'] and not utils.isCallbackType(m['value_type']):
  {
    "${m['name']}",
    _${module_name}_${m['name']}_set_to_miotmsg,
  },
%endif
%endif
%endfor
};

static EventNotifyEntryInfo _${module_name}_notifies[] = {
%for m in doc['members']:
%if utils.needGenerator(m):
%if m['type'] == 'property' and utils.isCallbackType(m['value_type']):
  {
    "${m['name']}",
    _${module_name}_${m['name']}_event_to_miotmsg,
  },
%endif
%endif
%endfor
};

EventActionInfo _${module_name}_action_info {
  "${module_name}",
  _${module_name}_actions,
  _${module_name}_notifies,
  sizeof(_${module_name}_actions) / sizeof(EventActionEntryInfo),
  sizeof(_${module_name}_notifies) / sizeof(EventNotifyEntryInfo)
};
