# Copyright 2023 Xiaomi, Inc. All rights reserved.

import sys
import ply.lex as lex
import ply.yacc as yacc
import jidlast as ast
import logging
import os
import random
import time

def CreateASTNode(p, t, *args):
  n = t(*args)
  p[0] = n
  for i in range(1, len(p)):
    if isinstance(p[i], ast.Node):
      n.lineno = p[i].lineno
      n.lexpos = p[i].lexpos
      return
    elif isinstance(p[i], list):
      for m in p[i]:
        if isinstance(m, ast.Node):
          n.lineno = m.lineno
          n.lexpos = m.lexpos
          return
    else:
      n.lineno = p.lineno(i)
      n.lexpos = p.lexpos(i)
      return

def handleAbsPath(path):
  if not path:
    return None
  if not os.path.isabs(path):
    path = os.path.abspath(path)
  if not os.path.exists(path):
    print(f"Error: Directory doesn't exist: {path}")
    return None
  elif not os.access(path, os.W_OK):
    print(f"Error: Directory is not writable: {path}")
    return None
  return path

########################################################
class Parser:
  """
  Base class for a lexer/parser that has the rules defined as methods
  """
  tokens = ()
  precedence = ()

  def __init__(self, **kw):
    self.debug = kw.get('debug', False)
    self.reporter = kw.get('errorReporter', None)
    self.jidl_file_name = kw.get('jidlFileName', "")
    self.out_dir = handleAbsPath(kw.get('outDir', None))
    self.names = {}
    try:
      modname = os.path.split(os.path.splitext(__file__)[0])[
        1] + "_" + self.__class__.__name__
    except:
      modname = "parser" + "_" + self.__class__.__name__
    self.debugfile = modname + ".dbg"
    timestamp = int(time.time() * 1000)
    random_field = random.randint(0, 9999)
    self.tabmodule = f"{modname}_parsetab_{self.jidl_file_name}_{timestamp}_{random_field}"

    #print(self.debugfile, self.tabmodule)
    logging.basicConfig(
        level = logging.ERROR,
        filename = 'parselog.txt',
        filemode = 'w',
        format = '%(filename)10s:%(lineno)4d:%(message)s'
    )

    # Build the lexer and parser
    lex.lex(module=self, debug=self.debug)
    yacc_args = {
        'module': self,
        'debug': self.debug,
        'debugfile': self.debugfile,
        'tabmodule': self.tabmodule
    }
    if self.out_dir:
      yacc_args['outputdir'] = self.out_dir
    yacc.yacc(**yacc_args)

  def parse(self, s):
    yacc.parse(s) #, debug=logging.getLogger())

  def log(self, *args):
    if self.debug:
      print(args)

  def clean(self):
    if self.out_dir:
      out_path = os.path.join(self.out_dir, self.tabmodule)
      tabmodule_file = f"{out_path}.py"
    else:
      dir_path = os.path.dirname(os.path.abspath(__file__))
      tabmodule_file = f"{dir_path}/{self.tabmodule}.py"
    if os.path.exists(tabmodule_file):
      os.remove(tabmodule_file)

def create_reserved_map(reserved):
  reserved_map = { }
  for r in reserved:
    reserved_map[r.lower()] = r
  return reserved_map

def check_name_in_list(l, name):
  for e in l:
    if e['name'] == name:
      return True
  return False

class JIDL(Parser):
  primary_types = (
    'ARRAY',
    'BOOLEAN',
    'DOUBLE',
    'FLOAT',
    'INT',
    'JSCONTEXT',
    'JSVALUE',
    'OBJECT',
    'STRING',
    'LONG',
    'VOID',
    'UINT',
    'ULONG',
    'PROMISE',
    'TYPED_ARRAY',
    'UNIQUE_BUFFER',
    'JSONOBJECT',
  )

  typed_array_types = (
    'UINT8',
    'INT8',
    'UINT16',
    'INT16',
    'UINT32',
    'INT32',
    'UINT64',
    'INT64'
  )

  reserved = primary_types + typed_array_types + (
    'MODULE',
    'CLASS',
    'STRUCT',
    'IMPORT',
    'INTERFACE',
    'EXTENDS',
    'CALLBACK',
    'CONST',
    'USE',
    'TYPE',
    'PRIVATE',
    'PROPERTY',
    'EVENT',
    'ENUM',
    'WRITEONLY',
    'READONLY',
    'CONSTRUCTOR',
    'ASYNC',
    'MAIN',
    'WORKER',
    'TRUE',
    'FALSE',
    'NULL',
    'IMPORT_MESSAGE',
    'FROM',
    'AS',
    'MESSAGE',
  )

  tokens = reserved + (
    'AT', 'LPAREN', 'RPAREN', 'COMMA', 'ELLIPSIS', 'COLON',
    'LBRACE', 'RBRACE', 'LBRACKET', 'RBRACKET', 'EQUALS',
    'LANGULARBRACKET', 'RANGULARBRACKET',
    'LITERAL', 'DOT', 'ID',
    'INTEGER', 'NUMBER'
  )


  # Tokens
  t_AT = '@'
  t_LPAREN = r'\('
  t_RPAREN = r'\)'
  t_LBRACE = r'\{'
  t_RBRACE = r'\}'
  t_LBRACKET = r'\['
  t_RBRACKET = r'\]'
  t_LANGULARBRACKET = r'<'
  t_RANGULARBRACKET = r'>'
  t_COMMA = r','
  t_EQUALS = r'='
  t_ELLIPSIS = r'\.\.\.'
  t_COLON = ':'
  t_DOT = r'\.'
  t_INTEGER = r'(\+|-)?\d+([uU]|[lL]|[uU][lL]|[lL][uU])?'
  t_NUMBER = r'(\+|-)?((\d+)(\.\d+)(e(\+|-)?(\d+))? | (\d+)e(\+|-)?(\d+))([lL]|[fF])?'
  t_LITERAL = r'\"([^\\\n]|(\\.))*?\"'
  t_ignore = " \t;"

  reserved_map = create_reserved_map(reserved)

  def t_newline(self, t):
    r'\n+'
    t.lexer.lineno += len(t.value)

  def t_comment(self, t):
    r'(/\*(.|\n)*?\*/)|(//.*\n)'
    t.lexer.lineno += t.value.count('\n')

  def t_ID(self, t):
    r'[A-Za-z_][\w_]*'
    t.type = self.reserved_map.get(t.value, "ID")
    return t

  def t_error(self, t):
    if self.reporter:
      self.reportLexError(t)
    else:
      print("Illegal character '%s' in line %d" % (t.value[0], t.lexer.lineno))
    t.lexer.skip(1)

  # Parsing rules
  def p_module_file(self, p):
    """
    module_file : module_head
                | module_head module_blocks
    """
    count = len(p)
    name = p[1]['name']
    version = p[1]['version']
    if count == 2:
      CreateASTNode(p, ast.ModuleDefine, name, version, None)
    else:
      CreateASTNode(p, ast.ModuleDefine, name, version, p[2])

    if 'imports' in p[1]:
      p[0].SetImports(p[1]['imports'])
    self.module = p[0]

  def p_module_head(self, p):
    """
    module_head : module_define
                | import_list module_define
    """
    count = len(p)
    if count == 2:
      p[0] = p[1]
    else:
      p[0] = p[2]
      p[0]['imports'] = p[1]

  def p_module_define(self, p):
    """
    module_define : MODULE module_name AT NUMBER
                  | MODULE module_name
    """
    count = len(p)
    if count == 5:
      p[0] = {'name': p[2], 'version': p[4]}
    else:
      p[0] = {'name': p[2], 'version' : '1.0'}

  def p_module_name(self, p):
    """
    module_name : ID
                | CALLBACK
                | MAIN
                | WORKER
                | EXTENDS
                | TYPE
                | USE
                | EVENT
                | PROPERTY
                | CONST
                | ASYNC
                | module_name DOT module_name
    """
    if len(p) == 2:
      p[0] = p[1]
    else:
      p[0] = p[1] + '.' + p[3]

  def p_module_blocks(self, p):
    """
    module_blocks : module_block
            | module_blocks module_block
    """
    count = len(p)
    if count == 1:
      CreateASTNode(p, ast.BlockList)
    elif count == 2:
      CreateASTNode(p, ast.BlockList)
      p[0].Append(p[1])
    elif count == 3:
      p[0] = p[1]
      p[0].Append(p[2])


  def p_module_block(self, p):
    """
    module_block : base_define_with_meta
           | callback_define
           | class_define
           | interface_define
           | struct_define
           | type_define
    """
    p[0] = p[1]

  def p_import_list(self, p):
    """
    import_list : import_block
             | import_list import_block
    """
    count = len(p)
    if count == 2:
      CreateASTNode(p, ast.ImportList, p[1])
    elif count == 3:
      CreateASTNode(p, ast.ImportList.Append, p[1], p[2])

  def p_import_block(self, p):
    """
    import_block : import
                 | import_message
    """
    p[0] = p[1]

  def p_import(self, p):
    'import : IMPORT module_name AT NUMBER'
    CreateASTNode(p, ast.ImportDefine, p[2], p[4])

  def p_struct_define(self, p):
    """
    struct_define : STRUCT type_name LBRACE struct_body RBRACE
             | meta_attributes_define STRUCT ID LBRACE struct_body RBRACE
    """
    if len(p) == 7:
      CreateASTNode(p, ast.StructDefine, p[3], p[5])
      p[0].SetMetaAttributes(p[1])
    else:
      CreateASTNode(p, ast.StructDefine, p[2], p[4])

  def p_struct_body(self, p):
    """
    struct_body : struct_block_with_meta
           | struct_body struct_block_with_meta
    """
    count = len(p)
    if count == 1:
      CreateASTNode(p, ast.BlockList)
    elif count == 2:
      CreateASTNode(p, ast.BlockList)
      p[0].Append(p[1])
    elif count == 3:
      p[0] = p[1]
      p[0].Append(p[2])

  def p_struct_block(self, p):
    """
    struct_block : struct_member_define
               | struct_member_struct
               | struct_member_callback
    """
    p[0] = p[1]

  def p_struct_block_with_meta(self, p):
    """
    struct_block_with_meta : struct_block
              | meta_attributes_define struct_block
    """
    if len(p) == 3:
      p[0] = p[2]
      p[0].SetMetaAttributes(p[1])
    else:
      p[0] = p[1]

  def p_type_define(self, p):
    """
    type_define : TYPE ID EQUALS primary_type
                | meta_attributes_define TYPE ID EQUALS primary_type
    """
    if len(p) == 5:
      CreateASTNode(p, ast.UserTypeDefine, p[2], p[4])
    else:
      CreateASTNode(p, ast.UserTypeDefine, p[3], p[5])
      p[0].SetMetaAttributes(p[1])

  def p_struct_member_define(self, p):
    """
    struct_member_define : value_type member_name
                      | value_type member_name EQUALS literal_value
    """
    count = len(p)
    CreateASTNode(p, ast.StructMemberBase, p[1], p[2])
    if count == 5:
      p[0].SetDefault(p[4])

  def p_struct_member_struct(self, p):
    """
    struct_member_struct : STRUCT type_name_id ID
                      | STRUCT type_name_id ID EQUALS null_literal
    """
    count = len(p)
    CreateASTNode(p, ast.StructMemberStruct, p[2], p[3])
    if count == 6:
      p[0].SetDefaultNull(p[5])

  def p_struct_member_callback(self, p):
    """
    struct_member_callback : CALLBACK type_name_id member_name
                      | CALLBACK type_name_id member_name EQUALS null_literal
    """
    count = len(p)
    CreateASTNode(p, ast.StructMemberCallback, p[2], p[3])
    if count == 6:
      p[0].SetDefaultNull(p[5])

  def p_class_define(self, p):
    """
    class_define : CLASS ID LBRACE class_body RBRACE
          | meta_attributes_define CLASS ID LBRACE class_body RBRACE
    """
    if len(p) == 7:
      CreateASTNode(p, ast.ClassDefine, p[3], p[5])
      p[0].SetMetaAttributes(p[1])
    else:
      CreateASTNode(p, ast.ClassDefine, p[2], p[4])

  def p_class_body_define(self, p):
    """
    class_body :
           | class_body class_block_define
    """
    count = len(p)
    if count == 1:
      CreateASTNode(p, ast.BlockList)
    elif count == 3:
      body = p[1]
      body.Append(p[2])
      p[0] = body

  def p_class_block_define(self, p):
    """
    class_block_define : base_define_with_meta
               | constructor_define
    """
    p[0] = p[1]

  def p_base_define(self, p):
    """
    base_define : function_define
          | property_define
          | use_define
          | event_define
          | const_define
          | enum_define
    """
    p[0] = p[1]

  def p_base_define_with_meta(self, p):
    """
    base_define_with_meta : base_define
            | meta_attributes_define base_define
    """
    if len(p) == 3:
      p[0] = p[2]
      p[0].SetMetaAttributes(p[1])
    else:
      p[0] = p[1]

  def p_interface_define(self, p):
    """
    interface_define : INTERFACE type_name interface_extends LBRACE interface_body RBRACE
              | meta_attributes_define INTERFACE type_name interface_extends LBRACE interface_body RBRACE
    """
    extends = None
    if len(p) == 8:
      CreateASTNode(p, ast.InterfaceDefine, p[3], ast.INTERFACE_DEFINE, p[6])
      p[0].SetMetaAttributes(p[1])
      extends = p[4]
    else:
      CreateASTNode(p, ast.InterfaceDefine, p[2], ast.INTERFACE_DEFINE, p[5])
      extends = p[3]

    if extends:
      p[0].SetExtends(extends)

  def p_interface_extends(self, p):
    """
    interface_extends :
              | EXTENDS interface_extends_list
    """
    count = len(p)
    if (count == 3):
      p[0] = p[2]
    else:
      p[0] = None

  def p_interface_extends_list(self, p):
    """
    interface_extends_list : ID
                     | interface_extends_list COMMA ID
    """
    count = len(p)
    if count == 2:
       p[0] = [p[1]]
    else:
       p[0] = p[1]
       p[0].append(p[3])

  def p_interface_body(self, p):
    """
    interface_body :
            | interface_body base_define_with_meta
    """
    count = len(p)
    if count == 1:
      CreateASTNode(p, ast.BlockList)
    else:
      p[0] = p[1]
      p[0].Append(p[2])

  def p_property_define(self, p):
    """
    property_define : PROPERTY value_type type_name
            | PROPERTY value_type type_name READONLY
            | PROPERTY value_type type_name WRITEONLY
            | CONST PROPERTY value_type type_name
    """
    count = len(p)
    if p[1] == 'property':
      CreateASTNode(p, ast.PropertyDefine, p[3], p[2])
      if count == 5:
        if p[4] == 'readonly':
          p[0].SetReadOnly()
        else:
          p[0].SetWriteOnly()
    elif p[1] == 'const':
      CreateASTNode(p, ast.PropertyDefine, p[4], p[3])
      p[0].SetConst()

  def p_constructor_define(self, p):
    """
    constructor_define : CONSTRUCTOR LPAREN param_list_may_defualts_args RPAREN
              | meta_attributes_define CONSTRUCTOR LPAREN param_list_may_defualts_args RPAREN
    """
    if len(p) == 6:
      CreateASTNode(p, ast.ConstructorDefine, p[4])
      p[0].SetMetaAttributes(p[1])
    else:
      CreateASTNode(p, ast.ConstructorDefine, p[3])

  def p_function_define(self, p):
    """
    function_define : base_function_define_return
            | PRIVATE base_function_define_return
    """
    count = len(p)
    function = p[count - 1]
    function.SetPrivate(count == 3 and True or False)
    p[0] = function

  def p_event_define(self, p):
    """
    event_define : EVENT ID LPAREN param_list_args RPAREN
                 | ASYNC EVENT ID LPAREN param_list_args RPAREN
    """
    if len(p) == 7:
      CreateASTNode(p, ast.EventDefine, p[3], p[5])
      p[0].SetForceAsync()
    else:
      CreateASTNode(p, ast.EventDefine, p[2], p[4]);

  def p_base_function_define_return(self, p):
    """
    base_function_define_return : return_type base_function_define
            | async_define return_type base_function_define
            | promise_type base_function_define
    """
    count = len(p)
    if count == 3:
        CreateASTNode(p, ast.FunctionDefine.SetReturn, p[2], p[1])
    elif count == 4:
        p[0] = p[3]
        p[0].SetReturn(p[2])
        p[0].SetAsync(p[1])

  def p_base_function_define(self, p):
    'base_function_define :  ID LPAREN param_list_may_defualts_args RPAREN'
    CreateASTNode(p, ast.FunctionDefine, p[1], p[3])

  def p_param_list_may_defualts_args(self, p):
    """
    param_list_may_defualts_args :
                  | param_ellipse
                  | param_list_may_defualts
                  | param_list_may_defualts COMMA param_ellipse
    """
    count = len(p)
    if count == 1:
      CreateASTNode(p, ast.ParamList)
    elif count == 2:
      if isinstance(p[1], ast.ParamList):
        p[0] = p[1]
      else:
        CreateASTNode(p, ast.ParamList, p[1])
    elif count == 4:
      p[0] = p[1]
      p[0].Append(p[3])

  def p_param_list_may_defualts(self, p):
    """
    param_list_may_defualts : param_may_defualt
                | param_list_may_defualts COMMA param_may_defualt
    """
    count = len(p)
    if count == 2:
      p[0] = [p[1]]
      CreateASTNode(p, ast.ParamList, p[1])
    elif count == 4:
      p[0] = p[1]
      p[0].Append(p[3])

  def p_param_may_default(self, p):
    """
    param_may_defualt : param_define
                      | param_define EQUALS literal_value
    """
    count = len(p)
    p[0] = p[1]
    if count == 4:
      p[0].SetDefault(p[3])

  def p_const_define(self, p):
    """
    const_define : CONST const_base
                 | CONST const_object
    """
    p[0] = p[2]

  def p_const_base(self, p):
    """
    const_base : ID EQUALS literal_value
    """
    CreateASTNode(p, ast.ConstDefine, p[1], p[3])

  def p_const_object(self, p):
    """
    const_object : ID EQUALS LBRACE const_body RBRACE
    """
    CreateASTNode(p, ast.ConstObjectDefine, p[1], p[4])

  def p_const_body(self, p):
    """
    const_body : const_block
               | const_body COMMA const_block
    """
    count = len(p)
    if count == 2:
      CreateASTNode(p, ast.BlockList)
      p[0].Append(p[1])
    elif count == 4:
      p[0] = p[1]
      p[0].Append(p[3])

  def p_const_block(self, p):
    """
    const_block : const_base
                | const_object
    """
    p[0] = p[1]

  def p_enum_define(self, p):
    """
    enum_define : ENUM ID LBRACE enum_members RBRACE
                | ENUM ID LBRACE RBRACE
    """
    if len(p) == 5:
      CreateASTNode(p, ast.EnumDefine, p[2], [])
    else:
      CreateASTNode(p, ast.EnumDefine, p[2], p[4])

  def p_enum_members(self, p):
    """
    enum_members : enum_member_meta
                 | enum_members COMMA
                 | enum_members COMMA enum_member_meta
    """
    if len(p) == 1:
      p[0] = []
    elif len(p) == 2:
      p[0] = [p[1]]
    elif len(p) == 3:
      p[0] = p[1]
    else:
      p[1].append(p[3])
      p[0] = p[1]

  def p_enum_member_meta(self, p):
    """
    enum_member_meta : meta_attributes_define enum_member
    """
    p[0] = p[2]
    p[0].SetMetaAttributes(p[1])

  def p_enum_member(self, p):
    """
    enum_member : ID
                | ID EQUALS literal_value
    """
    if len(p) == 2:
      CreateASTNode(p, ast.ConstDefine, p[1], None)
    else:
      CreateASTNode(p, ast.ConstDefine, p[1], p[3])

  def p_async_define(self, p):
    """
    async_define : ASYNC
                 | ASYNC LPAREN MAIN RPAREN
                 | ASYNC LPAREN WORKER RPAREN
                 | ASYNC LPAREN LITERAL RPAREN
                 | ASYNC LPAREN INTEGER RPAREN
                 | ASYNC LPAREN id_with_namespace RPAREN
    """
    count = len(p)
    if count == 2:
        CreateASTNode(p, ast.AsyncInfo, 'current')
    else:
        CreateASTNode(p, ast.AsyncInfo, p[3])

  def p_callback_define(self, p):
    """
    callback_define : CALLBACK type_name LPAREN param_list_args RPAREN
              | meta_attributes_define CALLBACK type_name LPAREN param_list_args RPAREN
    """
    if len(p) == 7:
      CreateASTNode(p, ast.CallbackDefine, p[3], p[5])
      p[0].SetMetaAttributes(p[1])
    else:
      CreateASTNode(p, ast.CallbackDefine, p[2], p[4])


  def p_type_name(self, p):
    """
    type_name : ID
              | CALLBACK
              | MAIN
              | WORKER
              | CONSTRUCTOR
              | EXTENDS
              | TYPE
    """
    p[0] = p[1]

  def p_type_name_id(self, p):
    'type_name_id : type_name'
    CreateASTNode(p, ast.IDType, p[1])

  def p_member_name(self, p):
    """
    member_name : ID
                | CALLBACK
                | MAIN
                | WORKER
                | EXTENDS
                | TYPE
                | USE
                | EVENT
                | PROPERTY
                | MODULE
                | CONST
                | ASYNC
                | MESSAGE
                | FROM
    """
    p[0] = p[1]

  def p_param_list_args(self, p):
    """
    param_list_args :
            | param_ellipse
            | param_list
            | param_list COMMA param_ellipse
    """
    count = len(p)
    if count == 1:
      CreateASTNode(p, ast.ParamList)
    elif count == 2:
      if isinstance(p[1], ast.EllipseType):
        CreateASTNode(p, ast.ParamList, p[1])
      else:
        p[0] = p[1]
    elif count == 4:
      CreateASTNode(p, ast.ParamList.Append, p[1], p[3])

  def p_param_list(self, p):
    """
    param_list : param_define
           | param_list COMMA param_define
    """
    count = len(p)
    if count == 2:
      CreateASTNode(p, ast.ParamList, p[1])
    else:
      CreateASTNode(p, ast.ParamList.Append, p[1], p[3])

  def p_ellipse(self, p):
    """
    param_ellipse : ELLIPSIS
            | primary_type ELLIPSIS
    """
    count = len(p)
    if count == 2:
      CreateASTNode(p, ast.EllipseType)
    else:
      CreateASTNode(p, ast.EllipseType, p[1])

  def p_use_define(self, p):
    """
    use_define : USE base_function_define EQUALS function_call
    """
    CreateASTNode(p, ast.UseDefine, p[2], p[4])

  def p_function_call(self, p):
    'function_call : ID LPAREN param_call_list_args RPAREN'
    CreateASTNode(p, ast.FunctionCall, p[1], p[3])

  def p_param_call_list_args(self, p):
    """
    param_call_list_args :
               | ELLIPSIS
               | param_call_list
               | param_call_list COMMA ELLIPSIS
    """
    count = len(p)
    if count == 1:
      CreateASTNode(p, ast.ParamCallList)
    elif count == 2:
      if p[1] == '...':
        CreateASTNode(p, ast.ParamCallList, ast.EllipseType())
      else:
        p[0] = p[1]
    elif count == 4:
      CreateASTNode(p, ast.ParamCallList.Append, p[1], ast.EllipseType())


  def p_param_call_list(self, p):
    """
    param_call_list : param_call
            | param_call_list COMMA param_call
    """
    count = len(p)
    if count == 2:
      CreateASTNode(p, ast.ParamCallList, p[1])
    else:
      CreateASTNode(p, ast.ParamCallList.Append, p[1], p[3])

  def p_param_call(self, p):
    """
    param_call : id
               | literal_value
    """
    p[0] = p[1]

  def p_param_define(self, p):
    """
    param_define : value_type param_id
                 | meta_attributes_define value_type param_id
    """
    if len(p) == 3:
      CreateASTNode(p, ast.ParamDefine, p[1], p[2])
    else:
      CreateASTNode(p, ast.ParamDefine, p[2], p[3])
      p[0].SetMetaAttributes(p[1])

  def p_param_id(self, p):
    """
    param_id : meta_id
    """
    p[0] = p[1]

  def p_return_type(self, p):
    """
    return_type : value_type
          | VOID
    """
    if type(p[1]) == str:
      p[0] = ast.GetPrimaryType(p[1])
    else:
      p[0] = p[1]

  def p_promise_type(self, p):
    """
    promise_type : PROMISE LANGULARBRACKET promise_sub_type RANGULARBRACKET
                 | PROMISE
    """
    count = len(p)
    void_type = ast.GetPrimaryType('void')
    if count == 2:
      CreateASTNode(p, ast.PromiseType, void_type)
    elif count == 5:
      CreateASTNode(p, ast.PromiseType, p[3])

  def p_promise_sub_type(self, p):
    """
    promise_sub_type : array_object_type
                     | id_array_type
                     | primary_type
                     | unique_buffer_type
                     | type_name_id
                     | VOID
    """
    if type(p[1]) == str:
      p[0] = ast.GetPrimaryType(p[1])
    else:
      p[0] = p[1]

  def p_value_type(self, p):
    """
    value_type : array_object_type
               | typed_array_type
               | id_array_type
               | primary_type
               | unique_buffer_type
               | message_type
               | id
    """
    p[0] = p[1]

  def p_id_array_type(self, p):
    """
    id_array_type : id LBRACKET RBRACKET
    """
    CreateASTNode(p, ast.IDArrayType, p[1])

  def p_array_object_type(self, p):
    "array_object_type : primary_type LBRACKET RBRACKET"
    CreateASTNode(p, ast.PrimaryArrayType, p[1])

  def p_id(self, p):
    'id : ID'
    CreateASTNode(p, ast.IDType, p[1])

  def p_id_with_namespace(self, p):
    """
    id_with_namespace : ID
                      | COLON COLON ID
                      | id_with_namespace COLON COLON ID
    """
    p[0] = ''.join(p[1:])

  def p_unique_buffer_type(self, p):
    """
    unique_buffer_type : UNIQUE_BUFFER
    """
    CreateASTNode(p, ast.UniqueBufferType, p[1])

  def p_typed_array_type(self, p):
    """
    typed_array_type : TYPED_ARRAY LANGULARBRACKET typed_array_elemenet_type RANGULARBRACKET
    """
    CreateASTNode(p, ast.TypedArrayType, p[3])

  def p_meta_attributes_define(self, p):
    """
    meta_attributes_define : LBRACKET meta_attribute_list RBRACKET
    """
    p[0] = p[2]

  def p_meta_attribute_list(self, p):
    """
    meta_attribute_list : meta_attribute
               | meta_attribute_list COMMA meta_attribute
    """
    count = len(p)
    if count == 2:
      p[0] = [p[1]]
    else:
      p[0] = p[1]
      p[0].append(p[3])

  def p_meta_attribute(self, p):
    """
    meta_attribute : meta_id EQUALS literal_value
               | meta_id EQUALS array_literal
    """
    CreateASTNode(p, ast.MetaAttribute, p[1], p[3])

  def p_meta_id(self, p):
    """
    meta_id : ID
            | CALLBACK
            | TYPE
            | MODULE
            | CLASS
            | CONST
            | USE
            | PRIVATE
            | EVENT
            | ENUM
            | CONSTRUCTOR
            | MAIN
            | WORKER
            | ASYNC
            | TRUE
            | FALSE
            | PROPERTY
            | EXTENDS
            | IMPORT_MESSAGE
            | FROM
            | MESSAGE
            | AS
    """
    p[0] = p[1]

  def p_message_type(self, p):
    """
    message_type : MESSAGE message_id
    """
    CreateASTNode(p, ast.MessageType, p[2])

  def p_import_message(self, p):
    """
    import_message : IMPORT_MESSAGE LBRACE message_list RBRACE FROM string_literal
    """
    CreateASTNode(p, ast.ImportMessage, p[3], p[6])

  def p_message_list(self, p):
    """
    message_list : message_decl
                 | message_list COMMA message_decl
    """
    if len(p) == 2:
        p[0] = [p[1]]
    else:
        p[0].append(p[3])

  def p_message_decl(self, p):
    """
    message_decl : message_id
                 | pb_message_id AS message_id
    """
    if len(p) == 2:
        CreateASTNode(p, ast.MessageDeclare, p[1], p[1])
    else:
        CreateASTNode(p, ast.MessageDeclare, p[1], p[3])

  def p_message_id(self, p):
    """
    message_id : type_name
    """
    p[0] = p[1]

  def p_pb_message_id(self, p):
    """
    pb_message_id : full_name
    """
    p[0] = p[1]

  def p_full_name(self, p):
   """
   full_name : normal_name
             | full_name DOT normal_name
   """
   if len(p) == 2:
     p[0] = p[1]
   else:
     p[0] = p[1] + '.' + p[3]


  def p_normal_name(self, p):
    """
    normal_name : ID
                  | EVENT
                  | USE
                  | FROM
                  | INTERFACE
                  | TRUE
                  | FALSE
                  | MODULE
                  | STRUCT
                  | CLASS
                  | CONST
                  | PROPERTY
                  | CALLBACK
                  | ENUM
                  | READONLY
                  | ASYNC
                  | MAIN
                  | WORKER
    """
    p[0] = p[1]

  def p_typed_array_element_type(self, p):
    """
    typed_array_elemenet_type : UINT8
                      | INT8
                      | UINT16
                      | INT16
                      | UINT32
                      | UINT
                      | INT32
                      | INT
                      | UINT64
                      | ULONG
                      | INT64
                      | LONG
                      | FLOAT
                      | DOUBLE
                      | VOID
    """
    p[0] = ast.GetTypedArrayElementType(p[1])

  def p_primary_type(self, p):
    """
    primary_type : INT
           | FLOAT
           | DOUBLE
           | STRING
           | BOOLEAN
           | LONG
           | UINT
           | ULONG
           | JSVALUE
           | JSCONTEXT
           | ARRAY
           | OBJECT
           | JSONOBJECT
    """
    p[0] = ast.GetPrimaryType(p[1])

  def p_array_literal(self, p):
    """
    array_literal : LBRACKET array_literal_elements RBRACKET
    """
    p[0] = p[2]

  def p_array_literal_elements(self, p):
    """
    array_literal_elements : array_literal_element
                | array_literal_elements COMMA array_literal_element
    """
    count = len(p)
    if count == 2:
      p[0] = [p[1]]
    else:
      p[0] = p[1]
      p[0].append(p[3])

  def p_array_literal_element(self, p):
    """
    array_literal_element : literal_value
                  | array_literal
    """
    p[0] = p[1]

  def p_literal_value(self, p):
    """
    literal_value : number_literal
            | int_literal
            | string_literal
            | boolean_literal
            | null_literal
    """
    p[0] = p[1]

  def p_number_literal(self, p):
    "number_literal : NUMBER"
    CreateASTNode(p, ast.LiteralValue, p[1], 'double')

  def p_int_literal(self, p):
    "int_literal : INTEGER"
    CreateASTNode(p, ast.LiteralValue, p[1], 'int')

  def p_string_literal(self, p):
    "string_literal : LITERAL"
    CreateASTNode(p, ast.LiteralValue, p[1], 'string')

  def p_boolean_literal(self, p):
    """
    boolean_literal : TRUE
                    | FALSE
    """
    CreateASTNode(p, ast.LiteralValue, p[1], 'boolean')

  def p_null_literal(self, p):
    "null_literal : NULL"
    CreateASTNode(p, ast.LiteralValue, p[1], 'null')

  def p_error(self, p):
    if p:
      print(p)
      if self.reporter:
         self.reporter.reportYaccError(p)
      else:
         print("Syntax error at '%s' in line: %d, %d" % (p.value, p.lineno, p.lexpos))

  def __init__(self, **kw):
    Parser.__init__(self, **kw)
    self.module = None

