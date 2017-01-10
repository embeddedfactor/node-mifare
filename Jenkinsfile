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
          export OLDPATH="$PATH"
          for arch in "x64" "x86" ; do
            for node in /opt/nodejs/${arch}/* ; do
              export PATH="${node}/bin:${OLDPATH}"
              export VER=$(basename ${node})
              npm install --release
              mkdir -p dist/${VER}/linux/${arch}/ || true
              cp -r build/${type}/node_mifare.node dist/${VER}/linux/${arch}/
              done
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
            cp -r build/${type}/node_mifare.node dist/${VER}/win32/${arch}/
            done
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
            cp -r build/${type}/node_mifare.node dist/${VER}/darwin/${arch}/
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
      sh 'cp bindings.gyp bindings.gyp.done'
    }
    sh "tar --exclude='./binding.gyp' --exclude='./.git '--exclude='./node_modules' --exclude='./build' -czf node-mifare-${CHANGE_ID}-.dist.tar.gz"
    sh "tar --exclude='./binding.gyp' --exclude='./.git '--exclude='./node_modules' --exclude='./build' --exclude='./src' --exclude='./test' --exclude='./docs' --exclude='./Jenkinsfile' -czf node-mifare-${CHANGE_ID}-.dist.min.tar.gz"
    archiveArtifacts artifacts: "node-mifare-${CHANGE_ID}-*.tar.gz", fingerprint: true
  }
}
