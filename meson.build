project('gpufuntime', 'c')

sdl3_dep = dependency('sdl3', fallback: ['sdl3', 'sdl3_dep'])
cglm_dep = dependency('cglm', fallback: ['cglm', 'cglm_dep'])

glslc = find_program('glslc')

vertex = custom_target('shader_vertex',
  output : 'vertex.spv',
  input : 'vertex.glsl',
  command : [glslc, '-fshader-stage=vertex', '@INPUT@', '-o', '@OUTPUT@'],
  build_always: true
)

fragment = custom_target('shader_fragment',
  output : 'fragment.spv',
  input : 'fragment.glsl',
  command : [glslc, '-fshader-stage=fragment', '@INPUT@', '-o', '@OUTPUT@'],
  build_always: true
)

executable('gpufuntime', 'gpufuntime.c',
            dependencies: [sdl3_dep, cglm_dep])
