#!groovy

def platforms = [
  [platform: 'linux', host: 'ArchLinux', python: 'python2' ],
  [platform: 'win32', host: 'Windows-7-Dev'],
  [platform: 'darwin', host: 'Yosemite-Dev']
]

def distexcludes = [
  'node-mifare/binding.gyp',
  'node-mifare/.git',
  'node-mifare/node_modules',
  'node-mifare/build'
]

def minexcludes = distexcludes + [
  'node-mifare/src',
  'node-mifare/test',
  'node-mifare/docs',
  'node-mifare/Jenkinsfile'
]

def nodejs_builds = [:]
def electron_builds = [:]

for (int i = 0; i < platforms.size(); i++) {
  def platform = platforms[i].get('platform')
  def host = platforms[i].get('host')
  def python = platforms[i].get('python', 'python')
  nodejs_builds[platform] = {
    node(host) {
      echo('Cleanup Workspace')
      deleteDir()

      sh 'mkdir -p node-mifare'
      dir('node-mifare') {
        echo 'Checkout SCM'
        checkout scm
        env.PYTHON = python
        env.PLATFORM = platform
        sh '''
          export OLDPATH="$PATH"
          for arch in x64 ia32 ; do
            for node in /c/nodejs/${arch}/* /opt/nodejs/${arch}/* ; do
              if [ ! -d ${node} ] ; then
                continue
              fi
              export PATH="${node}/bin:${node}:${OLDPATH}"
              export VER=$(basename ${node})
              npm install --release
              mkdir -p dist/node/${VER}/${PLATFORM}/${arch}/ || true
              cp -r build/Release/node_mifare.node dist/node/${VER}/${PLATFORM}/${arch}/
            done
          done
        '''
        stash includes: 'dist/**', name: "nodejs_${platform}"
      }
    }
  }
  electron_builds[platform] = {
    node(host) {
      dir('node-mifare') {
        env.PYTHON = python
        env.PLATFORM = platform
        sh '''
          export npm_config_disturl=https://atom.io/download/electron
          export npm_config_runtime=electron
          export npm_config_build_from_source=true
          export OLDPATH="$PATH"
          for arch in x64 ; do
            export NODEJS_VER=$(ls -1d /c/nodejs/${arch}/*/ /opt/nodejs/${arch}/*/ 2>/dev/null | tail -n1)
            if [ ! -d ${NODEJS_VER} ] ; then
              continue
            fi
            export PATH="${NODEJS_VER}/bin:${NODEJS_VER}:${OLDPATH}"
            for ELECTRON_VER in "1.4.14" ; do
              export npm_config_target=${ELECTRON_VER}
              export npm_config_arch=${arch}
              export npm_config_target_arch=${arch}
              HOME=~/.electron-gyp npm install --release
              mkdir -p dist/electron/${ELECTRON_VER}/${PLATFORM}/${arch}/ || true
              cp -r build/Release/node_mifare.node dist/electron/${ELECTRON_VER}/${PLATFORM}/${arch}/
            done
          done
        '''
        stash includes: 'dist/**', name: "electron_${platform}"
      }
    }
  }
}

stage('Build nodejs') {
  parallel nodejs_builds
}

stage('Build electron') {
  parallel electron_builds
}

stage('Bundle') {
  node('ArchLinux') {
    properties([pipelineTriggers([[$class: 'GitHubPushTrigger']])])
    dir('node-mifare') {
      unstash 'nodejs_win32'
      unstash 'nodejs_darwin'
      unstash 'electron_win32'
      unstash 'electron_darwin'
      sh 'cp binding.gyp binding.gyp.done'
    }
    sh "tar --exclude='${distexcludes.join("' --exclude='")}' -czf node-mifare-${BUILD_ID}.dist.tar.gz node-mifare"
    sh "tar --exclude='${minexcludes.join("' --exclude='")}' -czf node-mifare-${BUILD_ID}.dist.min.tar.gz node-mifare"
    archiveArtifacts artifacts: "node-mifare-*.tar.gz", fingerprint: true
    step([$class: 'WarningsPublisher', canComputeNew: false, canResolveRelativePaths: false, canRunOnFailed: true, defaultEncoding: '', excludePattern: '', healthy: '', includePattern: '', messagesPattern: '', unHealthy: ''])
  }
}
