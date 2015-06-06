//
//  File:       HLUI.cpp
//
//  Function:   Provides framework for handling basic button/menu-based UI
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLUI.h>

#include <HLDebugDraw.h>

#include <CLFileSpec.h>
#include <CLInputState.h>
#include <CLLog.h>
#include <CLUtilities.h>
#include <CLValue.h>
#include <CLVecUtil.h>

#if 0
    #define CL_INPUT_LOG(ARGS...) CL_LOG_D("Input", ARGS)
#else
    #define CL_INPUT_LOG(...)
#endif

using namespace nHL;
using namespace nCL;

namespace
{
}



// --- cUIMenuInfo ------------------------------------------------------------

cUIMenuInfo::cUIMenuInfo(float x, float y) :
    mPosition(x, y)
{
}

bool cUIMenuInfo::IsSelected(tUIItemID itemID)     // use IsSelected(kNullUIItemID) to see if nothing is selected.
{ 
    return mSelected == itemID; 
}
        
void cUIMenuInfo::Select(tUIItemID itemID)
{ 
    mSelected = itemID; 
}

void cUIMenuInfo::Toggle(tUIItemID itemID)
{ 
    if (mSelected == itemID)
        mSelected = kNullUIItemID;
    else
        mSelected = itemID;
}

void cUIMenuInfo::Clear()
{ 
    mSelected = kNullUIItemID; 
}


// --- cUIState ----------------------------------------------------------------

cUIState::cUIState(cInputState* inputState) :
    mInputState(inputState)
{
}

void cUIState::SetInputState(cInputState* inputState)
{
    mInputState = inputState;
}

cIUIDraw* cUIState::SetUIDraw(cIUIDraw* uiDraw)
{
    cIUIDraw* result = mUIDraw;

    if (result)
    {
        // Flush draw commands
        mUIDraw->DrawUI(mDrawCommands.size(), mDrawCommands.data(), &mDrawCommandsStore);

        mDrawCommands.clear();
        mDrawCommandsStore.Free(0);
    }

    mUIDraw = uiDraw;

    return result;
}

void cUIState::Config(const nCL::cValue& config)
{
    // TODO: want to re-init just these guys from defaults -- own struct?
    mCellBorder     = 4.0f;
    mScrollDistance = 10.0f;
    mScrollSpeed    = 20.0f;
    mSwipeMinTimeMS = 500;
    mSwipeMinSpeed  = 1.0f;
    mDragDistance   = 20.0f;

    mCellBorder     = config["cellBorder"    ].AsFloat(mCellBorder);
    mScrollDistance = config["scrollDistance"].AsFloat(mScrollDistance);
    mScrollSpeed    = config["scrollSpeed"   ].AsFloat(mScrollSpeed);
    mDragDistance   = config["dragDistance"  ].AsFloat(mDragDistance);
    mSwipeMinTimeMS = config["swipeMinFrames"].AsInt  (mSwipeMinTimeMS);
    mSwipeMinSpeed  = config["swipeMinSpeed" ].AsFloat(mSwipeMinSpeed);
}

bool cUIState::Begin(Vec2f screenSize)
{
    if (!mInputState)
        return false;

    for (int i = 0; i < kMaxPointers; i++)
        mActiveItem[i] = kNullUIItemID;

    mActiveCount = 0;
    mItemID = kNullUIItemID;

    mMenuIDStack.clear();

    mScreenSize = screenSize;

    mInputState->Begin();

    // Set some defaults
    SetCellSize(Vec2f(100, 20));
    SetCursor  (Vec2f(20, 20));

    return true;
}

#define DEFERRED_DRAW

void cUIState::End()
{
    // Handle trailing selected state for any items that weren't added this frame.
    for (int i = 0; i < kMaxPointers; i++)
        if (mInputState->PointerWentUp(i))
        {
            if ((mSelectedItem[i] != kNullUIItemID))
                SetSelected(kNullUIItemID, i);

            for (auto& kv : mIDToSelectInfo)
                kv.second.mSelectedItem[i] = kNullUIItemID;
        }

    mInputState->End();

    mUIDraw->DrawUI(mDrawCommands.size(), mDrawCommands.data(), &mDrawCommandsStore);

    mDrawCommands.clear();
    mDrawCommandsStore.Free(0);
}

void cUIState::SetCursor(Vec2f c)
{
    if (c[0] < 0)
    {
        c[0] += mScreenSize[0];
        mInfo.mDir[0] = -1.0f;
    }
    else
        mInfo.mDir[0] = +1.0f;

    if (c[1] < 0)
    {
        c[1] += mScreenSize[1];
        mInfo.mDir[1] = -1.0f;
    }
    else
        mInfo.mDir[1] = +1.0f;

    // Do this afterwards, otherwise the menu offset winds up causing menus to auto-wrap!
    // TODO: make auto-wrap a menu option?
    if (!mMenuIDStack.empty())
        c += CurrentMenuInfo()->mOffset;

    SetCursorInternal(c);
}

Vec2f cUIState::Cursor()
{
    Vec2f cursor(mInfo.mCursor);

    float& x = cursor[0];
    float& y = cursor[1];

    if (mInfo.mDir[0] < 0.0f)
        x -= mScreenSize[0];

    if (mInfo.mDir[1] < 0.0f)
        y -= mScreenSize[1];

    return cursor;
}

Vec2f cUIState::SetCellSize(Vec2f cellSize)
{
    Vec2f oldSize(mInfo.mCellSize);
    mInfo.mCellSize = cellSize;
    return oldSize;
}

Vec2f cUIState::CellSize()
{
    return mInfo.mCellSize;
}

void cUIState::AdvanceCol()
{
//    SetCursorInternal(mInfo.mCursorStart + mInfo.mDir * Vec2f(mCellBorder + mInfo.mMaxSize[0], 0));
    Vec2f c = mInfo.mCursorStart;
    c[0] += mInfo.mDir[0] * (mCellBorder + mInfo.mMaxSize[0]);

    mInfo.mCursor         = c;
    mInfo.mLastCursor     = c;
    mInfo.mCursorStart    = c;
}

void cUIState::AdvanceRow()
{
    SetCursorInternal(mInfo.mCursor + mInfo.mDir * Vec2f(mInfo.mMaxSize[0] + 1, -mInfo.mLastCellSize[1]));
}

void cUIState::SetColour(cColour c)
{
#ifndef DEFERRED_DRAW
    mUIDraw->SetColour(c);
#else
    mDrawStyleColour = c;
#endif
}

void cUIState::ClearColour()
{
#ifndef DEFERRED_DRAW
    mUIDraw->ClearColour();
#else
    mDrawStyleColour = Vec3f(-1.0f);
#endif
}

void cUIState::SetStyle(const cObjectValue* style)
{
#ifndef DEFERRED_DRAW
    mUIDraw->SetStyle(style);
#else
    mDrawStyle = style;
#endif
}

void cUIState::ResetStyle()
{
#ifndef DEFERRED_DRAW
    mUIDraw->SetStyle(0);
#else
    mDrawStyle = 0;
#endif
}

bool cUIState::HandleButton(tUIItemID itemID, const char* label, tUIDisplayFlags flags)
{
    if (!label)
        label = "(null)";

    Vec2f cellSize = mUIDraw->CellRect(label, mInfo, flags);

    bool buttonClicked = AddItemRect(itemID, cellSize, flags);

    if (flags & kUIDisabled)
        flags &= ~kUIHighlighted;

    if (IsActive(itemID))
        flags |= kUIActive;

    if (IsSelected(itemID))
        flags |= kUISelected;

    if (flags & (kUIActive | kUISelected | kUIHighlighted))     // de-ghost if we're interacting with this
        flags &= ~kUIGhost;

    if ((flags & kUIActive) && (mSelectedCount == 0 || (flags & kUISelected)))  // if nothing's selected, or this button is selected, highlight when we're over this button
        flags |= kUITextSelected;

#ifndef DEFERRED_DRAW
    mUIDraw->DrawCell(mInfo, flags, label);
#else
    tDataOffset labelOffset = AddCStr(label, &mDrawCommandsStore);

    cUIDrawCommand cmd = { kDrawItemStart, mInfo, flags, labelOffset };
    mDrawCommands.push_back(cmd);

    if (mDrawStyle)
    {
        mDrawStyle->Link(1);
        cUIDrawCommand cmd = { kDrawSetStyle, mInfo, flags, Add(&*mDrawStyle, &mDrawCommandsStore) };
        mDrawCommands.push_back(cmd);
    }

    if (mDrawStyleColour[0] >= 0.0f)
    {
        cUIDrawCommand cmd = { kDrawSetColour, mInfo, flags, Add(mDrawStyleColour, &mDrawCommandsStore) };
        mDrawCommands.push_back(cmd);
    }

    cmd = { kDrawCell, mInfo, flags, labelOffset };
    mDrawCommands.push_back(cmd);
#endif

    mInfo.mLastCursor = mInfo.mCursor;
    mInfo.mCursor[1] += mInfo.mDir[1] * cellSize[1];
    mInfo.mLastCellSize = cellSize;

    mInfo.mMaxSize = MaxElts(mInfo.mMaxSize, cellSize);

    return buttonClicked;
}

bool cUIState::HandleToggle(tUIItemID itemID, const char* name, bool* value, tUIDisplayFlags flags)
{
    // TODO: leave display details to mUIDraw
    if (HandleButtonWithSwatch(itemID, name, *value ? kColourGreen : kColourRed, flags))
    {
        *value = !*value;
        return true;
    }

    return false;
}

bool cUIState::HandleToggle(tUIItemID itemID, const char* name, bool value, tUIDisplayFlags flags)
{
    // TODO: leave display details to mUIDraw
    return HandleButtonWithSwatch(itemID, name, value ? kColourGreen : kColourRed, flags);
}

bool cUIState::HandleButtonWithSwatch(tUIItemID itemID, const char* label, const cColour& c, tUIDisplayFlags flags)
{
    bool result = HandleButton(itemID, label, flags | kUISwatch);

#ifndef DEFERRED_DRAW
    mUIDraw->DrawSwatch(mInfo, flags, c);
#else
    cUIDrawCommand cmd = { kDrawSwatch, mInfo, flags, Add(c, &mDrawCommandsStore) };
    mDrawCommands.push_back(cmd);
#endif

    return result;
}

bool cUIState::HandleButtonWithMark(tUIItemID itemID, const char* label, const char* mark, tUIDisplayFlags flags)
{
    bool result = HandleButton(itemID, label, flags | kUISwatch);

    if (mark)
    {
    #ifndef DEFERRED_DRAW
        mUIDraw->DrawMark(mInfo, flags, mark);
    #else
        cUIDrawCommand cmd = { kDrawMark, mInfo, flags, AddCStr(mark, &mDrawCommandsStore) };
        mDrawCommands.push_back(cmd);
    #endif
    }

    return result;
}

tSliderAction cUIState::HandleSlider(tUIItemID itemID, const char* name, float* value, tUIDisplayFlags flags)
{
    float valueIn = *value;
    float& startValue = mIDToSliderStart[itemID];

    if (valueIn < 0.0f)
        flags |= kUIDisabled;

    valueIn = ClampUnit(valueIn);

    bool wasSelected = IsSelected(itemID);

    bool result = HandleButton(itemID, name, flags | kUISlider);

    bool isSelected = IsSelected(itemID);

    if (isSelected)
    {
        if (!wasSelected)
            startValue = valueIn;

        mItemID = kNullUIItemID;
        SetInteractionItem(itemID); // compensate for the special menu-system AddRect which doesn't call this

        Vec2f delta = InteractionPointDelta(); // XXX

        if (fabsf(delta[1]) > 2.0f * mInfo.mMaxSize[1])
            *value = startValue;
        else
            *value = ClampUnit(startValue + delta[0] / 100.0f);
    }

#ifndef DEFERRED_DRAW
    mUIDraw->DrawSlider(mInfo, flags, *value);
#else
    cUIDrawCommand cmd = { kDrawSlider, mInfo, flags, Add(*value, &mDrawCommandsStore) };
    mDrawCommands.push_back(cmd);
#endif

    if (result && *value != startValue)
        return kSliderFinal;

    if (isSelected && *value != valueIn)
        return kSliderMove;

    return kSliderNone;
}

void cUIState::DrawLabel(const char* label, tUIDisplayFlags flags)
{
    if (!label)
        label = "(null)";

    Vec2f cellSize = mUIDraw->CellRect(label, mInfo, flags);

#ifndef DEFERRED_DRAW
    mUIDraw->DrawCell(mInfo, flags, label);
#else
    cUIDrawCommand cmd = { kDrawCell, mInfo, flags, AddCStr(label, &mDrawCommandsStore) };
    mDrawCommands.push_back(cmd);
#endif

    mInfo.mLastCursor = mInfo.mCursor;
    mInfo.mCursor[1] += mInfo.mDir[1] * cellSize[1];
    mInfo.mLastCellSize = cellSize;

    mInfo.mMaxSize = MaxElts(mInfo.mMaxSize, cellSize);
}

void cUIState::DrawSeparator()
{
    if (mInfo.mMaxSize[0] > 0.0f || mInfo.mCellSize[0] > 0.0f)
    {
        Vec2f cellSize(mInfo.mMaxSize[0], mCellBorder);

        if (cellSize[0] < mInfo.mCellSize[0])
            cellSize[0] = mInfo.mCellSize[0];

        mInfo.mCursor[1] += mInfo.mDir[1] * cellSize[1];
    }
}

void cUIState::DrawProgressBar(float s, tUIDisplayFlags flags)
{
#ifndef DEFERRED_DRAW
    mUIDraw->DrawSlider(mInfo, flags, s);
#else
    cUIDrawCommand cmd = { kDrawSlider, mInfo, flags, Add(s, &mDrawCommandsStore) };
    mDrawCommands.push_back(cmd);
#endif
}

cBounds2 cUIState::LastItemRect() const
{
    cBounds2 result(mInfo.mLastCursor);

    if (mInfo .mDir[0] < 0.0f)
        result.mMin[0] -= mInfo.mLastCellSize[0];
    else
        result.mMax[0] += mInfo.mLastCellSize[0];

    if (mInfo .mDir[1] < 0.0f)
        result.mMin[1] -= mInfo.mLastCellSize[1];
    else
        result.mMax[1] += mInfo.mLastCellSize[1];

    return result;
}

// --- Menu Handling -----------------------------------------------------------

bool cUIState::BeginMenu(tUIItemID itemID)
{
    CL_ASSERT_MSG(mMenuIDStack.empty(), "Already in a menu block");
    mMenuIDStack.push_back(itemID);

    return BeginMenu(MenuInfo(mMenuIDStack.back()));
}

void cUIState::EndMenu(tUIItemID itemID)
{
    CL_ASSERT_MSG(!mMenuIDStack.empty(), "Not in a menu block");
    CL_ASSERT_MSG(itemID == kNullUIItemID || mMenuIDStack.back() == itemID, "Ending wrong menu");

    EndMenu(MenuInfo(mMenuIDStack.back()));

    mMenuIDStack.pop_back();
    CL_ASSERT_MSG(mMenuIDStack.empty(), "Unbalanced EndMenu()");
}

bool cUIState::BeginSubMenu(tUIItemID itemID, const char* label, tUIDisplayFlags flags)
{
    CL_ASSERT_MSG(!mMenuIDStack.empty(), "Not in a menu block");

    if (BeginSubMenu(MenuInfo(mMenuIDStack.back()), itemID, label, flags))
    {
        mMenuIDStack.push_back(itemID);
        return true;
    }

    return false;
}

void cUIState::EndSubMenu(tUIItemID itemID)
{
    // Pop stack

    CL_ASSERT_MSG(!mMenuIDStack.empty(), "Not in a submenu block");
    CL_ASSERT_MSG(itemID == kNullUIItemID || mMenuIDStack.back() == itemID, "Ending wrong submenu");

    mMenuIDStack.pop_back();

    CL_ASSERT_MSG(!mMenuIDStack.empty(), "Not in a menu block");

    EndSubMenu(MenuInfo(mMenuIDStack.back()));
}

bool cUIState::ButtonSelected(tUIItemID itemID, const char* label, tUIDisplayFlags flags)
{
    CL_ASSERT_MSG(!mMenuIDStack.empty(), "Not in a menu block");

    // Possibly create cUIMenuInfo on demand
    cUIMenuInfo* menuInfo = MenuInfo(mMenuIDStack.back());

    if (menuInfo->IsSelected(itemID))
        flags |= kUIHighlighted;

    if (HandleButton(itemID, label, flags))
        menuInfo->Toggle(itemID);

    return menuInfo->IsSelected(itemID);
}


bool cUIState::BeginMenu(cUIMenuInfo* menuInfo)
{
    SetCursor(menuInfo->mPosition);

    mInfo.mAutoScroll = vl_0;
    mMenuDepth = 0;

    if (mInfo.mCellSize[0] < menuInfo->mTopMaxSize[0])  // Size menu to max width
        mInfo.mCellSize[0] = menuInfo->mTopMaxSize[0];

    if (mInfo.mCellSize[1] < menuInfo->mTopMaxSize[1])  // Size menu to max width
        mInfo.mCellSize[1] = menuInfo->mTopMaxSize[1];

    mInfo.mMaxSize = vl_0;

    return true;
}

void cUIState::EndMenu(cUIMenuInfo* menuInfo)
{
    CL_ASSERT_MSG(mMenuDepth == 0, "Not enough EndSubMenus");

    menuInfo->mOffset -= mInfo.mAutoScroll;
    menuInfo->mTopMaxSize = mInfo.mMaxSize;

    mInfo.mAutoScroll = vl_0;
}

bool cUIState::BeginSubMenu(cUIMenuInfo* menuInfo, tUIItemID itemID, const char* label, tUIDisplayFlags flags)
{
    bool isSelected = menuInfo->IsSelected(itemID);

    if (menuInfo->IsSelected(itemID))
        flags |= kUIHighlighted;

    if (HandleButton(itemID, label, flags))
        menuInfo->Toggle(itemID);

#ifndef DEFERRED_DRAW
    mUIDraw->DrawSubMenuMark(mInfo, flags);
#else
    cUIDrawCommand cmd = { kDrawSubMenuMark, mInfo, flags, 0 };
    mDrawCommands.push_back(cmd);
#endif

    if (isSelected)
    {
        Vec2f prevMaxSize = menuInfo->mCursorInfo.mMaxSize;

        Save(&menuInfo->mCursorInfo);
        AdvanceRow();
        
        mInfo.mMaxSize = vl_0;

        if (mInfo.mCellSize[0] < prevMaxSize[0])  // Size menu to max width
            mInfo.mCellSize[0] = prevMaxSize[0];

        mMenuDepth++;

        return true;
    }

    return false;
}

void cUIState::EndSubMenu(cUIMenuInfo* menuInfo)
{
    Vec2f autoScroll(mInfo.mAutoScroll);
    
    Vec2f maxSize = mInfo.mMaxSize;
    Restore(&menuInfo->mCursorInfo);
    menuInfo->mCursorInfo.mMaxSize = maxSize;

    if (autoScroll[0] != 0.0f)
        mInfo.mAutoScroll[0] = autoScroll[0];
    if (autoScroll[1] != 0.0f)
        mInfo.mAutoScroll[1] = autoScroll[1];

    mMenuDepth--;

    CL_ASSERT_MSG(mMenuDepth >= 0, "Too many EndSubMenus");
}

namespace
{
    // Generic common hit tests

    struct cHitTestCircle : public cUItemHitTester
    {
        Vec2f mCenter;
        float mSqrRadius;
        
        cHitTestCircle(const Vec2f& c, float r) : mCenter(c), mSqrRadius(r*r) {}
        
        void operator()(int numCoords, const Vec2f coords[], bool hit[]) const override
        {
            for (int i = 0; i < numCoords; i++)
                hit[i] = sqrlen(coords[i] - mCenter) < mSqrRadius;
        }
    };

    struct cHitTestRect : public cUItemHitTester
    {
        cBounds2 mRect;
        
        cHitTestRect(const cBounds2& r) : mRect(r) {}   // annoying there's no struct-POD-equiv {} construction.
        
        void operator()(int numCoords, const Vec2f coords[], bool hit[]) const override
        {
            for (int i = 0; i < numCoords; i++)
                hit[i] = mRect.Contains(coords[i]);
        }
    };
}

bool cUIState::AddItemCircle(tUIItemID itemID, const Vec2f& c, float r, tUIDisplayFlags flags)
{
    return AddItem(itemID, cHitTestCircle(c, r), flags);
}

bool cUIState::AddItemRect(tUIItemID itemID, cBounds2 rect, tUIDisplayFlags flags)
{
    return AddItem(itemID, cHitTestRect(rect), flags);
}

bool cUIState::AddItemCanvas(tUIItemID itemID, tUIDisplayFlags flags)
{
    return AddItem(itemID, cHitTestRect(cBounds2(vl_0, mScreenSize)), flags);
}

// Generic Rect-based UI

bool cUIState::AddItem(tUIItemID itemID, const cUItemHitTester& hitTest, tUIDisplayFlags flags)
{
    mItemClicked = false;
    
    if (flags & kUIDisabled)
        return false;

    Vec2f coords[kMaxPointers];
    bool  hits  [kMaxPointers];

    int numPointers = mInputState->NumActivePointers();
    if (numPointers > kMaxPointers)
        numPointers = kMaxPointers;
    
    for (int i = 0; i < numPointers; i++)
        coords[i] = mInputState->PointerInfo(i).mCurrentPos;

    hitTest(numPointers, coords, hits);

    for (int i = 0; i < numPointers; i++)
    {
        if (hits[i])
            SetActive(itemID, i);

        if (mInputState->PointerWentDown(i) && IsActive(itemID, i))
            SetSelected(itemID, i);

        if (IsSelected(itemID, i) && mInputState->PointerWentUp(i))
        {
            if (IsActive(itemID, i))
                mItemClicked = true;

            SetSelected(kNullUIItemID, i);    // IsSelected() may still return true if another pointer is active
        }
    }

    SetInteractionItem(itemID);

    return mItemClicked;
}

bool cUIState::ItemWasClicked()
{
    return mItemClicked;
}

// --- Interactions ------------------------------------------------------------

void cUIState::SetInteractionItem(tUIItemID itemID)
{
    if (itemID == mItemID)
        return;

    mItemID =  itemID;
    mItemInteractionCount = 0;
    mItemCurrentPos     = vl_0;
    mItemStartPos       = vl_0;
    mItemLastPos        = vl_0;
    mItemStartDeltaPos  = vl_0;
    mItemLastDeltaPos   = vl_0;
//        mItemFirstCounter = 0;
//        mItemLastCounter = INT32_MAX;

    if (mSelectedCount == 0)
        return;

    for (int i = 0; i < kMaxPointers; i++)
        if (mSelectedItem[i] == mItemID)
        {
            const cPointerInfo& info = mInputState->PointerInfo(i);

            mItemCurrentPos     += info.mCurrentPos;
            mItemStartPos       += info.mStartPos;
            mItemLastPos        += info.mLastPos;

            mItemStartDeltaPos  += info.mCurrentPos - info.mStartPos;
            mItemLastDeltaPos   += info.mCurrentPos - info.mLastPos;

            mItemInteractionPointers[mItemInteractionCount] = i;
            mItemInteractionCount++;
        }

    if (mItemInteractionCount > 1)
    {
        mItemCurrentPos     /= mItemInteractionCount;
        mItemStartPos       /= mItemInteractionCount;
        mItemLastPos        /= mItemInteractionCount;
        mItemStartDeltaPos  /= mItemInteractionCount;
        mItemLastDeltaPos   /= mItemInteractionCount;
    }
}

float cUIState::InteractionSpan() const
{
    float s2 = 0.0f;

    // Find largest distance to interaction centre.
    for (int i = 0; i < mItemInteractionCount; i++)
    {
        const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[i]);

        float d2 = sqrlen(info.mCurrentPos - mItemCurrentPos);

        if (d2 > s2)
            s2 = d2;
    }

    return sqrtf(s2);
}

float cUIState::InteractionSpanDelta() const
{
    float cs2 = 0.0f;
    float ss2 = 0.0f;

    for (int i = 0; i < mItemInteractionCount; i++)
    {
        const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[i]);

        float cd2 = sqrlen(info.mCurrentPos - mItemCurrentPos);
        float sd2 = sqrlen(info.mStartPos   - mItemStartPos);

        if (cd2 > cs2)
            cs2 = cd2;
        
        if (sd2 > ss2)
            ss2 = sd2;
    }

    return sqrtf(cs2) - sqrtf(ss2);
}

float cUIState::InteractionSpanDeltaLast() const
{
    float cs2 = 0.0f;
    float ls2 = 0.0f;

    for (int i = 0; i < mItemInteractionCount; i++)
    {
        const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[i]);

        float cd2 = sqrlen(info.mCurrentPos - mItemCurrentPos);
        float ld2 = sqrlen(info.mLastPos    - mItemLastPos);

        if (cd2 > cs2)
            cs2 = cd2;
        
        if (ld2 > ls2)
            ls2 = ld2;
    }

    return sqrtf(cs2) - sqrtf(ls2);
}

float cUIState::InteractionTwist() const
{
    float t = 0.0f;

    // angle of first interacting pointer with centre
    if (mItemInteractionCount > 0)
    {
        const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[0]);

        Vec2f dp = info.mCurrentPos - mItemCurrentPos;

        t = atan2(dp[1], dp[0]);
    }

    return t;
}

float cUIState::InteractionTwistDelta() const
{
    float t = 0.0f;

    if (mItemInteractionCount > 0)
    {
        const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[0]);

        Vec2f cdp = info.mCurrentPos - mItemCurrentPos;
        Vec2f sdp = info.mStartPos   - mItemStartPos;

        t = atan2f(cdp[1], cdp[0]) - atan2f(sdp[1], sdp[0]);
    }

    return t;
}

float cUIState::InteractionTwistDeltaLast() const
{
    float t = 0.0f;

    if (mItemInteractionCount > 0)
    {
        const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[0]);

        Vec2f cdp = info.mCurrentPos - mItemCurrentPos;
        Vec2f ldp = info.mLastPos    - mItemLastPos;

        t = atan2f(cdp[1], cdp[0]) - atan2f(ldp[1], ldp[0]);
    }

    return t;
}

bool cUIState::InteractionSwipe(Vec2f* direction)
{
    if (mItemInteractionCount == 0)
        return false;

    const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[0]);
    uint32_t deltaTimeMS = mInputState->TimeStamp() - info.mStartTimeStamp;

    if (deltaTimeMS >= mSwipeMinTimeMS)
    {
        float d = len(mItemCurrentPos - mItemStartPos);
        float speed = d / deltaTimeMS;

        if (speed > mSwipeMinSpeed)
        {
            if (direction)
                *direction = norm(mItemStartDeltaPos);

            // Not interested in this guy any more.
            for (int i = 0; i < mItemInteractionCount; i++)
                SetSelected(kNullUIItemID, mItemInteractionPointers[i]);

            mItemInteractionCount = 0;

            return true;
        }
    }

    return false;
}

int cUIState::InteractionHorizontalSwipe()
{
    if (mItemInteractionCount == 0)
        return 0;

    const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[0]);
    uint32_t deltaTimeMS = mInputState->TimeStamp() - info.mStartTimeStamp;

    if (deltaTimeMS >= mSwipeMinTimeMS)
    {
        float dx = mItemCurrentPos[0] - mItemStartPos[0];
        float dy = mItemCurrentPos[1] - mItemStartPos[1];
        float ad = fabsf(dx);

        if (ad < 5.0f * fabsf(dy))
            return 0;

        float speed = ad / deltaTimeMS;

        if (speed > mSwipeMinSpeed * 0.75f)
        {
            // Not interested in this guy any more.
            for (int i = 0; i < mItemInteractionCount; i++)
                SetSelected(kNullUIItemID, mItemInteractionPointers[i]);

            mItemInteractionCount = 0;
            
            return dx > 0.0f ? 1 : -1;
        }
    }

    return 0;
}

int cUIState::InteractionVerticalSwipe()
{
    if (mItemInteractionCount == 0)
        return 0;

    const cPointerInfo& info = mInputState->PointerInfo(mItemInteractionPointers[0]);
    uint32_t deltaTimeMS = mInputState->TimeStamp() - info.mStartTimeStamp;

    if (deltaTimeMS >= mSwipeMinTimeMS)
    {
        float dx = mItemCurrentPos[0] - mItemStartPos[0];
        float dy = mItemCurrentPos[1] - mItemStartPos[1];
        float ad = fabsf(dy);

        if (ad < 5.0f * fabsf(dx))
            return 0;

        float speed = ad / deltaTimeMS;

        if (speed > mSwipeMinSpeed)
        {
            // Not interested in this guy any more.
            for (int i = 0; i < mItemInteractionCount; i++)
                SetSelected(kNullUIItemID, mItemInteractionPointers[i]);

            mItemInteractionCount = 0;

            return dy > 0.0f ? 1 : -1;
        }
    }

    return 0;
}

void cUIState::PushInteractions(tUIItemID itemID)
{
    CL_ASSERT(itemID != kNullUIItemID);     // reserved for default interactions

    if (mSelectIDStack.empty())
    {
        mSelectIDStack.push_back(kNullUIItemID);
        mIDToSelectInfo[kNullUIItemID]; // force insertion
    }

    //mIDToSelectInfo.insert(make_pair(itemID, cUISelectInfo())); // force insertion
    mIDToSelectInfo[itemID]; // force insertion. frustrating that all other syntaxes are so clunky.

    auto& prevInfo = mIDToSelectInfo[mSelectIDStack.back()];
    auto& nextInfo = mIDToSelectInfo[itemID];

    // TODO: we want to save off the current Active state, and we need
    // to restore the Selected state these guys had from the previous frame.
    // When we're done with it, we need to save off the selected state, and restore
    // the previous active state.

    for (int i = 0; i < kMaxPointers; i++)
    {
        // Save off active state, and restore selected state from previous frame...
        prevInfo.mActiveItem  [i] = mActiveItem  [i];
        prevInfo.mSelectedItem[i] = mSelectedItem[i];

        mSelectedItem[i] = nextInfo.mSelectedItem[i];
    }

    mSelectIDStack.push_back(itemID);
}

void cUIState::PopInteractions()
{
    CL_ASSERT_MSG(mSelectIDStack.size() > 1, "Unbalanced PopInteractions()");

    auto& prevInfo = mIDToSelectInfo[mSelectIDStack.back()];
    mSelectIDStack.pop_back();
    auto& nextInfo = mIDToSelectInfo[mSelectIDStack.back()];

    for (int i = 0; i < kMaxPointers; i++)
    {
        prevInfo.mSelectedItem[i] = mSelectedItem[i];

        // restore active state from when we started, and save off selected state for next frame.
        mActiveItem  [i] = nextInfo.mActiveItem  [i];
        mSelectedItem[i] = nextInfo.mSelectedItem[i];
    }
}

void cUIState::ClaimAndPopInteractions()
{
    CL_ASSERT_MSG(mSelectIDStack.size() > 1, "Unbalanced PopInteractions()");

    auto& prevInfo = mIDToSelectInfo[mSelectIDStack.back()];
    mSelectIDStack.pop_back();
    auto& nextInfo = mIDToSelectInfo[mSelectIDStack.back()];

    for (int i = 0; i < kMaxPointers; i++)
    {
        prevInfo.mSelectedItem[i] = mSelectedItem[i];

        // Update prev state too
        nextInfo.mActiveItem  [i] = mActiveItem  [i];
        nextInfo.mSelectedItem[i] = mSelectedItem[i];
    }
}

bool cUIState::IsActive(tUIItemID itemID, int index)
{
    if (index < 0)
    {
        for (int i = 0; i < kMaxPointers; i++)
            if (mActiveItem[i] == itemID)
                return true;

        return false;
    }

    return mActiveItem[index] == itemID;
}

bool cUIState::SetActive(tUIItemID itemID, int index)
{
    if (mActiveItem[index] == kNullUIItemID)
    {
        mActiveItem[index] = itemID;
        mActiveCount++;
        return true;
    }

    return false;
}

bool cUIState::IsSelected(tUIItemID itemID, int index)
{
    if (index < 0)
    {
        for (int i = 0; i < kMaxPointers; i++)
            if (mSelectedItem[i] == itemID)
                return true;

        return false;
    }

    return mSelectedItem[index] == itemID;
}

bool cUIState::SetSelected(tUIItemID itemID, int index)
{
    if (mSelectedItem[index] != itemID)
    {
        if (mSelectedItem[index] != kNullUIItemID)
            mSelectedCount--;

        mSelectedItem[index] = itemID;

        if (itemID != kNullUIItemID)
            mSelectedCount++;

        return true;
    }

    return false;
}

int cUIState::SelectedCount(tUIItemID itemID) const
{
    int count = 0;

    for (int i = 0; i < kMaxPointers; i++)
        if (mSelectedItem[i] == itemID)
            count++;
    
    return count;
}

bool cUIState::HasControl() const
{
    return mActiveCount + mSelectedCount > 0;
}




bool cUIState::AddItemRect(tUIItemID itemID, Vec2f rect, tUIDisplayFlags flags)
{
    mItemID = itemID;
    mItemClicked = false;

#ifdef CL_IOS
    Vec2f startDeltaPos = vl_0;
    Vec2f lastDeltaPos  = vl_0;
#endif

    for (int i = 0; i < kMaxPointers; i++)
    {
    #ifndef CL_IOS
        if (i == 0 && mAutoScrollEnabled)
            AutoScroll(i, rect);    // only for first pointer, otherwise we get fighting
    #endif

        if (!(flags & kUIDisabled) && PointerInRect(i, rect))
            SetActive(itemID, i);

        if (IsActive(itemID, i) && mInputState->PointerWentDown(i))
            SetSelected(itemID, i);

        if (IsSelected(itemID, i))
        {
        #ifdef CL_IOS
            const cPointerInfo& info = mInputState->PointerInfo(i);
            startDeltaPos  += info.mCurrentPos - info.mStartPos;
            lastDeltaPos   += info.mCurrentPos - info.mLastPos;
        #endif
            if (mInputState->PointerWentUp(i))
            {
                if (IsActive(itemID, i))
                    mItemClicked = true;

                SetSelected(kNullUIItemID, i);    // IsSelected() may still return true if another pointer is active
            }
        }
    }

#ifdef CL_IOS
    if (mAutoScrollEnabled && sqrlen(startDeltaPos) > sqr(mDragDistance) && !(flags & kUISlider))
        mInfo.mAutoScroll = -lastDeltaPos;
#endif

    return mItemClicked;
}

// Internal

void cUIState::Save(cUICursorInfo* cursorInfo)
{
    *cursorInfo = mInfo;
}

void cUIState::Restore(cUICursorInfo* cursorInfo)
{
    mInfo = *cursorInfo;
}

bool cUIState::PointerInRect(int pointerIndex, Vec2f rect)
{
    Vec2f mp = mInputState->PointerInfo(pointerIndex).mCurrentPos;
    Vec2f rp = mInfo.mDir * (mp - mInfo.mCursor);

    return rp[0] >= 0 && rp[0] < rect[0]
        && rp[1] >= 0 && rp[1] < rect[1];
}

void cUIState::AutoScroll(int pointerIndex, Vec2f rect)
{
    // If we're within mScrollDistance of the given rect, and it lies partially offscreen, scroll the containing
    // menu to move it onscreen.

    Vec2f mp = mInputState->PointerInfo(pointerIndex).mCurrentPos;

    Vec2f sp = ClampUnit((mp - mInfo.mCursor) / (mInfo.mDir * rect));   // relative coordinates, clamped to get closest border point
    Vec2f dp = mp - (mInfo.mCursor + sp * mInfo.mDir * rect);   // delta to border point
    float d2 = sqrlen(dp);

    if (d2 < mScrollDistance * mScrollDistance)
    {
        float b = mCellBorder;   // Test if rect is within 'b' of the screen border, and scroll the other way if so.

        if (mInfo.mCursor[0] < b || (mInfo.mCursor[0] + mInfo.mDir[0] * rect[0]) < b)
            mInfo.mAutoScroll[0] = -mScrollSpeed;

        if (mInfo.mCursor[0] + b >= mScreenSize[0] || (mInfo.mCursor[0] + b + mInfo.mDir[0] * rect[0]) >= mScreenSize[0])
            mInfo.mAutoScroll[0] = +mScrollSpeed;

        if (mInfo.mCursor[1] < b || (mInfo.mCursor[1] + mInfo.mDir[1] * rect[1]) < b)
            mInfo.mAutoScroll[1] = -mScrollSpeed;

        if (mInfo.mCursor[1] + b >= mScreenSize[1] || mInfo.mCursor[1] + b + mInfo.mDir[1] * rect[1] >= mScreenSize[1])
            mInfo.mAutoScroll[1] = +mScrollSpeed;
    }
}

void cUIState::SetCursorInternal(Vec2f c)
{
    mInfo.mCursor         = c;
    mInfo.mLastCursor     = c;
    mInfo.mCursorStart    = c;
    mInfo.mMaxSize        = vl_0;
    mInfo.mLastCellSize   = vl_0;
}



/// --- Utilities --------------------------------------------------------------


Vec2f nHL::UpdateScroll(Vec2f scroll, float taper)
{
    if      (scroll[0] < 0.0f)
        scroll[0] = min(0.0f, scroll[0] + taper);
    else if (scroll[0] > 0.0f)
        scroll[0] = max(0.0f, scroll[0] - taper);

    if      (scroll[1] < 0.0f)
        scroll[1] = min(0.0f, scroll[1] + taper);
    else if (scroll[1] > 0.0f)
        scroll[1] = max(0.0f, scroll[1] - taper);

    return scroll;
}

namespace
{
    string sFormattedText;
}

const char* nHL::Format(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    sFormattedText.vformat(format, args);
    va_end(args);

    return sFormattedText.c_str();
}

void nHL::ConfigUI(const cObjectValue* config, cUIState* uiState)
{
    if (!config)
    {
        uiState->SetAutoScroll(true);
        uiState->ClearColour();
        uiState->SetStyle(0);
        return;
    }

    const cValue& position = config->Member("position");

    if (!position.IsNull())
        uiState->SetCursor(AsVec2(position));
    else
    {
        if (config->Member("advanceColumn").AsBool())
            uiState->AdvanceCol();
        if (config->Member("advanceRow").AsBool())
            uiState->AdvanceRow();

        const cValue& offset = config->Member("offset");

        if (!offset.IsNull())
            uiState->OffsetCursor(AsVec2(offset));
    }

    const cValue& size = config->Member("size");

    if (!size.IsNull())
        uiState->SetCellSize(AsVec2(size));

    const cValue& autoScroll = config->Member("autoScroll");

    if (!autoScroll.IsNull())
        uiState->SetAutoScroll(autoScroll.AsBool(true));

    const cValue& colour = config->Member("colour");

    if (!colour.IsNull())
        uiState->SetColour(AsVec3(colour, kColourWhite));

    const cObjectValue* style = config->Member("style").AsObject();

    if (style)
        uiState->SetStyle(style);
}


void nHL::SetInputShaderData(const cInputState* state, cIRenderer* renderer)
{
    int nc = max(state->NumActivePointers(), 4);

    for (int i = 1; i < nc; i++)
        renderer->SetShaderDataT<Vec2f>(kDataIDPointer1 + i, state->PointerInfo(0).mCurrentPos);
}

void nHL::DebugDrawInputState(const cInputState& inputState, cDebugDraw* dd)
{
#if TARGET_OS_IPHONE
    #if TARGET_IPHONE_SIMULATOR == 0
        const float kPointerSize = 20.0f;
    #else
        const float kPointerSize = 10.0f;
    #endif
#else
    const float kPointerSize = 5.0f;
#endif

    for (int i = 0, n = inputState.NumActivePointers(); i < n; i++)
    {
        Vec2f p = inputState.PointerInfo(i).mCurrentPos;

        if (inputState.PointerWentDown(i))
        {
            CL_INPUT_LOG("pointer %d down at %d\n", i, inputState.Counter());

            dd->SetColour(kColourGreen);
            FillCircle(dd, p[0], p[1], kPointerSize * 2.0f);
        }
        else if (inputState.PointerWentUp(i))
        {
            CL_INPUT_LOG("pointer %d up at %d\n", i, inputState.Counter());
            
            dd->SetColour(kColourRed);
            FillCircle(dd, p[0], p[1], kPointerSize * 2.0f);
        }
        else if (inputState.PointerIsDown(i))
        {
            CL_INPUT_LOG("pointer %d still down at %d\n", i, inputState.Counter());

            Vec2f sp = inputState.PointerInfo(i).mStartPos;
            dd->SetColour(kColourCyan * 0.75f);
            FillCircle(dd, sp[0], sp[1], kPointerSize);

            Vec2f lp = inputState.PointerInfo(i).mLastPos;
            dd->SetColour(kColourBlue * 0.5f);
            FillCircle(dd, lp[0], lp[1], kPointerSize);

            dd->SetColour(kColourBlue);
            FillCircle(dd, p[0], p[1], kPointerSize);
        }
        else
        {
            CL_INPUT_LOG("pointer %d still up at %d\n", i, inputState.Counter());

            dd->SetColour(kColourOrange * 0.5f);
            FillCircle(dd, p[0], p[1], kPointerSize);
        }
    }
}

bool nHL::ShowObjectMenu
(
    cUIState*           uiState,
    tUIItemID           itemID,
    const char*         name,
    const cObjectValue* object,
    tObjectMenuType     objectMenuType,
    const char*         mark
)
{
    bool open = uiState->InputState()->KeyModifiers() & kModControl;
    string s;

    if (open)
        mark = "^";

    cUIMenuInfo* menuInfo = uiState->CurrentMenuInfo();
    bool wasSelected = menuInfo->IsSelected(itemID);
    cFileSpec spec;
    int index = 0;

    if (uiState->BeginSubMenu(itemID, name))
    {
        if (objectMenuType == kObjectMenuNormal)
        {
            for (auto c : object->Children())
                if (!MemberIsHidden(c.Tag()))
                    if (ShowValueMenu(uiState, itemID + index++, c.Name(), c.Value(), objectMenuType, mark) && open)
                    {
                        FindSpec(&spec, c.Owner());
                        OpenTextFile(spec.Path(), 0);
                    }
        }
        else if (objectMenuType == kObjectMenuLocalAndParents)
        {
            int numParents = object->NumParents();

            if (numParents > 0)
            {
                for (int i = 0; i < numParents; i++)
                {
                    const cObjectValue* parent = object->Parent(i);
                    FindSpec(&spec, parent);

                    s.format("<%s>", spec.Name());

                    if (ShowObjectMenu(uiState, itemID + 256 + i, s.c_str(), parent, objectMenuType, mark) && open)
                    {
                        OpenTextFile(spec.Path(), 0);
                    }
                }

                uiState->DrawSeparator();
            }

            for (int i = 0, n = object->NumMembers(); i < n; i++)
                if (!MemberIsHidden(object->MemberTag(i)))
                    if (ShowValueMenu(uiState, itemID + index++, object->MemberName(i), object->MemberValue(i), objectMenuType, mark) && open)
                    {
                        FindSpec(&spec, object);
                        OpenTextFile(spec.Path(), 0);
                    }
        }

        uiState->EndSubMenu(itemID);
    }

    menuInfo = uiState->CurrentMenuInfo();  // refresh in case stale...
    bool isSelected = menuInfo->IsSelected(itemID);

    return isSelected != wasSelected;
}

bool nHL::ShowValueMenu
(
    cUIState*           uiState,
    tUIItemID           itemID,
    const char*         name,
    const cValue&       value,
    tObjectMenuType     objectMenuType,
    const char*         mark
)
{
    nCL::string s;

    switch (value.Type())
    {
    case kValueNull:
        return uiState->HandleButtonWithMark(itemID, name, mark, kUIDisabled);
    case kValueBool:
        return uiState->HandleToggle(itemID, name, value.AsBool());
    case kValueInt:
        s.format("%s: %d", name, value.AsInt());
        return uiState->HandleButtonWithMark(itemID, s.c_str(), mark);
    case kValueUInt:
        s.format("%s: %u", name, value.AsUInt());
        return uiState->HandleButtonWithMark(itemID, s.c_str(), mark);
    case kValueDouble:
        s.format("%s: %f", name, value.AsFloat());
        return uiState->HandleButtonWithMark(itemID, s.c_str(), mark);
    case kValueString:
        s.format("%s: %s", name, value.AsString());
        return uiState->HandleButtonWithMark(itemID, s.c_str(), mark);
    case kValueArray:
        {
            uint32_t uid = itemID >> 32;
            uid = StrIHashU32(name, uid);
            itemID = ItemID(uid, 0);

            int numElts = value.NumElts();

            if (numElts == 0)
            {
                s.format("%s: []", name);
                return uiState->HandleButtonWithMark(itemID, s.c_str(), mark);
            }
            else if (numElts <= 4)
            {
                bool hasFP = false;
                bool hasBool = false;
                bool hasOther = false;

                for (int i = 0; i < numElts; i++)
                {
                    const cValue& elt = value.Elt(i);

                    if (!elt.IsNumeric())
                    {
                        hasOther = true;
                        break;
                    }
                    if (elt.IsDouble())
                        hasFP = true;
                    //if (elt.IsBool())
                    //    hasBool = true;
                }

                if (!hasOther)
                {
                    if (hasFP)
                    {
                        s.format("%s: [%.2f", name, value.Elt(0).AsDouble());
                        for (int i = 1; i < numElts; i++)
                            s.append_format(", %.2f", value.Elt(i).AsDouble());
                        s += "]";
                    }
                    else
                    {
                        s.format("%s: [%d", name, value.Elt(0).AsInt());
                        for (int i = 1; i < numElts; i++)
                            s.append_format(", %d", value.Elt(i).AsInt());
                        s += "]";
                    }

                    return uiState->HandleButtonWithMark(itemID, s.c_str(), mark);
                }
            }

            if (uiState->BeginSubMenu(itemID++, name))
            {
                for (int i = 0, n = value.NumElts(); i < n; i++)
                {
                    s.format("%d", i);

                    ShowValueMenu(uiState, itemID++, s.c_str(), value.Elt(i), objectMenuType, mark);
                }

                uiState->EndSubMenu();
            }
        }
        return false;
    case kValueObject:
        uint32_t uid = itemID >> 32;
        uid = StrIHashU32(name, uid);
        itemID = ItemID(uid, 0);

        return ShowObjectMenu(uiState, itemID, name, value.AsObject(), objectMenuType, mark);
    }
}
