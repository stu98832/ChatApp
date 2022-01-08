#pragma once
#include <portaudio.h>
#include "CCircularQueue.h"

class CChatAudio {
private:
	PaStream              *stream;
	PaStreamParameters     inputSetting;
	PaStreamParameters     outputSetting;
	PaError                lastError;
	bool                   recoding;
	CCircularQueue<float> *outputBuffer;
	CCircularQueue<float> *inputBuffer;
	int                    sampleRate;
	bool                   initialized;

public:
	CChatAudio();
	~CChatAudio();

	void Initialize(int sampleRate, float buffedSenonds);
	bool IsInitialized();
	bool IsRecoding();
	void SetRecoding(bool recoding);
	const char *GetLastErrorText();

	int Read(size_t count, float *buffer);
	int Write(size_t count, float *buffer);

	static int AudioCallback(
		const void                     *input,
		void                           *output,
		unsigned long                   frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags           statusFlags,
		void                           *userData
	);
};