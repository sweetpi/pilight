{
  "targets": [
   {
   		"target_name": "pilight",
   		'type': 'static_library',
        'defines': [
          'NODEJS_MODULE',
          'UPDATE',
          'FIRMWARE',
        ],
        "include_dirs": [
            'libs/pilight',
            'libs/avrdude'
        ],
        'cflags_cc': [ '-Wfloat-equal -Wpointer-arith -Wcast-align -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wformat=2 -g -Wall -Wconversion -Wunreachable-code -Wno-conversion -Wno-sign-compare' ],
        'cflags_cc-': [
          '-Wsign-compare'
        ],
        "sources": [ 
          '<!@(ls -1 libs/hardware/*.c)',
          '<!@(ls -1 libs/pilight/*.c|sed "s/webserver.c//g")',
          '<!@(ls -1 libs/protocols/*.c)',
          '<!@(ls -1 libs/avrdude/*.c)',
          "daemon.c",
        ],
        'sources!': ['webserver.c'],
        'libraries': []
    },
    {
        "target_name": "pilite",
        'defines': [
            'NODEJS_MODULE',
        ],
        'dependencies': ['pilight'],
        "libraries": [ "./Release/pilight.a", "-lz" ],
        "sources": [ "module.cc" ]
    }
  ]
}