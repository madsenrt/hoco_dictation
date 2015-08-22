#include "record.h"

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
				const AudioStreamPacketDescription *inPacketDesc)
{
	AQRecorderState *pAqData = (AQRecorderState *)aqData;

	/*
	 * evaluates to true if audio queue buffer contains CBR data
	 * if false (VBR data), audio queue supplies the number of packets
	 * 	in the buffer when it invokes the callback
	*/
	if (inNumPackets == 0 && pAqData->mDataFormat.mBytesPerPacket != 0)
       		inNumPackets = inBuffer->mAudioDataByteSize / pAqData->mDataFormat.mBytesPerPacket;

	/*
	 * AudioFileWritePackets: writes the contents of a buffer to an audio data file
	 * pAqData->mAudioFile: the audio file object (of type AudioFileID) that represents
	 * 	the audio file to write to, pAqData varibale is a pointer to the data
	 * 	struct AQRecorderState
	 * false indicates that the function should not cache the data when writing
	 * inBuffer->mAudioDataByteSize: number of bytes of audio data being written,
	 * 	the inBuffer represents the audio queue buffer handed to the callback
	 *	by the audio queue
	 * inPacketDesc: an array of packet descriptions for the audio data, NULL indicates
	 * 	no packet descriptions are requeired (i.e. for CBR audio data)
	 * AqData->mCurrentPacket: the packet index for the first packet to be written
	 * &inNumPackets: on input, the number of packets to write;
	 * 	on output, the number of packets actually written
	 * inBuffer->mAudioData: the new audio data to write to audio file
	 */
	if (AudioFileWritePackets(pAqData->mAudioFile, false, inBuffer->mAudioDataByteSize,
				inPacketDesc, pAqData->mCurrentPacket, &inNumPackets,
				inBuffer->mAudioData) == noErr)
		/*
		 * if succesful in writing the audio data, increment the audio data file's
		 * packet index to be ready for writing the next buffer's worht of audio data
		 */
		pAqData->mCurrentPacket += inNumPackets;

	//if the audio queue has stopped, return
	if (pAqData->mIsRunning == 0)
		return;
	
	/*
	 * enqueues the audio queue buffer whose contents have just been written to the audio file
	 * pAqData->mQueue: the audio queue to add the designated audio queue buffer to
	 * inBuffer: the audio queue buffer to enqueue
	 * 0: the number of packet descriptions in the audio queueu buffer's data,
	 * 	set to 0 because this parameter is unused for recording
	 * NULL: the array of packet descriptions describing the audio queue buffer's data,
	 * 	set to NULL because this parameter is unused for recording
	 */
	AudioQueueEnqueueBuffer (pAqData->mQueue, inBuffer, 0, NULL);
}

/*
 * audioQueue: the audio queue the owns that buffers whose size you want to specify
 * the AudioStreamBasicDescription structure for the audio queue
 * seconds: the size you are specifying for each audio queue buffer, in seconds
 * outBufferSize: on output, the sie for each audio queue buffer, in bytes
*/
void DeriveBufferSize(AudioQueueRef audioQueue, AudioStreamBasicDescription &ASBDescription,
			Float64 seconds, UInt32 *outBufferSize) 
{
	/*
	 * an upper bound for the audio queue buffer size, in bytes
	 * x50000 ~ 320kB: corresponds to about 5 seconds of stereo, 24 bit
	 * audio at a sample rate of 96kHz
	 */
	static const int maxBufferSize = 0x50000;

	/*
	 * for CBR audio data, get the (constant) packet size from the AudioStreamBasicDescription,
	 * use this value as the maximum packet size.  This assignment has the side effect of determing
	 * if audio data to be recorded is CBR of VBR, if it is VBR the audio queue's 
	 * AudioStreamBasicDescription structure lists the value of bytes-per-packet as 0
	 */
	int maxPacketSize = ASBDescription.mBytesPerPacket;

	// for VBR data, query the audio queue to get the estimated maximum packet size
	if (maxPacketSize == 0) {
        	UInt32 maxVBRPacketSize = sizeof(maxPacketSize);
        	AudioQueueGetProperty(audioQueue, kAudioQueueProperty_MaximumOutputPacketSize,
					&maxPacketSize, &maxVBRPacketSize);
	}
	
	// buffer size in bytes
	Float64 numBytesForTime = ASBDescription.mSampleRate * maxPacketSize * seconds;
	// limit buffer size if needed to previously set upper bound
	*outBufferSize = UInt32 (numBytesForTime < maxBufferSize ?
					numBytesForTime : maxBufferSize);
}
