//
//  File:       CLGrid.h
//
//  Function:   Utils for dealing with 2D or 3D grids.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_GRID_H
#define CL_GRID_H

#include <CLBounds.h>
#include <CLMath.h>
#include <VL234i.h>

namespace nCL
{
    // --- Cell Utilities ------------------------------------------------------

    Vec2i FindCell (Vec2f p);                                   ///< Find cell containing p
    void  FindCells(const cBounds2& b, Vec2i* m0, Vec2i* m1);   ///< Find cell range spanned by b
    void  ClipCell (Vec2i numCells, Vec2i* m);                  ///< Clip cell to a 0-based grid
    void  ClipCells(Vec2i numCells, Vec2i* m0, Vec2i* m1);      ///< Clip cell range to a 0-based grid
    void  FindQuad (Vec2i numCells, Vec2f p, Vec2i* ciOut, float w[4]); ///< finds cell corner and weights for bilinear mat lookup

    Vec3i FindCell (Vec3f p);                                   ///< Find cell containing p
    void  FindCells(const cBounds3& b, Vec3i* m0, Vec3i* m1);   ///< Find cell range spanned by b
    void  ClipCell (Vec3i numCells, Vec3i* m);                  ///< Clip cell to a 0-based grid
    void  ClipCells(Vec3i numCells, Vec3i* m0, Vec3i* m1);      ///< Clip cell range to a 0-based grid
    void  FindOct  (Vec3i numCells, Vec3f p, Vec3i* ciOut, float w[8]); ///< finds cell corner and weights for bilinear mat lookup




    // --- cGridInfo2 ----------------------------------------------------------

    struct cGridInfo2
    /// Stores info about a 2D grid in world space.
    {
        // Const access only.
        Vec2i      mN            = vl_0;    ///< Grid dimensions
        int        mTotalCells   = vl_0;    ///< Total cell count
        cBounds2   mGridBounds;             ///< Bounds of grid cells
        cBounds2   mCellBounds;             ///< Bounds of origin cell -- useful for mapping directly to cell coordinates
        Vec2f      mInvCellWidth = vl_0;    ///< For acceleration

        void  SetGridInfo(const cBounds2& bounds, Vec2i numCells);
        ///< Explicitly set grid bounds and dimensions
        void  FitGridInfo(float cellWidth, int n, const Vec2f pos[], int stride);
        ///< Fits tracking grid according to the bounds of the given object positions and desired cell size.

        bool  CellIsValid(Vec2i ci) const;                  ///< Returns true if given cell is in bounds

        Vec2f MapToCellCoords  (Vec2f p) const;             ///< Maps p into cell coordinates
        Vec2f MapFromCellCoords(Vec2f c) const;             ///< Maps c into grid space

        int   NumCells () const;                            ///< Returns size of flat array holding cells
        int   CellIndex(Vec2i c) const;                     ///< Returns flat array index for a valid cell

        Vec2i FindCell (Vec2f p) const;                     ///< Finds cell enclosing p.
        void  FindCells(const cBounds2& b, Vec2i* c0, Vec2i* c1) const; ///< Returns cells spanned by b (specified in grid space)
        Vec2i FindCellClip (Vec2f p) const;                 ///< Finds cell enclosing p, or nearest cell to p if it's outside of the grid.
        void  FindCellsClip(const cBounds2& b, Vec2i* c0, Vec2i* c1) const; ///< Returns cells spanned by b (specified in grid space)
    };

    // --- cGridInfo3 ----------------------------------------------------------

    struct cGridInfo3
    /// Stores info about a 3D grid in world space.
    {
        Vec3i       mN            = vl_0;   ///< Grid dimensions
        int         mTotalCells   = vl_0;   ///< Total cell count
        cBounds3    mGridBounds;            ///< Bounds of grid cells
        cBounds3    mCellBounds;            ///< Bounds of origin cell -- useful for mapping directly to cell coordinates
        Vec3f       mInvCellWidth = vl_0;   ///< For acceleration

        void  SetGridInfo(const cBounds3& bounds, Vec3i numCells);
        ///< Explicitly set grid bounds and dimensions
        void  FitGridInfo(float cellWidth, int n, const Vec3f pos[], size_t stride);
        ///< Fits tracking grid according to the bounds of the given object positions and desired cell size.
        
        bool  CellIsValid(Vec3i ci) const;                   ///< Returns true if given cell is in bounds

        int   NumCells () const;                            ///< Returns size of flat array holding cells
        int   CellIndex(Vec3i c) const;                     ///< Returns flat array index for a valid cell

        Vec3f MapToCellCoords  (Vec3f p) const;             ///< Maps p into cell coordinates
        Vec3f MapFromCellCoords(Vec3f c) const;             ///< Maps c into grid space

        Vec3i FindCell (Vec3f p) const;                     ///< Finds cell enclosing p.
        void  FindCells(const cBounds3& b, Vec3i* c0, Vec3i* c1) const; ///< Returns cells spanned by b (specified in grid space)
        Vec3i FindCellClip (Vec3f p) const;                 ///< Finds cell enclosing p, or nearest cell to p if it's outside of the grid.
        void  FindCellsClip(const cBounds3& b, Vec3i* c0, Vec3i* c1) const; ///< Returns cells spanned by b (specified in grid space)
    };


    // --- Inlines -------------------------------------------------------------

    // Utils
    inline Vec2i FindCell(Vec2f p)
    {
        return Vec2i
        {
            FloorToSInt32(p[0]),
            FloorToSInt32(p[1])
        };
    }

    inline void FindCells(const cBounds2& b, Vec2i* m0Out, Vec2i* m1Out)
    {
        Vec2i& m0 = *m0Out;
        Vec2i& m1 = *m1Out;

        m0[0] = FloorToSInt32(b.mMin[0]);
        m0[1] = FloorToSInt32(b.mMin[1]);

        m1[0] = CeilToSInt32 (b.mMax[0]);
        m1[1] = CeilToSInt32 (b.mMax[1]);
    }

    inline void ClipCell(Vec2i numCells, Vec2i* mOut)
    {
        const int* n = numCells.Ref();
        int*       m = mOut->Ref();

        for (int i = 0; i < 2; i++)
        {
                 if (m[i] < 0)
                     m[i] = 0;
            else if (m[i] >= n[i])  // for a single cell we clip to exclusive bounds
                     m[i] =  n[i] - 1;
        }
    }

    inline void ClipCells(Vec2i numCells, Vec2i* m0Out, Vec2i* m1Out)
    {
        const int* n  = numCells.Ref();
        int*       m0 = m0Out->Ref();
        int*       m1 = m1Out->Ref();

        for (int i = 0; i < 2; i++)
        {
                 if (m0[i] < 0)
                     m0[i] = 0;
            else if (m0[i] > n[i])  // for a range we clip to inclusive bounds
                     m0[i] = n[i];

                 if (m1[i] < 0)
                     m1[i] = 0;
            else if (m1[i] > n[i])
                     m1[i] = n[i];
        }
    }

    inline Vec3i FindCell(Vec3f p)
    {
        return Vec3i
        {
            FloorToSInt32(p[0]),
            FloorToSInt32(p[1]),
            FloorToSInt32(p[2])
        };
    }

    inline void FindCells(const cBounds3& b, Vec3i* m0Out, Vec3i* m1Out)
    {
        Vec3i& m0 = *m0Out;
        Vec3i& m1 = *m1Out;

        m0[0] = FloorToSInt32(b.mMin[0]);
        m0[1] = FloorToSInt32(b.mMin[1]);
        m0[2] = FloorToSInt32(b.mMin[2]);

        m1[0] = CeilToSInt32 (b.mMax[0]);
        m1[1] = CeilToSInt32 (b.mMax[1]);
        m1[2] = CeilToSInt32 (b.mMax[2]);
    }

    inline void ClipCell(Vec3i numCells, Vec3i* mOut)
    {
        const int* n = numCells.Ref();
        int*       m = mOut->Ref();

        for (int i = 0; i < 3; i++)
        {
                 if (m[i] < 0)
                     m[i] = 0;
            else if (m[i] >= n[i])  // for a single cell we clip to exclusive bounds
                     m[i] =  n[i] - 1;
        }
    }

    inline void ClipCells(Vec3i numCells, Vec3i* m0Out, Vec3i* m1Out)
    {
        const int* n  = numCells.Ref();
        int*       m0 = m0Out->Ref();
        int*       m1 = m1Out->Ref();

        for (int i = 0; i < 3; i++)
        {
                 if (m0[i] < 0)
                     m0[i] = 0;
            else if (m0[i] >= n[i])  // for a range we clip to inclusive bounds
                     m0[i] =  n[i] - 1;

                 if (m1[i] < 0)
                     m1[i] = 0;
            else if (m1[i] > n[i])
                     m1[i] = n[i];
        }
    }

    // cGridInfo2
    inline bool cGridInfo2::CellIsValid(Vec2i ci) const
    {
        return (uint(ci[0]) < mN[0])
            && (uint(ci[1]) < mN[1]);
    }

    inline int cGridInfo2::NumCells () const
    {
        return mTotalCells;
    }

    inline int cGridInfo2::CellIndex(Vec2i c) const
    {
        int result = c[0] + c[1] * mN[0];
        CL_INDEX(result, mTotalCells);
        return result;
    }

    inline Vec2f cGridInfo2::MapToCellCoords(Vec2f p) const
    {
        return (p - mCellBounds.mMin) * mInvCellWidth;
    }

    inline Vec2f cGridInfo2::MapFromCellCoords(Vec2f c) const
    {
        return mCellBounds.MapFromLocal(c);
    }

    inline Vec2i cGridInfo2::FindCell(Vec2f p) const
    {
        return nCL::FindCell(MapToCellCoords(p));
    }

    inline Vec2i cGridInfo2::FindCellClip(Vec2f p) const
    {
        Vec2i ci = nCL::FindCell(MapToCellCoords(p));
        ClipCell(mN, &ci);
        return ci;
    }

    // cGridInfo3
    inline bool cGridInfo3::CellIsValid(Vec3i ci) const
    {
        return (uint(ci[0]) < mN[0])
            && (uint(ci[1]) < mN[1])
            && (uint(ci[2]) < mN[2]);
    }

    inline int cGridInfo3::NumCells() const
    {
        return mTotalCells;
    }

    inline int cGridInfo3::CellIndex(Vec3i c) const
    {
        int result = c[0] + c[1] * mN[0] + c[2] * mN[1];
        CL_INDEX(result, mTotalCells);
        return result;
    }

    inline Vec3f cGridInfo3::MapToCellCoords(Vec3f p) const
    {
        return (p - mCellBounds.mMin) * mInvCellWidth;
    }

    inline Vec3f cGridInfo3::MapFromCellCoords(Vec3f c) const
    {
        return mCellBounds.MapFromLocal(c);
    }

    inline Vec3i cGridInfo3::FindCell(Vec3f p) const
    {
        return nCL::FindCell(MapToCellCoords(p));
    }

    inline Vec3i cGridInfo3::FindCellClip(Vec3f p) const
    {
        Vec3i ci = nCL::FindCell(MapToCellCoords(p));
        ClipCell(mN, &ci);
        return ci;
    }
}

#endif
