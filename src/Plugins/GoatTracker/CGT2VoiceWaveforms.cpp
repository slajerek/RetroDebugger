#include "CGT2VoiceWaveforms.h"
#include "CWaveformData.h"
#include "DBG_Log.h"

CWaveformData *gt2VoiceWaveform[3] = { nullptr, nullptr, nullptr };
CWaveformData *gt2MixWaveform      = nullptr;

void GT2_VoiceWaveforms_Create(int sampleWindow)
{
	if (sampleWindow < 16) sampleWindow = 16;
	for (int i = 0; i < 3; i++)
	{
		if (gt2VoiceWaveform[i] == nullptr)
			gt2VoiceWaveform[i] = new CWaveformData(sampleWindow);
	}
	if (gt2MixWaveform == nullptr)
		gt2MixWaveform = new CWaveformData(sampleWindow);
}

void GT2_VoiceWaveforms_Destroy()
{
	for (int i = 0; i < 3; i++)
	{
		delete gt2VoiceWaveform[i];
		gt2VoiceWaveform[i] = nullptr;
	}
	delete gt2MixWaveform;
	gt2MixWaveform = nullptr;
}

void GT2_VoiceWaveforms_UpdatePerFrame()
{
	// Snapshot the producer side of the rolling buffer to the renderer
	// side and find the rising-edge trigger — the same two-step the
	// VICE-backed waveforms run every frame. Without these the
	// oscilloscope display would be stale / unsynchronised.
	for (int i = 0; i < 3; i++)
	{
		if (gt2VoiceWaveform[i])
		{
			gt2VoiceWaveform[i]->CopySampleData();
			gt2VoiceWaveform[i]->CalculateTriggerPos();
		}
	}
	if (gt2MixWaveform)
	{
		gt2MixWaveform->CopySampleData();
		gt2MixWaveform->CalculateTriggerPos();
	}
}

extern "C" void c64d_gt2_capture_voice_samples(int v0, int v1, int v2, short mix)
{
	// Scale the SID Voice::output() value (≈ wave_dac − wave_zero) × env_dac,
	// max magnitude ~522240 — into a signed-16 range so it sits cleanly in
	// the CWaveformData sample window without saturation for any reasonable
	// instrument volume. Divide by 16 keeps the same scaling the existing
	// VU-meter level math uses.
	if (gt2VoiceWaveform[0]) gt2VoiceWaveform[0]->AddSample((short)(v0 / 16));
	if (gt2VoiceWaveform[1]) gt2VoiceWaveform[1]->AddSample((short)(v1 / 16));
	if (gt2VoiceWaveform[2]) gt2VoiceWaveform[2]->AddSample((short)(v2 / 16));
	if (gt2MixWaveform)      gt2MixWaveform->AddSample(mix);
}
