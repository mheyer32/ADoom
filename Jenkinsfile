def notify(status){
	emailext (
		body: '$DEFAULT_CONTENT', 
		recipientProviders: [
			[$class: 'CulpritsRecipientProvider'],
			[$class: 'DevelopersRecipientProvider'],
			[$class: 'RequesterRecipientProvider']
		], 
		replyTo: '$DEFAULT_REPLYTO', 
		subject: '$DEFAULT_SUBJECT',
		to: '$DEFAULT_RECIPIENTS'
	)
}

def buildStep(ext) {
	sh "rm -rfv adoom_src/bin/*"

	dir("adoom_src") {
		sh "make clean && make all -j8"
	}

	if (!env.CHANGE_ID) {
		sh "mkdir -p publishing/deploy/adoom/$ext/"
		sh "mv adoom_src/bin/* publishing/deploy/adoom/$ext/"
	}
}

node {
	try{
		slackSend color: "good", message: "Build Started: ${env.JOB_NAME} #${env.BUILD_NUMBER} (<${env.BUILD_URL}|Open>)"
		
		stage('Checkout and pull') {
			properties([pipelineTriggers([githubPush()])])
			if (env.CHANGE_ID) {
				echo 'Trying to build pull request'
			}

			checkout scm
		}
	
		if (!env.CHANGE_ID) {
			stage('Generate publishing directories') {
				sh "rm -rfv publishing/deploy/*"
				sh "mkdir -p publishing/deploy/adoom"
			}
		}

		stage('Build AmigaOS 3.x version') {
			buildStep('68k')
		}

		stage('Deploying to stage') {
			if (env.TAG_NAME) {
				sh "echo $TAG_NAME > publishing/deploy/STABLE"
				sh "ssh $DEPLOYHOST mkdir -p public_html/downloads/releases/adoom/$TAG_NAME"
				sh "scp -r publishing/deploy/adoom/* $DEPLOYHOST:~/public_html/downloads/releases/adoom/$TAG_NAME/"
				sh "scp publishing/deploy/STABLE $DEPLOYHOST:~/public_html/downloads/releases/adoom/"
			} else if (env.BRANCH_NAME.equals('master')) {
				def deploy_url = sh (
				    script: 'echo "/downloads/nightly/adoom/`date +\'%Y\'`/`date +\'%m\'`/`date +\'%d\'`/"',
				    returnStdout: true
				).trim()
				sh "date +'%Y-%m-%d %H:%M:%S' > publishing/deploy/BUILDTIME"
				sh "ssh $DEPLOYHOST mkdir -p public_html/downloads/nightly/adoom/`date +'%Y'`/`date +'%m'`/`date +'%d'`/"
				sh "scp -r publishing/deploy/adoom/* $DEPLOYHOST:~/public_html/downloads/nightly/adoom/`date +'%Y'`/`date +'%m'`/`date +'%d'`/"
				sh "scp publishing/deploy/BUILDTIME $DEPLOYHOST:~/public_html/downloads/nightly/adoom/"
			}
			slackSend color: "good", message: "Build Succeeded: ${env.JOB_NAME}"
		}
	
	} catch(err) {
		slackSend color: "danger", message: "Build Failed: ${env.JOB_NAME} #${env.BUILD_NUMBER} (<${env.BUILD_URL}|Open>)"
		currentBuild.result = 'FAILURE'
		notify('Build failed')
		throw err
	}
}
