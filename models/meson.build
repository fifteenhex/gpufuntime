openscad = find_program('openscad')
assimp = find_program('assimp')

openscad_to_off = [openscad, '-o', '@OUTPUT@', '@INPUT@']
assimp_to_gltf = [assimp, 'export', '@INPUT@', '@OUTPUT@']

mdl_cube = custom_target('mdl_cube',
  output: 'cube.off',
  input: 'cube.scad',
  command: openscad_to_off,
)

mdl_cube_glb = custom_target('mdl_cube_glb',
  output: 'cube.glb',
  input: mdl_cube,
  command: assimp_to_gltf,
)

mdl_cube_o = custom_target('mdl_cube_o',
  output: 'cube.o',
  input: mdl_cube_glb,
  command: objcopy_bin_to_o,
)

mdl_donut = custom_target('mdl_donut',
  output: 'donut.off',
  input: 'donut.scad',
  command: openscad_to_off,
)

mdl_donut_glb = custom_target('mdl_donut_glb',
  output: 'donut.glb',
  input: mdl_donut,
  command: assimp_to_gltf,
)

mdl_donut_o = custom_target('mdl_donut_o',
  output: 'donut.o',
  input: mdl_donut_glb,
  command: objcopy_bin_to_o,
)
