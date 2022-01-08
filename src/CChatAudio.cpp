#include "CChatAudio.h"

CChatAudio::CChatAudio()
{
	this->initialized  = false;
	this->lastError    = paNoError;
	this->sampleRate   = 0;
	this->recoding     = false;
	this->inputBuffer  = NULL;
	this->outputBuffer = NULL;
}

CChatAudio::~CChatAudio()
{
	if (this->initialized) {
		delete this->inputBuffer;
		delete this->outputBuffer;
		Pa_CloseStream(this->stream);
		Pa_Terminate();
	}
}

void CChatAudio::Initialize(int sampleRate, float bufferSeconds)
{
	PaDeviceIndex inputDevice;
	PaDeviceIndex outputDevice;

	if (this->initialized) {
		return;
	}

	lastError = Pa_Initialize();
	if (lastError != paNoError) goto ERROR_ON_INITIALIZE;

	inputDevice  = Pa_GetDefaultInputDevice();
	outputDevice = Pa_GetDefaultOutputDevice();

	// setting input/output parameter
	if (inputDevice != paNoDevice) {
		inputSetting.channelCount              = 2;
		inputSetting.device                    = Pa_GetDefaultInputDevice();
		inputSetting.sampleFormat              = paFloat32;
		inputSetting.suggestedLatency          = Pa_GetDeviceInfo(inputDevice)->defaultLowInputLatency;
		inputSetting.hostApiSpecificStreamInfo = NULL;
	}

	if (outputDevice != paNoDevice) {
		outputSetting.channelCount              = 2;
		outputSetting.device                    = Pa_GetDefaultOutputDevice();
		outputSetting.sampleFormat              = paFloat32;
		outputSetting.suggestedLatency          = Pa_GetDeviceInfo(outputDevice)->defaultLowOutputLatency;
		outputSetting.hostApiSpecificStreamInfo = NULL;
	}

	// open stream
	lastError = Pa_OpenStream(
		&this->stream,
		inputDevice  == paNoDevice ? NULL : &this->inputSetting,
		outputDevice == paNoDevice ? NULL : &this->outputSetting,
		sampleRate,
		200,
		paClipOff | paDitherOff,
		CChatAudio::AudioCallback,
		this
	);
	if (lastError != paNoError) goto ERROR_ON_PORT_AUDIO_INITIALIZE;

	lastError = Pa_StartStream(this->stream);
	if (lastError != paNoError) goto ERROR_ON_STREAM;

	this->sampleRate   = sampleRate;
	this->inputBuffer  = new CCircularQueue<float>((size_t)(sampleRate * bufferSeconds));
	this->outputBuffer = new CCircularQueue<float>((size_t)(sampleRate * bufferSeconds));
	this->initialized = true;
	return;

ERROR_ON_STREAM:
	Pa_CloseStream(this->stream);
ERROR_ON_PORT_AUDIO_INITIALIZE:
	Pa_Terminate();
ERROR_ON_INITIALIZE:
	return;
}

bool CChatAudio::IsInitialized()
{
	return this->initialized;
}

bool CChatAudio::IsRecoding()
{
	return this->recoding;
}

void CChatAudio::SetRecoding(bool recoding)
{
	this->recoding = recoding;
}

const char *CChatAudio::GetLastErrorText()
{
	return Pa_GetErrorText(this->lastError);
}

int CChatAudio::Read(size_t count, float * buffer)
{
	if (this->inputBuffer == NULL) {
		return 0;
	}
	return this->inputBuffer->Pop(count, buffer);
}

int CChatAudio::Write(size_t count, float * buffer)
{
	if (this->outputBuffer == NULL) {
		return 0;
	}
	return this->outputBuffer->Push(count, buffer);
}

int CChatAudio::AudioCallback(
	const void                     *input, 
	void                           *output, 
	unsigned long                   frameCount, 
	const PaStreamCallbackTimeInfo *timeInfo, 
	PaStreamCallbackFlags           statusFlags, 
	void                           *userData
) {
	CChatAudio *audio;
	size_t      available;

	audio = (CChatAudio*)userData;

	// Record Audio
	if (audio->recoding) {
		audio->inputBuffer->Push(frameCount * audio->inputSetting.channelCount, (float*)input);
	}

	// Play Audio
	available = min(frameCount * audio->outputSetting.channelCount, audio->outputBuffer->Count());
	if (available) {
		audio->outputBuffer->Pop(available, (float*)output);
	} else {
		memset(output, 0, sizeof(float) * audio->outputSetting.channelCount * frameCount);
	}

	return paContinue;
}
