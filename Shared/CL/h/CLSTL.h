//
//  File:       CLSTL.h
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#ifndef CL_STL_H
#define CL_STL_H

#include <CLDefs.h>

#ifdef CL_USE_STD

    #include <algorithm>
    #include <vector>
    #include <string>
    #include <map>
    #include <multimap>
    #include <set>
    #include <multiset>
    #include <bitset>

    namespace nCL
    {
        using std::vector;
        using std::string;
        using std::map;
        using std::set;
        using std::bitset;
    }

#else

    #include <ustl/algorithm.h>
    #include <ustl/vector.h>
    #include <ustl/string.h>
    #include <ustl/map.h>
    #include <ustl/multimap.h>
    #include <ustl/set.h>
    #include <ustl/multiset.h>
    #include <ustl/bitset.h>

    namespace nCL
    {
        using namespace ustl;

        // TEMPORARY -- these will become local-store specialised versions
        typedef ustl::string local_string;
        template<class T_V, class T_A> using local_vector = ustl::vector<T_V>;
        template<class T_K, class T_V, class T_C> using local_map = ustl::map<T_K, T_V, T_C>;
    }
#endif

#endif
