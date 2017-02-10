#!groovy

def project = "node-mifare"
def binary = "node_mifare.node"

def platforms = [
  [platform: 'linux', host: 'trustydev', bear: 'bear'],
  [platform: 'win32', host: 'Windows-7-Dev'],
  [platform: 'darwin', host: 'Yosemite-Dev']
]

def distexcludes = [
  "${project}/binding.gyp",
  "${project}/.git",
  "${project}/node_modules",
  "${project}/build"
]

def minexcludes = distexcludes + [
  "${project}/src",
  "${project}/test",
  "${project}/docs",
  "${project}/Jenkinsfile"
]

def nodejs_builds = [:]
def electron_builds = [:]

for (int i = 0; i < platforms.size(); i++) {
  def platform = platforms[i].get('platform')
  def host = platforms[i].get('host')
  def python = platforms[i].get('python', 'python')
  def bear = platforms[i].get('bear', '')
  nodejs_builds[platform] = {
    node(host) {
      echo('Cleanup Workspace')
      deleteDir()

      sh "mkdir -p ${project}"
      dir(project) {
        echo 'Checkout SCM'
        checkout scm
        env.PYTHON = python
        env.PLATFORM = platform
        env.BINARY = binary
        env.BEAR = bear
        sh '''
          export OLDPATH="$PATH"
          for arch in x64 ia32 ; do
            for node in /c/nodejs/${arch}/* /opt/nodejs/${arch}/* ; do
              if [ ! -d ${node} ] ; then
                continue
              fi
              export PATH="${node}/bin:${node}:${OLDPATH}"
              export VER=$(basename ${node})
              V=1 ${BEAR} npm install --release
              mkdir -p dist/node/${VER}/${PLATFORM}/${arch}/ || true
              cp -r build/Release/${BINARY} dist/node/${VER}/${PLATFORM}/${arch}/
            done
          done
        '''
        stash includes: 'dist/**', name: "nodejs_${platform}"
      }
    }
  }
  electron_builds[platform] = {
    node(host) {
      dir(project) {
        env.PYTHON = python
        env.PLATFORM = platform
        env.BINARY = binary
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
            for ELECTRON_VER in "1.4.14" "1.6.0" ; do
              export npm_config_target=${ELECTRON_VER}
              export npm_config_arch=${arch}
              export npm_config_target_arch=${arch}
              HOME=~/.electron-gyp npm install --release
              mkdir -p dist/electron/${ELECTRON_VER}/${PLATFORM}/${arch}/ || true
              cp -r build/Release/${BINARY} dist/electron/${ELECTRON_VER}/${PLATFORM}/${arch}/
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
  node('trustydev') {
    properties([pipelineTriggers([[$class: 'GitHubPushTrigger']])])
    dir(project) {
      unstash 'nodejs_win32'
      unstash 'nodejs_darwin'
      unstash 'electron_win32'
      unstash 'electron_darwin'
      sh 'cp binding.gyp binding.gyp.done'
      //sh 'oclint-json-compilation-database'
    }
    sh "tar --exclude='${distexcludes.join("' --exclude='")}' -czf ${project}.dist.tar.gz ${project}"
    sh "tar --exclude='${minexcludes.join("' --exclude='")}' -czf ${project}.dist.min.tar.gz ${project}"
    archiveArtifacts artifacts: "${project}*.tar.gz", fingerprint: true
    //stash includes: '**', name: 'all'
  }
}
/*
stage('Linter') {
  node('master') {
    unstash 'all'
    dir(project) {
      //step([$class: 'PmdPublisher', canComputeNew: false, canRunOnFailed: true, defaultEncoding: '', healthy: '', pattern: '**/build/oclint.xml', unHealthy: ''])
      //step([$class: 'WarningsPublisher', canComputeNew: false, canRunOnFailed: true, consoleParsers: [[parserName: 'GNU Make + GNU C Compiler (gcc)']], defaultEncoding: '', excludePattern: '', healthy: '', includePattern: '', messagesPattern: '', unHealthy: ''])
    }
  }
}
*/
