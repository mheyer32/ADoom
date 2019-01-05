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
	sh "rm -rfv adoom_src/bin"

	dir("adoom_src") {
		sh "make -j8"
	}

	if (!env.CHANGE_ID) {
		sh "mv adoom_src/bin/ADoom publishing/deploy/adoom/adoom.$ext"
		//sh "cp publishing/amiga-spec/adoom.info publishing/deploy/adoom/adoom.$ext.info"
	}
}

node {
	try{
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
				sh "scp publishing/deploy/adoom/* $DEPLOYHOST:~/public_html/downloads/releases/adoom/$TAG_NAME/"
				sh "scp publishing/deploy/STABLE $DEPLOYHOST:~/public_html/downloads/releases/adoom/"
			} else if (env.BRANCH_NAME.equals('master')) {
				sh "date +'%Y-%m-%d %H:%M:%S' > publishing/deploy/BUILDTIME"
				sh "ssh $DEPLOYHOST mkdir -p public_html/downloads/nightly/adoom/`date +'%Y'`/`date +'%m'`/`date +'%d'`/"
				sh "scp publishing/deploy/adoom/* $DEPLOYHOST:~/public_html/downloads/nightly/adoom/`date +'%Y'`/`date +'%m'`/`date +'%d'`/"
				sh "scp publishing/deploy/BUILDTIME $DEPLOYHOST:~/public_html/downloads/nightly/adoom/"
			}
		}
	
	} catch(err) {
		currentBuild.result = 'FAILURE'
		notify('Build failed')
		throw err
	}
}
