/*
    File:           VLStream.h

    Function:       iostream support
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 2005, Andrew Willmott
 */

#ifndef VL_Stream_H
#define VL_Stream_H

#include <iostream>

class TVec2;
class TVec3;
class TVec4;
class TMat2;
class TMat3;
class TMat4;

std::ostream &operator << (std::ostream &s, const TVec2 &v);
std::istream &operator >> (std::istream &s, TVec2 &v);
std::ostream &operator << (std::ostream &s, const TVec3 &v);
std::istream &operator >> (std::istream &s, TVec3 &v);
std::ostream &operator << (std::ostream &s, const TVec4 &v);
std::istream &operator >> (std::istream &s, TVec4 &v);
    
std::ostream &operator << (std::ostream &s, const TMat2 &m);
std::istream &operator >> (std::istream &s, TMat2 &m);
std::ostream &operator << (std::ostream &s, const TMat3 &m);
std::istream &operator >> (std::istream &s, TMat3 &m);
std::ostream &operator << (std::ostream &s, const TMat4 &m);
std::istream &operator >> (std::istream &s, TMat4 &m);


class TVec;
class TSubVec;
class TSparseVec;
class TSubSVec;
class TMat;
class TSubMat;
class TSparseMat;
class TSubSMat;

std::ostream     &operator << (std::ostream &s, const TVec &v);
std::istream     &operator >> (std::istream &s, TVec &v);
std::ostream     &operator << (std::ostream &s, const TSubVec &v);
std::istream     &operator >> (std::istream &s, TSubVec &v);
std::ostream     &operator << (std::ostream &s, const TSparseVec &v);
std::istream     &operator >> (std::istream &s, TSparseVec &v);
std::ostream     &operator << (std::ostream &s, const TSubSVec &v);
std::istream     &operator >> (std::istream &s, TSubSVec &v);

std::ostream     &operator << (std::ostream &s, const TMat &m);
std::istream     &operator >> (std::istream &s, TMat &m);
std::ostream     &operator << (std::ostream &s, const TSubMat &m);
std::istream     &operator >> (std::istream &s, TSubMat &m);
std::ostream     &operator << (std::ostream &s, const TSparseMat &m);
std::istream     &operator >> (std::istream &s, TSparseMat &m);
std::ostream     &operator << (std::ostream &s, const TSubSMat &m);
std::istream     &operator >> (std::istream &s, TSubSMat &m);

#endif
