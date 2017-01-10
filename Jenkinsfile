#!groovy

stage("Build") {
  parallel linux: {
    node('ArchLinux') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      sh '''#!/bin/bash
        export OLDPATH="$PATH"
        for arch in "x64" "x86" ; do
          for node in /opt/nodejs/${arch}/* ; do
            export PATH="${node}/bin:${OLDPATH}"
            export VER=$(basename ${node})
            for type in "Debug" "Release" ; do
              if [ "$VER" = "v0.10.24" ] ; then
                export PYTHON=python2
              fi
              npm install --${type,,}
              mkdir -p precomp || true
              cp -r build/${type}/node_mifare.node precomp/node_mifare-${VER}-linux-${arch}-${type,,}.node
            done
          done
        done
      '''
      dir('precomp') {
        archiveArtifacts artifacts: '**', fingerprint: true
      }
    }
  }, windows: {
    node('Windows-7-Dev') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      sh '''
        export OLDPATH="$PATH"
        for arch in "x64" "x86" ; do
          for node in /c/nodejs/${arch}/* ; do
            export PATH="${node}/bin:${OLDPATH}"
            export VER=$(basename ${node})
            for type in "Debug" "Release" ; do
              #if [ "$VER" = "v0.10.24" ] ; then
              #  export PYTHON=python2
              #fi
              npm install --${type,,}
              mkdir -p precomp || true
              cp -r build/${type}/node_mifare.node precomp/node_mifare-${VER}-win-${arch}-${type,,}.node
            done
          done
        done
      '''
      dir('precomp') {
        archiveArtifacts artifacts: '**', fingerprint: true
      }
    }
  }, macos: {
    node('Yosemite-Dev') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      sh '''#!/bin/bash
        export OLDPATH="$PATH"
        for arch in "x64" "x86" ; do
          for node in /opt/nodejs/${arch}/* ; do
            export PATH="${node}/bin:${OLDPATH}"
            export VER=$(basename ${node})
            for type in "debug" "release" ; do
              #if [ "$VER" = "v0.10.24" ] ; then
              #  export PYTHON=python2
              #fi
              npm install --${type}
              mkdir -p precomp || true
              cp -r build/${type}/node_mifare.node precomp/node_mifare-${VER}-darwin-${arch}-${type}.node
            done
          done
        done
      '''
      dir('precomp') {
        archiveArtifacts artifacts: '**', fingerprint: true
      }
    }
  }
}
