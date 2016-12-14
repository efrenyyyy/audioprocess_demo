#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/audio_buffer.h"
#include "webrtc/modules/audio_conference_mixer/include/audio_conference_mixer.h"
#include "webrtc/modules/audio_conference_mixer/source/audio_conference_mixer_impl.h"
#include "webrtc/modules/include/module_common_types.h"
#include "webrtc/modules/utility/include/audio_frame_operations.h"

void InitAudioProcessing(webrtc::AudioProcessing *apm)
{
	apm->level_estimator()->Enable(true);

	apm->echo_cancellation()->Enable(true);

	apm->echo_cancellation()->enable_metrics(true);

	apm->echo_cancellation()->enable_drift_compensation(true); //为TRUE时必须调用set_stream_drift_samples

	apm->gain_control()->Enable(true);

	apm->high_pass_filter()->Enable(true);

	apm->noise_suppression()->Enable(true);

	apm->voice_detection()->Enable(true);

	apm->voice_detection()->set_likelihood(webrtc::VoiceDetection::kModerateLikelihood);

	apm->Initialize();
}


void DoMixFrames(webrtc::AudioFrame* mixed_frame, webrtc::AudioFrame* frame, bool use_limiter)
{
	assert(mixed_frame->num_channels_ >= frame->num_channels_);
	if (use_limiter) {
		// Divide by two to avoid saturation in the mixing.
		// This is only meaningful if the limiter will be used.
		*frame >>= 1;
	}
	if (mixed_frame->num_channels_ > frame->num_channels_) {
		// We only support mono-to-stereo.
		assert(mixed_frame->num_channels_ == 2 &&
			frame->num_channels_ == 1);
		webrtc::AudioFrameOperations::MonoToStereo(frame);
	}

	*mixed_frame += *frame;
}

void AudioProcessWork(webrtc::AudioProcessing *apm)
{
	int16_t data[80], data2[80];
	int capture_level = 255;
	int stream_has_voice;
	int ns_speech_prob;
	int ret = 80, length = 0;
	webrtc::AudioFrame mixed_frame;
	webrtc::AudioFrame frame;

	webrtc::StreamConfig conf(8000, 1, false);

	FILE * fp = fopen("out.pcm", "rb");
	FILE * fp2 = fopen("speaker.pcm", "rb");
	FILE * dest = fopen("audio_1C_8K_16bit_proess.pcm", "wb");

	apm->set_stream_delay_ms(0); //local file is 0,remote is delay = (t_render - t_analyze) + (t_process - t_capture)
	apm->echo_cancellation()->set_stream_drift_samples(0);
	while (ret > 0)
	{
		ret = fread(data, sizeof(int16_t), 80, fp);
		fread(data2, sizeof(int16_t), 80, fp2);

		mixed_frame.UpdateFrame(0, 0, (const int16_t *)data, 80, 8000, webrtc::AudioFrame::kNormalSpeech, webrtc::AudioFrame::kVadActive, 1);
		frame.UpdateFrame(0, 0, (const int16_t *)data2, 80, 8000, webrtc::AudioFrame::kNormalSpeech, webrtc::AudioFrame::kVadActive, 1);

		DoMixFrames(&mixed_frame, &frame, true);

		//float * array = data;
		//float * const pdata = array;
		//const float *const *ptr = &pdata;
		//apm->gain_control()->set_stream_analog_level(capture_level);
		//apm->AnalyzeReverseStream(ptr, 80, 8000, webrtc::AudioProcessing::ChannelLayout::kMono);
		//apm->ProcessStream(ptr, conf, conf, &pdata);
		//capture_level = apm->gain_control()->stream_analog_level();
		//stream_has_voice = apm->voice_detection()->stream_has_voice();
		//ns_speech_prob = apm->noise_suppression()->speech_probability();
		//fwrite(pdata, sizeof(float), ret, dest);
		fwrite(mixed_frame.data_, sizeof(int16_t), 80, dest);
		length += ret;
		printf("read %d samples already %d\n", ret, length);
	}
	fclose(fp);
	fclose(fp2);
	fclose(dest);
}


void WorkMixer()
{
	webrtc::AudioFrame mixed_frame;
	webrtc::AudioFrame frame;


}

int main()
{
	webrtc::AudioProcessing *apm = webrtc::AudioProcessing::Create();
	InitAudioProcessing(apm);
	
	AudioProcessWork(apm);
	getchar();
	delete apm;
	apm = NULL;
}