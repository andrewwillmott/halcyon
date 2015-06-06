//
//  File:       CLGrid.cpp
//
//  Function:   Utils for dealing with 2D or 3D grids.
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <CLGrid.h>

using namespace nCL;

namespace
{

}

// Utilities

// finds cell corner and weights for bilinear mat lookup
void nCL::FindQuad(Vec2i numCells, Vec2f p, Vec2i* ciOut, float w[4])
{
    Vec2i& ci = *ciOut;

    ci = FindCell(p);
    ClipCell(numCells - Vec2i(1), &ci);

    float sx = ClampUnit(p[0] - ci[0]);
    float sy = ClampUnit(p[1] - ci[1]);

    float tx = 1.0f - sx;
    float ty = 1.0f - sy;

    w[0] = tx * ty;
    w[1] = sx * ty;
    w[2] = tx * sy;
    w[3] = sx * sy;
}

// finds cell corner and weights for trilinear volume lookup
void nCL::FindOct(Vec3i numCells, Vec3f p, Vec3i* ciOut, float w[8])
{
    Vec3i& ci = *ciOut;

    ci = FindCell(p);
    ClipCell(numCells - Vec3i(1), &ci);

    float sx = ClampUnit(p[0] - ci[0]);
    float sy = ClampUnit(p[1] - ci[1]);
    float sz = ClampUnit(p[2] - ci[2]);

    float tx = 1.0f - sx;
    float ty = 1.0f - sy;
    float tz = 1.0f - sz;

    w[0] = tx * ty * tz;
    w[1] = sx * ty * tz;
    w[2] = tx * sy * tz;
    w[3] = sx * sy * tz;
    w[4] = tx * ty * sz;
    w[5] = sx * ty * sz;
    w[6] = tx * sy * sz;
    w[7] = sx * sy * sz;
}



// --- cGridInfo2 --------------------------------------------------------------

void cGridInfo2::SetGridInfo(const cBounds2& bounds, Vec2i numCells)
{
    mGridBounds = bounds;

    mN = numCells;
    mTotalCells = mN[0] * mN[1];

    mCellBounds.mMin = mGridBounds.mMin;
    mCellBounds.mMax = mGridBounds.mMin + mGridBounds.Width() / Vec2f(mN[0], mN[1]);
    mInvCellWidth = inv(mCellBounds.Width());
}

void cGridInfo2::FitGridInfo(float cellWidth, int n, const Vec2f pos[], int stride)
{
    // Fills the various grid/cell data structures from the given particle list.
    uint8_t* posData = (uint8_t*) pos;
    if (!stride)
        stride = sizeof(*pos);

    mGridBounds.MakeEmpty();

    for (int i = 0; i < n; i++)
    {
        mGridBounds.Add(*pos);
        (uint8_t*&) pos += stride;
    }

    Vec2f cellCounts = mGridBounds.Width() / cellWidth;
    mN[0] = FloorToSInt32(cellCounts[0]) + 1;
    mN[1] = FloorToSInt32(cellCounts[1]) + 1;
    mTotalCells = mN[0] * mN[1];

    // re-adjust max for cells that spill outside tight bounds
    mGridBounds.mMax = mGridBounds.mMin + cellWidth * Vec2f(mN[0], mN[1]);

    mCellBounds.mMin = mGridBounds.mMin;
    mCellBounds.mMax = mGridBounds.mMin + Vec2f(cellWidth);
    mInvCellWidth = inv(mCellBounds.Width());
}

void cGridInfo2::FindCells(const cBounds2& b, Vec2i* c0, Vec2i* c1) const
{
    cBounds2 cb = { MapToCellCoords(b.mMin), MapToCellCoords(b.mMax) };
    nCL::FindCells(cb, c0, c1);
}

void cGridInfo2::FindCellsClip(const cBounds2& b, Vec2i* c0, Vec2i* c1) const
{
    cBounds2 cb = { MapToCellCoords(b.mMin), MapToCellCoords(b.mMax) };
    nCL::FindCells(cb, c0, c1);
    ClipCells(mN, c0, c1);
}




// --- cGridInfo3 --------------------------------------------------------------

void cGridInfo3::SetGridInfo(const cBounds3& bbox, Vec3i numCells)
{
    mGridBounds = bbox;

    mN = numCells;
    mTotalCells = mN[0] * mN[1] * mN[2];

    mCellBounds.mMin = mGridBounds.mMin;
    mCellBounds.mMax = mGridBounds.mMin + mGridBounds.Width() / Vec3f(mN[0], mN[1], mN[2]);
    mInvCellWidth = inv(mCellBounds.Width());
}

void cGridInfo3::FitGridInfo(float cellWidth, int n, const Vec3f pos[], size_t stride)
{
    // Fills the various grid/cell data structures from the given particle list.
    if (!stride)
        stride = sizeof(*pos);

    mGridBounds.MakeEmpty();

    for (int i = 0; i < n; i++)
    {
        mGridBounds.Add(*pos);
        (uint8_t*&) pos += stride;
    }

    Vec3f cellCounts = mGridBounds.Width() / cellWidth;
    mN[0] = FloorToSInt32(cellCounts[0]) + 1;
    mN[1] = FloorToSInt32(cellCounts[1]) + 1;
    mN[2] = FloorToSInt32(cellCounts[2]) + 1;
    mTotalCells = mN[0] * mN[1] * mN[2];

    // re-adjust max for cells that spill outside tight bounds
    mGridBounds.mMax = mGridBounds.mMin + cellWidth * Vec3f(mN[0], mN[1], mN[2]);

    mCellBounds.mMin = mGridBounds.mMin;
    mCellBounds.mMax = mGridBounds.mMin + Vec3f(cellWidth);
    mInvCellWidth = inv(mCellBounds.Width());
}

void cGridInfo3::FindCells(const cBounds3& b, Vec3i* c0, Vec3i* c1) const
{
    cBounds3 cb = { MapToCellCoords(b.mMin), MapToCellCoords(b.mMax) };
    ::FindCells(cb, c0, c1);
}

void cGridInfo3::FindCellsClip(const cBounds3& b, Vec3i* c0, Vec3i* c1) const
{
    cBounds3 cb = { MapToCellCoords(b.mMin), MapToCellCoords(b.mMax) };
    ::FindCells(cb, c0, c1);
    ClipCells(mN, c0, c1);
}
