sources = files('main.c')
subdir('shared')
subdir('pipe')
subdir('shm')
xxd = find_program('xxd')

helpstr_h = custom_target(
    'helpstr_h',
    output : 'helpstr.h',
    input : 'help_str.txt',
    command : [xxd, '-i', '@INPUT@', '@OUTPUT@'],
)

lib_help = static_library('helpstr', [helpstr_h])