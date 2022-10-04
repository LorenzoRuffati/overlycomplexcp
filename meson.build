project('overlycomplex_cp', 'c',
    version: '0.1',
    license: 'MIT')

openssl = dependency('openssl', static: true)
subdir('src')
executable('ocp', sources: sources + [helpstr_h], link_with: lib_help, install: true, link_args: '-lrt')
