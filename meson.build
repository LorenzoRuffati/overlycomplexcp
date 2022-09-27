project('overlycomplex_cp', 'c',
    version: '0.1',
    license: 'MIT')

openssl = dependency('openssl', static: true)
subdir('src')
executable('ocp', sources: sources + [helpstr_h], link_with: lib_help, dependencies: openssl, link_args: '-lrt')
executable('test_mmap', sources: 'src/test_mmap.c', link_args: '-lrt')
