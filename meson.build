project('logik', 'cpp', default_options: ['cpp_std=c++20'])

files=[
    'src/main.cpp'
]

x11_dep=dependency('x11')
xft_dep=dependency('xft')

dependencies=[
    x11_dep,
    xft_dep
]

executable('logik', files, dependencies: dependencies)