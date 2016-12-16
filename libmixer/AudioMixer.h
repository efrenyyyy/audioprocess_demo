/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_INCLUDE_MODULE_COMMON_TYPES_H_
#define WEBRTC_MODULES_INCLUDE_MODULE_COMMON_TYPES_H_

#include <assert.h>
#include <string.h>  // memcpy

#include <algorithm>
#include <limits>

//#include "webrtc/base/constructormagic.h"
//#include "webrtc/common_types.h"
//#include "webrtc/common_video/rotation.h"
//#include "webrtc/typedefs.h"
#include "constructormagic.h"
#include "AudioTypes.h"

//void DoMixFrames(webrtc::AudioFrame* mixed_frame, webrtc::AudioFrame* frame, bool use_limiter);

namespace webrtc {

/* This class holds up to 60 ms of super-wideband (32 kHz) stereo audio. It
 * allows for adding and subtracting frames while keeping track of the resulting
 * states.
 *
 * Notes
 * - The total number of samples in |data_| is
 *   samples_per_channel_ * num_channels_
 *
 * - Stereo data is interleaved starting with the left channel.
 *
 * - The +operator assume that you would never add exactly opposite frames when
 *   deciding the resulting state. To do this use the -operator.
 */
class AudioFrame {
    public:
        // Stereo, 32 kHz, 60 ms (2 * 32 * 60)
        enum : size_t {
            kMaxDataSizeSamples = 3840
        };

        enum VADActivity {
            kVadActive = 0,
            kVadPassive = 1,
            kVadUnknown = 2
        };
        enum SpeechType {
            kNormalSpeech = 0,
            kPLC = 1,
            kCNG = 2,
            kPLCCNG = 3,
            kUndefined = 4
        };

        AudioFrame();

        // Resets all members to their default state (except does not modify the
        // contents of |data_|).
        void Reset();

        void UpdateFrame(int id, uint32_t timestamp, const int16_t* data,
            size_t samples_per_channel, int sample_rate_hz,
            SpeechType speech_type, VADActivity vad_activity,
            size_t num_channels = 1);

        void CopyFrom(const AudioFrame& src);

        void Mute();

        AudioFrame& operator>>=(const int rhs);
        AudioFrame& operator+=(const AudioFrame& rhs);

        int id_;
        // RTP timestamp of the first sample in the AudioFrame.
        uint32_t timestamp_;
        // Time since the first frame in milliseconds.
        // -1 represents an uninitialized value.
        int64_t elapsed_time_ms_;
        // NTP time of the estimated capture time in local timebase in milliseconds.
        // -1 represents an uninitialized value.
        int64_t ntp_time_ms_;
        int16_t data_[kMaxDataSizeSamples];
        size_t samples_per_channel_;
        int sample_rate_hz_;
        size_t num_channels_;
        SpeechType speech_type_;
        VADActivity vad_activity_;

    private:
        //RTC_DISALLOW_COPY_AND_ASSIGN(AudioFrame);

        AudioFrame(const AudioFrame&){};
        void operator=(const AudioFrame&) {};
};

// TODO(henrik.lundin) Can we remove the call to data_()?
// See https://bugs.chromium.org/p/webrtc/issues/detail?id=5647.
inline AudioFrame::AudioFrame()
    : data_() {
  Reset();
}

inline void AudioFrame::Reset() {
  id_ = -1;
  // TODO(wu): Zero is a valid value for |timestamp_|. We should initialize
  // to an invalid value, or add a new member to indicate invalidity.
  timestamp_ = 0;
  elapsed_time_ms_ = -1;
  ntp_time_ms_ = -1;
  samples_per_channel_ = 0;
  sample_rate_hz_ = 0;
  num_channels_ = 0;
  speech_type_ = kUndefined;
  vad_activity_ = kVadUnknown;
}

inline void AudioFrame::UpdateFrame(int id,
                                     uint32_t timestamp,
                                    const  int16_t* data,
                                    size_t samples_per_channel,
                                    int sample_rate_hz,
                                    SpeechType speech_type,
                                    VADActivity vad_activity,
                                    size_t num_channels) {
  id_ = id;
  timestamp_ = timestamp;
  samples_per_channel_ = samples_per_channel;
  sample_rate_hz_ = sample_rate_hz;
  speech_type_ = speech_type;
  vad_activity_ = vad_activity;
  num_channels_ = num_channels;

  const size_t length = samples_per_channel * num_channels;
  assert(length <= kMaxDataSizeSamples);
  if (data != NULL) {
    memcpy(data_, data, sizeof( int16_t) * length);
  } else {
    memset(data_, 0, sizeof( int16_t) * length);
  }
}

inline void AudioFrame::CopyFrom(const AudioFrame& src) {
  if (this == &src) return;

  id_ = src.id_;
  timestamp_ = src.timestamp_;
  elapsed_time_ms_ = src.elapsed_time_ms_;
  ntp_time_ms_ = src.ntp_time_ms_;
  samples_per_channel_ = src.samples_per_channel_;
  sample_rate_hz_ = src.sample_rate_hz_;
  speech_type_ = src.speech_type_;
  vad_activity_ = src.vad_activity_;
  num_channels_ = src.num_channels_;

  const size_t length = samples_per_channel_ * num_channels_;
  assert(length <= kMaxDataSizeSamples);
  memcpy(data_, src.data_, sizeof( int16_t) * length);
}

inline void AudioFrame::Mute() {
  memset(data_, 0, samples_per_channel_ * num_channels_ * sizeof( int16_t));
}

inline AudioFrame& AudioFrame::operator>>=(const int rhs) {
  assert((num_channels_ > 0) && (num_channels_ < 3));
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;

  for (size_t i = 0; i < samples_per_channel_ * num_channels_; i++) {
    data_[i] = static_cast< int16_t>(data_[i] >> rhs);
  }
  return *this;
}

namespace {
inline  int16_t ClampToInt16( int32_t input) {
  if (input < -0x00008000) {
    return -0x8000;
  } else if (input > 0x00007FFF) {
    return 0x7FFF;
  } else {
    return static_cast< int16_t>(input);
  }
}
}

inline AudioFrame& AudioFrame::operator+=(const AudioFrame& rhs) {
  // Sanity check
  assert((num_channels_ > 0) && (num_channels_ < 3));
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;
  if (num_channels_ != rhs.num_channels_) return *this;

  bool noPrevData = false;
  if (samples_per_channel_ != rhs.samples_per_channel_) {
    if (samples_per_channel_ == 0) {
      // special case we have no data to start with
      samples_per_channel_ = rhs.samples_per_channel_;
      noPrevData = true;
    } else {
      return *this;
    }
  }

  if ((vad_activity_ == kVadActive) || rhs.vad_activity_ == kVadActive) {
    vad_activity_ = kVadActive;
  } else if (vad_activity_ == kVadUnknown || rhs.vad_activity_ == kVadUnknown) {
    vad_activity_ = kVadUnknown;
  }

  if (speech_type_ != rhs.speech_type_) speech_type_ = kUndefined;

  if (noPrevData) {
    memcpy(data_, rhs.data_,
           sizeof( int16_t) * rhs.samples_per_channel_ * num_channels_);
  } else {
    // IMPROVEMENT this can be done very fast in assembly
    for (size_t i = 0; i < samples_per_channel_ * num_channels_; i++) {
       int32_t wrap_guard =
          static_cast< int32_t>(data_[i]) + static_cast< int32_t>(rhs.data_[i]);
      data_[i] = ClampToInt16(wrap_guard);
    }
  }
  return *this;
}


void DoMixFrames(AudioFrame* mixed_frame, AudioFrame* frame, bool use_limiter);

}  // namespace webrtc

#endif  // WEBRTC_MODULES_INCLUDE_MODULE_COMMON_TYPES_H_

