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
install_subdir('include', install_dir : '.')
install_headers(slash_config_h, install_dir : 'include/slash')

slash_inc = include_directories('.', 'include')
	
slash_dep = declare_dependency(
	sources: [slash_sources, slash_config_h],
	include_directories : slash_inc,
	)
