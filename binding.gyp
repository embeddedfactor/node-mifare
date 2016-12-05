{
  "variables": {
    "source_dir": "src",
  },
  "targets": [
    {
      "target_name": "node_mifare",
      "dependencies": ["node_modules/libfreefare-pcsc/binding.gyp:freefare_pcsc"],
      "conditions": [
        ['OS=="linux"', {
          "variables": { "ARCH%": "<!(uname -m | grep ^arm && echo arm || /bin/true)", },
          "conditions": [
            ['ARCH=="arm"', {
              "defines": [
                "USE_LIBNFC",
              ],
            }],
          ],
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11','-stdlib=libc++'],
            'OTHER_LDFLAGS': ['-stdlib=libc++'],
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
          }
        }]
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "<(source_dir)"
      ],
      "sources": [
        "src/mifare.cc",
        "src/reader.cc",
        "src/desfire.cc",
        "src/utils.cc"
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1
        }
      },
      "cflags": [
        "-std=c++11",
        '-stdlib=libc++',
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
        "-fPIC",
        "-fno-strict-aliasing",
        "-pedantic"
      ],
      "cflags_cc": [
        "-std=c++11",
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
        "-fPIC",
        "-fno-strict-aliasing",
        "-pedantic"
      ],
      "cflags!": [ '-fno-exceptions' ],
      "cflags_cc!": [ '-fno-exceptions' ],
    }
  ]
}
