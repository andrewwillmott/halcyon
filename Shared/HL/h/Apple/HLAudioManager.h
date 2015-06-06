//
//  File:       HLAudioManager.h
//
//  Function:   Audio system
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_AUDIO_H
#define HL_AUDIO_H

#include <IHLAudioManager.h>

#include <HLDefs.h>

#include <CLFIFO.h>
#include <CLMemory.h>
#include <CLSTL.h>
#include <CLSlotArray.h>
#include <CLValue.h>

#include <AudioToolbox/AudioToolbox.h>

namespace nHL
{
    typedef AudioUnitSampleType tSample;
    ///< Currently AudioUnitSampleType is float on OSX and 16-bit fixed point on iOS.

    struct cPlayCommand
    /// Details a sound to be played
    {
        bool            mInterrupt   = false;   ///< Set if this should interrupt current sound
        int             mRepeatCount = 0;       ///< How many times to play, 0 = loop forever
        uint32_t        mNumSamples  = 0;
        const tSample*  mDataLeft    = 0;
        const tSample*  mDataRight   = 0;
        cSlotRef        mPlayRef;               ///< ID of this invocation of the sound
        int             mSoundRef    = -1;      ///< Sound we're playing, for reload convenience
    };

    struct cInputSourceInfo
    /// Control info for a single input source
    {
        // Accessed by audio callback in audio thread
        nCL::vector<cPlayCommand> mQueuedCommands;   ///< Queued commands to play next, transferred from mCommands as appropriate

        int32_t         mSampleCursor   = 0;    ///< Where we are in the current sample being played.
        int32_t         mLoopCount      = 0;    ///< Number of times we've played/looped

        cPlayCommand    mCurrent;               ///< Current command

        // Communication
        nCL::cFIFO<cPlayCommand> mCommands;     ///< Written by manager thread, read by player thread
        nCL::cFIFO<cSlotRef>     mResults;      ///< Written by player thread, read by manager thread.

        // Accessed by manager in main thread
        int mActiveCount    = 0;    ///< Number of active playing/queued sounds
        int mAge            = 0;    ///< Used in voice allocation

        // Read-only
        int mUnitIndex = -1;    // Unit input this source is attached to.
        int mUnitBus   = 0;

        bool PopNextCommand(int inputBus);
    };

    struct cSoundInfo
    /// Per-sound info
    {
        int      mUseCount = 0;     ///< How many sources are using this -- can't remove when non-zero.

        uint32_t mNumFrames   = 0;
        int      mRepeatCount = 1;

        const tSample* mDataLeft  = 0;  ///< the complete left (or mono) channel of audio data read from an audio file
        const tSample* mDataRight = 0;  ///< the complete right channel of audio data read from an audio file, or 0 if mono

        float   mVolume = 1.0f;     ///< Default volume
        float   mPan    = 0.0f;     ///< Default pan

        tTag    mTag = 0;   ///< For debugging, source tag
    };

    enum tStreamFormat
    {
        kFormatMono,
        kFormatStereo,
        kFormatStereoReverb,
        kMaxStreamFormats
    };

    struct cGroupInfo
    {
        int mNumSources = 0;
        int mSourceBase = 0;
    };

    struct cPlayInfo
    {
        int mSourceIndex = -1;  // Source we're being played on
        int mSoundRef = -1;     // Sound we're playing
        // ...
    };

    struct cParamInfo
    {
        float mValue = 0.0f;    // 0-1 parameter input

        // Direct mapping for now
        Vec2f                 mParamRange = { 0.0f, 1.0f };      // useful range of unit parameter
        AudioUnit             mUnit = 0;
        AudioUnitParameterID  mID   = 0;
        AudioUnitScope        mScope = kAudioUnitScope_Global;
        AudioUnitElement      mElement = 0;
    };

    // --- cAudioManager -------------------------------------------------------

    class cAudioManager :
        public cIAudioManager,
        public nCL::cAllocatable
    {
    public:
        // cIAudioManager
        void Init() override;
        void Shutdown() override;

        void Update(float dt) override;

        void LoadSounds(const nCL::cObjectValue* config) override;

        int  NumChannels() const override;

        void  SetMasterEnabled(bool enabled) override;
        bool  MasterEnabled   () const override;
        void  SetMasterGain   (float gain) override;
        float MasterGain      () const override;

        int  SoundRefFromTag  (tTag tag) override;
        int  GroupRefFromTag  (tTag tag) override;

        tAudioPlayRef PlaySound (int soundRef, int groupRef) override;
        bool          StopSound (tAudioPlayRef playRef, bool immediately) override;
        bool          StopSounds(tAudioGroupRef groupRef, bool immediately) override;

        void SetChannelEnabled(int channel, bool  enabled) override;
        bool    ChannelEnabled(int channel) const;
        void SetChannelGain   (int channel, float gain) override;
        float   ChannelGain   (int channel) const;
        void SetChannelPan    (int channel, float pan) override;
        float   ChannelPan    (int channel) const;

        int  ChannelPlaying   (int channel) const override;

        tAudioPlayRef PlayChannel(int channelRef, int soundRef) override;
        void SilenceChannel(int channelRef, bool immediately) override;     // Stop current sound at end of its next loop, or immediately.

        tAudioParamRef ParameterRefFromTag(tTag tag) override;
        int            NumParameters() const override;
        tTag           ParameterTag(int i) const override;
        tAudioParamRef ParameterRef(int i) const override;

        void  SetParameter(tAudioParamRef paramRef, float value) override;
        float Parameter   (tAudioParamRef paramRef) const override;

        void DebugMenu(cUIState* uiState) override;

        // cAudioManager
        void StartPlaying();
        void StopPlaying();
        bool IsPlaying();

        // Sessions
        void BeginInterruption();
        void EndInterruption();

    protected:
        // cAudioManager
        cIAllocator* Allocator();

        void SetupAudioSession();
        void SetupStreamFormats();

        bool ReadAudioFile(const cFileSpec& spec, cSoundInfo* soundInfo);

        bool ConfigAudio  (const cObjectValue* config);
        void ConfigMaster (const cObjectValue* config, AUNode node, AudioUnit unit);
        void ConfigMixer  (const cObjectValue* config, AUNode node, AudioUnit unit);
        void ConfigReverb (const cObjectValue* config, AUNode node, AudioUnit unit);
        void ConfigLowPass(const cObjectValue* config, AUNode node, AudioUnit unit);

        int UnitIndexFromTag(tTag tag) const;

        // Debug
        void DumpASBD(const AudioStreamBasicDescription& asbd);
        void ShowMixerUI(cUIState* uiState);

    protected:
        // Data decls
        typedef nCL::map<tTag, int> tTagToIndexMap;

        // Data
        cIAllocator*                    mSampleAllocator = 0;
        nCL::tConstObjectLink           mConfig;
        uint32_t                        mConfigModCount = 0;
        nCL::tObjectLink                mPreferences;

        tTagToIndexMap                  mGroupTagToIndex;
        nCL::vector<cGroupInfo>         mGroupInfo;
        int                             mDefaultGroupRef = 0;

        tTagToIndexMap                  mParamTagToIndex;
        nCL::vector<cParamInfo>         mParamInfo;

        int                             mNumSources = 0;
        Float64                         mGraphSampleRate = 44100.0;

        float                           mMasterGain = -1.0f;
        bool                            mMasterEnabled = false;
        
        nCL::vector<bool>               mChannelEnabled;
        nCL::vector<float>              mChannelGain;
        nCL::vector<float>              mChannelPan;

        tTagToIndexMap                  mSoundTagToIndex;
        nCL::vector<cSoundInfo>         mSoundInfo;

        bool                            mIsPlaying = false;
        bool                            mPlaybackInterrupted = false;

        nCL::vector<cInputSourceInfo>   mInputSources;    ///< One of these for each input source

        nCL::cSlotArrayT<cPlayInfo>     mPlayInstances;    ///< Sound instances currently being played

        // AudioUnit stuff
        AudioStreamBasicDescription     mStreamFormats[kMaxStreamFormats]   = { 0 };

        AUGraph                         mAudioGraph = 0;
        tTagToIndexMap                  mUnitTagToIndex;
        nCL::vector<AudioUnit>          mUnits;
        nCL::vector<AUNode>             mUnitNodes;
        int                             mMixerUnitIndex = -1;    ///< hardcoded for now
    };


    inline int cAudioManager::NumChannels() const
    {
        return mNumSources;
    }

    inline bool cAudioManager::MasterEnabled() const
    {
        return mMasterEnabled;
    }
    inline float cAudioManager::MasterGain() const
    {
        return mMasterGain;
    }

    inline bool cAudioManager::ChannelEnabled(int channel) const
    {
        return mChannelEnabled[channel];
    }

    inline float cAudioManager::ChannelGain(int channel) const
    {
        return mChannelGain[channel];
    }

    inline float cAudioManager::ChannelPan(int channel) const
    {
        return mChannelPan[channel];
    }

    int  cAudioManager::NumParameters() const
    {
        return mParamTagToIndex.size();
    }

    tTag cAudioManager::ParameterTag(int i) const
    {
        return mParamTagToIndex.at(i).first;
    }
    tAudioParamRef cAudioManager::ParameterRef(int i) const
    {
        return mParamTagToIndex.at(i).second;
    }

    inline cIAllocator* cAudioManager::Allocator()
    {
        return AllocatorFromObject(this);
    }
    inline bool cAudioManager::IsPlaying()
    {
        return mIsPlaying;
    }

    inline int cAudioManager::UnitIndexFromTag(tTag tag) const
    {
        auto it = mUnitTagToIndex.find(tag);

        if (it != mUnitTagToIndex.end())
            return it->second;

        return -1;
    }
}

#endif
