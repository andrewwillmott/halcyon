import lldb
import shlex

def parray(debugger, command, result, dict):
  args = shlex.split(command)
  if len(args) == 2:
    count = int(args[1])
    indices = range(count)
  elif len(args) == 3:
    first = int(args[1])
    count = int(args[2])
    indices = range(first, first + count)
  else:
    print 'Usage: parray ARRAY [FIRST] COUNT'
    return
  for i in indices:
    print lldb.frame.GetValueForVariablePath(args[0] + "[" + str(i) + "]")

def __lldb_init_module(debugger, internal_dict):
  debugger.HandleCommand('command script add -f parray.parray parray')
