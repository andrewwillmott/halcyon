//
//  File:       HLUIDraw.cpp
//
//  Function:   UI render support (see HLUI.h)
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLUIDraw.h>

#include <HLDebugDraw.h>
#include <HLRenderUtils.h>
#include <HLUI.h>

#include <CLVecUtil.h>

using namespace nHL;
using namespace nCL;

namespace
{

}

void nHL::DrawUI(int numCommands, const cUIDrawCommand commands[], const cDataStore* store, cIUIDraw* uiDraw)
{
    // Dispatch queued draw commands in reverse order
    int r1 = numCommands;
    int r0 = r1 - 1;

    while (r1 > 0)
    {
        while (r0 > 0 && commands[r0].mCommand != kDrawItemStart)
            r0--;

        for (int r = r0; r < r1; r++)
        {
            const cUIDrawCommand& cmd = commands[r];

            // execute
            switch (cmd.mCommand)
            {
            case kDrawItemStart:
                uiDraw->CellRect(AsCStr(cmd.mParam, store), cmd.mInfo, cmd.mFlags);
                uiDraw->ClearColour();
                uiDraw->SetStyle(0);
                break;
            case kDrawCell:
                uiDraw->DrawCell(cmd.mInfo, cmd.mFlags, AsCStr(cmd.mParam, store));
                break;
            case kDrawSwatch:
                uiDraw->DrawSwatch(cmd.mInfo, cmd.mFlags, As<cColour>(cmd.mParam, store));
                break;
            case kDrawSubMenuMark:
                uiDraw->DrawSubMenuMark(cmd.mInfo, cmd.mFlags);
                break;
            case kDrawMark:
                uiDraw->DrawMark(cmd.mInfo, cmd.mFlags, AsCStr(cmd.mParam, store));
                break;
            case kDrawProgressBar:
                uiDraw->DrawProgressBar(cmd.mInfo, cmd.mFlags, As<float>(cmd.mParam, store));
                break;
            case kDrawSlider:
                uiDraw->DrawSlider(cmd.mInfo, cmd.mFlags, As<float>(cmd.mParam, store));
                break;
            case kDrawSetColour:
                uiDraw->SetColour(As<cColour>(cmd.mParam, store));
                break;
            case kDrawSetStyle:
                cObjectValue* style = As<cObjectValue*>(cmd.mParam, store);
                uiDraw->SetStyle(style);
                style->Link(-1);
                break;
            }
        }

        r1 = r0;
        r0--;
    }
}

// --- cUIRenderInfo -----------------------------------------------------------

void cUIRenderInfo::Config(const nCL::cObjectValue* config)
{
    if (!config)
    {
        *this = cUIRenderInfo();
        return;
    }
    
    mCellTextBorder     = config->Member(CL_TAG("textBorder")).AsFloat(4.0f);
    mSwatchSize         = config->Member(CL_TAG("swatchSize")).AsFloat(10.0f);
    mSliderBarSize      = config->Member(CL_TAG("sliderBar" )).AsFloat(100.0f);

    mMenuAlpha          = config->Member(CL_TAG("menuAlpha")).AsFloat(0.7f);
    mMenuGhostAlpha     = config->Member(CL_TAG("menuGhostAlpha")).AsFloat(0.2f);
    SetFromValue(config->Member(CL_TAG("menuGradient")),         &mMenuGradient);
    SetFromValue(config->Member(CL_TAG("menuInset")),            &mMenuInset);

    SetFromValue(config->Member(CL_TAG("colourNormal"   )),      &mColourNormal);
    SetFromValue(config->Member(CL_TAG("colourSelected" )),      &mColourSelected);
    SetFromValue(config->Member(CL_TAG("colourHighlight")),      &mColourHighlight);
    SetFromValue(config->Member(CL_TAG("colourOutline"  )),      &mColourOutline);

    SetFromValue(config->Member(CL_TAG("textColourEnabled") ),   &mTextColourNormal);
    SetFromValue(config->Member(CL_TAG("textColourNormal") ),    &mTextColourNormal);
    SetFromValue(config->Member(CL_TAG("textColourDisabled")),   &mTextColourDisabled);
    SetFromValue(config->Member(CL_TAG("textColourSelected")),   &mTextColourSelected);
    SetFromValue(config->Member(CL_TAG("textColourGhost")),      &mTextColourGhost);
}


// --- cStandardUIDraw ---------------------------------------------------------

namespace nHL
{
    class cStandardUIDraw :
        public cIUIDraw,
        public cAllocatable
    {
    public:
        // cIUIDraw
        void Config(const nCL::cObjectValue* config) override;

        Vec2f CellRect(const char* label, const cUICursorInfo& info, tUIDisplayFlags flags) override;

        void DrawUI(int numCommands, const cUIDrawCommand commands[], const nCL::cDataStore* store) override;

        void DrawCell       (const cUICursorInfo& info, tUIDisplayFlags flags, const char* label) override;
        void DrawSwatch     (const cUICursorInfo& info, tUIDisplayFlags flags, cColour c) override;
        void DrawSubMenuMark(const cUICursorInfo& info, tUIDisplayFlags flags) override;
        void DrawMark       (const cUICursorInfo& info, tUIDisplayFlags flags, const char* mark) override;
        void DrawProgressBar(const cUICursorInfo& info, tUIDisplayFlags flags, float s) override;
        void DrawSlider     (const cUICursorInfo& info, tUIDisplayFlags flags, float s) override;

        void SetColour  (cColour c) override;
        void ClearColour() override;
        void SetStyle   (const cObjectValue* style) override;

        // cStandardUIDraw
        void Dispatch(nHL::cIRenderer* renderer, const nHL::cRenderLayerState& state);

        void SetDebugLabels(bool enabled);
        bool DebugLabels();

    protected:
        // Data definitions
        struct cTextureRectInfo
        {
            cTextureRectInfo(tTextureRef ref, cBounds2 rect) : mTextureRef(ref), mRect(rect) {}

            tTextureRef mTextureRef = -1;
            cBounds2    mRect;
        };

        // Data
        cDebugDraw* mDraw = HL()->mDebugDraw;
        Vec2f mCellRect = vl_0;
        Vec2f mTextRect = vl_0;

        cLink<const cObjectValue> mStyle;

        cUIRenderInfo mCurrRI;
        cUIRenderInfo mBaseRI;

        bool mShowDebugLabels = true;

        vector<cTextureRectInfo> mDisabledTextureRects;
        vector<cTextureRectInfo> mTextureRects;
        vector<cTextureRectInfo> mHighlightedTextureRects;
    };
}

cIUIDraw* nHL::CreateStandardUIDraw(nCL::cIAllocator* alloc)
{
    return new(alloc) cStandardUIDraw;
}

void cStandardUIDraw::Config(const nCL::cObjectValue* config)
{
    mBaseRI.Config(config);

    mCurrRI = mBaseRI;
}

Vec2f cStandardUIDraw::CellRect(const char* label, const cUICursorInfo& info, tUIDisplayFlags flags)
{
    mTextRect = mDraw->TextSize(label);

    if (flags & kUISwatch)
        mTextRect[0] += mCurrRI.mSwatchSize + mCurrRI.mCellTextBorder;
    if (flags & kUISlider)
        mTextRect[0] += mCurrRI.mSliderBarSize;

    mCellRect = Vec2f(mTextRect + 2.0f * Vec2f(mCurrRI.mCellTextBorder));
    mCellRect = MaxElts(mCellRect, info.mCellSize);

    return mCellRect;
}

void cStandardUIDraw::DrawUI(int numCommands, const cUIDrawCommand commands[], const nCL::cDataStore* store)
{
    mDisabledTextureRects.clear();
    mTextureRects.clear();
    mHighlightedTextureRects.clear();

    ::DrawUI(numCommands, commands, store, this);
}

void cStandardUIDraw::DrawCell(const cUICursorInfo& info, tUIDisplayFlags flags, const char* label)
{
    Vec2f dirAdjust = (info.mDir - vl_1) * 0.5f;
    Vec2f cursor = info.mCursor + dirAdjust * mCellRect;

    cBounds2 cellRect(cursor, cursor + mCellRect);

    if (mStyle)
    {
        int textureRef = mStyle->Member(CL_TAG("textureRef")).AsInt(-1);

        if (textureRef < 0)
        {
            tTag textureTag = mStyle->Member(CL_TAG("texture")).AsTag();
            textureRef = HL()->mRenderer->TextureRefFromTag(textureTag);
        }

        if (textureRef >= 0)
        {
            cTextureRectInfo rectInfo(textureRef, cellRect);

            if (flags & kUIDisabled)
                mDisabledTextureRects.push_back(rectInfo);
            else if (flags & kUIHighlighted)
                mHighlightedTextureRects.push_back(rectInfo);
            else if ((flags & (kUISelected | kUIActive)) == (kUISelected | kUIActive))
                mHighlightedTextureRects.push_back(rectInfo);
            else
                mTextureRects.push_back(rectInfo);
        }
    }

    if (!mShowDebugLabels)
        return;

    // DEBUG TEXT LABEL
    Vec2f textPos = info.mCursor;

    if (flags & kUILabelCentre)
        textPos[0] += floorf(0.5f * (mCellRect[0] - mTextRect[0]));
    else
        textPos[0] += mCurrRI.mCellTextBorder;

    textPos[1] += mCellRect[1] + 3 * mCurrRI.mCellTextBorder;

    Vec2f invDir(Vec2f(vl_1) - info.mDir);
    textPos -= invDir * mCellRect * 0.5f;

    cBounds2 backRect(textPos, textPos + mTextRect);
    backRect.Inflate(mCurrRI.mCellTextBorder);

    float menuAlpha;
    if (!(flags & kUIGhost))
        menuAlpha = mCurrRI.mMenuAlpha;
    else
        menuAlpha = mCurrRI.mMenuGhostAlpha;

    cColour menuColour;
    if (flags & kUISelected)
        menuColour = mCurrRI.mColourSelected;
    else if (flags & kUIHighlighted)
        menuColour = mCurrRI.mColourHighlight;
    else
        menuColour = mCurrRI.mColourNormal;

    mDraw->SetAlpha(menuAlpha);
    mDraw->SetColour(menuColour);
    FillRect(mDraw, backRect.mMin[0], backRect.mMin[1], backRect.mMax[0], backRect.mMax[1]);

    mDraw->SetAlpha(1.0f);

    if (flags & kUITextSelected)
        mDraw->SetColour(mCurrRI.mTextColourSelected);
    else if (flags & kUIDisabled)
        mDraw->SetColour(mCurrRI.mTextColourDisabled);
    else
        mDraw->SetColour(mCurrRI.mTextColourNormal);

    mDraw->DrawText(textPos[0], textPos[1], label);

    mDraw->SetColour(mCurrRI.mColourOutline);
    DrawRect(mDraw, cellRect.mMin[0], cellRect.mMin[1], cellRect.mMax[0], cellRect.mMax[1]);
}

void cStandardUIDraw::DrawSwatch(const cUICursorInfo& info, tUIDisplayFlags flags, cColour c)
{
}

void cStandardUIDraw::DrawSubMenuMark(const cUICursorInfo& info, tUIDisplayFlags flags)
{
}

void cStandardUIDraw::DrawMark(const cUICursorInfo& info, tUIDisplayFlags flags, const char* mark)
{
}

void cStandardUIDraw::DrawProgressBar(const cUICursorInfo& info, tUIDisplayFlags flags, float s)
{
}

void cStandardUIDraw::DrawSlider(const cUICursorInfo& info, tUIDisplayFlags flags, float s)
{
}

void cStandardUIDraw::SetColour(cColour c)
{
    mCurrRI.mColourNormal = c;

    Vec3f ratio = c / MaxElts(mBaseRI.mColourNormal, Vec3f(0.05f));

    mCurrRI.mColourSelected  = ClampUnit(mBaseRI.mColourSelected  * ratio);
    mCurrRI.mColourHighlight = ClampUnit(mBaseRI.mColourHighlight * ratio);
    mCurrRI.mColourOutline   = ClampUnit(mBaseRI.mColourOutline   * ratio);
}

void cStandardUIDraw::ClearColour()
{
    mCurrRI.mColourSelected     = mBaseRI.mColourSelected;
    mCurrRI.mColourHighlight    = mBaseRI.mColourHighlight;
    mCurrRI.mColourNormal       = mBaseRI.mColourNormal;
    mCurrRI.mColourOutline      = mBaseRI.mColourOutline;
}

void cStandardUIDraw::SetStyle(const cObjectValue* style)
{
    mStyle = style;

    mCurrRI = mBaseRI;

    if (style)
        mCurrRI.Config(style);
}

void cStandardUIDraw::Dispatch(nHL::cIRenderer* renderer, const nHL::cRenderLayerState& state)
{
    // Do not look here for efficient rendering =P
    int disabledMaterial = renderer->MaterialRefFromTag(CL_TAG("uiRectDisabled"));
    int enabledMaterial = renderer->MaterialRefFromTag(CL_TAG("uiRect"));

    renderer->SetMaterial(disabledMaterial);

    for (auto const& ri : mDisabledTextureRects)
    {
        renderer->SetTexture(kTextureDiffuseMap, ri.mTextureRef);
        DrawScreenRect(renderer, kRGBA32WhiteA1, ri.mRect);
    }

    renderer->SetMaterial(enabledMaterial);
    cRGBA32 dimWhite = { 225, 225, 225, 255 };

    for (auto const& ri : mTextureRects)
    {
        renderer->SetTexture(kTextureDiffuseMap, ri.mTextureRef);
        DrawScreenRect(renderer, dimWhite, ri.mRect);
    }

    for (auto const& ri : mHighlightedTextureRects)
    {
        renderer->SetTexture(kTextureDiffuseMap, ri.mTextureRef);
        DrawScreenRect(renderer, kRGBA32WhiteA1, ri.mRect);
    }
}

void cStandardUIDraw::SetDebugLabels(bool enabled)
{
    mShowDebugLabels = enabled;
}

bool cStandardUIDraw::DebugLabels()
{
    return mShowDebugLabels;
}


#ifndef CL_RELEASE

// --- cDebugUIDraw ------------------------------------------------------------

namespace nHL
{
    class cDebugUIDraw :
        public cIUIDraw,
        public cAllocatable
    {
    public:
        // cIUIDraw
        void Config(const nCL::cObjectValue* config) override;

        Vec2f CellRect(const char* label, const cUICursorInfo& info, tUIDisplayFlags flags) override;

        void DrawUI(int numCommands, const cUIDrawCommand commands[], const nCL::cDataStore* store) override;

        void DrawCell       (const cUICursorInfo& info, tUIDisplayFlags flags, const char* label) override;
        void DrawSwatch     (const cUICursorInfo& info, tUIDisplayFlags flags, cColour c) override;
        void DrawSubMenuMark(const cUICursorInfo& info, tUIDisplayFlags flags) override;
        void DrawMark       (const cUICursorInfo& info, tUIDisplayFlags flags, const char* mark) override;
        void DrawProgressBar(const cUICursorInfo& info, tUIDisplayFlags flags, float s) override;
        void DrawSlider     (const cUICursorInfo& info, tUIDisplayFlags flags, float s) override;

        void SetColour  (cColour c) override;
        void ClearColour() override;
        void SetStyle   (const cObjectValue* style) override;

    protected:
        cDebugDraw* mDraw = HL()->mDebugDraw;
        Vec2f mCellRect = vl_0;
        Vec2f mTextRect = vl_0;

        cUIRenderInfo mBaseRI;
        cUIRenderInfo mCurrRI;
    };
}

void cDebugUIDraw::Config(const nCL::cObjectValue* config)
{
    mBaseRI.Config(config);
    mCurrRI = mBaseRI;
}

Vec2f cDebugUIDraw::CellRect(const char* label, const cUICursorInfo& info, tUIDisplayFlags flags)
{
    mTextRect = mDraw->TextSize(label);

    if (flags & kUISwatch)
        mTextRect[0] += mCurrRI.mSwatchSize + mCurrRI.mCellTextBorder;
    if (flags & kUISlider)
        mTextRect[0] += mCurrRI.mSliderBarSize;

    mCellRect = Vec2f(mTextRect + 2.0f * Vec2f(mCurrRI.mCellTextBorder));
    mCellRect = MaxElts(mCellRect, info.mCellSize);

    return mCellRect;
}

void cDebugUIDraw::DrawUI(int numCommands, const cUIDrawCommand commands[], const nCL::cDataStore* store)
{
    ::DrawUI(numCommands, commands, store, this);
}

void cDebugUIDraw::DrawCell(const cUICursorInfo& info, tUIDisplayFlags flags, const char* label)
{
    float menuAlpha;
    if (!(flags & kUIGhost))
        menuAlpha = mCurrRI.mMenuAlpha;
    else
        menuAlpha = mCurrRI.mMenuGhostAlpha;

    cColour menuColour;
    if (flags & kUISelected)
        menuColour = mCurrRI.mColourSelected;
    else if (flags & kUIHighlighted)
        menuColour = mCurrRI.mColourHighlight;
    else
        menuColour = mCurrRI.mColourNormal;

    Vec2f dirAdjust = (info.mDir - vl_1) * 0.5f;
    Vec2f cursor = info.mCursor + dirAdjust * mCellRect;

    float rectX0 = cursor[0];
    float rectY0 = cursor[1];
    float rectX1 = cursor[0] + mCellRect[0];
    float rectY1 = cursor[1] + mCellRect[1];

    mDraw->SetAlpha(menuAlpha);

    cColourAlpha baseColour(menuColour, menuAlpha);
    if (mCurrRI.mMenuGradient[0] != mCurrRI.mMenuGradient[1])
        FillGradientRect(mDraw, rectX0, rectY0, rectX1, rectY1, baseColour, mCurrRI.mMenuGradient[0], mCurrRI.mMenuGradient[1]);
    else
    {
        mDraw->SetColour(menuColour);
        FillRect(mDraw, rectX0, rectY0, rectX1, rectY1);
    }

    if (mCurrRI.mMenuInset[0] != 1.0f)
    {
        mDraw->SetColour(mCurrRI.mColourNormal * mCurrRI.mMenuInset[0]);
        DrawLine(mDraw, rectX0 + 1, rectY0 + 1, rectX1 - 2, rectY0 + 1);
        DrawLine(mDraw, rectX0 + 1, rectY0, rectX0 + 1, rectY1 - 1);

        mDraw->SetColour(mCurrRI.mColourNormal * mCurrRI.mMenuInset[1]);
        DrawLine(mDraw, rectX0 + 1, rectY1 - 1, rectX1 - 1, rectY1 - 1);
        DrawLine(mDraw, rectX1 - 1, rectY0 + 1, rectX1 - 1, rectY1 - 1);
    }

    if (!(flags & kUIGhost))
    {
        mDraw->SetAlpha(1.0f);

        if (flags & kUITextSelected)
            mDraw->SetColour(mCurrRI.mTextColourSelected);
        else if (flags & kUIDisabled)
            mDraw->SetColour(mCurrRI.mTextColourDisabled);
        else
            mDraw->SetColour(mCurrRI.mTextColourNormal);
    }
    else
        mDraw->SetColour(mCurrRI.mTextColourGhost);

    // TEXT
    Vec2f textPos = info.mCursor;

    if (flags & kUILabelCentre)
        textPos[0] += floorf(0.5f * (mCellRect[0] - mTextRect[0]));
    else
        textPos[0] += mCurrRI.mCellTextBorder;

    textPos[1] += floorf(0.5f * (mCellRect[1] - mTextRect[1]));

    Vec2f invDir(Vec2f(vl_1) - info.mDir);
    textPos -= invDir * mCellRect * 0.5f;

    mDraw->DrawText(textPos[0], textPos[1], label);

    mDraw->SetColour(mCurrRI.mColourOutline);
    DrawRect(mDraw, rectX0, rectY0, rectX1, rectY1);
}

void cDebugUIDraw::DrawSwatch(const cUICursorInfo& info, tUIDisplayFlags flags, cColour c)
{
    if (flags & kUIDisabled)
        c *= 0.5f;

    float ss = mCurrRI.mSwatchSize;
    float sb = mCurrRI.mCellTextBorder;

    float swatchRight  = info.mCursor[0] - sb;
    swatchRight  += 0.5f * (+info.mDir[0] + 1.0f) * info.mLastCellSize[0];

    float swatchBottom = info.mCursor[1] - ceilf((info.mLastCellSize[1] - ss) * 0.5f);
    swatchBottom += 0.5f * (-info.mDir[1] + 1.0f) * info.mLastCellSize[1];

    float swatchLeft = swatchRight  - ss;
    float swatchTop  = swatchBottom - ss;

    mDraw->SetColour(c);
    FillRect(mDraw, swatchLeft, swatchTop, swatchRight, swatchBottom);
    mDraw->SetColour(0, 0, 0);
    DrawRect(mDraw, swatchLeft, swatchTop, swatchRight, swatchBottom);
}

void cDebugUIDraw::DrawSubMenuMark(const cUICursorInfo& info, tUIDisplayFlags flags)
{
    Vec2f cellSize = info.mLastCellSize;
    Vec2f dirAdjust = (info.mDir - vl_1) * 0.5f;
    dirAdjust[1] = -dirAdjust[1];
    Vec2f cursor = info.mCursor + dirAdjust * cellSize;

    float markRight  = cursor[0] + cellSize[0] - mCurrRI.mCellTextBorder;
    float markLeft   = markRight - 7.0f;
    float markTop    = cursor[1] - (info.mLastCellSize[1] + 7.0f) * 0.5f;
    float markBottom = cursor[1] - (info.mLastCellSize[1] - 7.0f) * 0.5f;
    float markMid = (markTop + markBottom) * 0.5f;

    mDraw->SetColour(mCurrRI.mColourNormal * mCurrRI.mMenuInset[1]);
    DrawLine(mDraw, markLeft, markTop, markRight, markMid);

    mDraw->SetColour(mCurrRI.mColourNormal * mCurrRI.mMenuInset[0]);
    DrawLine(mDraw, markLeft, markBottom, markRight, markMid);

    mDraw->SetColour(mCurrRI.mColourOutline);
    DrawLine(mDraw, markLeft, markTop, markLeft, markBottom);
}

void cDebugUIDraw::DrawMark(const cUICursorInfo& info, tUIDisplayFlags flags, const char* mark)
{
    Vec2f ts = mDraw->TextSize(mark);

    Vec2f cellSize = info.mLastCellSize;

    Vec2f markPos = info.mCursor;
    markPos[1] -= info.mDir[1] * info.mLastCellSize[1]; // jump back to previous line
    markPos += Vec2f(cellSize[0] - mCurrRI.mCellTextBorder - ts[0], floorf(0.5f * (cellSize[1] - ts[1])));
    Vec2f invDir(Vec2f(vl_1) - info.mDir);
    markPos -= invDir * cellSize * 0.5f;

    mDraw->SetColour(mCurrRI.mTextColourNormal);
    DrawText(mDraw, markPos[0], markPos[1], mark);
}

void cDebugUIDraw::DrawProgressBar(const cUICursorInfo& info, tUIDisplayFlags flags, float s)
{
    Vec2f cellSize = info.mLastCellSize;
    Vec2f dirAdjust = (info.mDir - vl_1) * 0.5f;
    Vec2f cursor = info.mLastCursor + dirAdjust * cellSize;

    float right  = cursor[0] + cellSize[0] - mCurrRI.mCellTextBorder;
    float left   = cursor[0] + mCurrRI.mCellTextBorder;
    float top    = cursor[1] - (info.mLastCellSize[1] + mCurrRI.mSwatchSize) * 0.5f;
    float bottom = cursor[1] - (info.mLastCellSize[1] - mCurrRI.mSwatchSize) * 0.5f;

    float middle = floorf(lerp(left, right, s));

    mDraw->SetColour(kColourYellow);
    FillRect(mDraw, left, top, middle, bottom);
    mDraw->SetColour(kColourWhite * 0.25f);
    FillRect(mDraw, middle, top, right, bottom);

    mDraw->SetColour(0, 0, 0);
    DrawRect(mDraw, left, top, right, bottom);
}

void cDebugUIDraw::DrawSlider(const cUICursorInfo& info, tUIDisplayFlags flags, float s)
{
    Vec2f cellSize = info.mLastCellSize;
    Vec2f dirAdjust = (info.mDir - vl_1) * 0.5f;
    Vec2f cursor = info.mLastCursor + dirAdjust * cellSize;

    float right  = cursor[0] + cellSize[0] - mCurrRI.mCellTextBorder;
    float left   = cursor[0] + mCurrRI.mCellTextBorder + cellSize[0] - mCurrRI.mSliderBarSize;
    float top    = cursor[1] + (info.mLastCellSize[1] + mCurrRI.mSwatchSize) * 0.5f;
    float bottom = cursor[1] + (info.mLastCellSize[1] - mCurrRI.mSwatchSize) * 0.5f;

    float middle = floorf(lerp(left, right, s));

    if (flags & kUIDisabled)
        mDraw->SetColour(lerp(kColourYellow, mCurrRI.mTextColourDisabled, 0.5f));
    else
        mDraw->SetColour(kColourYellow);

    FillRect(mDraw, left, top, middle, bottom);

    if (flags & kUIDisabled)
        mDraw->SetColour(lerp(kColourWhite * 0.25f, static_cast<const Vec3f&>(mCurrRI.mTextColourDisabled), 0.5f));
    else
        mDraw->SetColour(kColourWhite * 0.25f);

    FillRect(mDraw, middle, top, right, bottom);

    mDraw->SetColour(kColourBlack);
    DrawRect(mDraw, left, top, right, bottom);
}

void cDebugUIDraw::SetColour(cColour c)
{
    mCurrRI.mColourNormal = c;

    Vec3f ratio = c / MaxElts(mBaseRI.mColourNormal, Vec3f(0.05f));

    mCurrRI.mColourSelected  = ClampUnit(mBaseRI.mColourSelected  * ratio);
    mCurrRI.mColourHighlight = ClampUnit(mBaseRI.mColourHighlight * ratio);
    mCurrRI.mColourOutline   = ClampUnit(mBaseRI.mColourOutline   * ratio);
}

void cDebugUIDraw::ClearColour()
{
    mCurrRI.mColourSelected     = mBaseRI.mColourSelected;
    mCurrRI.mColourHighlight    = mBaseRI.mColourHighlight;
    mCurrRI.mColourNormal       = mBaseRI.mColourNormal;
    mCurrRI.mColourOutline      = mBaseRI.mColourOutline;
}

void cDebugUIDraw::SetStyle(const cObjectValue* style)
{
    mCurrRI = mBaseRI;

    if (style)
        mCurrRI.Config(style);
}

cIUIDraw* nHL::CreateDebugUIDraw(nCL::cIAllocator* alloc)
{
    return new(alloc) cDebugUIDraw;
}

#endif

