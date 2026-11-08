from pprint import pprint
from textwrap import dedent

print("hi")

script = """
emit immediate
colon 
semi-colon
exit
""".strip()


dict_spec = []

for line in script.split('\n'):
    name, *args = line.split(' ')
    code_name = name
    immediate = False
    dict_spec.append((name, code_name, immediate))


def build_node(node_spec):
    name, code_name, immediate = node_spec
    name_len = len(name)
    return dedent(f"""
    @node-{name}
    &name-length {name_len:02x}
    &name \"{name} 00
    &link 0000
    &code :{code_name}
    """)

print('\n'.join(list(map(build_node, dict_spec))))

"""
@Builtin-emit
&name-length 04
&name "emit 00
&link :Builtin-colon
&code :emit

@Builtin-colon
&name-length 01
&name ": 00
&link :Builtin-semi-colon
&code :colon

@Builtin-semi-colon
&name-length 81
&name "; 00
&link :Builtin-exit
&code :semi-colon

@Builtin-exit
&name-length 04
&name "exit 00
&link :Builtin-dot
&code :exit

@Builtin-dot
&name-length 01
&name ". 00
&link :Builtin-add
&code :dot

@Builtin-add
&name-length 01
&name "+ 00
&link :Builtin-minus
&code :add

@Builtin-minus
&name-length 01
&name "- 00
&link :Builtin-mult
&code :minus

@Builtin-mult
&name-length 01
&name "* 00
&link :Builtin-div
&code :mult

@Builtin-div
&name-length 01
&name "/ 00
&link :Builtin-dup
&code :div

@Builtin-dup
&name-length 03
&name "dup 00
&link :Builtin-print-stack
&code :dup

@Builtin-print-stack
&name-length 02
&name ".s 00
&link :Builtin-here
&code :print-stack

@Builtin-here
&name-length 04
&name "here 00
&link :Builtin-peek16
&code :push-here

@Builtin-peek16
&name-length 01
&name "@ 00
&link :Builtin-poke16
&code :peek16

@Builtin-poke16
&name-length 01
&name "! 00
&link :Builtin-peek8
&code :poke16

@Builtin-peek8
&name-length 02
&name "c@ 00
&link :Builtin-poke8
&code :peek8

@Builtin-poke8
&name-length 02
&name "c! 00
&link :Builtin-comma8
&code :poke8

@Builtin-comma16
&name-length 01
&name ", 00
&link :Builtin-comma8
&code :comma16

@Builtin-comma8
&name-length 02
&name "c, 00
&link 0000
&code :comma8
"""
