project('CamTest','c','cpp',
    default_options :[
        'werror=false',
        'cpp_std=c++17',
    ]
)
qt5 = import('qt5')
thread_dep = dependency('threads')

q_moc_headers = files(
    ['main.h']
)

qt5_dep = dependency('qt5',
                     method : 'pkg-config',
                     modules : ['Core', 'Gui', 'Widgets'])

qtRes = qt5.preprocess(
    moc_headers : q_moc_headers,
    dependencies : qt5_dep
)

qt5_cpp_args = ['-DQT_NO_KEYWORDS','-g']
src_files = files(['main.cpp'])
dep = dependency('libcamera',required : true)
cam = executable('CamTest',src_files,qtRes,dependencies : [dep,qt5_dep,thread_dep],install:true,cpp_args: qt5_cpp_args)