/*
    File:           VLVec.h

    Function:       Defines a generic resizeable vector.
                    
    Author(s):      Andrew Willmott

    Copyright:      (c) 1995-2000, Andrew Willmott
 */

#ifndef VL_Vec_H
#define VL_Vec_H

#include "VL.h"
#include "VLSubVec.h"


// --- Vec Class --------------------------------------------------------------


class TVec
{
public:
    // Constructors
    TVec();                         // Null vector: space allocated later
    TVec(int n);                                     
    TVec(int n, double elt0, ...);  // Vec(3, 1.1, 2.0, 3.4)
    TVec(int n, TVReal* data);      // Vector reference
    TVec(const TVec& v);            // Copy constructor
    TVec(int n, ZeroOrOne);         // Zero or all-ones vector  
    TVec(int n, Axis a);            // Unit vector
    explicit TVec(const TSubVec& v);
#ifdef VL_Vec2_H
    TVec(const TVec2& v);               
    TVec(const TVec3& v);               
    TVec(const TVec4& v);               
#endif
    
   ~TVec();
    
    // Accessor functions
    int           Elts() const;

    TVReal&       operator [] (int i);  
    const TVReal& operator [] (int i) const;            

    TVReal*       Ref() const;                   // Return pointer to data
    void          SetSize(int n);                // resize the vector
    
    // Assignment operators
    TVec&   operator =  (const TVec& v);   // v = a etc.
    TVec&   operator =  (const TSubVec& v);        
    TVec&   operator =  (ZeroOrOne k);
    TVec&   operator =  (Axis a);
#ifdef VL_Vec2_H
    TVec&   operator =  (const TVec2& v);          
    TVec&   operator =  (const TVec3& v);          
    TVec&   operator =  (const TVec4& v);              
#endif
        
    // In-Place operators
    TVec&   operator += (const TVec& v);
    TVec&   operator -= (const TVec& v);
    TVec&   operator *= (const TVec& v);
    TVec&   operator *= (TVReal s);
    TVec&   operator /= (const TVec& v);
    TVec&   operator /= (TVReal s);

    //  Vector initialisers
    TVec&   MakeZero();
    TVec&   MakeUnit(int i, TVReal k = vl_one);
    TVec&   MakeBlock(TVReal k = vl_one);

    void    MakeNull();             ///< Make this a zero-sized vector
    void    MakeRef(const TVec& v); ///< Make this a reference of 'v'

    bool    IsNull() const;         ///< Returns true if this is a null-sized vector.
    bool    IsRef() const;          ///< Returns true if this is a reference to storage elsewhere

    TVec&   Normalize();            ///< normalize vector in-place

    TVec&   Clamp(Real fuzz);       ///< Clamp all entries < fuzz to 0.
    TVec&   Clamp();

protected:
    TVReal* mData;
    int     mElts;
};


// --- Reference Vec Class -----------------------------------------------------

class TRefVec : public TVec
{
public:
    TRefVec(const TVec& v);
    TRefVec(int elts, TVReal* data);

#ifdef VL_Vec2_H
    TRefVec(const TVec2& v);
    TRefVec(const TVec3& v);
    TRefVec(const TVec4& v);
#endif

    // Assignment operators
    TVec&   operator = (const TVec& v)     { return TVec::operator=(v); }
    TVec&   operator = (const TSubVec& v)  { return TVec::operator=(v); }
    TVec&   operator = (ZeroOrOne k)       { return TVec::operator=(k); }
};

// --- Vec Comparison Operators -----------------------------------------------

bool    operator == (const TVec& a, const TVec& b);
bool    operator != (const TVec& a, const TVec& b);

// --- Vec Arithmetic Operators -----------------------------------------------

TVec    operator + (const TVec& a, const TVec& b);
TVec    operator - (const TVec& a, const TVec& b);
TVec    operator - (const TVec& v);
TVec    operator * (const TVec& a, const TVec& b);      
TVec    operator * (const TVec& v, TVReal s);
TVec    operator / (const TVec& a, const TVec& b);
TVec    operator / (const TVec& v, TVReal s);
TVec    operator * (TVReal s, const TVec& v);

TVReal  dot(const TVec& a, const TVec& b);  // v . a
TVReal  len(const TVec& v);                 // || v ||
TVReal  sqrlen(const TVec& v);              // v . v
TVec    norm(const TVec& v);                // v / || v ||
TVec    norm_safe(const TVec& v);           // v / || v ||, handles || v || = 0
void    normalize(TVec& v);                 // v = norm(v)
TVec    clamped(const TVec& v, Real fuzz);
TVec    clamped(const TVec& v);


// --- Sub-vector functions ---------------------------------------------------

TVec sub  (const TVec& v, int start, int length);  
TVec first(const TVec& v, int length);   
TVec last (const TVec& v, int length);    


// --- Vec Functions ----------------------------------------------------------

void Add     (const TVec& a, const TVec& b, TVec& result);
void Subtract(const TVec& a, const TVec& b, TVec& result);
void Negate  (const TVec& v, TVec& result);
void Multiply(const TVec& a, const TVec& b, TVec& result);
void Multiply(const TVec& v, TVReal s, TVec& result);
void MultiplyAccum(const TVec& v, TVReal s, TVec& result);
void Divide  (const TVec& a, const TVec& b, TVec& result);
void Divide  (const TVec& v, TVReal s, TVec& result);

void Normalize(const TVec& v, TVec& result);
void NormalizeSafe(const TVec& v, TVec& result);


// --- Vec inlines ------------------------------------------------------------


inline TVec::TVec() : mData(0), mElts(0)
{
}

inline TVec::TVec(int elts) : mData(), mElts(elts)
{
    CL_ASSERT_MSG(elts > 0,"(Vec) illegal vector size");

    mData = VL_NEW TVReal[elts];
}

inline TVec::TVec(int elts, TVReal* data) : mData(data), mElts(elts | VL_REF_FLAG)
{
}

inline TVec::TVec(int n, ZeroOrOne k) : mData(0), mElts(n)
{
    CL_ASSERT_MSG(n > 0,"(Vec) illegal vector size");

    mData = VL_NEW TVReal[n];
    
    MakeBlock(TVReal(k));
}

inline TVec::TVec(int n, Axis a) : mData(VL_NEW TVReal[n]), mElts(n)
{
    CL_ASSERT_MSG(n > 0,"(Vec) illegal vector size");

    MakeUnit(a);
}

#ifdef VL_Vec2_H
inline TVec::TVec(const TVec2& v) : mData(v.Ref()), mElts(v.Elts() | VL_REF_FLAG)
{
}

inline TVec::TVec(const TVec3& v) : mData(v.Ref()), mElts(v.Elts() | VL_REF_FLAG)
{
}

inline TVec::TVec(const TVec4& v) : mData(v.Ref()), mElts(v.Elts() | VL_REF_FLAG)
{
}
#endif

inline int TVec::Elts() const
{
    return mElts & VL_REF_MASK;
}

inline TVReal& TVec::operator [] (int i)
{
    CL_RANGE_MSG(i, 0, Elts(), "Vec::[i]");
    
    return mData[i];
}

inline const TVReal& TVec::operator [] (int i) const
{
    CL_RANGE_MSG(i, 0, Elts(), "Vec::[i]");

    return mData[i];
}

inline TVReal* TVec::Ref() const
{
    return mData;
}

inline TVec& TVec::operator = (ZeroOrOne k)
{
    MakeBlock(TVReal(k));

    return(*this);
}

inline TVec& TVec::operator = (Axis a)
{
    MakeUnit(a);

    return SELF;
}

inline void TVec::MakeNull()
{
    if (!IsRef())
        delete[] mData;

    mData = 0;
    mElts &= VL_REF_FLAG;
}

inline void TVec::MakeRef(const TVec& v)
{
    if (!IsRef())
        delete[] mData;

    mData = v.Ref();
    mElts = v.Elts() | VL_REF_FLAG;
}

inline bool TVec::IsNull() const
{
    return (mElts & VL_REF_MASK) == 0;
}

inline bool TVec::IsRef() const
{
    return (mElts & VL_REF_FLAG) != 0;
}


inline TVec& TVec::Normalize()
{
    CL_ASSERT_MSG(TVReal(vl_0) < sqrlen(*this), "normalising length-zero vector");
    *this /= len(*this);
    return(*this);
}

// TRefVec
inline TRefVec::TRefVec(const TVec& v) : TVec(v.Elts(), v.Ref())
{
}

inline TRefVec::TRefVec(int elts, TVReal* data) : TVec(elts, data)
{
}

#ifdef VL_Vec2_H
inline TRefVec::TRefVec(const TVec2& v) : TVec(v.Elts(), v.Ref())
{
}

inline TRefVec::TRefVec(const TVec3& v) : TVec(v.Elts(), v.Ref())
{
}

inline TRefVec::TRefVec(const TVec4& v) : TVec(v.Elts(), v.Ref())
{
}
#endif

// Utilities
inline TVReal len(const TVec& v)
{
    return(sqrt(dot(v, v)));
}

inline TVReal sqrlen(const TVec& v)
{
    return(dot(v, v));
}

inline TVec norm(const TVec& v) 
{
    CL_ASSERT_MSG(TVReal(vl_0) < sqrlen(v), "normalising length-zero vector");
    return(v / len(v));
}

inline TVec norm_safe(const TVec& v) 
{
    return(v / (len(v) + TVReal(1e-8)));
}

inline void normalize(TVec& v)
{
    v /= len(v);
}

inline TVec sub(const TVec& v, int start, int length)
{
    CL_ASSERT_MSG(start >= 0 && length > 0 && start + length <= v.Elts(),
        "(sub(Vec)) illegal subset of vector");

    return(TVec(length, v.Ref() + start));
}

inline TVec first(const TVec& v, int length)
{
    CL_ASSERT_MSG(length > 0 && length <= v.Elts(),
        "(first(Vec)) illegal subset of vector");

    return(TVec(length, v.Ref()));
}

inline TVec last(const TVec& v, int length)
{
    CL_ASSERT_MSG(length > 0 && length <= v.Elts(),
        "(last(Vec)) illegal subset of vector");

    return(TVec(length, v.Ref() + v.Elts() - length));
}


#endif

