project('overlycomplex_cp', 'c',
    version: '0.1',
    license: 'MIT')

openssl = dependency('openssl', static: true)
subdir('src')
executable('ocp', sources: sources, dependencies: openssl)
executable('test_mmap', sources: 'src/test_mmap.c')
