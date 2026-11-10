project('slash', 'c', version: '0.0.1', default_options: ['warning_level=3', 'werror=true'])
add_project_arguments(['-Wstrict-prototypes', '-Wmissing-prototypes', '-Wmissing-declarations', '-Wcast-align', '-Wpointer-arith', '-Wshadow'], language: 'c')

slash_sources = files([
	'src/slash.c',
	'src/completer.c',
	'src/optparse.c',
	'src/slash_list.c',
	])

conf = configuration_data()
if meson.get_compiler('c').has_header('sys/termios.h')
	conf.set('SLASH_HAVE_TERMIOS_H', true)
endif

if meson.get_compiler('c').has_header('sys/select.h') and host_machine.system() == 'linux'
	conf.set('SLASH_HAVE_SELECT', true)
endif

if get_option('timestamp') == true
	conf.set('SLASH_TIMESTAMP', true)
endif

slash_config_h = configure_file(output: 'slash_config.h', configuration: conf)

slash_inc = include_directories('.', 'include')

dependencies = dependency('libc', fallback: ['picolibc', 'picolibc_dep'], default_options: ['default_library=static'], required: false)

slash_lib = library('slash',
	sources: [slash_sources, slash_config_h],
	include_directories : slash_inc,
	dependencies : dependencies,
	install : false,
)

slash_builtins_lib = library('slash_builtins',
	sources: files('src/builtins.c', 'src/run.c'),
	include_directories : slash_inc,
	dependencies : dependencies,
	install : false,
)

slash_builtins_dep = declare_dependency(
	include_directories : slash_inc,
	link_whole : slash_builtins_lib,
)

slash_dep = declare_dependency(
	include_directories : slash_inc,
	link_with : slash_lib,
)

if get_option('default_library') == 'static'
	slash_link_whole_dep = declare_dependency(
		include_directories : slash_inc,
		link_with : slash_lib,
		link_whole: [slash_lib]
	)
endif
