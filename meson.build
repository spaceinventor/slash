project('slash', 'c')

slash_sources = [
	'src/slash.c',
	'src/slash_csp.c',
	'src/base16.c',
	]

slash_deps = []
slash_deps += dependency('libcsp', fallback : ['csp', 'libcsp_dep'])

have_termios = meson.get_compiler('c').has_header('termios.h')
slash_c_args = []
if have_termios
	slash_c_args += '-DHAVE_TERMIOS_H'
endif

slash_inc = include_directories('include')

slash = static_library('slash',
	slash_sources,
	include_directories : slash_inc,
	dependencies : slash_deps,
	c_args : slash_c_args, 
	install: true
	)
	
slash_dep = declare_dependency(
	include_directories : slash_inc,
	dependencies : dependency('libcsp', fallback : ['csp', 'libcsp_dep']),
	link_with: slash,
	)
