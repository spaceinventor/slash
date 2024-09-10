project('slash', 'c')

slash_sources = files([
	'src/slash.c',
	'src/completer.c',
	'src/optparse.c',
	'src/slash_list.c',
	'src/dflopt.c',
	'src/builtins.c',
	'src/run.c',
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

dependencies = dependency('libc', fallback: ['picolibc', 'picolibc_dep'], default_options: ['default_library=static'], required: false)

slash_lib = library('slash',
	sources: [slash_sources, slash_config_h],
	include_directories : slash_inc,
	dependencies : dependencies,
	c_args: ['-DSLASH_SECTION="slash"'],
	pic:true,
	install : true,
)
	
slash_dep = declare_dependency(
	include_directories : slash_inc,
	link_with : slash_lib
)

pkg = import('pkgconfig')
pkg.generate(slash_lib, filebase: 'slash')
install_subdir('include', install_dir : '.')
install_headers(slash_config_h, install_dir : 'include')

if not meson.is_subproject()
	subdir('test')
endif
