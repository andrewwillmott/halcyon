//
//  File:       IHLAudioManager.h
//
//  Function:   Audio system
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_I_AUDIO_MANAGER_H
#define HL_I_AUDIO_MANAGER_H

#include <HLDefs.h>
#include <CLSlotRef.h>

namespace nHL
{
    class cUIState;

    typedef int tAudioParamRef;
    typedef int tAudioGroupRef;
    typedef int tAudioSoundRef;

    //enum tAudioPlayRef : int { kNullAudioPlayRef = -1 };
    typedef cSlotRef tAudioPlayRef;
    const cSlotRef kNullAudioPlayRef;

    const int kDefaultGroupRef = -1;

    class cIAudioManager
    {
    public:
        virtual void Init() = 0;
        virtual void Shutdown() = 0;

        virtual void Update(float dt) = 0;

        virtual void LoadSounds(const nCL::cObjectValue* config) = 0;

        virtual int  NumChannels() const = 0;

        virtual void  SetMasterEnabled(bool enabled) = 0;
        virtual bool  MasterEnabled   () const = 0;
        virtual void  SetMasterGain   (float gain) = 0; ///< Set master volume, 0-1
        virtual float MasterGain      () const = 0;

        virtual int   SoundRefFromTag  (tTag tag) = 0;      ///< Returns ref to given sound, or -1 for none
        virtual int   GroupRefFromTag  (tTag tag) = 0;      ///< Returns ref to the given playback group, or -1 for none

        virtual tAudioPlayRef PlaySound (tAudioSoundRef soundRef, tAudioGroupRef groupRef = -1) = 0;  ///< Play the given sound using the given playback group. Returns a play reference.
        virtual bool          StopSound (tAudioPlayRef  playRef,  bool immediately = true) = 0;       ///< Stop sound indicated by the given play ref
        virtual bool          StopSounds(tAudioGroupRef groupRef, bool immediately = true) = 0;       ///< Stop all sounds in the given group.

        // Note: channel APIs are deprecated.
        virtual void SetChannelEnabled(int channel, bool enabled) = 0;  // Set whether channel is currently playing
        virtual void SetChannelPan    (int channel, float pan) = 0;     // -1 = left, +1 = right, 0 = centre
        virtual void SetChannelGain   (int channel, float gain) = 0;    // Set volume for this channel, 0 - 1
        virtual int  ChannelPlaying   (int channel) const = 0;          // Returns number of sounds playing and queued to play on this channel

        virtual tAudioPlayRef PlayChannel   (int channelRef, int soundRef) = 0;     // Play the given sound on the given channel. Returns a play reference
        virtual void          SilenceChannel(int channelRef, bool immediately = false) = 0;     // Stop current sound at end of its next loop, or immediately.


        // Parameters
        virtual tAudioParamRef ParameterRefFromTag(tTag tag) = 0;
        virtual int            NumParameters() const = 0;
        virtual tTag           ParameterTag(int i) const = 0;
        virtual tAudioParamRef ParameterRef(int i) const = 0;

        virtual void  SetParameter(tAudioParamRef paramRef, float value) = 0;
        virtual float Parameter   (tAudioParamRef paramRef) const = 0;

    #ifndef CL_RELEASE
        // Development
        virtual void DebugMenu(cUIState* uiState) = 0;
    #endif
    };

    cIAudioManager* CreateAudioManager(nCL::cIAllocator* alloc);
}

#endif
