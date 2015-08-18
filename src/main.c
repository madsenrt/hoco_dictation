//
//  main.c
//  hoco_dictation
//
//  Created by Rob Madsen on 7/20/15.
//  Copyright (c) 2015 Rob Madsen. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioComponent.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <math.h>
#include "audio_unit.h"
#include "rendersin.h"

AudioUnit gOutputUnit;

OSStatus MyRenderer(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, 
			const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, 
			UInt32 inNumberFrames, AudioBufferList *ioData)
{
	RenderSin(sSinWaveFrameCount, inNumberFrames,
			ioData->mBuffers[0].mData, sSampleRate,
			sAmplitude, sToneFrequency, sWhichFormat);

	//we're just going to copy the data into each channel
	for (UInt32 channel = 1; channel < ioData->mNumberBuffers; channel++)
		//dest, source, size
		memcpy(ioData->mBuffers[channel].mData, ioData->mBuffers[0].mData, 
			ioData->mBuffers[0].mDataByteSize);

	sSinWaveFrameCount += inNumberFrames;

	return noErr;
}

void createDefaultAU()
{
	OSStatus err = noErr;
	AudioComponent component;
	AudioComponentDescription dsc;

	//Open default output unit
	dsc.componentType = kAudioUnitType_Output;
	dsc.componentSubType = kAudioUnitSubType_DefaultOutput;
	dsc.componentManufacturer = kAudioUnitManufacturer_Apple;
	dsc.componentFlags = 0;
	dsc.componentFlagsMask = 0;
	
	component = AudioComponentFindNext(NULL, &dsc);
	if (component == NULL) 
		return;
	//the audio component you want to create a new instance of,
	//output: the new audio component instance
	err = AudioComponentInstanceNew(component, &gOutputUnit);
	if (err != noErr)
		return;
	if (component == NULL)
		return;
	/* @struct AURenderCallbackStruct 
	 * @abstract Used by a host when registering a callback
	 * with the audio unit to provide input

	typedef struct AURenderCallbackStruct {
		AURenderCallback inputProc;
		void *inputProcRefCon;
	}

	Find AURenderCallback in AudiUnit/AUComponent.h
	*/	
	AURenderCallbackStruct input;
	input.inputProc = MyRenderer;
	input.inputProcRefCon = NULL;

	/* the audio unit that you want to set a property value for
	 * the audio unit property identifier:
	 * 	specify AudioUnit will obtain its input data from
	 *	its render callback
	 * the audio unit scope for the property
	 * the audio unit element for the property
	 * the value that you want to apply to the propery, always
	 *	pass by reference, may be NULL
	 * the size of the data you are providing in previous input
	*/
	err = AudioUnitSetProperty(gOutputUnit, kAudioUnitProperty_SetRenderCallback,
					kAudioUnitScope_Input, 0,
					&input, sizeof(input));
	if (err) {
		printf("AudioUnitSetProperty-CB=%d\n", (int)err);
		return;
	}
}

void TestDefaultAU()
{
	OSStatus err = noErr;

	// We tell the Output Unit what format we're going to supply data to it
	// this is necessary if you're providing data through an input callback
	// AND you want the DefaultOutputUnit to do any format conversions
	// necessary from your format to the device's format.
	AudioStreamBasicDescription streamFormat;
		streamFormat.mSampleRate = sSampleRate;		//the sample rate of the audio stream
		streamFormat.mFormatID = theFormatID;		//the specific encoding type of audio stream
		streamFormat.mFormatFlags = theFormatFlags;	//flags specific to each format
		streamFormat.mBytesPerPacket = theBytesInAPacket;	
		streamFormat.mFramesPerPacket = theFramesPerPacket;	
		streamFormat.mBytesPerFrame = theBytesPerFrame;		
		streamFormat.mChannelsPerFrame = sNumChannels;	
		streamFormat.mBitsPerChannel = theBitsPerChannel;	

	printf("Rendering source:\n\t");
	printf("SampleRate=%f,", streamFormat.mSampleRate);
	printf("BytesPerPacket=%u,", (unsigned int)streamFormat.mBytesPerPacket);
	printf("FramesPerPacket=%u,", (unsigned int)streamFormat.mFramesPerPacket);
	printf("BytesPerFrame=%u,", (unsigned int)streamFormat.mBytesPerFrame);
	printf("BitsPerChannel=%u,", (unsigned int)streamFormat.mBitsPerChannel);
	printf("ChannelsPerFrame=%u\n", (unsigned int)streamFormat.mChannelsPerFrame);
	
	/* 
	 * sets the value of an audio unit property
	 * the audio unit that you want to set a property for
	 * the audio unit property identifier
	 * the audio unit scope for the property
	 * the audio unit element for the property
	 * the value that you want to apply to the property
	 * 	may be Null
	 * the size of the date for above parameter
	 */
	err = AudioUnitSetProperty(gOutputUnit, kAudioUnitProperty_StreamFormat,
					kAudioUnitScope_Input, 0, &streamFormat,
					sizeof(AudioStreamBasicDescription));
	if (err) { 
		printf ("AudioUnitSetProperty-SF=%4.4s, %d\n", 
				(char*)&err, (int)err); 
		return;
	}
	
    	// Initialize unit
	err = AudioUnitInitialize(gOutputUnit);
	if (err) { 
		printf ("AudioUnitInitialize=%d\n", (int)err);
		return;
	}

	Float64 outSampleRate;
	UInt32 size = sizeof(Float64);
	/*
	 * gets the value of an audio unit property
	 * the audio unit that you want to get a property value from
	 * the identifer for the property
	 * the audio unit scope for the property
	 * the audio unit element for the property
	 * on success, the current value for the specified audio unit
	 * 	property
	 * on input, the expected size of the property value
	 */
	err = AudioUnitGetProperty(gOutputUnit, kAudioUnitProperty_SampleRate,
				kAudioUnitScope_Output, 0, &outSampleRate, &size);
	if (err) {
		printf ("AudioUnitSetProperty-GF=%4.4s, %d\n", (char*)&err, (int)err);
		return;
	}

	// Start the rendering
	// The DefaultOutputUnit will do any format conversions to the format of the default device
	err = AudioOutputUnitStart (gOutputUnit);
	if (err) {
		printf ("AudioOutputUnitStart=%d\n", (int)err);
		return;
	}
			
	// we call the CFRunLoopRunInMode to service any notifications that the audio
	// system has to deal with
	// 2: the length of time to run the run loop
	// if false, continues processing events until (2) seconds have passed
	CFRunLoopRunInMode(kCFRunLoopDefaultMode, 2, false);

	// REALLY after you're finished playing STOP THE AUDIO OUTPUT UNIT!!!!!!	
	// but we never get here because we're running until the process is nuked...	
	verify_noerr (AudioOutputUnitStop (gOutputUnit));
	
    	err = AudioUnitUninitialize (gOutputUnit);
	if (err) {
		printf ("AudioUnitUninitialize=%d\n", (int)err);
		return;
	}
}

void CloseDefaultAU()
{
	AudioComponentInstanceDispose(gOutputUnit);
}
void CreateArgListFromString(int *ioArgCount, char **ioArgs, char *inString)
{
	int i, length;
	length = (int)strlen(inString);

	//prime for first argument
	ioArgs[0] = inString;
	*ioArgCount = 1;

	//get subsequent arguments
	for (i = 0; i < length; i++) {
		if (inString[i] == ' ') {
			inString[i] = 0;	//terminate string
			++(*ioArgCount);	//increment count
			ioArgs[*ioArgCount-1] = inString + i + 1; //set next arg pointer
		}
	}
}

int main(int argc, const char *argv[]) 
{
	createDefaultAU();

	//no input
        Float64 sampleRateList[] = {8000.0, 11025.0, 16000.0,
					22050.0,32000.0, 44100.0,
					48000.0, 64000.0,88200.0,
					96000.0, 192000.0, 320000.0,
					55339.75};
        UInt32 sampleRateListLength = sizeof(sampleRateList) / sizeof(Float64);
        UInt32 channelCountList[] = {2, 4, 8};
        UInt32 channelCountListLength = sizeof(channelCountList) / sizeof(UInt32);
        UInt32 bitDepthList[] = {16, 24, 32};
        UInt32 bitDepthListLength = sizeof(bitDepthList) / sizeof(UInt32);
        
        UInt32 i, j, k;
        char testString[1024];
        
        int internalArgc;
        char *internalArgv[256];
        OSStatus result;
        
        for (i = 0; i < bitDepthListLength; i++) {
		for (j = 0; j < channelCountListLength; j++) {
			for (k = 0; k < sampleRateListLength; k++) {
				sprintf (testString, "executable -d %lu -c %u -s %f",
					(unsigned long)bitDepthList[i], (unsigned int)channelCountList[j], sampleRateList[k]);
				printf ("# Testing at %lu bits, with %u channels, at %fHz.\n",
					(unsigned long)bitDepthList[i], (unsigned int)channelCountList[j], sampleRateList[k]);

				CreateArgListFromString (&internalArgc, internalArgv, testString);
				if (ParseArgsAndSetup (internalArgc, (const char**)internalArgv)) return -1;
				TestDefaultAU();

				// reset unit
				// the audio unit whose render state you are resetting
				// the audio unit scope
				// the audio unit element, usually 0
				result = AudioUnitReset (gOutputUnit, kAudioUnitScope_Input, 0);
				if (result != noErr) {
					printf ("Error %d while trying to reset output unit.\n", (int)result);
					exit(-1);
				}
			}
		}
	}
	CloseDefaultAU();
	return 0;
}
