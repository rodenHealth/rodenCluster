/*
 * This file exposes the functions used to interact with Google Cloud Storage and
 * Google Speech API
 */


const Storage = require('@google-cloud/storage');
const storage = Storage();
const bucketName = "roden_audio";
const fileName = "test.flac";
const filePath = "./testUpload"



function processFile(fileName, filePath) {
	let bucketName = "roden_audio";
	uploadFile(bucketName, filePath);
	getSpeech(bucketName, fileName);
}

// Function that uploads a file from memory onto Google Cloud Storage
function uploadFile(bucketName, filePath) {
	storage
	.bucket(bucketName)
	.upload(filePath)
	.then(() => {
		console.log(`${filePath} uploaded to ${bucketName}.`);
	})
	.catch (err => {
		console.error('ERROR:', err);
	});
}

// Function that makes a request for a file in Google Cloud Storage
// to be processed by Google Speech Api.
function getSpeech(bucketName, fileName)
{
	const speech = require('@google-cloud/speech');
	const gcsUri = 'gs://' + bucketName + '/' + fileName;
	const client = new speech.SpeechClient();
	const encoding = 'FLAC';
	const sampleHertz = 16000;
	const languageCode = 'en-US';

	const config = {
		encoding: encoding,
		sampleRateHertz: sampleHertz,
		languageCode: languageCode,
	};
	const audio = {
		uri: gcsUri,
	};
	const request = {
		config: config,
		audio: audio,
	};

	client
  .longRunningRecognize(request)
  .then(data => {
    const operation = data[0];
    // Get a Promise representation of the final result of the job
    return operation.promise();
  })
  .then(data => {
    const response = data[0];
    const transcription = response.results
      .map(result => result.alternatives[0].transcript)
      .join('\n');
    console.log(`Transcription: ${transcription}`);
  })
  .catch(err => {
    console.error('ERROR:', err);
  });
}

processFile(fileName, filePath);
