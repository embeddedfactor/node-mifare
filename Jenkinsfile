stage("Build") {
  parallel linux: {
    node('ArchLinux') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      echo 'Build Debug via NPM'
      npm install --debug
      echo 'Build Release via NPM'
      npm install
      archiveArtifacts artifacts: 'build/**/node_mifare.node', fingerprint: true
    }
  }, windows: {
    node('Windows-7-Dev') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      echo 'Build Debug via NPM'
      npm install --debug
      echo 'Build Release via NPM'
      npm install
      archiveArtifacts artifacts: 'build/**/node_mifare.node', fingerprint: true
    }
  }, macos: {
    node('Yosemite-Dev') {
      echo 'Cleanup Workspace'
      deleteDir()
      echo 'Checkout SCM'
      checkout scm
      echo 'Build Debug via NPM'
      npm install --debug
      echo 'Build Release via NPM'
      npm install
      archiveArtifacts artifacts: 'build/**/node_mifare.node', fingerprint: true
    }
  }
}
