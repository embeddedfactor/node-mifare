#!groovy

def platforms = [
  [platform: 'linux', host: 'ArchLinux', python: 'python2', arch:'x64' ],
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

def builds = [:]

for (int i = 0; i < platforms.size(); i++) {
  def platform = platforms[i].get('platform')
  def host = platforms[i].get('host')
  def python = platforms[i].get('python', 'python')
  def arch = platforms[i].get('arch', 'x64 x86')
  builds[platform] = {
    node(host) {
      env.PYTHON = python
      env.PLATFORM = platform
      env.ARCH = arch

      echo('Cleanup Workspace')
      deleteDir()
      sh 'mkdir -p node-mifare'
      dir('node-mifare') {
        echo 'Checkout SCM'
        checkout scm
        sh '''
          export OLDPATH="$PATH"
          for arch in ${ARCH} ; do
            for node in /opt/nodejs/${arch}/* ; do
              export PATH="${node}/bin:${OLDPATH}"
              export VER=$(basename ${node})
              npm install --release
              mkdir -p dist/${VER}/linux/${arch}/ || true
              cp -r build/Release/node_mifare.node dist/${VER}/${PLATFORM}/${arch}/
            done
          done
        '''
        stash includes: 'dist/**', name: platform
      }
    }
  }
}

stage('Build') {
  parallel builds
}

stage('Bundle') {
  node('ArchLinux') {
    properties([pipelineTriggers([[$class: 'GitHubPushTrigger']])])
    dir('node-mifare') {
      unstash 'win32'
      unstash 'darwin'
      sh 'cp binding.gyp binding.gyp.done'
    }
    env.DATETIME = sh(shell: 'date "+%Y-%m-%d-%H-%M"', returnStdout: true).trim()
    sh "tar --exclude='${distexcludes.join("' --exclude='")}' -czf node-mifare-${DATETIME}.dist.tar.gz node-mifare"
    sh "tar --exclude='${minexcludes.join("' --exclude='")}' -czf node-mifare-${DATETIME}.dist.min.tar.gz node-mifare"
    archiveArtifacts artifacts: "node-mifare-*.tar.gz", fingerprint: true
  }
}
