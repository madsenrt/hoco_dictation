#include <AudioToolbox/AudioQueue.h>
#include <CoreAudio/CoreAudioTypes.h>
//probably others

//https://developer.apple.com/library/mac/documentation/MusicAudio/Conceptual/AudioQueueProgrammingGuide/AQRecord/RecordingAudio.html

static const int kNumberBuffers = 3;

/*
 * mDataFormat: the audio data format to write to disk
 * mQueue: the recording audio queue created by application
 * mBuffers[]: an array holding pointers to the audio queue buffers,
 * 		managed by the audio queue
 * mAudioFile: and audiofile object representing the file into which your
 * 		program records audio data
 * bufferByteSize: size, in bytes, for each audio queue buffer
 * mCurrentPacket: packet index for the first packet to be written from the current
 * 		audio queue buffer
 * mIsRunning: indicates whether or not the audio queue is running
 */
struct AQRecorderState {
	AudioStreamBasicDescription	mDataFormat;
	AudioQueueRef			mQueue;
	AudioQueueBufferRef		mBuffers[kNumberBuffers]; 
	AudioFileID			mAudioFile;
	UInt32				bufferByteSize;
	SInt64				mCurrentPacket;
	bool				mIsRunning;
};

/*
 * *aqData: some custom structure that contains state data for the audio queue
 * inAQ: the audio queue that owns this callback
 * inBuffer: the audio queue buffer contaiting the incoming audio data to record
 * *inStartTime: the sample time of the first sample in the audio queue buffer 
 *			(not needed for simple recording)
 * inNumPackets: the number of packet descriptions in the inPacketDesc parameter
 *			(a value of 0 indicated CBR data)
 * *inPacketDesc: for compressed audio data formats that require packet decriptions,
 * 			the packet descriptions produced by the encoder for the packets
 * 			in the buffer
 */
static void HandleInputBuffer (void *aqData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer,
				const AudioTimeStamp *inStartTime, UInt32 inNumPackets, 
				const AudioStreamPacketDescription *inPacketDesc);


void DeriveBufferSize(AudioQueueRef audioQueue, AudioStreamBasicDescription &ASBDescription,
			Float64 seconds, UInt32 *outBufferSize); 
