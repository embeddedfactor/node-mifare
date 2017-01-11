#!groovy

stage("Build") {
  parallel linux: {
    node('ArchLinux') {
      echo 'Cleanup Workspace'
      deleteDir()
      sh 'mkdir -p node-mifare'
      dir('node-mifare') {
        echo 'Checkout SCM'
        properties([pipelineTriggers([[$class: 'GitHubPushTrigger']])])
        checkout scm
        sh '''
          export PYTHON=python2
          export OLDPATH="$PATH"
          for arch in "x64" ; do  # No Versions in "x86" ; do
            for node in /opt/nodejs/${arch}/* ; do
              export PATH="${node}/bin:${OLDPATH}"
              export VER=$(basename ${node})
              npm install --release
              mkdir -p dist/${VER}/linux/${arch}/ || true
              cp -r build/Release/node_mifare.node dist/${VER}/linux/${arch}/
            done
          done
        '''
      }
    }
  }, win32: {
    node('Windows-7-Dev') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      sh '''
        export OLDPATH="$PATH"
        for arch in "x64" "x86" ; do
          for node in /c/nodejs/${arch}/* ; do
            export PATH="${node}:${OLDPATH}"
            export VER=$(basename ${node})
            npm install --release
            mkdir -p dist/${VER}/win32/${arch}/ || true
            cp -r build/Release/node_mifare.node dist/${VER}/win32/${arch}/
          done
        done
      '''
      stash includes: 'dist/**', name: 'win32'
    }
  }, macos: {
    node('Yosemite-Dev') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      sh '''
        export OLDPATH="${PATH}"
        for arch in "x64" "x86" ; do
          for node in /opt/nodejs/${arch}/* ; do
            export PATH="${node}/bin:${OLDPATH}"
            export VER=$(basename ${node})
            npm install --release
            mkdir -p dist/${VER}/darwin/${arch}/ || true
            cp -r build/Release/node_mifare.node dist/${VER}/darwin/${arch}/
          done
        done
      '''
      stash includes: 'dist/**', name: 'darwin'
    }
  }
}
stage('Bundle') {
  node('ArchLinux') {
    dir('node-mifare') {
      unstash 'win32'
      unstash 'darwin'
      sh 'cp binding.gyp binding.gyp.done'
    }
    sh "tar --exclude='node-mifare/binding.gyp' --exclude='node-mifare/.git '--exclude='node-mifare/node_modules' --exclude='node-mifare/build' -czf node-mifare-$(date "+%Y-%m-%d-%H-%M")-.dist.tar.gz node-mifare"
    sh "tar --exclude='node-mifare/binding.gyp' --exclude='node-mifare/.git '--exclude='node-mifare/node_modules' --exclude='node-mifare/build' --exclude='node-mifare/src' --exclude='node-mifare/test' --exclude='node-mifare/docs' --exclude='node-mifare/Jenkinsfile' -czf node-mifare-$(date "+%Y-%m-%d-%H-%M")-.dist.min.tar.gz node-mifare"
    archiveArtifacts artifacts: "node-mifare-*.tar.gz", fingerprint: true
  }
}
