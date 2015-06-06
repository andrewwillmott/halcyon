#include "VLStream.h"

#include <iomanip>
#include <ctype.h>

using namespace std;

template<class T> ostream &operator << (ostream &s, vector<T>& array)
{	
	s << '[';

	char sepChar;

	if (array.NumItems() >= 16)
		sepChar = '\n';
	else
		sepChar = ' ';
	
	if (array.NumItems() > 0)
	{
		s << array[0];

		for (int i = 1; i < array.NumItems(); i++)
			s << sepChar << array[i];
	}
	
	s << ']';

	return(s);
}

template<class T> istream &operator >> (istream &s, vector<T>& array)
{
    T tmp;
    char c;
	
	//	Expected format: [a b c d ...]
	
    while (isspace(s.peek()))			// 	chomp white space
		s.get(c);
		
    if (s.peek() == '[')						
    {
    	s.get(c);
    	array.clear();
    	
	    while (isspace(s.peek()))		// 	chomp white space
			s.get(c);
    	
		while (s.peek() != ']')
		{
            s >> tmp;

			if (!s)
			{
				CL_WARNING("Couldn't read array component");
				return(s);
			}
	
			array.push_back(tmp);

		    while (isspace(s.peek()))	// 	chomp white space
				s.get(c);
		}			
		s.get(c);
	}
    else
	{
	    s.clear(ios::failbit);
	    CL_WARNING("Error: Expected '[' while reading array");
	    return(s);
	}
	
    return(s);
}


// Vec234

ostream &operator << (ostream &s, const TVec2 &v)
{
    int w = s.width();

    return(s << '[' << v[0] << ' ' << setw(w) << v[1] << ']');
}

istream &operator >> (istream &s, TVec2 &v)
{
    TVec2   result;
    char    c;
    
    // Expected format: [1 2]
    
    while (s >> c && isspace(c))        
        ;
        
    if (c == '[')                       
    {
        s >> result[0] >> result[1];    

        if (!s)
        {
            cerr << "Error: Expected number while reading vector\n";
            return(s);
        }
            
        while (s >> c && isspace(c))
            ;
            
        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Error: Expected ']' while reading vector\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Error: Expected '[' while reading vector\n";
        return(s);
    }
    
    v = result;
    return(s);
}


ostream &operator << (ostream &s, const TVec3 &v)
{
    int w = s.width();

    return(s << '[' << v[0] << ' ' << setw(w) << v[1] << ' ' << setw(w) << v[2] << ']');
}

istream &operator >> (istream &s, TVec3 &v)
{
    TVec3   result;
    char    c;
    
    // Expected format: [1 2 3]
    
    while (s >> c && isspace(c))        
        ;
        
    if (c == '[')                       
    {
        s >> result[0] >> result[1] >> result[2];   

        if (!s)
        {
            cerr << "Error: Expected number while reading vector\n";
            return(s);
        }
            
        while (s >> c && isspace(c))
            ;
            
        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Error: Expected ']' while reading vector\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Error: Expected '[' while reading vector\n";
        return(s);
    }
    
    v = result;
    return(s);
}


ostream &operator << (ostream &s, const TVec4 &v)
{
    int w = s.width();
        
    return(s << '[' << v[0] << ' ' << setw(w) << v[1] << ' '
        << setw(w) << v[2] << ' ' << setw(w) << v[3] << ']');
}

istream &operator >> (istream &s, TVec4 &v)
{
    TVec4   result;
    char    c;
    
    // Expected format: [1 2 3 4]
    
    while (s >> c && isspace(c))        
        ;
        
    if (c == '[')                       
    {
        s >> result[0] >> result[1] >> result[2] >> result[3];  

        if (!s)
        {
            cerr << "Error: Expected number while reading vector\n";
            return(s);
        }
            
        while (s >> c && isspace(c))
            ;
            
        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Error: Expected ']' while reading vector\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Error: Expected '[' while reading vector\n";
        return(s);
    }
    
    v = result;
    return(s);
}


// Mat234

ostream &operator << (ostream &s, const TMat2 &m)
{
    int w = s.width();

    return(s << '[' << m[0] << endl << setw(w) << m[1] << ']' << endl);
}

istream &operator >> (istream &s, TMat2 &m)
{
    TMat2   result;
    char    c;
    
    // Expected format: [[1 2] [3 4]]
    // Each vector is a row of the row matrix.
    
    while (s >> c && isspace(c))        // ignore leading white space
        ;
        
    if (c == '[')           
    {
        s >> result[0] >> result[1];

        if (!s)
        {
            cerr << "Expected number while reading matrix\n";
            return(s);
        }
            
        while (s >> c && isspace(c))
            ;
            
        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Expected ']' while reading matrix\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Expected '[' while reading matrix\n";
        return(s);
    }
    
    m = result;
    return(s);
}


ostream &operator << (ostream &s, const TMat3 &m)
{
    int w = s.width();

    return(s << '[' << m[0] << endl << setw(w) << m[1] << endl << setw(w) 
           << m[2] << ']' << endl);
}

istream &operator >> (istream &s, TMat3 &m)
{
    TMat3   result;
    char    c;
    
    // Expected format: [[1 2 3] [4 5 6] [7 8 9]]
    // Each vector is a column of the matrix.
    
    while (s >> c && isspace(c))        // ignore leading white space
        ;
        
    if (c == '[')           
    {
        s >> result[0] >> result[1] >> result[2];

        if (!s)
        {
            cerr << "Expected number while reading matrix\n";
            return(s);
        }
            
        while (s >> c && isspace(c))
            ;
            
        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Expected ']' while reading matrix\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Expected '[' while reading matrix\n";
        return(s);
    }
    
    m = result;
    return(s);
}


ostream &operator << (ostream &s, const TMat4 &m)
{
    int w = s.width();

    return(s << '[' << m[0] << endl << setw(w) << m[1] << endl
         << setw(w) << m[2] << endl << setw(w) << m[3] << ']' << endl);
}

istream &operator >> (istream &s, TMat4 &m)
{
    TMat4   result;
    char    c;
    
    // Expected format: [[1 2 3] [4 5 6] [7 8 9]]
    // Each vector is a column of the matrix.
    
    while (s >> c && isspace(c))        // ignore leading white space
        ;
        
    if (c == '[')           
    {
        s >> result[0] >> result[1] >> result[2] >> result[3];

        if (!s)
        {
            cerr << "Expected number while reading matrix\n";
            return(s);
        }
            
        while (s >> c && isspace(c))
            ;
            
        if (c != ']')
        {
            s.clear(ios::failbit);
            cerr << "Expected ']' while reading matrix\n";
            return(s);
        }
    }
    else
    {
        s.clear(ios::failbit);
        cerr << "Expected '[' while reading matrix\n";
        return(s);
    }
    
    m = result;
    return(s);
}



// VecN

ostream &operator << (ostream &s, const TVec &v)
{
    s << '[';
    
    if (v.Elts() > 0)
    {
        int w = s.width();
        s << v[0];
    
        for (int i = 1; i < v.Elts(); i++)
            s << ' ' << setw(w) << v[i];
    }
    
    s << ']';
    
    return(s);
}

istream &operator >> (istream &s, TVec &v)
{
    vector<TVReal> array;
    
    // Expected format: [1 2 3 4 ...]

    s >> array;                              // Read input into variable-sized array

    v = TVec(array.size(), &array.front());  // Copy input into vector
    
    return(s);
}


ostream &operator << (ostream &s, const TSubVec &v)
{
    s << '[';
    
    if (v.Elts() > 0)
    {
        int w = s.width();
        s << v[0];
    
        for (int i = 1; i < v.Elts(); i++)
            s << ' ' << setw(w) << v[i];
    }
    
    s << ']';
    
    return(s);
}

istream &operator >> (istream &s, TSubVec &v)
{
    vector<TVReal> array;
    
    // Expected format: [1 2 3 4 ...]

    s >> array;                              // Read input into variable-sized array

    v = TVec(array.size(), &array.front());  // Copy input into vector
    
    return(s);
}



ostream &operator << (ostream &s, const TSparsePair &sp)
{
    s << sp.index << ':' << sp.elt;

    return(s);
}

ostream &operator << (ostream &s, const TSparseVec &v)
{
    if (TSparseVec::IsCompact())
    {
        int i;

        s << '[';

        for (i = 0; i < v.pairs.NumItems() - 2; i++)
            s << v.pairs[i] << ' ';

        s << v.pairs[i] << ']';
    }
    else
    {
        int i, j;

        s << '[';
        
        for (i = 0, j = 0; i < v.Elts() - 1; i++)
            if (i < v.pairs[j].index)
                s << "0 ";
            else
                s << v.pairs[j++].elt << ' ';

        if (i < v.pairs[j].index)
            s << "0]";
        else
            s << v.pairs[j].elt << ']';
    }

    return(s);
}

istream &operator >> (istream &s, TSparseVec &v)
{
    char    c;
    int     i = 0;
    TVReal  elt;

    //  Expected format: [a b c d ...]
    
    while (isspace(s.peek()))           //  chomp white space
        s.get(c);
        
    if (s.peek() == '[')                        
    {
        v.Begin();

        s.get(c);
        
        while (isspace(s.peek()))       //  chomp white space
            s.get(c);
        
        while (s.peek() != ']')
        {           
            s >> elt;
            
            if (!s)
            {
                CL_WARNING("Couldn't read array component");
                return(s);
            }
            else
            {
                v.AddElt(i, elt);
                i++;
            }
                
            while (isspace(s.peek()))   //  chomp white space
                s.get(c);
        }
        s.get(c);
    }
    else
    {
        s.clear(ios::failbit);
        CL_WARNING("Error: Expected '[' while reading array");
        return(s);
    }
    
    v.End();
    v.SetNumElts(i);
    
    return(s);
}

ostream &operator << (ostream &s, const TSubSVec &v)
{
    return s << TSparseVec(v);
}

istream &operator >> (istream &s, TSubSVec &v)
{
    TSparseVec vecRef(v);
    return s >> vecRef;
}


// MatN

ostream &operator << (ostream &s, const TMat &m)
{
    int i, w = s.width();
    
    s << '[';
    for (i = 0; i < m.Rows() - 1; i++)
        s << setw(w) << m[i] << endl;
    s << setw(w) << m[i] << ']' << endl;
    return(s);
}

istream &operator >> (istream &s, TMat &m)
{
    vector< vector<TMReal> > array; 
    int     i;
    
    s >> array;                     // Read input into array of arrays
    
    m.SetSize(array.size(), array[0].size());
    
    for (i = 0; i < m.Rows(); i++)  // copy the result into m
    {
        CL_ASSERT_MSG(m.Cols() == array[i].size(), "(Mat/>>) different sized matrix rows");
        m[i] = TMVec(m.Cols(), &array[i].front());
    }
    
    return(s);
}


ostream &operator << (ostream &s, const TSubMat &m)
{
    int i, w = s.width();
    
    s << '[';
    for (i = 0; i < m.Rows() - 1; i++)
        s << setw(w) << m[i] << endl;
    s << setw(w) << m[i] << ']' << endl;
    return(s);
}

istream &operator >> (istream &s, TSubMat &m)
{
    vector< vector<TMReal> > array; 
    
    s >> array;                     // Read input into array of arrays
    
    CL_ASSERT_MSG(m.Rows() == array.size(), "(SubMat:>>) different sized matrix");
    
    for (int i = 0; i < m.Rows(); i++)  // copy the result into m
    {
        CL_ASSERT_MSG(m.Cols() == array[i].size(), "(SubMat:>>) different sized matrix rows");
        m[i] = TMVec(m.Cols(), &array[i].front());
    }
    
    return(s);
}


ostream &operator << (ostream &s, const TSparseMat &m)
{
    int i, w = s.width();

    s << '[';
    for (i = 0; i < m.Rows() - 1; i++)
        s << setw(w) << m[i] << endl;
    s << setw(w) << m[i] << ']' << endl;
    
    return(s);
}

istream &operator >> (istream &s, TSparseMat &m)
{
    vector< TMSparseVec > array;
    int     i;
    
    s >> array;                     // Read input into array of SparseVecs
    
    m.SetSize(array.size(), array[0].Elts());
    
    for (i = 0; i < m.Rows(); i++)  // copy the result into m
    {
        CL_ASSERT_MSG(m.Cols() == array[i].pairs.NumItems(), "(Mat/>>) different sized matrix rows");
        m[i] = array[i];
    }
    
    return(s);
}

ostream &operator << (ostream &s, const TSubSMat &m)
{
    return s << TSparseMat(m);
}

istream &operator >> (istream &s, TSubSMat &m)
{
    TSparseMat matRef(m);
    return s >> matRef;
}
