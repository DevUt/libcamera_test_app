project('CamTest','c','cpp',
    default_options :[
        'werror=false',
        'cpp_std=c++17',
    ]
)
qt5 = import('qt5')
thread_dep = dependency('threads')
qt5_dep = dependency('qt5',
                     method : 'pkg-config',
                     modules : ['Core', 'Gui', 'Widgets'])

qt5_cpp_args = ['-DQT_NO_KEYWORDS','-g']
src_files = files(['main.cpp'])
dep = dependency('libcamera',required : true)
cam = executable('CamTest',src_files,dependencies : [dep,qt5_dep,thread_dep],install:true,cpp_args: qt5_cpp_args)