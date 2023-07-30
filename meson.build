project(
	'wayfire-uberzugpp',
    'c',
	'cpp',
	version: '0.1',
	license: 'MIT',
	meson_version: '>=0.51.0',
	default_options: [
		'cpp_std=c++17',
        'c_std=c11',
		'warning_level=2',
		'werror=false',
	],
)

wayfire = dependency('wayfire')
wlroots = dependency('wlroots')
wfconfig = dependency('wf-config')
wayland_protos = dependency('wayland-protocols')

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: ['cpp', 'c'])
add_project_arguments(['-DWAYFIRE_PLUGIN'], language: ['cpp', 'c'])
add_project_link_arguments(['-rdynamic'], language:'cpp')

# Build xdg-shell protocol
wl_protocol_dir = wayland_protos.get_variable(pkgconfig: 'pkgdatadir')
wayland_scanner = find_program('wayland-scanner', native: true)

wayland_scanner_server = generator(wayland_scanner, output: '@BASENAME@-protocol.h', arguments: ['server-header', '@INPUT@', '@OUTPUT@'])

sources = ['uberzug.cpp']
sources += wayland_scanner_server.process(join_paths(wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'))

uberzug = shared_module('uberzug', sources, dependencies: [wayfire, wlroots],
    install: true, install_dir: wayfire.get_variable(pkgconfig: 'plugindir'))
