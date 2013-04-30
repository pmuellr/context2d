{
  'targets': [
    {
      'target_name': 'CopySkiaAppDeps',
      'type': 'none',
      'dependencies': [
        'SampleApp.gyp:SampleApp',
        'bench.gyp:bench',
        'gm.gyp:gm',
        'tests.gyp:tests',
        'pathops_unittest.gyp:pathops_unittest',
        'tools.gyp:bench_pictures',
        'tools.gyp:render_pictures',
        'tools.gyp:render_pdfs',
        'tools.gyp:skimage',
      ],
      'variables': {
        'conditions': [
          [ 'skia_arch_type == "x86"', {
            'android_arch%': "x86",
          }, {
            'conditions': [
              [ 'armv7', {
                'android_arch%': "armeabi-v7a",
              }, {
               'android_arch%': "armeabi",
              }],
            ],
          }],
        ],
      },
      'copies': [
        # Copy gdbserver into the lib.target directory, so that it gets packaged
        # in the APK.  This is necessary for debugging.
        {
          'destination': '<(PRODUCT_DIR)/lib.target',
          'files': [
            '<(android_base)/toolchains/<(android_toolchain)/gdbserver',
          ],
        },
        # Copy all shared libraries into the Android app's libs folder.  Note
        # that this copy requires us to build SkiaAndroidApp after those
        # libraries, so that they exist by the time it occurs.  If there are no
        # libraries to copy, this will cause an error in Make, but the app will
        # still build.
        {
          'destination': '<(PRODUCT_DIR)/android/libs/<(android_arch)',
          'files': [
            '<(PRODUCT_DIR)/lib.target/libbench.so',
            '<(PRODUCT_DIR)/lib.target/libbench_pictures.so',
            '<(PRODUCT_DIR)/lib.target/libgm.so',
            '<(PRODUCT_DIR)/lib.target/librender_pdfs.so',
            '<(PRODUCT_DIR)/lib.target/librender_pictures.so',
            '<(PRODUCT_DIR)/lib.target/libSampleApp.so',
            '<(PRODUCT_DIR)/lib.target/libskimage.so',
            '<(PRODUCT_DIR)/lib.target/libtests.so',
            '<(PRODUCT_DIR)/lib.target/libpathops_unittest.so',
            '<(PRODUCT_DIR)/lib.target/gdbserver',
          ],
        },
      ],
    },
    {
      'target_name': 'skia_launcher',
      'type': 'executable',
      'sources': [
        '../launcher/skia_launcher.cpp',
      ],
    },
    {
      'target_name': 'SkiaAndroidApp',
      'type': 'none',
      'dependencies': [
        'CopySkiaAppDeps',
        'skia_launcher',
      ],
      'variables': {
         'ANDROID_SDK_ROOT': '<!(echo $ANDROID_SDK_ROOT)'
       },
      'actions': [
        {
          'action_name': 'SkiaAndroid_apk',
          'inputs': [
            '<(android_base)/app/AndroidManifest.xml',
            '<(android_base)/app/build.xml',
            '<(android_base)/app/project.properties',
            '<(android_base)/app/jni/com_skia_SkiaIntentService.h',
            '<(android_base)/app/jni/com_skia_SkiaIntentService.cpp',
            '<(android_base)/app/jni/com_skia_SkiaSampleRenderer.h',
            '<(android_base)/app/jni/com_skia_SkiaSampleRenderer.cpp',
            '<(android_base)/app/src/com/skia/SkiaReceiver.java',
            '<(android_base)/app/src/com/skia/SkiaIntentService.java',
            '<(android_base)/app/src/com/skia/SkiaSampleActivity.java',
            '<(android_base)/app/src/com/skia/SkiaSampleRenderer.java',
            '<(android_base)/app/src/com/skia/SkiaSampleView.java',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/../android/bin/SkiaAndroid.apk',
          ],
          'action': [
            'ant',
            '-f',
            '<(android_base)/app/build.xml',
            '-Dout.dir=<(PRODUCT_DIR)/android/bin',
            '-Dgen.absolute.dir=<(PRODUCT_DIR)/android/gen',
            '-Dnative.libs.absolute.dir=<(PRODUCT_DIR)/android/libs',
            '-Dout.final.file=<(PRODUCT_DIR)/android/bin/SkiaAndroid.apk',
            '-Dsdk.dir=<(ANDROID_SDK_ROOT)',
            'debug',
          ],
        },
      ],
    },
  ],
}
