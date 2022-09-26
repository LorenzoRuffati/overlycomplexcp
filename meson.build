project('overlycomplex_cp', 'c',
    version: '0.1',
    license: 'MIT')

openssl = dependency('openssl', static: true)
subdir('src')
executable('ocp', sources: sources, dependencies: openssl, install: true, link_args: '-lrt')
