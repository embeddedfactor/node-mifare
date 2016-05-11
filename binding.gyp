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
      "cflags": [
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",
        "-fPIC",
        "-fno-strict-aliasing",
        "-fno-exceptions",
        "-pedantic"
      ],
    }
  ]
}
