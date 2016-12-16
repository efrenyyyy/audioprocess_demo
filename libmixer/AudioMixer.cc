#include "AudioMixer.h"
#include "AudioFrameOperation.h"

namespace webrtc{
    void DoMixFrames(AudioFrame* mixed_frame, AudioFrame* frame, bool use_limiter)
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
}
