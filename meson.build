project('slash', 'c')

slash_sources = files([
	'src/slash.c',
	])

conf = configuration_data()
if meson.get_compiler('c').has_header('sys/termios.h')
	conf.set('SLASH_HAVE_TERMIOS_H', true)
endif

if meson.get_compiler('c').has_header('sys/select.h') and host_machine.system() == 'linux'
	conf.set('SLASH_HAVE_SELECT', true)
endif

slash_config_h = configure_file(output: 'slash_config.h', configuration: conf)

slash_inc = include_directories('.', 'include')

dependencies = dependency('libc', fallback: ['picolibc', 'picolibc_dep'], required: false)
	
slash_lib = static_library('slash',
	sources: [slash_sources, slash_config_h],
	include_directories : slash_inc,
	dependencies : dependencies,
)
	
slash_dep = declare_dependency(
	include_directories : slash_inc,
	link_with : slash_lib
)
