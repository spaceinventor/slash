project('slash', 'c')

slash_sources = files([
	'src/slash.c',
	'src/base16.c',
	])

slash_deps = []
csp_dep = dependency('libcsp', fallback : ['csp', 'libcsp_dep'], required: false)
if csp_dep.found()
	slash_deps += csp_dep
	slash_sources += 'src/slash_csp.c'
endif

conf = configuration_data()
if meson.get_compiler('c').has_header('termios.h')
	conf.set('HAVE_TERMIOS_H', true)
endif

slash_config_h = configure_file(output: 'slash_config.h', configuration: conf)

slash_inc = include_directories('.', 'include')

slash = static_library('slash',
	slash_sources,
	include_directories : slash_inc,
	dependencies : slash_deps,
	install: true
	)
	
slash_dep = declare_dependency(
	sources: slash_config_h,
	include_directories : slash_inc,
	dependencies : slash_deps,
	link_with: slash,
	)