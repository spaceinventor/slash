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

if get_option('timestamp') == true
	conf.set('SLASH_TIMESTAMP', true)
endif

slash_config_h = configure_file(output: 'slash_config.h', configuration: conf)

slash_inc = include_directories('.', 'include')

dependencies = dependency('libc', fallback: ['picolibc', 'picolibc_dep'], default_options: ['default_library=static'], required: false)

do_install = not meson.is_subproject()

slash_lib = both_libraries('slash',
	sources: [slash_sources, slash_config_h],
	include_directories : slash_inc,
	dependencies : dependencies,
	install : do_install,
)
	
slash_shared_dep = declare_dependency(
	include_directories : slash_inc,
	link_with : slash_lib.get_shared_lib(),
)

slash_dep = declare_dependency(
	include_directories : slash_inc,
	link_with : slash_lib.get_static_lib(),
)

if do_install
	pkg = import('pkgconfig')
	pkg.generate(slash_lib, filebase: 'slash')
	install_subdir('include', install_dir : '.')
	install_headers(slash_config_h, install_dir : 'include')
endif

