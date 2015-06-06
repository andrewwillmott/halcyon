//
//  File:       CLInputState.h
//
//  Function:   Centralises state for UI interaction, allowing a poll-style
//              model rather than event-handling.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_INPUT_STATE_H
#define CL_INPUT_STATE_H

#include <VL234f.h>
#include <CLDefs.h>

namespace nCL
{
    // These are special keycodes, mapping directly to keys hit on a keyboard.
    // Text input should use a different mechanism.
    // Keys not represented here simply use the standard ascii character, e.g.,
    // 'A', ';', etc.
    enum tKeyCode : uint8_t
    {
        // Known 'standard' codes
        kNullKey        = 0,
        
        kTabKey         = 8,
        kReturnKey      = 13,
        kEscapeKey      = 27,
        kSpaceKey       = 32,

        // General
        kLeftShiftKey   = 96,
        kLeftControlKey,
        kLeftAltKey,
        kLeftCommandKey,

        kRightShiftKey,
        kRightControlKey,
        kRightAltKey,
        kRightCommandKey,

        kLeftKey,     
        kRightKey,    
        kDownKey,     
        kUpKey,

        kInsertKey,
        kDeleteKey,
        kHomeKey,
        kEndKey,
        kPageUpKey,
        kPageDownKey, 

        kHelpKey,
        kForwardDeleteKey,
        kCapsLockKey,
        kFunctionKey,

        kF1,
        kF2,
        kF3,
        kF4,
        kF5,
        kF6,
        kF7,
        kF8,
        kF9,
        kF10,
        kF11,
        kF12,
        kF13,
        kF14,
        kF15,
        kF16,
        kF17,
        kF18,
        kF19,
        kF20,

        kVolumeUpKey,
        kVolumeDownKey,
        kVolumeMuteKey,

        kKeyPadPlus,  	
        kKeyPadMinus, 	
        kKeyPadMultiply,
        kKeyPadDivide,	
        kKeyPadEnter, 	
        kKeyPadPeriod,  
        kKeyPadEquals,  
        kKeyPad0,
        kKeyPad1,
        kKeyPad2,
        kKeyPad3,
        kKeyPad4,
        kKeyPad5,
        kKeyPad6,
        kKeyPad7,
        kKeyPad8,
        kKeyPad9,

        kMaxKeyCode = 255
    };

    typedef uint32_t tModifierSet;
    enum tModifiers : tModifierSet
    {
        kModNone        = 0,
        
        // Keyboard
        kModShift       = 0x01,
        kModControl     = 0x02,
        kModAlt         = 0x04,
        kModCommand     = 0x08,
        kModFunction    = 0x10,
        
        // Pointer
        kModTouch       = 0x0100,   // Finger or stylus down
        kModButton1     = 0x0100,   // E.g., Mouse button
        kModButton2     = 0x0200,
        kModButton3     = 0x0400,
        kModButton4     = 0x0800,
        kModButton5     = 0x1000,
        
        kModPointerMask = 0xFF00,   // Whether pointer is regarded as 'down', i.e., active interaction.
    };

    struct cPointerInfo
    /// Extended information about a pointer
    {
        tModifierSet    mModifiers      = kModNone; ///< Current modifiers (e.g., buttons or finger down). See tModifiers
        tModifierSet    mModifiersDown  = kModNone; ///< Modifiers that went down this period
        tModifierSet    mModifiersUp    = kModNone; ///< Modifiers that went up this period
        tModifierSet    mLastModifiers  = kModNone; ///< Last modifiers.
        
        Vec2f           mCurrentPos     = vl_0;     ///< Current pointer location.
        Vec2f           mLastPos        = vl_0;     ///< Last pointer location.
        
        Vec2f           mStartPos       = vl_0;     ///< Where pointer last went down
        tModifierSet    mStartModifiers = kModNone; ///< Modifiers when pointer went down, including global modifiers (like keyboard modifiers)
        uint32_t        mStartCounter   = 0;        ///< Counter when pointer went down
        uint64_t        mStartTimeStamp = 0;
    };

    class cInputState
    {
    public:
        enum tConstants
        {
            kMaxPointers = 16,

            kMaxKeyCodeWords = (kMaxKeyCode + 1) / 32,
            kKeyCodeWordShift = 5,
            kKeyCodeWordMask  = 0x1F
        };

        cInputState();
        
        void Begin();
        void End();

        // Pointer state
        int  NumActivePointers() const; ///< Returns the number of currently active pointers.

        bool PointerIsDown  (int index) const;
        bool PointerWentDown(int index) const;
        bool PointerWentUp  (int index) const;

        uint32_t PointerStateWentOn (int index) const;    ///< Returns state bits that went on
        uint32_t PointerStateWentOff(int index) const;    ///< Returns state bits that went off

        const cPointerInfo& PointerInfo(int index) const;
        ///< Return extended pointer info. Note that a given pointer will have a consistent index
        ///< for as long as it is down. Because of this, there may be some "active" pointers
        ///< (index < NumActivePointers) that are currently up. If you place fingers 1..4 down in
        ///< order, and then lift finger 2, NumActivePointers() will still be 4, and PointerIsDown(1)
        ///< will be false.

        // Keyboard state
        bool KeyIsDown  (int keyCode, tModifierSet modifiers = kModNone) const;
        bool KeyChanged (int keyCode, tModifierSet modifiers = kModNone) const;
        bool KeyWentDown(int keyCode, tModifierSet modifiers = kModNone) const;
        bool KeyWentUp  (int keyCode, tModifierSet modifiers = kModNone) const;

        tModifierSet KeyModifiers() const;

        // Secondary controllers
        Vec3f Acceleration() const;
        Vec2f ScrollDelta() const;

        // Timestamps
        int        Counter() const;
        uint64_t   TimeStamp() const;

        void Reset();   ///< Reset all state back to defaults.

        // Callbacks -- owner should call these to update state
        void OnPointerDown (int index, float x, float y, int button);   ///< Given pointer went down
        void OnPointerUp   (int index, float x, float y, int button);   ///< Given pointer went up
        void OnPointerMove (int index, float x, float y);               ///< Given pointer moved
        void OnPointersCancel();  ///< Reset all pointers

        void OnKeyDown(int index, int keyCode);
        void OnKeyUp  (int index, int keyCode);
        void OnKeyModifiers(int index, tModifierSet modifiers);   ///< Call with updated values of (all) keyboard modifiers, whenever the state of one changes

        void OnAcceleration(Vec3f acc);
        void OnScroll(Vec2f delta);
        void OnTimeStamp(uint64_t timeStampMS);

    protected:
        int             mNumPointers = 0;          /// Number of pointers currently being tracked. (Either down or went down during the course of input.
        cPointerInfo    mPointers[kMaxPointers];

        // Just one keyboard for now...
        uint32_t        mKeyStates    [kMaxKeyCodeWords] = { 0 };
        uint32_t        mLastKeyStates[kMaxKeyCodeWords] = { 0 };
        tModifierSet    mKeyModifiers                    = kModNone;
        tModifierSet    mLastKeyModifiers                = kModNone;

        Vec3f           mAcceleration = vl_0;

        Vec2f           mScrollDelta     = vl_0;
        Vec2f           mNextScrollDelta = vl_0;

        uint32_t        mCounter = 0;       ///< History counter, incremented for every Begin()/End()
        uint64_t        mTimeStamp = 0;     ///< Timestamp in ms
    };

    // --- Inlines -------------------------------------------------------------

    inline int cInputState::NumActivePointers() const
    {
        return mNumPointers;
    }

    inline bool cInputState::PointerIsDown(int index) const
    {
        CL_ASSERT(uint32_t(index) < kMaxPointers);

        return mPointers[index].mModifiers != 0;
    }

    inline bool cInputState::PointerWentDown(int index) const
    {
        CL_ASSERT(uint32_t(index) < kMaxPointers);

//        uint32_t changedOn = (mPointers[index].mModifiers ^ mPointers[index].mLastModifiers) & mPointers[index].mModifiers;
        uint32_t changedOn = mPointers[index].mModifiersDown;

        return changedOn != 0;
    }

    inline bool cInputState::PointerWentUp(int index) const
    {
        CL_ASSERT(uint32_t(index) < kMaxPointers);

//        uint32_t changedOff = (mPointers[index].mModifiers ^ mPointers[index].mLastModifiers) & ~mPointers[index].mModifiers;
        uint32_t changedOff = mPointers[index].mModifiersUp;

        return changedOff != 0;
    }

    inline uint32_t cInputState::PointerStateWentOn(int index) const
    {
        CL_ASSERT(uint32_t(index) < kMaxPointers);

        return (mPointers[index].mModifiers ^ mPointers[index].mLastModifiers) & mPointers[index].mModifiers;
    }

    inline uint32_t cInputState::PointerStateWentOff(int index) const
    {
        CL_ASSERT(uint32_t(index) < kMaxPointers);

        return (mPointers[index].mModifiers ^ mPointers[index].mLastModifiers) & ~mPointers[index].mModifiers;
    }

    inline const cPointerInfo& cInputState::PointerInfo(int index) const
    {
        return mPointers[index];
    }

    inline bool cInputState::KeyIsDown(int keyCode, tModifierSet modifiers) const
    {
        CL_INDEX(keyCode, kMaxKeyCode);
        return mKeyModifiers == modifiers
            && (mKeyStates[keyCode >> kKeyCodeWordShift] >> (keyCode & kKeyCodeWordMask)) & 1;
    }

    inline bool cInputState::KeyChanged(int keyCode, tModifierSet modifiers) const
    {
        CL_INDEX(keyCode, kMaxKeyCode);
        return mKeyModifiers == modifiers
            && ((mLastKeyStates[keyCode >> kKeyCodeWordShift] ^ mKeyStates[keyCode >> 5]) >> (keyCode & 0x1F)) & 1;
    }

    inline bool cInputState::KeyWentDown(int keyCode, tModifierSet modifiers) const
    {
        CL_INDEX(keyCode, kMaxKeyCode);
        return mKeyModifiers == modifiers
            && ((~mLastKeyStates[keyCode >> kKeyCodeWordShift] & mKeyStates[keyCode >> 5]) >> (keyCode & 0x1F)) & 1;
    }

    inline bool cInputState::KeyWentUp(int keyCode, tModifierSet modifiers) const
    {
        CL_INDEX(keyCode, kMaxKeyCode);
        return mKeyModifiers == modifiers
            && ((mLastKeyStates[keyCode >> kKeyCodeWordShift] & mKeyStates[keyCode >> 5]) >> (keyCode & 0x1F)) & 1;
    }

    inline tModifierSet cInputState::KeyModifiers() const
    {
        return mKeyModifiers;
    }

    inline Vec3f cInputState::Acceleration() const
    {
        return mAcceleration;
    }

    inline Vec2f cInputState::ScrollDelta() const
    {
        return mScrollDelta;
    }

    inline int cInputState::Counter() const
    {
        return mCounter;
    }

    inline uint64_t cInputState::TimeStamp() const
    {
        return mTimeStamp;
    }

    inline void cInputState::OnTimeStamp(uint64_t timeStampMS)
    {
        mTimeStamp = timeStampMS;
    }
}

#endif
