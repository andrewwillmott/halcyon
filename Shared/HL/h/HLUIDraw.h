//
//  File:       HLUIDraw.h
//
//  Function:   UI render support (see HLUI.h)
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef HL_UI_DRAW_H
#define HL_UI_DRAW_H

#include <HLDefs.h>
#include <HLServices.h>

#include <CLColour.h>
#include <CLValue.h>

namespace nCL
{
    class cDataStore;
}

namespace nHL
{
    class cIRenderer;
    class cDebugDraw;
    class cUICursorInfo;
    class cUIDrawCommand;

    // UI Display
    enum tUIDisplayFlag : uint32_t
    {
        kUINoFlags      = 0x0000,
        kUIEnabled      = 0x0000,
        kUIDisabled     = 0x0001,
        kUIHighlighted  = 0x0002,
        kUIActive       = 0x0004,
        kUISelected     = 0x0008,
        kUIGhost        = 0x0010,
        kUITextSelected = 0x0020,
        kUISwatch       = 0x0040,
        kUISlider       = 0x0080,
        kUILabelCentre  = 0x0100,       ///< Centre labels
        kUIFlagMask     = 0x01FF
    };
    typedef uint32_t tUIDisplayFlags;

    // class that handles the actual UI display.
    class cIUIDraw
    {
    public:
        virtual void Config(const cObjectValue* config) = 0;

        virtual Vec2f CellRect(const char* label, const cUICursorInfo& info, tUIDisplayFlags flags) = 0;    ///< Returns size drawn item will be

        virtual void DrawUI(int numCommands, const cUIDrawCommand commands[], const nCL::cDataStore* store) = 0;

        virtual void DrawCell       (const cUICursorInfo& info, tUIDisplayFlags flags, const char* label) = 0;  ///< Draw base cell
        virtual void DrawSwatch     (const cUICursorInfo& info, tUIDisplayFlags flags, cColour c) = 0;          ///< Draw a coloured swatch over current cell
        virtual void DrawSubMenuMark(const cUICursorInfo& info, tUIDisplayFlags flags) = 0;                     ///< Draw submenu indicator over current cell
        virtual void DrawMark       (const cUICursorInfo& info, tUIDisplayFlags flags, const char* mark) = 0;   ///< Draw a mark over current cell
        virtual void DrawProgressBar(const cUICursorInfo& info, tUIDisplayFlags flags, float s) = 0;            ///< Draw a progress bar in the current cell
        virtual void DrawSlider     (const cUICursorInfo& info, tUIDisplayFlags flags, float s) = 0;            ///< Draw slider

        virtual void SetColour  (cColour c) = 0;  ///< Set base UI colour. For convenience only.
        virtual void ClearColour() = 0;

        virtual void SetStyle(const cObjectValue* style) = 0;
    };

    // UI draw commands
    enum tDrawCommand : uint8_t
    {
        kDrawItemStart,
        kDrawCell,
        kDrawSwatch,
        kDrawSubMenuMark,
        kDrawMark,
        kDrawProgressBar,
        kDrawSlider,
        kDrawSetColour,
        kDrawSetStyle,
        kMaxDrawCommands
    };


    void DrawUI(int numCommands, const cUIDrawCommand commands[], const nCL::cDataStore* store, cIUIDraw* uiDraw);   ///< Helper to send given commands to uiDraw.


    // DebugDraw implementation of cIUIDraw

    // --- cDebugUIDraw --------------------------------------------------------

    struct cUIRenderInfo
    {
        // tuning factors
        float   mCellTextBorder     =  4.0f;
        float   mSwatchSize         = 10.0f;
        float   mSliderBarSize      = 100.0f;

        float   mMenuAlpha          = 0.7f;
        float   mMenuGhostAlpha     = 0.2f;
        Vec2f   mMenuGradient       = { 1.0f, 1.0f };
        Vec2f   mMenuInset          = { 1.0f, 1.0f };

        cColour mColourNormal       = { 0.6f, 0.6f, 0.8f };
        cColour mColourSelected     = { 0.6f, 0.7f, 0.9f };
        cColour mColourHighlight    = { 0.8f, 0.6f, 0.6f };
        cColour mColourOutline      = { 0.6f, 0.6f, 0.8f };

        cColour mTextColourNormal   = { 0.4f, 1.0f, 0.1f };
        cColour mTextColourDisabled = { 0.4f, 0.4f, 0.4f };
        cColour mTextColourSelected = { 1.0f, 1.0f, 0.1f };
        cColour mTextColourGhost    = { 0.6f, 0.6f, 0.8f };

        void Config(const nCL::cObjectValue* config);
    };

    cIUIDraw* CreateStandardUIDraw(nCL::cIAllocator* alloc);
    cIUIDraw* CreateDebugUIDraw   (nCL::cIAllocator* alloc);
}

#endif
