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
