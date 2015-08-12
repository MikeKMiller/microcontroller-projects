#ifndef SAMPLE_H
#define SAMPLE_H

#include <Audio.h>
#include <SerialFlash.h>
#include <play_serial_raw.h>
#include <math.h>

#define SAMPLE_COUNT				12

//The base of the exponential curve for audio mapping.  Nothing particularly scientific here, I just 
// played with a bunch of graphs until I found one that looked and sounded right.  Take note of the
// max value here (i.e. what is the number EXPONENTIAL_BASE^256); this is the maximum value that can 
// be output from the forumula.  You must pick VOLUME_DIVISOR to be a number which is larger than 
// the maximum.  I find that picking the maximum to be about 80% of VOLUME_DIVISOR works well enough.
#define EXPONENTIAL_BASE			1.022
//The volume divisor converts a number from EXPONENTIAL_BASE^rawValue to 0..1 (with a bit of headroom 
// to prevent clipping)
#define VOLUME_DIVISOR				300.0
//The linear part of the curve.  We take the max of (rawValue / LINEAR_DIVISOR) and (EXPONENTIAL_BASE^rawValue)
// as the final value (which is then scaled by the VOLUME_DIVISOR).  This value just keeps a non-zero
// value for very low velocity hits.
#define LINEAR_DIVISOR				5.0

namespace digitalcave {

	class Sample {
		private:
			static AudioPlaySerialRaw samples[];
			static AudioInputI2S input;

			//Mixer
			static AudioMixer16 mixer;

			//Output
			static AudioOutputI2S output;

			//Samples to mixer
			static AudioConnection sample0ToMixer;
			static AudioConnection sample1ToMixer;
			static AudioConnection sample2ToMixer;
			static AudioConnection sample3ToMixer;
			static AudioConnection sample4ToMixer;
			static AudioConnection sample5ToMixer;
			static AudioConnection sample6ToMixer;
			static AudioConnection sample7ToMixer;
			static AudioConnection sample8ToMixer;
			static AudioConnection sample9ToMixer;
			static AudioConnection sample10ToMixer;
			static AudioConnection sample11ToMixer;

			//Input passthrough to mixer
			static AudioConnection inputToMixer0;
			static AudioConnection inputToMixer1;

			//Mixer to output
			static AudioConnection mixerToOutput0;
			static AudioConnection mixerToOutput1;

			//This sample's index into mixer
			uint8_t index;
			
			//The most recently played channel
			uint8_t lastChannel;
			
		public:
			Sample(uint8_t index);
			
			void play(uint8_t channel, uint8_t volume);
			uint8_t isPlaying();
			uint32_t getPositionMillis();
			void stop();
			void setGain(uint8_t volume);
			uint8_t getLastChannel();
	};
	
}

#endif