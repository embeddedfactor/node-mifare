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
              for type in "Debug" "Release" ; do
                if [ "$VER" = "v0.10.26" ] ; then
                  export PYTHON=python2
                fi
                npm install --${type,,}
                mkdir -p dist || true
                cp -r build/${type}/node_mifare.node dist/node_mifare-${VER}-linux-${arch}-${type,,}.node
              done
            done
          done
        '''
        //dir('dist') {
        //  archiveArtifacts artifacts: '**', fingerprint: true
        //  stash includes: '**', name: 'linux'
        //}
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
            export PATH="${node}:${OLDPATH}"
            export VER=$(basename ${node})
            for type in "Debug" "Release" ; do
              npm install --${type,,}
              mkdir -p dist || true
              cp -r build/${type}/node_mifare.node dist/node_mifare-${VER}-win-${arch}-${type,,}.node
            done
          done
        done
      '''
      dir('dist') {
        //archiveArtifacts artifacts: '**', fingerprint: true
        stash includes: '**', name: 'windows'
      }
    }
  }, macos: {
    node('Yosemite-Dev') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      sh '''
        export OLDPATH="$PATH"
        for arch in "x64" "x86" ; do
          for node in /opt/nodejs/${arch}/* ; do
            export PATH="${node}/bin:${OLDPATH}"
            export VER=$(basename ${node})
            for type in "debug" "release" ; do
              npm install --${type}
              mkdir -p dist || true
              cp -r build/${type}/node_mifare.node dist/node_mifare-${VER}-darwin-${arch}-${type}.node
            done
          done
        done
      '''
      dir('dist') {
        //archiveArtifacts artifacts: '**', fingerprint: true
        stash includes: '**', name: 'macos'
      }
    }
  }
}
stage('Bundle') {
  node('ArchLinux') {
    dir('node-mifare/dist') {
      unstash 'windows'
      unstash 'macos'
    }
    sh "tar -czf node-mifare-${CHANGE_ID}-.dist.tar.gz"
    archiveArtifacts artifacts: "node-mifare-${CHANGE_ID}-.dist.tar.gz", fingerprint: true
  }
}
