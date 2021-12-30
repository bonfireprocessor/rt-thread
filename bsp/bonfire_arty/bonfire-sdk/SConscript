# RT-Thread building script for component

Import('rtconfig')
Import('RTT_ROOT')
from building import *

cwd = GetCurrentDir()
src = [ 'src/uart.c', 
         'src/console.c'  ]
        
CPPPATH = [ cwd + '/bonfire-sdk/src/' ]

CPPDEFINES = []

group = DefineGroup('Libraries', src, depend = [''], CPPPATH = CPPPATH, CPPDEFINES=CPPDEFINES)

Return('group')
