#!/usr/bin/env python3
"""Compile GLSL using the system shaderc C library (no Python packages)."""
import ctypes as c
import ctypes.util
from pathlib import Path
import sys

library=ctypes.util.find_library('shaderc_shared')
if not library:
    raise SystemExit('Install shaderc/glslc or provide libshaderc_shared to build shaders.')
lib=c.CDLL(library)
def api(name,args,result):
    function=getattr(lib,name)
    function.argtypes=args
    function.restype=result
    return function
initialize=api('shaderc_compiler_initialize',[],c.c_void_p)
compile_shader=api('shaderc_compile_into_spv',[c.c_void_p,c.c_char_p,c.c_size_t,c.c_int,c.c_char_p,c.c_char_p,c.c_void_p],c.c_void_p)
status=api('shaderc_result_get_compilation_status',[c.c_void_p],c.c_int)
message=api('shaderc_result_get_error_message',[c.c_void_p],c.c_char_p)
length=api('shaderc_result_get_length',[c.c_void_p],c.c_size_t)
data=api('shaderc_result_get_bytes',[c.c_void_p],c.c_void_p)
release=api('shaderc_result_release',[c.c_void_p],None)
cleanup=api('shaderc_compiler_release',[c.c_void_p],None)
output=Path(sys.argv[1]);output.mkdir(parents=True,exist_ok=True)
compiler=initialize()
if not compiler:raise SystemExit('Shader compiler initialization failed')
try:
    for kind,suffix in enumerate(('vert','frag')):
        source=Path(__file__).parent/'shaders'/('vehicle.'+suffix)
        content=source.read_bytes()
        result=compile_shader(compiler,content,len(content),kind,str(source).encode(),b'main',None)
        if not result:raise SystemExit('Shader compiler returned no result')
        try:
            if status(result):raise SystemExit(message(result).decode())
            (output/('vehicle.'+suffix+'.spv')).write_bytes(c.string_at(data(result),length(result)))
        finally:release(result)
finally:cleanup(compiler)
