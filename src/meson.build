#slangcomp = find_program('slangc')
# Slang compiler
#gen_src = custom_target('gen-output',
#                        input : ['simple.slang'],
#                        output : ['slang.spv'],
#                        command : [slangcomp, '@INPUT@',
#                        	   '-target','spirv',
#          			   '-profile','spirv_1_4+spvRayQueryKHR',
#          			    '-emit-spirv-directly',
#          			    '-fvk-use-entrypoint-name','-entry','vertMain','-entry','fragMain',
#                                   '-o', '@OUTPUT0@'])

# Blueprint compiler ( TODO: Blueprint needs -o output file ????)
#gen_src2 = custom_target('gen-output2',
#                       input : ['window-v2.blp'],
#                        output : ['window-v2.ui'],
#                        command : ['blueprint-compiler', 'compile' ,'@INPUT@',
#                                   '>', '@OUTPUT0@'])

sources = files(
'demo-vulkan-base-impl.h',
'demo-vulkan-base-impl.cpp',
'demo-vulkan-impl.h',
'demo-vulkan-impl.cpp',
'main.cpp'
)


# Set data
conf_data = configuration_data()
conf_data.set('version','1.2.3')
conf_data.set('build_dir',meson.current_build_dir())
configure_file(input : 'app-resource.xml.in',
  output : 'app-resource.xml',
  configuration : conf_data,
  )

asresources = gnome.compile_resources(
    'as-resources', meson.current_build_dir()+'/app-resource.xml',
    source_dir: meson.current_build_dir(),
    c_name: 'as',
    #dependencies: [gen_src]
    dependencies: files('slang.spv','style.css','window.ui')
)

executable('app',
  sources,
  asresources,
  dependencies: [
    dependency('gtk4'),
    dependency('libadwaita-1'),
    dependency('vulkan'),
    dependency('glm'),
    dependency('wayland-client')
  ],
  cpp_args:['-Wall','-Wextra','-std=gnu++23'],
  link_args:[],
  install: true,
)