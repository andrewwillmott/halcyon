//
//  File:       HLAudioManager.cpp
//
//  Function:   Audio system, implementation based off Apple's Audio Unit API
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <Apple/HLAudioManager.h>

#include <IHLConfigManager.h>

#include <CLDirectories.h>
#include <CLFileSpec.h>
#include <CLLog.h>
#include <CLValue.h>

#ifndef CL_RELEASE
    #include <HLUI.h>
#endif
#include <AudioToolbox/AudioServices.h>
#ifdef CL_IOS
    #include <AudioToolbox/AudioSession.h>
#endif

// #define LOCAL_DEBUG

#ifdef LOCAL_DEBUG
    #define LOCAL_LOG printf
#else
    #define LOCAL_LOG(M_FORMAT...)
#endif

using namespace nHL;
using namespace nCL;

#define FX_ENABLED

namespace
{
    const tTag kTagMasterEnabled = CL_TAG("masterEnabled");
    const tTag kTagMasterGain    = CL_TAG("masterGain");


    /*
    TODO:
    - if we have audio to play, and it covers the required sample count, return pointers.
    - if sample count < 0, return empty space for +sample, and chain to re-evaluate
    - if samples remaining <

    */
    cPlayCommand kCommandSilence;

    void CopyFrames
    (
        int             numFrames,
        int             inStart,
        const tSample*  inLeft,
        const tSample*  inRight,
        tSample*        outLeft,
        tSample*        outRight
    )
    {
        inLeft += inStart;

        for (int i = 0; i < numFrames; i++)
            outLeft[i] = inLeft[i];

        if (outRight)
        {
            if (inRight)
            {
                inRight += inStart;
                for (int i = 0; i < numFrames; i++)
                    outRight[i] = inRight[i];
            }
            else
                for (int i = 0; i < numFrames; i++)
                    outRight[i] = inLeft[i];
        }
    }

    void CopySilence
    (
        int numFrames,
        tSample*        outLeft,
        tSample*        outRight
    )
    {
        memset(outLeft, 0, numFrames * sizeof(tSample));

        if (outRight)
            memset(outRight, 0, numFrames * sizeof(tSample));
    }


    // Callback from Audio Unit
    OSStatus InputRenderCallback
    (
        void*                       userData,       // cInputSourceInfo
        AudioUnitRenderActionFlags* ioActionFlags,  // For silence, set kAudioUnitRenderAction_OutputIsSilence and memset buffers to 0
        const AudioTimeStamp*       inTimeStamp,    // Time to generate for(?)
        UInt32                      inputBus,       // Input bus that is requesting some new frames of audio data to play.
        UInt32                      numFrames,      // The number of frames of audio to provide to the buffer(s) pointed to by the ioData parameter.
        AudioBufferList*            ioData          // On output, the audio data to play. The callback's primary responsibility is to fill the buffer(s) in the AudioBufferList.
    )
    {
        CL_ASSERT(ioData->mNumberBuffers > 0);

        cInputSourceInfo& sourceInfo = *(cInputSourceInfo*) userData;
        cPlayCommand& sampleInfo = sourceInfo.mCurrent;

        AudioUnitSampleType* outLeft  = (AudioUnitSampleType*) ioData->mBuffers[0].mData;
        AudioUnitSampleType* outRight;

        if (ioData->mNumberBuffers > 1)
        {
            CL_ASSERT(ioData->mBuffers[1].mNumberChannels == 1);
            CL_ASSERT(ioData->mBuffers[1].mDataByteSize == sizeof(AudioUnitSampleType) * numFrames);

            outRight = (AudioUnitSampleType*) ioData->mBuffers[1].mData;
        }
        else
            outRight = 0;
        
        CL_ASSERT(ioData->mBuffers[0].mNumberChannels == 1);    // Not expecting to interleave data
        CL_ASSERT(ioData->mBuffers[0].mDataByteSize == sizeof(AudioUnitSampleType) * numFrames);

        const tSample* outLeftEnd  = outLeft + numFrames;

        ////////////////////////////////////

        // Check next command...
        const cPlayCommand* command = sourceInfo.mCommands.NextReadItem();

        // At the moment, we may enter here having finished a loop on the previous frame. In which case we
        // drop through until we hit the "finished sample" code below
        // This does lead to us potentially missing a zero copy optimisation if there is a next sample
        // and it's >= numFrames.
        // CL_ASSERT(sampleInfo.mNumSamples == 0 || sourceInfo.mSampleCursor < sampleInfo.mNumSamples);

        if (command)
        {
            if (command->mInterrupt)    // Ignore whether sound is currently playing.
            {
                if (sampleInfo.mNumSamples != 0)
                {
                    LOCAL_LOG("Interrupting sound %d on input %d\n", sampleInfo.mSoundRef, int(inputBus));
                    sourceInfo.mResults.Write(sampleInfo.mPlayRef);
                }

                sourceInfo.PopNextCommand(inputBus);
            }
            else if (sampleInfo.mNumSamples == 0)
                sourceInfo.PopNextCommand(inputBus);
        }

        // Is the entire frame silence?
        bool allSilence = !sampleInfo.mDataLeft;

        if (!allSilence && sourceInfo.mSampleCursor < 0 && numFrames <= -sourceInfo.mSampleCursor)
        {
            allSilence = true;
            sourceInfo.mSampleCursor += numFrames;
        }

        if (allSilence)
        {
            *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;

            // Unfortunately we still need to do this. TODO: prealloc zero buffers & return pointers.
            for (int i = 0; i < ioData->mNumberBuffers; i++)
                memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);

            return noErr;
        }

        // Is the entire frame a continuous section from one sound?
        // If so, can just fill in pointers rather than copying data
        bool allSample = sourceInfo.mSampleCursor >= 0 && numFrames <= (sampleInfo.mNumSamples - sourceInfo.mSampleCursor);

        if (allSample)
        {
        #ifdef CL_IOS
            // iOS seems to have a bug if you try to pass data directly like this, it starts to distort horribly after
            // the first time through.
            if (false)  // even enabling this on just one buffer eventually leads to distortion
            {
        #endif
                ioData->mBuffers[0].mData = const_cast<tSample*>(sampleInfo.mDataLeft + sourceInfo.mSampleCursor);

                if (outRight)
                {
                    if (sampleInfo.mDataRight)
                        ioData->mBuffers[1].mData = const_cast<tSample*>(sampleInfo.mDataRight + sourceInfo.mSampleCursor);
                    else
                        ioData->mBuffers[1].mData = const_cast<tSample*>(sampleInfo.mDataLeft + sourceInfo.mSampleCursor);
                }
        #ifdef CL_IOS
            }
            else
                CopyFrames(numFrames, sourceInfo.mSampleCursor, sampleInfo.mDataLeft, sampleInfo.mDataRight, outLeft, outRight);
        #endif

            sourceInfo.mSampleCursor += numFrames;

            return noErr;
        }

        LOCAL_LOG("split slice for %d frames on input %d\n", int(numFrames), int(inputBus));

        // At this point we have to deal with a mixed slice. This should be relatively infrequent.
        do
        {
            // Handle partial silence lead-in to next sample
            if (sourceInfo.mSampleCursor < 0)
            {
                int silentFrames = -sourceInfo.mSampleCursor;

                if (silentFrames > numFrames)   // TODO: this is guaranteed true the first time
                    silentFrames = numFrames;

                LOCAL_LOG("  silence %d frames before %d\n", silentFrames, sampleInfo.mSoundRef);

                CopySilence(silentFrames, outLeft, outRight);
                sourceInfo.mSampleCursor += silentFrames;
                outLeft  += silentFrames;
                if (outRight)
                    outRight += silentFrames;
                numFrames -= silentFrames;

                if (numFrames == 0)
                    break;
            }

            // Play remainder of sample
            CL_ASSERT(sampleInfo.mSoundRef >= 0);

            int playFrames = sampleInfo.mNumSamples - sourceInfo.mSampleCursor;

            if (playFrames > numFrames) // TODO: this is guaranteed false the first time
                playFrames = numFrames;

            LOCAL_LOG("  play %d frames of %d\n", playFrames, sampleInfo.mSoundRef);

            CopyFrames
            (
                playFrames,
                sourceInfo.mSampleCursor,
                sampleInfo.mDataLeft,
                sampleInfo.mDataRight,
                outLeft,
                outRight
            );

            sourceInfo.mSampleCursor += playFrames;
            outLeft  += playFrames;
            if (outRight)
                outRight += playFrames;
            numFrames -= playFrames;

            // Finished the sample...
            if (sourceInfo.mSampleCursor == sampleInfo.mNumSamples)
            {
                sourceInfo.mLoopCount++;

                LOCAL_LOG("  finished loop %d of sound %d\n", sourceInfo.mLoopCount, sampleInfo.mSoundRef);

                bool done;

                if (sampleInfo.mRepeatCount == 0)   // or mCanInterrupt?
                {
                    // Stop playing if there's another sound queued.
                    done = (sourceInfo.mCommands.NextReadItem() != 0);
                }
                else
                    done = (sourceInfo.mLoopCount == sampleInfo.mRepeatCount);

                if (done)
                {
                    sourceInfo.mLoopCount = 0;
                    sourceInfo.mResults.Write(sampleInfo.mPlayRef);

                    if (!sourceInfo.PopNextCommand(inputBus) || sourceInfo.mCurrent.mDataLeft == 0)
                    {
                        LOCAL_LOG("  queue now empty, filling %d with silence\n", int(numFrames));

                        CopySilence(numFrames, outLeft, outRight);
                        outLeft  += numFrames;
                        if (outRight)
                            outRight += numFrames;

                        numFrames = 0;
                    }
                }
                else
                    sourceInfo.mSampleCursor = 0;
            }

        }
        while (numFrames > 0);

        CL_ASSERT(outLeft == outLeftEnd);

        return noErr;
    }

#ifdef CL_IOS

    // Audio session callback function for responding to audio route changes. If playing back audio and
    //   the user unplugs a headset or headphones, or removes the device from a dock connector for hardware  
    //   that supports audio playback, this callback detects that and stops playback. 
    //
    // Refer to AudioSessionPropertyListener in Audio Session Services Reference.
    void RouteChangeListenerCallback
    (
       void*                     inUserData,
       AudioSessionPropertyID    inPropertyID,
       UInt32                    inPropertyValueSize,
       const void*               inPropertyValue
    )
    {
        // Ensure that this callback was invoked because of an audio route change
        if (inPropertyID != kAudioSessionProperty_AudioRouteChange)
            return;

        cAudioManager* audioObject = (cAudioManager*) inUserData;
        
        if (!audioObject->IsPlaying())
        {
            CL_LOG("Audio", "Audio route change while application audio is stopped.");
            return;
        }
        else 
        {
            // Determine the specific type of audio route change that occurred.
            CFDictionaryRef routeChangeDictionary = (CFDictionaryRef) inPropertyValue;
            
            CFNumberRef routeChangeReasonRef =
                (CFNumberRef) CFDictionaryGetValue
                (
                    routeChangeDictionary,
                    CFSTR (kAudioSession_AudioRouteChangeKey_Reason)
                );

            SInt32 routeChangeReason;
            
            CFNumberGetValue
            (
                routeChangeReasonRef,
                kCFNumberSInt32Type,
                &routeChangeReason
            );
            
            // "Old device unavailable" indicates that a headset or headphones were unplugged, or that 
            //    the device was removed from a dock connector that supports audio output. In such a case,
            //    pause or stop audio (as advised by the iOS Human Interface Guidelines).
            if (routeChangeReason == kAudioSessionRouteChangeReason_OldDeviceUnavailable) 
            {
                CL_LOG("Audio", "Audio output device was removed; stopping audio playback.");

                audioObject->StopPlaying();
            } 
            else 
            {
                CL_LOG("Audio", "A route change occurred that does not require stopping application audio.");
            }
        }
    }

    void InterruptionCallback
    (
       void*    clientData,
       UInt32   state
    )
    {
        cAudioManager* mixer = (cAudioManager*) clientData;

        if (state == kAudioSessionBeginInterruption)
        {
            mixer->BeginInterruption();
        }
        else if (state == kAudioSessionEndInterruption)
        {
            mixer->EndInterruption();
        }
    }
#endif

    AudioBufferList* CreateAudioBuffers(cIAllocator* alloc, int numBuffers)
    {
        AudioBufferList* list = (AudioBufferList*) alloc->Alloc
        (
            sizeof (AudioBufferList) + sizeof (AudioBuffer) * (numBuffers - 1)
        );

        list->mNumberBuffers = numBuffers;

        AudioBuffer emptyBuffer = { 0 };

        for (int i = 0; i < numBuffers; i++)
            list->mBuffers[i] = emptyBuffer;

        return list;
    }
}



// --- cInputSourceInfo ---------------------------------------------------------


bool cInputSourceInfo::PopNextCommand(int inputBus)
{
    mSampleCursor = 0;
    mLoopCount = 0;

    const cPlayCommand* command = mCommands.NextReadItem();

    if (command)
    {
        mCurrent = *command;

        mCommands.CommitRead();
        LOCAL_LOG("  started new sound %d (interrupt=%d) on input %d\n", mCurrent.mSoundRef, mCurrent.mInterrupt, inputBus);
        return true;
    }

    mCurrent = kCommandSilence;
    return false;
}


// --- cAudioManager -----------------------------------------------------------

// Get the app ready for playback.
void cAudioManager::Init()
{
    mConfig = HL()->mConfigManager->Config()->Member("audio").AsObject();

    if (!mConfig)
        return;

    mConfigModCount = mConfig->ModCount();

    mSampleAllocator = Allocator(); // for now
    mPlaybackInterrupted = false;

    SetupStreamFormats();

    SetupAudioSession();
    ConfigAudio(mConfig);

    mChannelEnabled.resize(mNumSources, true);
    mChannelGain   .resize(mNumSources, 0.75f);
    mChannelPan    .resize(mNumSources, 0.0f);

    for (int i = 0; i < mNumSources; i++)
    {
        SetChannelEnabled(i, mChannelEnabled[i]);
        SetChannelGain   (i, mChannelGain[i]);
        SetChannelPan    (i, mChannelPan[i]);
    }

    auto prefs = HL()->mConfigManager->Preferences();

    float gain    = prefs->Member(kTagMasterGain   ).AsFloat(0.75f);
    bool  enabled = prefs->Member(kTagMasterEnabled).AsBool (true);

    // Force initial update
    SetMasterGain(gain);
    SetMasterEnabled(enabled);

    mPreferences = prefs;
}

void cAudioManager::Shutdown()
{
    if (mConfig)
    {
        mConfig = 0;
        mPreferences = 0;
        StopPlaying();

        if (mAudioGraph)
            DisposeAUGraph(mAudioGraph);

        mInputSources.clear();

        for (int i = 0, n = mSoundInfo.size(); i < n; i++)
        {
            // Can't do this without some way of aborting input callbacks
    //         CL_ASSERT(mSoundInfo[i].mUseCount == 0);

            DestroyArray(const_cast<tSample**>(&mSoundInfo[i].mDataLeft) , 0, mSampleAllocator);
            DestroyArray(const_cast<tSample**>(&mSoundInfo[i].mDataRight), 0, mSampleAllocator);
        }
    }
}

void cAudioManager::Update(float dt)
{
    if (mAudioGraph)
        AUGraphUpdate(mAudioGraph, 0);

    cSlotRef playRef;

    for (int i = 0; i < mNumSources; i++)
    {
        cInputSourceInfo& sourceInfo = mInputSources[i];

        if (sourceInfo.mResults.Read(&playRef))
        {
            CL_ASSERT(sourceInfo.mActiveCount > 0);
            sourceInfo.mActiveCount--;

            cPlayInfo* playInfo = mPlayInstances.Slot(playRef);

            if (playInfo)
            {
                mSoundInfo[playInfo->mSoundRef].mUseCount--;

                mPlayInstances.DestroySlot(playRef);
            }
        }
    }
}

void cAudioManager::LoadSounds(const cObjectValue* config)
{
    if (!config)
        return;

    bool wasPlaying = IsPlaying();
    StopPlaying();  // this is a bit heavy handed, also, does it guarantee callbacks won't be called afterwards?

    // TODO: want to wait until all outstanding render callbacks are done.

    for (auto c : config->Children())
    {
        const cObjectValue* info = c.ObjectValue();
        tTag                tag  = c.Tag();
        const char*         name = c.Name();

        if (!info || MemberIsHidden(name))
            continue;

        CL_LOG("Audio", "Adding sound %s\n", name);

        auto it = mSoundTagToIndex.find(tag);
        int soundRef;

        if (it != mSoundTagToIndex.end())
        {
            soundRef = it->second;

            Destroy(const_cast<tSample**>(&mSoundInfo[soundRef].mDataLeft ), mSampleAllocator);
            Destroy(const_cast<tSample**>(&mSoundInfo[soundRef].mDataRight), mSampleAllocator);
        }
        else
        {
            soundRef = mSoundInfo.size();
            mSoundInfo.push_back();
            mSoundTagToIndex[tag] = soundRef;
        }

        const char* filePath = info->Member("file").AsString();

        if (filePath)
        {
            cFileSpec spec;
            FindSpec(&spec, c, filePath);

            if (!spec.HasExtension())
                spec.SetExtension("aif");

            if (!ReadAudioFile(spec, &mSoundInfo[soundRef]))
            {
                CL_LOG_E("Audio", "Failed to read %s\n", spec.Path());
            }
        }

        if (info->Member("loop").AsBool(false))
            mSoundInfo[soundRef].mRepeatCount = 0;
        else
            mSoundInfo[soundRef].mRepeatCount = info->Member("repeat").AsInt(1);


        mSoundInfo[soundRef].mVolume = info->Member("volume").AsFloat(1.0f);
        mSoundInfo[soundRef].mPan    = info->Member("pan").AsFloat(0.0f);

        mSoundInfo[soundRef].mTag = tag;
    }

    // Handle play info fixup: we are reaching into audio callback-owned data here!
    for (int i = 0, n = mInputSources.size(); i < n; i++)
    {
        cInputSourceInfo& sourceInfo = mInputSources[i];
        cPlayCommand& command = mInputSources[i].mCurrent;

        if (command.mSoundRef >= 0)
        {
            const cSoundInfo& soundInfo = mSoundInfo[command.mSoundRef];

            command.mNumSamples = soundInfo.mNumFrames;
            command.mDataLeft   = soundInfo.mDataLeft;
            command.mDataRight  = soundInfo.mDataRight;
        }

        // TODO: fix these up, or restart?
        sourceInfo.mCommands.Clear();
    }

    if (wasPlaying)
        StartPlaying();
}

void cAudioManager::StartPlaying()
{
    CL_LOG("Audio", "Starting audio processing graph");

    OSStatus result = AUGraphStart (mAudioGraph);

    if (noErr != result)
    {
        CL_LOG_E("Audio", "AUGraphStart error: %d\n", result);
        return;
    }

    mIsPlaying = true;
}

// Stop playback
void cAudioManager::StopPlaying()
{
    CL_LOG("Audio", "Stopping audio processing graph");

    Boolean isRunning = false;
    OSStatus result = AUGraphIsRunning(mAudioGraph, &isRunning);

    if (noErr != result)
    {
        CL_LOG_E("Audio", "AUGraphIsRunning error: %d\n", result);
        return;
    }

    if (isRunning) 
    {
        result = AUGraphStop (mAudioGraph);

        if (noErr != result)
        {
            CL_LOG_E("Audio", "AUGraphStop error: %d\n", result);
            return;
        }

        mIsPlaying = false;
    }
}



int cAudioManager::SoundRefFromTag(tTag tag)
{
    auto it = mSoundTagToIndex.find(tag);

    if (it != mSoundTagToIndex.end())
        return it->second;

    return -1;
}

int cAudioManager::GroupRefFromTag(tTag tag)
{
    auto it = mGroupTagToIndex.find(tag);

    if (it != mGroupTagToIndex.end())
        return it->second;

    return mDefaultGroupRef;
}

void cAudioManager::SetMasterEnabled(bool enabled)
{
    if (mPreferences && (mMasterEnabled == enabled))
        return;

    if (enabled && !mIsPlaying)
        StartPlaying();
    else if (!enabled && mIsPlaying)
        StopPlaying();

    mMasterEnabled = enabled;

    if (mPreferences)
        mPreferences->InsertMember(kTagMasterEnabled) = mMasterEnabled;
}

// Set the mxer unit output volume
void cAudioManager::SetMasterGain(AudioUnitParameterValue newGain)
{
    if (mPreferences && (newGain == mMasterGain))
        return;

    if (mMixerUnitIndex >= 0)
    {
        OSStatus result = AudioUnitSetParameter
        (
            mUnits[mMixerUnitIndex],
            kMultiChannelMixerParam_Volume,
            kAudioUnitScope_Output,
            0,
            newGain,
            0
        );

        if (noErr != result)
            CL_LOG_E("Audio", "AudioUnitSetParameter (set mixer unit output volume) error: %d\n", result);
    }

    mMasterGain = newGain;

    if (mPreferences)
        mPreferences->InsertMember(kTagMasterGain) = mMasterGain;
}

tAudioPlayRef cAudioManager::PlaySound(int soundRef, int groupRef)
{
    if (groupRef < 0)
        groupRef = mDefaultGroupRef;
    else if (groupRef >= mGroupInfo.size())
    {
        CL_ERROR("Bad group ref");
        return kNullAudioPlayRef;
    }

    int s0 = mGroupInfo[groupRef].mSourceBase;
    int s1 = mGroupInfo[groupRef].mNumSources + s0;

    int oldestAge = 0;
    int oldestActive = -1;
    int source = -1;

    for (int i = s0; i < s1; i++)
    {
        cInputSourceInfo& sourceInfo = mInputSources[i];

        if (!sourceInfo.mActiveCount)
            source = i;
        else if (oldestAge < ++sourceInfo.mAge)
        {
            oldestAge = sourceInfo.mAge;
            oldestActive = i;
        }
    }

    if (source < 0)
        source = oldestActive;

    if (source >= 0)
    {
        mInputSources[source].mAge = 0;
        return PlayChannel(source, soundRef);
    }

    return kNullRef;
}

bool cAudioManager::StopSound(tAudioPlayRef playRef, bool immediately)
{
    // For now
    if (mPlayInstances.InUse(playRef))
    {
        const cPlayInfo& playInfo = mPlayInstances[playRef];
        // TODO: queue a command specific to this playref
        SilenceChannel(playInfo.mSourceIndex, immediately);
        return true;
    }

    return false;
}

bool cAudioManager::StopSounds(tAudioGroupRef groupRef, bool immediately)
{
    if (groupRef >= 0 && groupRef < mNumSources)
    {
        int s0 = mGroupInfo[groupRef].mSourceBase;
        int s1 = mGroupInfo[groupRef].mNumSources + s0;

        for (int i = s0; i < s1; i++)
            SilenceChannel(i, immediately);

        return true;
    }

    return false;
}

void cAudioManager::SetChannelEnabled(int channel, bool enabled)
{
    CL_LOG_D("Audio", "Channel %d now %s", channel, enabled ? "on" : "off");
         
    cInputSourceInfo& sourceInfo = mInputSources[channel];

    if (sourceInfo.mUnitIndex >= 0)
    {
        OSStatus result = AudioUnitSetParameter
        (
            mUnits[sourceInfo.mUnitIndex],
            kMultiChannelMixerParam_Enable,
            kAudioUnitScope_Input,
            sourceInfo.mUnitBus,
            enabled ? 1.0f : 0.0f,
            0
        );

        if (noErr != result)
            CL_LOG("Audio", "AudioUnitSetParameter (enable the mixer unit) error: %d\n", result);
    }

    mChannelEnabled[channel] = enabled;
}


// Set the mixer unit input volume for a specified bus
void cAudioManager::SetChannelGain(int channel, float gain)
{
    // TODO: if (syncTrack && newGain < 0.01f)
    //  newGain = 0.0f

    cInputSourceInfo& sourceInfo = mInputSources[channel];

    if (sourceInfo.mUnitIndex >= 0)
    {
        OSStatus result = AudioUnitSetParameter
        (
            mUnits[sourceInfo.mUnitIndex],
            kMultiChannelMixerParam_Volume,
            kAudioUnitScope_Input,
            sourceInfo.mUnitBus,
            gain,
            0
        );

        if (noErr != result)
            CL_LOG("Audio", "AudioUnitSetParameter (set mixer unit input volume) error: %d\n", result);
    }

    mChannelGain[channel] = gain;
}

void cAudioManager::SetChannelPan(int channel, float pan)
{
    cInputSourceInfo& sourceInfo = mInputSources[channel];

    if (sourceInfo.mUnitIndex >= 0)
    {
        OSStatus result = AudioUnitSetParameter
        (
            mUnits[sourceInfo.mUnitIndex],
            kMultiChannelMixerParam_Pan,
            kAudioUnitScope_Input,
            sourceInfo.mUnitBus,
            pan,
            0
        );

        if (noErr != result)
            CL_LOG("Audio", "AudioUnitSetParameter (set mixer unit input volume) error: %d\n", result);
    }

    mChannelPan[channel] = pan;
}

int cAudioManager::ChannelPlaying(int channel) const
{
    return mInputSources[channel].mActiveCount;
}

tAudioPlayRef cAudioManager::PlayChannel(int source, int soundRef)
{
    if (soundRef < 0 || source < 0)
        return kNullAudioPlayRef;

    const cSoundInfo& soundInfo = mSoundInfo[soundRef];
    cInputSourceInfo& sourceInfo = mInputSources[source];

    cPlayCommand* command = sourceInfo.mCommands.NextWriteItem();

    if (command)
    {
        // TODO: we want the callback to do this once it executes the command, if possible.
        // If not, we may have to manually mix ourselves?

        cSlotRef playRef = mPlayInstances.CreateSlot();
        mPlayInstances[playRef].mSourceIndex = source;
        mPlayInstances[playRef].mSoundRef = soundRef;

        if (sourceInfo.mUnitIndex >= 0)
        {
            SetChannelGain(source, soundInfo.mVolume);
            SetChannelPan (source, soundInfo.mPan);
        }

        command->mInterrupt   = soundInfo.mRepeatCount != 0;
        command->mRepeatCount = soundInfo.mRepeatCount;
        command->mNumSamples  = soundInfo.mNumFrames;
        command->mDataLeft    = soundInfo.mDataLeft;
        command->mDataRight   = soundInfo.mDataRight;

        command->mPlayRef     = playRef;
        command->mSoundRef    = soundRef;

        sourceInfo.mCommands.CommitWrite();
        sourceInfo.mActiveCount++;

        mSoundInfo[soundRef].mUseCount++;

        return playRef;
    }

    return kNullRef;
}

void cAudioManager::SilenceChannel(int channelRef, bool immediately)
{
    if (channelRef < 0)
        return;

    cInputSourceInfo& sourceInfo = mInputSources[channelRef];

    cPlayCommand* command = sourceInfo.mCommands.NextWriteItem();

    if (command)
    {
        *command = kCommandSilence;
        command->mInterrupt   = immediately;

        sourceInfo.mCommands.CommitWrite();
        sourceInfo.mActiveCount++;
    }
}

tAudioParamRef cAudioManager::ParameterRefFromTag(tTag tag)
{
    auto it = mParamTagToIndex.find(tag);

    if (it != mParamTagToIndex.end())
        return it->second;

    return -1;
}

void cAudioManager::SetParameter(tAudioParamRef paramRef, float value)
{
    cParamInfo& info = mParamInfo[paramRef];
    info.mValue = value;

    if (info.mUnit)
    {
        float s = lerp(info.mParamRange[0], info.mParamRange[1], value);

        OSStatus result = AudioUnitSetParameter(info.mUnit, info.mID, info.mScope, info.mElement, s, 0);

        if (noErr != result)
            CL_LOG_E("Audio", "AudioUnitSetParameter error: %d for param %d\n", result, paramRef);
    }
}

float cAudioManager::Parameter(tAudioParamRef paramRef) const
{
    return mParamInfo[paramRef].mValue;
}

#ifndef CL_RELEASE
void cAudioManager::DebugMenu(cUIState* uiState)
{
    tUIItemID id = ItemID(0x018c26c2);

    if (!mConfig)
    {
        uiState->DrawLabel("No Config", kUIDisabled);
        return;
    }

    if (uiState->HandleToggle(id++, "Enabled", mMasterEnabled))
        SetMasterEnabled(!mMasterEnabled);

    float gain = mMasterGain;

    if (uiState->HandleSlider(id++, "Volume", &gain))
        SetMasterGain(gain);

    uiState->DrawSeparator();

    if (uiState->BeginSubMenu(id++, "Mixer"))
    {
        ShowMixerUI(uiState);

        uiState->EndSubMenu(id - 1);
    }

    if (uiState->BeginSubMenu(id++, "Params"))
    {
        tUIItemID itemID = ItemID(0x018c26c3);

        for (int i = 0, n = NumParameters(); i < n; i++)
        {
            const char* tag = ParameterTag(i);
            tAudioParamRef ref = ParameterRef(i);

            float param = Parameter(ref);

            if (uiState->HandleSlider(itemID++, tag, &param))
                SetParameter(ref, param);
        }

        uiState->EndSubMenu(id - 1);
    }

    if (uiState->BeginSubMenu(id++, "Sounds"))
    {
        tUIItemID itemID = ItemID(0x018c26c4);

        for (int i = 0, n = mSoundTagToIndex.size(); i < n; i++)
        {
            auto keyValue = mSoundTagToIndex.at(i);
            tTag tag = mSoundTagToIndex.at(i).first;
            const cSoundInfo& soundInfo = mSoundInfo[keyValue.second];

            if (uiState->HandleButtonWithSwatch(itemID++, tag, soundInfo.mUseCount ? kColourGreen : kColourGrey))
                PlaySound(keyValue.second, kDefaultGroupRef);
        }

        uiState->EndSubMenu(id - 1);
    }

    if (uiState->BeginSubMenu(id++, "Playing"))
    {
        tUIItemID itemID = ItemID(0x018c26c5);

        for (int i = 0, n = mPlayInstances.NumSlots(); i < n; i++)
        {
            if (!mPlayInstances.InUse(i))
                continue;

            const cPlayInfo& playInfo = mPlayInstances[i];

            const cSoundInfo& soundInfo = mSoundInfo[playInfo.mSoundRef];

            if (uiState->HandleButton(itemID++, soundInfo.mTag))
                StopSound(mPlayInstances.RefFromIndex(i), true);
        }

        uiState->EndSubMenu(id - 1);
    }
}

void cAudioManager::ShowMixerUI(cUIState* uiState)
{
    tUIItemID info = ItemID(0x0195db25, 0);

    string menuText;

    for (int i = 0, n = NumChannels(); i < n; i++)
    {
        bool channelEnabled = ChannelEnabled(i);

        if (uiState->HandleToggle(info++, menuText.format("Track %d", i).c_str(), &channelEnabled))
            SetChannelEnabled(i, channelEnabled);

        float gain = ChannelGain(i);

        if (uiState->HandleSlider(info++, "Level", &gain))
            SetChannelGain(i, gain);

        float pan = 0.5f * (ChannelPan(i) + 1.0f);

        if (uiState->HandleSlider(info++, "Pan", &pan))
            SetChannelPan(i, 2.0f * pan - 1.0f);

        int playing = ChannelPlaying(i);

        if (playing > 1)
            uiState->HandleButton(info++, menuText.format("Playing %d", playing).c_str());
        else if (playing > 0)
            uiState->HandleButton(info++, "Playing");
        else
            uiState->HandleButton(info++, "Silent");

        uiState->DrawSeparator();
    }
}
#endif



// cAudioManager

void cAudioManager::SetupAudioSession()
{
#ifdef CL_IOS
    // TODO: this is supposed to be replaced with shitty ObjC AVAudioSession. UGH.
    OSStatus result = AudioSessionInitialize(0, 0, InterruptionCallback, this);

    if (result != 0 && result != kAudioSessionAlreadyInitialized)   // kAudioSessionAlreadyInitialized because AudioSessionInitialize can be called implicitly by sys code =P
    {
        // ASI returns a fourCC as an error code! E.g. kAudioSessionAlreadyInitialized is 'init'.
        CL_LOG_E("Audio", "Couldn't initialise audio session: %c\n", result);
        return;
    }

    // if there is other audio playing, we don't want to play the background music
    uint32_t backgroundMusicPlaying;

    size_t size = sizeof(backgroundMusicPlaying);
    result = AudioSessionGetProperty(kAudioSessionProperty_OtherAudioIsPlaying, &size, &backgroundMusicPlaying);

    if (result != 0)
        CL_LOG_E("Audio", "Can't get property: %d\n", result);

    // kAudioSessionCategory_MediaPlayback;
    UInt32 category = (backgroundMusicPlaying) ? kAudioSessionCategory_AmbientSound : kAudioSessionCategory_SoloAmbientSound;
                
    result = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
    if (result != 0)
        CL_LOG_E("Audio", "Error setting session property: %d\n", result);

    result = AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange, RouteChangeListenerCallback, this);
    if (result != 0)
        CL_LOG_E("Audio", "Error adding listener: %d\n", result);

    // Request the desired hardware sample rate.
    mGraphSampleRate = 44100.0;

    result = AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate, sizeof(mGraphSampleRate), &mGraphSampleRate);
    if (result != 0)
        CL_LOG_E("Audio", "Can't set property: %d\n", result);

    result = AudioSessionSetActive(true);
    if (result != 0)
        CL_LOG_E("Audio", "Can't set session active: %d\n", result);

    size = sizeof(mGraphSampleRate);
    result = AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &size, &mGraphSampleRate);

    if (result != 0)
        CL_LOG_E("Audio", "Can't get property: %d\n", result);
#endif
}

void cAudioManager::SetupStreamFormats()
{
    // The AudioUnitSampleType data type is the recommended type for sample data in audio
    //    units. This obtains the byte size of the type for use in filling in the ASBD.
    size_t bytesPerSample = sizeof (AudioUnitSampleType);

    // Fill the application audio format struct's fields to define a linear PCM, 
    //        stereo, noninterleaved stream at the hardware sample rate.
    mStreamFormats[kFormatMono].mFormatID          = kAudioFormatLinearPCM;
    mStreamFormats[kFormatMono].mFormatFlags       = kAudioFormatFlagsAudioUnitCanonical;
    mStreamFormats[kFormatMono].mBytesPerPacket    = bytesPerSample;
    mStreamFormats[kFormatMono].mFramesPerPacket   = 1;
    mStreamFormats[kFormatMono].mBytesPerFrame     = bytesPerSample;
    mStreamFormats[kFormatMono].mChannelsPerFrame  = 1;                  // 1 indicates mono
    mStreamFormats[kFormatMono].mBitsPerChannel    = 8 * bytesPerSample;
    mStreamFormats[kFormatMono].mSampleRate        = mGraphSampleRate;

    // Fill the application audio format struct's fields to define a linear PCM,
    //        stereo, noninterleaved stream at the hardware sample rate.
    mStreamFormats[kFormatStereo].mFormatID          = kAudioFormatLinearPCM;
    mStreamFormats[kFormatStereo].mFormatFlags       = kAudioFormatFlagsAudioUnitCanonical;
    mStreamFormats[kFormatStereo].mBytesPerPacket    = bytesPerSample;
    mStreamFormats[kFormatStereo].mFramesPerPacket   = 1;
    mStreamFormats[kFormatStereo].mBytesPerFrame     = bytesPerSample;
    mStreamFormats[kFormatStereo].mChannelsPerFrame  = 2;                    // 2 indicates stereo
    mStreamFormats[kFormatStereo].mBitsPerChannel    = 8 * bytesPerSample;
    mStreamFormats[kFormatStereo].mSampleRate        = mGraphSampleRate;

    mStreamFormats[kFormatStereoReverb].mFormatID          = kAudioFormatLinearPCM;
    mStreamFormats[kFormatStereoReverb].mFormatFlags       = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
    mStreamFormats[kFormatStereoReverb].mBytesPerPacket    = sizeof(float);
    mStreamFormats[kFormatStereoReverb].mFramesPerPacket   = 1;
    mStreamFormats[kFormatStereoReverb].mBytesPerFrame     = sizeof(float);
    mStreamFormats[kFormatStereoReverb].mChannelsPerFrame  = 2;
    mStreamFormats[kFormatStereoReverb].mBitsPerChannel    = 8 * sizeof(float);
    mStreamFormats[kFormatStereoReverb].mSampleRate        = mGraphSampleRate;

#ifdef LOCAL_DEBUG
    for (int i = 0; i < kMaxStreamFormats; i++)
    {
        DumpASBD(mStreamFormats[kFormatMono]);
        DumpASBD(mStreamFormats[kFormatStereo]);
    }
#endif
}


bool cAudioManager::ReadAudioFile(const cFileSpec& spec, cSoundInfo* soundInfo)
{
    CL_ASSERT(!soundInfo->mDataLeft);
    CL_ASSERT(!soundInfo->mDataRight);

    CFURLRef sourceURL = CFURLCreateFromFileSystemRepresentation(0, (const UInt8*) spec.Path(), strlen(spec.Path()), false);

    // Instantiate an extended audio file object.
    ExtAudioFileRef audioFileObject = 0;
    
    // Open an audio file and associate it with the extended audio file object.
    OSStatus result = ExtAudioFileOpenURL(sourceURL, &audioFileObject);
    CFRelease(sourceURL);

    if (noErr != result || NULL == audioFileObject) 
    {
        CL_LOG_E("Audio", "ExtAudioFileOpenURL error: %d\n", result);
        return false;
    }

    // Get the audio file's length in frames.
    UInt64 totalFramesInFile = 0;
    UInt32 frameLengthPropertySize = sizeof (totalFramesInFile);
    
    result = ExtAudioFileGetProperty 
    (
        audioFileObject,
        kExtAudioFileProperty_FileLengthFrames,
        &frameLengthPropertySize,
        &totalFramesInFile
    );

    if (noErr != result) 
    {
        CL_LOG_E("Audio", "ExtAudioFileGetProperty (audio file length in frames) error: %d\n", result);
        return false;
    }
    
    // Assign the frame count to the mTrackSampleInfo instance variable
    soundInfo->mNumFrames = totalFramesInFile;

    // Get the audio file's number of channels.
    AudioStreamBasicDescription fileAudioFormat = {0};
    UInt32 formatPropertySize = sizeof (fileAudioFormat);
    
    result = ExtAudioFileGetProperty 
    (
        audioFileObject,
        kExtAudioFileProperty_FileDataFormat,
        &formatPropertySize,
        &fileAudioFormat
    );

    if (noErr != result)
    {
        CL_LOG_E("Audio", "ExtAudioFileGetProperty (file audio format) error: %d\n", result);
        return false;
    }

    UInt32 channelCount = fileAudioFormat.mChannelsPerFrame;
    
    // Allocate memory in the mTrackSampleInfo instance variable to hold the left channel, 
    //    or mono, audio data
    tSample* dataLeft = CreateArray<AudioUnitSampleType>(mSampleAllocator, totalFramesInFile);
    tSample* dataRight = 0;

    AudioStreamBasicDescription importFormat = { 0 };

    if (channelCount > 1)
    {
        dataRight = CreateArray<AudioUnitSampleType>(mSampleAllocator, totalFramesInFile);
        importFormat = mStreamFormats[kFormatStereo];
    }
    else if (channelCount > 0)
    {
        importFormat = mStreamFormats[kFormatMono];
    } 
    else 
    {
        CL_LOG_E("Audio", "*** WARNING: File format not supported - wrong number of channels");
        ExtAudioFileDispose(audioFileObject);
        mSampleAllocator->Free(dataLeft);
        
        return false;
    }

    // Assign the appropriate mixer input bus stream data format to the extended audio 
    //        file object. This is the format used for the audio data placed into the audio 
    //        buffer in the SoundStruct data structure, which is in turn used in the 
    //        InputRenderCallback callback function.

    result = ExtAudioFileSetProperty
    (
        audioFileObject,
        kExtAudioFileProperty_ClientDataFormat,
        sizeof(importFormat),
        &importFormat
    );

    if (noErr != result)
    {
        CL_LOG_E("Audio", "ExtAudioFileSetProperty (client data format) error: %d\n", result);
        return false;
    }
    
    // Set up an AudioBufferList struct, which has two roles:
    //
    //        1. It gives the ExtAudioFileRead function the configuration it 
    //            needs to correctly provide the data to the buffer.
    //
    //        2. It points to the mTrackSampleInfo[i].mAudioDataLeft buffer, so 
    //            that audio data obtained from disk using the ExtAudioFileRead function
    //            goes to that buffer

    // Allocate memory for the buffer list struct according to the number of 
    //    channels it represents.
    AudioBufferList* bufferList = CreateAudioBuffers(Allocator(), channelCount);

    // set up the AudioBuffer structs in the buffer list
    bufferList->mBuffers[0].mNumberChannels  = 1;
    bufferList->mBuffers[0].mDataByteSize    = totalFramesInFile * sizeof (AudioUnitSampleType);
    bufferList->mBuffers[0].mData            = dataLeft;

    if (channelCount > 1)
    {
        bufferList->mBuffers[1].mNumberChannels  = 1;
        bufferList->mBuffers[1].mDataByteSize    = totalFramesInFile * sizeof (AudioUnitSampleType);
        bufferList->mBuffers[1].mData            = dataRight;
    }

    // Perform a synchronous, sequential read of the audio data out of the file and
    //    into the mTrackSampleInfo[i].mAudioDataLeft and (if stereo) .mAudioDataRight members.
    UInt32 numberOfPacketsToRead = (UInt32) totalFramesInFile;
    
    result = ExtAudioFileRead
    (
        audioFileObject,
        &numberOfPacketsToRead,
        bufferList
    );

    Destroy(&bufferList, Allocator());
    ExtAudioFileDispose(audioFileObject);

    if (noErr != result)
    {
        CL_LOG_E("Audio", "ExtAudioFileRead failure -  error: %d\n", result);
        
        // If reading from the file failed, then free the memory for the sound buffers.
        mSampleAllocator->Free(dataLeft);
        mSampleAllocator->Free(dataRight);
        return false;
    }

    soundInfo->mDataLeft  = dataLeft;
    soundInfo->mDataRight = dataRight;

    return true;
}

namespace
{
    enum tAudioUnitType
    {
        kUnitMaster,
        kUnitMixer,
        kUnitReverb,
        kUnitLowPass,
        kMaxUnitTypes
    };

    const cEnumInfo kAudioUnitEnum[] =
    {
        "master",   kUnitMaster,
        "mixer",    kUnitMixer,
        "reverb",   kUnitReverb,
        "lowPass",   kUnitLowPass,
        0, 0
    };


    CL_STATIC_ASSERT(sizeof(AudioComponentDescription) == 5 * sizeof(uint32_t));
    AudioComponentDescription kUnitDescriptions[kMaxUnitTypes] =
    {
    //    OSType              componentType;
    //    OSType              componentSubType;
    //    OSType              componentManufacturer;
    //    UInt32              componentFlags;
    //    UInt32              componentFlagsMask;
        {
            kAudioUnitType_Output,
        #ifdef CL_IOS
            kAudioUnitSubType_RemoteIO,
        #else
            kAudioUnitSubType_DefaultOutput,
        #endif
            kAudioUnitManufacturer_Apple,
            0,
            0
        },

        // Multichannel mixer unit
        {
            kAudioUnitType_Mixer,
            kAudioUnitSubType_MultiChannelMixer,
            kAudioUnitManufacturer_Apple,
            0,
            0,
        },

        // Reverb
        {
            kAudioUnitType_Effect,
        #ifdef CL_IOS
            kAudioUnitSubType_Reverb2,
        #else
            kAudioUnitSubType_MatrixReverb,
        #endif
            kAudioUnitManufacturer_Apple,
            0,
            0
        },

        // Lowpass
        {
            kAudioUnitType_Effect,
            kAudioUnitSubType_LowPassFilter,
            kAudioUnitManufacturer_Apple,
            0,
            0
        }
    };
}

bool cAudioManager::ConfigAudio(const cObjectValue* config)
{
    OSStatus result = NewAUGraph(&mAudioGraph);

    if (noErr != result)
    {
        CL_LOG_E("Audio", "NewAUGraph error: %d\n", result);
        return false;
    }

    mGroupTagToIndex.clear();
    mGroupInfo.clear();
    mNumSources = 0;
    vector<pair<int, tTag>> aliases;

    for (auto c: config->Member("groups").Children())
    {
        int groupIndex = mGroupInfo.size();
        mGroupInfo.push_back();
        mGroupTagToIndex[c.Tag()] = groupIndex;

        cGroupInfo& info = mGroupInfo[groupIndex];

        tTag aliasTag = c.Value().Member("alias").AsTag();

        if (aliasTag)
            aliases.push_back( { groupIndex, aliasTag } );
        else
        {
            int numGroupSources = c.Value()["sources"].AsInt(1);
            if (numGroupSources < 1)
                numGroupSources = 1;

            info.mNumSources = numGroupSources;
            info.mSourceBase = mNumSources;

            mNumSources += numGroupSources;
        }
    }

    // Apply any aliases after all groups read.
    for (auto alias : aliases)
    {
        auto it = mGroupTagToIndex.find(alias.second);

        if (it != mGroupTagToIndex.end())
            mGroupInfo[alias.first] = mGroupInfo[it->second];
    }

    auto itDefault = mGroupTagToIndex.find(CL_TAG("default"));  // kDefaultTag

    if (itDefault != mGroupTagToIndex.end())
        mDefaultGroupRef = itDefault->second;
    else
        mDefaultGroupRef = 0;

    // temp backwards compatibility
    if (mNumSources == 0)
    {
        mNumSources = 8;

        mGroupInfo.push_back();
        mGroupInfo.back().mNumSources = 8;
    }

    mInputSources.resize(mNumSources);

    for (int i = 0; i < mNumSources; i++)
    {
        mInputSources[i].mCommands.Resize(64, Allocator());
        mInputSources[i].mResults.Resize(64, Allocator());
    }

    // Configure the rest of the system

    const tObjectChildren& unitChildren = config->Member("units").Children();
    int numUnits = unitChildren.size();

    int    unitTypes[numUnits];
    AUNode unitNodes[numUnits];

    for (int i = 0; i < numUnits; i++)
    {
        int& unitType = unitTypes[i];

        const cObjectValue* v = unitChildren[i].ObjectValue();

        if (!v)
        {
            unitType = kMaxUnitTypes;
            continue;
        }

        unitType = AsEnum(v->Member("type"), kAudioUnitEnum, kMaxUnitTypes);

        if (unitType != kMaxUnitTypes)
        {
            result = AUGraphAddNode
            (
                mAudioGraph,
                &kUnitDescriptions[unitType],
                &unitNodes[i]
            );
            
            if (noErr != result) 
            {
                CL_LOG_E("Audio", "AUGraphNewNode failed for I/O unit error: %d\n", result);
                unitType = kMaxUnitTypes;
            }
        }
    }

    result = AUGraphOpen(mAudioGraph);
    
    if (noErr != result)
    {
        CL_LOG_E("Audio", "AUGraphOpen error: %d\n", result);
        return false;
    }

    // Now set up audio units
    mUnits.clear();
    mUnitTagToIndex.clear();

    for (int i = 0; i < numUnits; i++)
    {
        if (unitTypes[i] == kMaxUnitTypes)
            continue;

        tTag tag = unitChildren[i].Tag();
        const cObjectValue* v = unitChildren[i].ObjectValue();
        CL_ASSERT(v);

        mUnitTagToIndex[tag] = mUnits.size();

        AudioUnit unit = 0;

        OSErr result = AUGraphNodeInfo
        (
            mAudioGraph,
            unitNodes[i],
            NULL,
            &unit
        );

        if (result != noErr)
            CL_LOG_E("Audio", "AUGraphNodeInfo failed: %d\n", result);

        mUnits.push_back(unit);
        mUnitNodes.push_back(unitNodes[i]);

        switch (unitTypes[i])
        {
        case kUnitMixer:
            ConfigMixer(v, unitNodes[i], unit);
            break;
        case kUnitReverb:
            ConfigReverb(v, unitNodes[i], unit);
            break;
        case kUnitLowPass:
            ConfigLowPass(v, unitNodes[i], unit);
            break;
        }
    }

    mMixerUnitIndex = UnitIndexFromTag(CL_TAG("mixer"));

    // Finally, connect them together
    const cValue& connectionsV = config->Member("connections");

    for (int i = 0, n = connectionsV.NumElts(); i < n; i++)
    {
        // { from: unitA, to: unitB }

        const cObjectValue* v = connectionsV[i].AsObject();

        if (!v)
            continue;

        tTag sourceUnitTag = v->Member("from").AsTag();
        tTag   destUnitTag = v->Member("to"  ).AsTag();

        int sourceUnitIndex = UnitIndexFromTag(sourceUnitTag);
        int   destUnitIndex = UnitIndexFromTag(  destUnitTag);

        if (sourceUnitIndex < 0)
            CL_LOG_E("Audio", "Unknown connection source: " CL_TAG_FMT "\n", sourceUnitTag);
        if (destUnitIndex < 0)
            CL_LOG_E("Audio", "Unknown connection dest: " CL_TAG_FMT "\n", destUnitTag);

        if (sourceUnitIndex < 0 || destUnitIndex < 0)
            continue;

        int sourceBus = v->Member("fromBus").AsInt(0);
        int   destBus = v->Member(  "toBus").AsInt(0);

        AUNode sourceUnit = mUnitNodes[sourceUnitIndex];
        AUNode   destUnit = mUnitNodes[  destUnitIndex];

        result = AUGraphConnectNodeInput(mAudioGraph, sourceUnit, sourceBus, destUnit, destBus);

        if (result != noErr)
            CL_LOG_E("Audio", "AUGraphConnectNodeInput failed: %d\n", result);
    }

#ifdef LOCAL_DEBUG
    CL_LOG_E("Audio", "Audio graph state before Initialize:");
    CAShow (mAudioGraph);
#endif

    result = AUGraphInitialize (mAudioGraph);

    if (noErr != result)
    {
        CL_LOG_E("Audio", "AUGraphInitialize error: %d\n", result);
        return false;
    }

    return true;
}

void cAudioManager::ConfigMaster(const cObjectValue* config, AUNode node, AudioUnit unit)
{
}

void cAudioManager::ConfigMixer(const cObjectValue* config, AUNode node, AudioUnit unit)
{
    const cValue& channelsV = config->Member("channels");
    UInt32 busCount = channelsV.size();

    OSErr result = AudioUnitSetProperty
    (
        unit,
        kAudioUnitProperty_ElementCount,
        kAudioUnitScope_Input,
        0,
        &busCount,
        sizeof (busCount)
    );

    if (noErr != result)
    {
        CL_LOG_E("Audio", "AudioUnitSetProperty (set mixer unit bus count) error: %d\n", result);
        return;
    }

    // Increase the maximum frames per slice allows the mixer unit to accommodate the
    //    larger slice size used when the screen is locked.
    UInt32 maximumFramesPerSlice = 4096;
    
    result = AudioUnitSetProperty 
    (
         unit,
         kAudioUnitProperty_MaximumFramesPerSlice,
         kAudioUnitScope_Global,
         0,
         &maximumFramesPerSlice,
         sizeof (maximumFramesPerSlice)
     );

    if (noErr != result)
    {
        CL_LOG_E("Audio", "AudioUnitSetProperty (set mixer unit input stream format) error: %d\n", result);
        // non-fatal error...
    }

    // Attach the input render callback and context to each input bus
    for (int i = 0; i < busCount; i++)
    {
        const cValue& channelV = channelsV[i];

        tTag sourceGroup = channelV["group"].AsTag();
        int sourceIndex = channelV["source"].AsInt(-1);

        if (sourceGroup)
        {
            auto it = mGroupTagToIndex.find(sourceGroup);
            if (it != mGroupTagToIndex.end())
            {
                const cGroupInfo& groupInfo = mGroupInfo[it->second];

                if (sourceIndex < groupInfo.mNumSources)
                    sourceIndex += groupInfo.mSourceBase;
                else
                    sourceIndex = -1;
            }
        }
        else if (sourceIndex >= mNumSources)
            sourceIndex = -1;

        if (sourceIndex >= 0 && sourceIndex < mNumSources)
        {
            // Setup the struture that contains the input render callback
            AURenderCallbackStruct inputCallbackStruct =
            {
                &InputRenderCallback,
                &mInputSources[sourceIndex]
            };

            result = AUGraphSetNodeInputCallback
            (
                mAudioGraph,
                node,
                i,
                &inputCallbackStruct
            );

            if (noErr != result)
            {
                CL_LOG_E("Audio", "AUGraphSetNodeInputCallback error: %d\n", result);
            }

            mInputSources[sourceIndex].mUnitIndex = mUnits.size() - 1;    // TODO, pass index directly
            mInputSources[sourceIndex].mUnitBus = i;
        }

        tStreamFormat format;
        if (channelV["stereo"].AsBool())
            format = kFormatStereo;
        else
            format = kFormatMono;

        result = AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
            i, &mStreamFormats[format], sizeof (mStreamFormats[format]));

        if (noErr != result)
            CL_LOG_E("Audio", "AudioUnitSetProperty (set mixer unit input bus stream format) error: %d\n", result);
    }

    result = AudioUnitSetProperty
    (
        unit,
        kAudioUnitProperty_SampleRate,
        kAudioUnitScope_Output,
        0,
        &mGraphSampleRate,
        sizeof (mGraphSampleRate)
    );

    if (noErr != result)
        CL_LOG_E("Audio", "AudioUnitSetProperty (set mixer unit output stream format) error: %d\n", result);
}

namespace
{
    void SetGlobalParam(AudioUnit unit, AudioUnitParameterID id, AudioUnitParameterValue value)
    {
        OSStatus result = AudioUnitSetParameter(unit, id, kAudioUnitScope_Global, 0, value, 0);

        if (result != noErr)
            CL_LOG_E("Audio", "AudioUnitSetProperty error: %d\n", result);
    }
}

void cAudioManager::ConfigReverb(const cObjectValue* config, AUNode node, AudioUnit unit)
{
#ifdef CL_IOS
    // TEMP
    SetGlobalParam(unit, kReverb2Param_DryWetMix,            config->Member("dryWetMix" ).AsFloat(50.0f));
    SetGlobalParam(unit, kReverb2Param_Gain,                 config->Member("gain"      ).AsFloat(0.0f));

    SetGlobalParam(unit, kReverb2Param_DecayTimeAt0Hz,       config->Member("decayAtMin").AsFloat(1.00f));
    SetGlobalParam(unit, kReverb2Param_DecayTimeAtNyquist,   config->Member("decayAtMax").AsFloat(0.50f));
    SetGlobalParam(unit, kReverb2Param_MinDelayTime,         config->Member("minDelay"  ).AsFloat(0.01f));
    SetGlobalParam(unit, kReverb2Param_MaxDelayTime,         config->Member("maxDelay"  ).AsFloat(0.02f));
    SetGlobalParam(unit, kReverb2Param_RandomizeReflections, config->Member("randomiseReflections").AsFloat(10.0f));
#else
    SetGlobalParam(unit, kReverbParam_DryWetMix,             config->Member("dryWetMix" ).AsFloat(50.0f)); // 0 - 100
    SetGlobalParam(unit, kReverbParam_FilterGain,            config->Member("gain" ).AsFloat(0.0f));    // -18 - 18

#if 0
		// Global, Hertz, 10.0 -> 20000.0, 800.0
	kReverbParam_FilterFrequency					= 14,

		// Global, Octaves, 0.05 -> 4.0, 3.0
	kReverbParam_FilterBandwidth					= 15,

		// Global, EqPow CrossFade, 0->100, 50
	kReverbParam_SmallLargeMix						= 1,
		
		// Global, Secs, 0.005->0.020, 0.06
	kReverbParam_SmallSize							= 2,
		
		// Global, Secs, 0.4->10.0, 3.07
	kReverbParam_LargeSize							= 3,
		
		// Global, Secs, 0.001->0.03, 0.025
	kReverbParam_PreDelay							= 4,
		
		// Global, Secs, 0.001->0.1, 0.035
	kReverbParam_LargeDelay							= 5,
		
		// Global, Genr, 0->1, 0.28
	kReverbParam_SmallDensity						= 6,
		
		// Global, Genr, 0->1, 0.82
	kReverbParam_LargeDensity						= 7,
		
		// Global, Genr, 0->1, 0.3
	kReverbParam_LargeDelayRange					= 8,
		
		// Global, Genr, 0.1->1, 0.96
	kReverbParam_SmallBrightness					= 9,
		
		// Global, Genr, 0.1->1, 0.49
	kReverbParam_LargeBrightness					= 10,

		// Global, Genr, 0->1 0.5
	kReverbParam_SmallDelayRange					= 11,

		// Global, Hz, 0.001->2.0, 1.0
	kReverbParam_ModulationRate						= 12,

		// Global, Genr, 0.0 -> 1.0, 0.2
	kReverbParam_ModulationDepth					= 13,

		// Global, Hertz, 10.0 -> 20000.0, 800.0
	kReverbParam_FilterFrequency					= 14,

		// Global, Octaves, 0.05 -> 4.0, 3.0
	kReverbParam_FilterBandwidth					= 15,

		// Global, Decibels, -18.0 -> +18.0, 0.0
	kReverbParam_FilterGain							= 16
#endif

#endif
}

void cAudioManager::ConfigLowPass(const cObjectValue* config, AUNode node, AudioUnit unit)
{
		// Global, Hz, 10->(SampleRate/2), 6900
    SetGlobalParam(unit, kLowPassParam_CutoffFrequency, config->Member("cutoff").AsFloat(6900.0f));
    SetGlobalParam(unit, kLowPassParam_Resonance, config->Member("resonance").AsFloat(0.0f));
}

void cAudioManager::BeginInterruption()
{
    CL_LOG("Audio", "Audio session was interrupted.");
    
    if (mIsPlaying)
    {
        mPlaybackInterrupted = true;
        StopPlaying();
    }
}


void cAudioManager::EndInterruption()
{
    if (mPlaybackInterrupted)
    {
        mPlaybackInterrupted = false;

        StartPlaying();
    }
}


// Debug

// You can use this method during development and debugging to look at the
//    fields of an AudioStreamBasicDescription struct.
void cAudioManager::DumpASBD(const AudioStreamBasicDescription& asbd)
{

    char formatIDString[5];
    UInt32 formatID = CFSwapInt32HostToBig (asbd.mFormatID);
    bcopy (&formatID, formatIDString, 4);
    formatIDString[4] = '\0';
    
    CL_LOG("Audio", "  Sample Rate:         %10.0f",  asbd.mSampleRate);
    CL_LOG("Audio", "  Format ID:           %10s",    formatIDString);
    CL_LOG("Audio", "  Format Flags:        %10X",    asbd.mFormatFlags);
    CL_LOG("Audio", "  Bytes per Packet:    %10d",    asbd.mBytesPerPacket);
    CL_LOG("Audio", "  Frames per Packet:   %10d",    asbd.mFramesPerPacket);
    CL_LOG("Audio", "  Bytes per Frame:     %10d",    asbd.mBytesPerFrame);
    CL_LOG("Audio", "  Channels per Frame:  %10d",    asbd.mChannelsPerFrame);
    CL_LOG("Audio", "  Bits per Channel:    %10d",    asbd.mBitsPerChannel);
}


cIAudioManager* nHL::CreateAudioManager(nCL::cIAllocator* alloc)
{
    return new(alloc) cAudioManager;
}
