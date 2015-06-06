//
//  File:       HLUI.h
//
//  Function:   UI support
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_UI_H
#define HL_UI_H

#include <HLDefs.h>
#include <HLUIDraw.h>

#include <CLBounds.h>
#include <CLData.h>
#include <CLInputState.h>
#include <CLSTL.h>
#include <CLString.h>
#include <CLValue.h>

namespace nCL
{
    class cInputState;
    class cValue;
    class cObjectValue;
}

namespace nHL
{
    typedef uint64_t tUIItemID;

    const tUIItemID  kNullUIItemID = 0;
    const int        kMaxUIPointers = 8;

    tUIItemID ItemID(uint32_t uid, uint32_t index = 0);
    ///< Construct unique item info out of a UID and an optional index. (E.g. for menu items or button groups.)

    struct cUICursorInfo
    /// Cursor state that can be saved/restored.
    {
        Vec2f mCursor           = vl_0;     ///< Current UI cursor location
        Vec2f mLastCursor       = vl_0;     ///< Previous cursor location
        Vec2f mDir              = vl_1;     ///< w/h direction, e.g., (-1, 1) for right-hand-side menus
        Vec2f mCursorStart      = vl_0;
        Vec2f mMaxSize          = vl_0;     ///< Track max cell size
        Vec2f mCellSize         = vl_0;
        Vec2f mLastCellSize     = vl_0;     ///< For overlays
        Vec2f mAutoScroll       = vl_0;     ///< Scroll amount for autoscroll
    };

    /// For use with menus/submenus
    struct cUIMenuInfo
    {
        tUIItemID      mSelected    = kNullUIItemID; ///< Currently selected item if any
        cUICursorInfo  mCursorInfo;                 ///< For saving info while executing a sub menu

        Vec2f          mOffset      = vl_0;         ///< For top-level menu scrolling
        Vec2f          mPosition    = vl_0;         ///< Anchor position of top-level menu
        Vec2f          mTopMaxSize  = vl_0;         ///< Additional size field just for top level menu

        cUIMenuInfo() = default;
        cUIMenuInfo(float x, float y);
        
        bool IsSelected(tUIItemID itemID);    // use IsSelected(kNullUIItemID) to check if nothing is selected.
        void Select(tUIItemID itemID);
        void Toggle(tUIItemID itemID);
        void Clear();
    };

    struct cUIDrawCommand
    {
        uint8_t         mCommand;
        cUICursorInfo   mInfo;
        tUIDisplayFlags mFlags;
        nCL::tDataOffset mParam;
    };

    struct cUISelectInfo
    {
        tUIItemID mActiveItem  [kMaxUIPointers];
        tUIItemID mSelectedItem[kMaxUIPointers];
    };

    enum tSliderAction
    {
        kSliderNone  = 0,   ///<
        kSliderMove  = 1,   ///< Slider is being moved and the value changed
        kSliderFinal = 2,   ///< Slider was released with changed value.
        kMaxSliderActions
    };


    // --- cUIState ------------------------------------------------------------
    
    struct cUItemHitTester
    {
        virtual void operator()(int numCoords, const Vec2f coords[], bool hit[]) const = 0;
    };
    ///< For generically shaped items -- each screenspace coord[i] should be tested against
    ///< the item bounds and hit[i] set accordingly.

    using nCL::cPointerInfo;

    // Manages state of UI via ordered rects
    class cUIState
    {
    public:
        enum { kMaxPointers = kMaxUIPointers };  // Maximum number of pointers we bother tracking.

        cUIState(nCL::cInputState* inputState = 0);

        void      SetCanvasRect(cBounds2 rect);                  ///< Set the bounds we're active within
        void      SetInputState(nCL::cInputState* inputState);   ///< Set the input state to use.
        cIUIDraw* SetUIDraw(cIUIDraw* uiDraw);                   ///< Set UI draw system to use -- returns previous UI draw or 0 if none.
        void      Config(const cValue& config);

        cBounds2            CanvasRect() const;    ///< Return canvas rect for this
        nCL::cInputState*   InputState() const;    ///< Input state associated with this
        cIUIDraw*           UIDraw() const;

        bool Begin(Vec2f canvasSize);       ///< Start input processing: only call further commands if this returns true.
        void End();

        // --- Menu/Button interaction system ----------------------------------
        
        void  SetCursor   (Vec2f cursor); ///< Set where to start drawing. Negative values are relative to the far side of the screen. Returns previous cursor.
        void  OffsetCursor(Vec2f offset); ///< Offset current cursor position without affecting current anchor
        Vec2f Cursor();                   ///< Return current (relative) cursor position

        Vec2f SetCellSize(Vec2f cellSize);  ///< Set expected maximum size of cells. Returns previous size.
        Vec2f CellSize();                   ///< Returns current cell size.

        void AdvanceCol();    ///< Advance horizontally to another column of cells
        void AdvanceRow();    ///< Advance to a cell in the next row (button, slider, etc.)

        void SetAutoScroll(bool autoScroll);
        bool    AutoScroll() const;

        void SetColour(cColour c);  ///< Set style colour
        void ClearColour();         ///< Reset back to default

        void SetStyle(const cObjectValue* style);   ///< Set style info
        void ResetStyle();                          ///< Reset style back to default

        bool HandleButton          (tUIItemID itemID, const char* name, tUIDisplayFlags flags = 0);
        ///< Display a button, return true if it's clicked.
        bool HandleToggle          (tUIItemID itemID, const char* name, bool* value, tUIDisplayFlags flags = 0);
        ///< Display a toggle, update 'value' and return true if clicked.
        bool HandleToggle          (tUIItemID itemID, const char* name, bool value, tUIDisplayFlags flags = 0);
        ///< Display a toggle, return true if it should change.
        bool HandleButtonWithSwatch(tUIItemID itemID, const char* name, const cColour& c, tUIDisplayFlags flags = 0);
        ///< Display a button with a swatch of the given colour, return true if it's clicked.
        bool HandleButtonWithMark  (tUIItemID itemID, const char* name, const char* mark, tUIDisplayFlags flags = 0);
        ///< Display a button with the given text mark on the right, return true if it's clicked.

        tSliderAction HandleSlider(tUIItemID itemID, const char* name, float* value, tUIDisplayFlags flags = 0);
        ///< Display a 0-1 slider that can be scrubbed. 'value' will be modified while this happens, but will be restored
        ///< to its original value if the action is cancelled.

        void DrawLabel(const char* label, tUIDisplayFlags flags = 0);   ///< Draw non-interactive item with given label.
        void DrawSeparator();                                               ///< Add a separator before the next cell
        void DrawProgressBar(float s, tUIDisplayFlags flags = 0);           ///< Display a progress bar

        cBounds2 LastItemRect() const;  ///< Containing rect for the last item.

        // --- Menu handling ---------------------------------------------------

        bool BeginMenu(tUIItemID itemID);  ///< Start a top-level menu controlled by cUIMenuInfo. Returns true if you should display the menu.
        void EndMenu  (tUIItemID itemID = kNullUIItemID);  ///< Call at the end of defining the menu, only if BeginMenu returned true. Pass the same itemID to check you're ending the right menu.

        bool BeginSubMenu(tUIItemID itemID, const char* label, tUIDisplayFlags flags = 0);
        ///< Defines a submenu, and returns true if you should display that sub-menu.
        void EndSubMenu  (tUIItemID itemID = kNullUIItemID);
        ///< Call at the end of displaying the sub-menu, i.e. only if BeginSubMenu returns true. Pass the itemID to check you're ending the right submenu.

        bool ButtonSelected(tUIItemID itemID, const char* label, tUIDisplayFlags flags = 0);
        ///< Menu-attached selectable button - handles deselecting other menuInfo-related buttons.

        cUIMenuInfo* MenuInfo(tUIItemID itemID);    ///< Retrieves menu info for given item.
        cUIMenuInfo* CurrentMenuInfo();             ///< Retrieves menu info for given item.

        // --- Item-based interaction system -----------------------------------

        // Note: the pattern is to add a UI item via AddItemXXX, then query any interactions with it via the UI below.
        // Items are added in priority order -- the first item added that contains an interaction 'wins' that interaction.
        bool  AddItemCircle(tUIItemID itemID, const Vec2f& c, float r, tUIDisplayFlags flags = 0); ///< Sets this item as active if it's the first to contain a pointer
        bool  AddItemRect  (tUIItemID itemID, cBounds2 rect, tUIDisplayFlags flags = 0);    ///< Sets this item as active if it's the first to contain a pointer
        bool  AddItemCanvas(tUIItemID itemID, tUIDisplayFlags flags = 0);                   ///< Add item that covers the entire remaider of the screen.
        bool  AddItem      (tUIItemID itemID, const cUItemHitTester& hitTester, tUIDisplayFlags flags = 0); ///< Sets this item as active if it's the first to contain a pointer

        bool  ItemWasClicked(); ///< Returns true if the item was just clicked (pointer down inside the item, pointer up inside the item)

        // These are helpers for implementing common gestures
        void  SetInteractionItem(tUIItemID itemID); ///< Explicitly sets the item the interaction API is referring to. (Usually it's implicitly that from the last AddItem*.)

        int   InteractionCount() const;             ///< Returns the number of pointers interacting with the current item.

        int   InteractionPointerIndex(int i) const; ///< cInputState index of the i'th interacting pointer. This is guaranteed to remain the same over the course of a drag.
        const cPointerInfo& InteractionPointerInfo(int i) const; ///< Pointer info for the i'th interacting pointer

        Vec2f InteractionPoint() const;           ///< Representative point for the interaction.
        Vec2f InteractionPointDelta() const;      ///< Delta of point since the start of the interaction
        Vec2f InteractionPointDeltaLast() const;  ///< Delta of point since the last frame

        float InteractionSpan() const;            ///< Distance the interaction spans -- useful for stretch/pinch gestures.
        float InteractionSpanDelta() const;       ///< Span delta since interaction start
        float InteractionSpanDeltaLast() const;   ///< Span delta since last frame

        float InteractionTwist() const;           ///< Twist of the interaction.
        float InteractionTwistDelta() const;      ///< Twist of the interaction since the start.
        float InteractionTwistDeltaLast() const;  ///< Change in twist since last frame

        bool  InteractionSwipe(Vec2f* direction); ///< Returns true if a swipe has occurred, along with its direction.
        int   InteractionHorizontalSwipe();       ///< Returns -1 for swipe left, +1 for swipe right, 0 otherwise.
        int   InteractionVerticalSwipe();         ///< Returns -1 for swipe down, +1 for swipe up, 0 otherwise.

        void  PushInteractions(tUIItemID itemID);   ///< Save interaction state. Allows testing of further items via AddItem* without permanently affecting the state.
        void  PopInteractions();
        void  ClaimAndPopInteractions();            ///< Partner to PushInteractions() that claims any interactions that were started.

        // --- Basic active/selection management -------------------------------

        bool IsActive (tUIItemID itemID, int index = -1);   ///< Is the given item active, e.g., has a pointer over it or is otherwise activated. (If index is specified, check only for that pointer.)
        bool SetActive(tUIItemID itemID, int index);        ///< If there is not already an active item, make the given item active and return true
        
        bool IsSelected (tUIItemID itemID, int index = -1); ///< Is given item selected (e.g., a click/drag started on the active item)
        bool SetSelected(tUIItemID itemID, int index);      ///< Set the given item as selected by the given pointer
        int  SelectedCount(tUIItemID itemID) const;         ///< Returns number of selections, basically, the number of pointers interacting with this item.

        int  ActiveCount() const;
        int  SelectedCount() const;
        bool HasControl() const;        ///< Returns true if UI is currently interacting with input, i.e. an item is active or selected.

    protected:
        // Internal utilities
        void Save   (cUICursorInfo* cursorInfo);
        void Restore(cUICursorInfo* cursorInfo);

        bool AddItemRect(tUIItemID itemID, Vec2f rect, tUIDisplayFlags flags); ///< Sets this item as active if it's the first to contain a pointer
        bool PointerInRect(int pointerIndex, Vec2f rect);
        void AutoScroll   (int pointerIndex, Vec2f rect);
        void SetCursorInternal(Vec2f c);

        bool BeginMenu   (cUIMenuInfo* menuInfo);  ///< Start a top-level menu controlled by cUIMenuInfo. Returns true if you should display the menu.
        void EndMenu     (cUIMenuInfo* menuInfo);  ///< Call at the end of defining the menu, only if BeginMenu returned true.
        bool BeginSubMenu(cUIMenuInfo* menuInfo, tUIItemID itemID, const char* label, tUIDisplayFlags flags = 0);
        void EndSubMenu  (cUIMenuInfo* menuInfo);

        // Data
        nCL::cInputState* mInputState = 0;

        tUIItemID       mActiveItem  [kMaxPointers] = { kNullUIItemID };      ///< Pointer is over this item
        tUIItemID       mSelectedItem[kMaxPointers] = { kNullUIItemID };      ///< This item is selected: a UI action (usually a drag) started in it and has not yet completed.
        int             mActiveCount    = 0;        ///< Number of active rects
        int             mSelectedCount  = 0;        ///< Number of selected rects

        nCL::map<tUIItemID, cUISelectInfo>  mIDToSelectInfo;
        nCL::vector<tUIItemID>              mSelectIDStack;

        // Info about last AddItem
        tUIItemID       mItemID     = kNullUIItemID;
        bool            mItemClicked    = false;      ///< pointer down and up occurred in the rect

        // Menu system
        cUICursorInfo   mInfo;
        Vec2f           mScreenSize     = vl_0;         ///< TODO: needs to become mCanvasRect, full bounds
        int             mMenuDepth      = 0;
        nCL::string     mFormatText;                    ///< Scratch used by 'F' variants

        nCL::map<tUIItemID, cUIMenuInfo> mIDToMenuInfo;
        nCL::vector<tUIItemID>           mMenuIDStack;

        // Sliders
        nCL::map<tUIItemID, float> mIDToSliderStart;

        // Tuning
        float           mCellBorder     = 4.0f;     ///< Controls inter-cell spacing
        bool            mAutoScrollEnabled = true;
        float           mScrollDistance = 10.0f;    ///< Distance threshold for menu auto-scroll
        float           mScrollSpeed    = 20.0f;    ///< Speed for menu auto-scroll
        float           mDragDistance   = 20.0f;    ///< Movement distance before menu drag is enabled

        // Drawing
        cIUIDraw*                   mUIDraw = 0;
        cColour                     mDrawStyleColour = { -1.0f };
        nCL::tConstObjectLink       mDrawStyle;
        nCL::vector<cUIDrawCommand> mDrawCommands;
        nCL::cWriteableDataStore    mDrawCommandsStore;

        // Interactions
        int             mItemInteractionCount = 0;
        int             mItemInteractionPointers[kMaxPointers] = { 0 };
        Vec2f           mItemCurrentPos     = vl_0;     ///< Average current position for item mRectItemID interactions
        Vec2f           mItemStartPos       = vl_0;     ///< Average start position for item mRectItemID interactions
        Vec2f           mItemLastPos        = vl_0;     ///< Average last position for item mRectItemID interactions
        Vec2f           mItemStartDeltaPos  = vl_0;     ///< Average start position for item mRectItemID interactions
        Vec2f           mItemLastDeltaPos   = vl_0;     ///< Average last position for item mRectItemID interactions

        int             mSwipeMinTimeMS = 60;          ///< How long a swipe must persist to be treated as such
        float           mSwipeMinSpeed  = 1.3f;        ///< How fast a drag must be to register as a swipe
    };


    // --- Utilities -----------------------------------------------------------

    Vec2f UpdateScroll(Vec2f scroll, float taper = 2.0f);  ///< Tapers off a scroll delta over time, e.g., after a swipe

    const char* Format(const char* format, ...);

    void ConfigUI(const cObjectValue* config, cUIState* uiState);   ///< Applies options from 'config' to the UI state

    void SetInputShaderData (const nCL::cInputState* state, cIRenderer* renderer);
    void DebugDrawInputState(const nCL::cInputState& inputState, cDebugDraw* dd);

    enum tObjectMenuType
    {
        kObjectMenuNormal,
        kObjectMenuLocalAndParents,
        kMaxObjectMenuTypes
    };

    bool ShowObjectMenu
    (
        cUIState*           uiState,
        tUIItemID           itemID,
        const char*         name,
        const cObjectValue* object,
        tObjectMenuType     objectMenuType = kObjectMenuNormal,
        const char*         mark = 0
    );
    bool ShowValueMenu
    (
        cUIState*           uiState,
        tUIItemID           itemID,
        const char*         name,
        const cValue&       value,
        tObjectMenuType     objectMenuType = kObjectMenuNormal,
        const char*         mark = 0
    );
    ///< Display a menu based off the contents of 'value'


    // --- Inlines -------------------------------------------------------------

    inline tUIItemID ItemID(uint32_t uid, uint32_t index)
    {
        return (uint64_t(uid) << 32) | index;
    }

    inline nCL::cInputState* cUIState::InputState() const
    {
        return mInputState;
    }

    inline cIUIDraw* cUIState::UIDraw() const
    {
        return mUIDraw;
    }

    inline cBounds2 cUIState::CanvasRect() const
    {
        return cBounds2(vl_0, mScreenSize);
    }

    inline void cUIState::OffsetCursor(Vec2f offset)
    {
        mInfo.mCursor += offset;
    }

    inline void cUIState::SetAutoScroll(bool autoScroll)
    {
        mAutoScrollEnabled = autoScroll;
    }

    inline bool cUIState::AutoScroll() const
    {
        return mAutoScrollEnabled;
    }

    inline cUIMenuInfo* cUIState::MenuInfo(tUIItemID itemID)
    {
        auto it = mIDToMenuInfo.insert(decltype(mIDToMenuInfo)::value_type(itemID, cUIMenuInfo())).first;
        return &it->second;
    }

    inline cUIMenuInfo* cUIState::CurrentMenuInfo()
    {
        return MenuInfo(mMenuIDStack.back());
    }


    inline int cUIState::ActiveCount() const
    {
        return mActiveCount;
    }

    inline int cUIState::SelectedCount() const
    {
        return mSelectedCount;
    }

    inline int cUIState::InteractionPointerIndex(int i) const
    {
        CL_INDEX(i, mItemInteractionCount);
        return mItemInteractionPointers[i];
    }
    
    inline const cPointerInfo& cUIState::InteractionPointerInfo(int i) const
    {
        CL_INDEX(i, mItemInteractionCount);
        return mInputState->PointerInfo(mItemInteractionPointers[i]);
    }

    inline int cUIState::InteractionCount() const
    {
        return mItemInteractionCount;
    }

    inline Vec2f cUIState::InteractionPoint() const
    {
        return mItemCurrentPos;
    }

    inline Vec2f cUIState::InteractionPointDelta() const
    {
        return mItemStartDeltaPos;
    }

    inline Vec2f cUIState::InteractionPointDeltaLast() const
    {
        return mItemLastDeltaPos;
    }


}

#endif
