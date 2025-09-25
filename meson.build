project('gpufuntime', 'c')

objcopy = find_program('objcopy')
objcopy_bin_to_o = [objcopy, '-Ibinary', '-Oelf64-x86-64', '@INPUT@', '@OUTPUT@']

subdir('external')
subdir('models')

sdl3_dep = dependency('sdl3', fallback: ['sdl3', 'sdl3_dep'])
cglm_dep = dependency('cglm', fallback: ['cglm', 'cglm_dep'])

glslc = find_program('glslc')
glsclc_compile_to_spv_vertex = [glslc, '-fshader-stage=vertex', '@INPUT@', '-o', '@OUTPUT@']
glsclc_compile_to_spv_fragment = [glslc, '-fshader-stage=fragment', '@INPUT@', '-o', '@OUTPUT@']


vertex = custom_target('shader_vertex',
  output : 'vertex.spv',
  input : 'vertex.glsl',
  command : glsclc_compile_to_spv_vertex,
)

vertex_o = custom_target('shader_vertex_o',
  output: 'vertex.spv.o',
  input: vertex,
  command: objcopy_bin_to_o,
)

fragment = custom_target('shader_fragment',
  output : 'fragment.spv',
  input : 'fragment.glsl',
  command : glsclc_compile_to_spv_fragment,
)

fragment_o = custom_target('shader_fragment_o',
  output: 'fragment.spv.o',
  input: fragment,
  command: objcopy_bin_to_o,
)

srcs = ['gpufuntime', 'gpufuntime.c', vertex_o, fragment_o, mdl_cube_o]

executable(srcs,
           dependencies: [sdl3_dep, cglm_dep],
           include_directories: cgltf_incdir)
