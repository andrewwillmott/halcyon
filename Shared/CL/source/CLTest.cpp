/*
	File:		CLTest.cpp

	Function:	Test code for CL lib. This is ancient and has bitrotted

	Author:		Andrew Willmott

	Notes:		
*/


#include <string.h>
#include <stdlib.h>

#include <CLFileSpec.h>
#include <CLHeap.h>

#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace nCL;


void MyPrint(tStrConst bob)
{
    cout << "print(StrConst): " << bob << endl;
}

void TestString()
{
    tString         string;
    tStrConst       strconst = "strconst";
    int             i;
    tStrConstArray  a;
    
    string = "string";
    
    cout << "\nString Test" << endl;
    cout << "-----------\n" << endl;

    cout << "sprintf test: " 
         << tString().Printf("number %d, strings '%s' '%s' '%s'",
            20, "cstring", strconst.c_str(), string.c_str()) << endl;

    cout << "StrConst + CStr: " << strconst + "cstr" << endl;
    cout << "String + CStr: " << string + "cstr" << endl;
    cout << "CStr + String: " << ("cstr" + string) << endl;
    cout << "String + StrConst + CStr: " << string + strconst + "cstr" << endl;

    MyPrint(string);
    MyPrint("blah-" + string + "-blah");

    cout << "iso for string/strconst == ";
    if (string == string && strconst == strconst)
        cout << "good" << endl;
    else
        cout << "bad" << endl;

    cout << "string/cstr == ";
    if (string == "string" && "string" == string)
        cout << "good" << endl;
    else
        cout << "bad" << endl;

    cout << "strconst/cstr == ";
    if (strconst == "strconst" && "strconst" == strconst)
        cout << "good" << endl;
    else
        cout << "bad" << endl;

    string = strconst;
    cout << "string/strconst == ";
    if (strconst == string && string == strconst)
        cout << "good" << endl;
    else
        cout << "bad" << endl;

    string = "abcdefgh";

    cout << "Suffix/Prefix test: " << string.Suffix(2) << ':' << string.Suffix(-2) << ':'
         << string.Prefix(2) << ':' << string.Prefix(-2) << '<' << endl;

    cout << "Suffix/Prefix bounds test: " << string.Suffix(10) << ':' << string.Suffix(-10) << ':'
         << string.Prefix(10) << ':' << string.Prefix(-10) << '<' << endl;

    cout << "SubString test: " << string.SubString(2, 4) << ':' << string.SubString(-6, 4) << '<' << endl;

    cout << "SubString bounds test: " << string.SubString(10, 4) << ':' << string.SubString(-10, 4)
         << ':' << string.SubString(2, 10) << '<' << endl;

    cout << "Find char test: " << string.FindChar('c')
         << ", " << string.FindCharLast('x')
         << ", " << strconst.FindChar('s')
         << ", " << strconst.FindCharLast('s') << endl;

    cout << "Find tokens test: " << endl;
    string = "   there is a disease   that is tearing apart the  lives of thousands";
    cout << "string = " << string << endl;
    Split(string, a);
    for (i = 0; i < a.size(); i++)
        cout << "token " << i << " = '" << a[i] << "'" << endl;
}

static void TestLoad(const cFileSpec &fname)
{
    cout << "exists: " << fname.Path() << ": " << fname.IsReadable() << endl;
}

cFileSpecExtension kExtensions[] =
{
    "a", 0,
    "b", 1,
    0, 0
};

void TestFileSpec()
{
    cFileSpec    t;

    cout << "\nFileSpec Test" << endl;
    cout << "-------------\n" << endl;

    t.SetPath("/one/two/three/and/wibble.sl.gz");   

    cout << "the path is " << t.Path() << endl;
    cout << "parent: "     << t.Directory() << endl;
    cout << "name: "       << t.File() << endl;
    cout << "extensions: " << t.Extension() << endl;
    t.RemoveExtension();
    cout << "removed extension" << endl;
    cout << "new name: " << t.File() << endl;
    cout << "new ext: "  << t.Extension() << endl;

    TestLoad(t);
    TestLoad(cFileSpec().SetPath("Makefile"));
    TestLoad(cFileSpec().SetPath("Makefile.depend"));

    cFileSpec fname;

    int x;

    x = fname.SetPath("a").FindFileExtension(kExtensions);
    cout << tString().Printf("a: file %s %d [%d]\n", fname.Path().c_str(),
        uint32_t(fname.flags), x);

    x = fname.SetPath("a.b").FindFileExtension(kExt);
    cout << String().Printf("a.b.gz: file %s %d [%d]\n", fname.Path().c_str(),
        uint32_t(fname.flags), x);

    x = fname.SetPath("a.e").FindFileExtension(kExt);
    cout << String().Printf("a.e: file %s %d [%d]\n", fname.Path().c_str(),
        uint32_t(fname.flags), x);

    x = fname.SetPath("b").FindFileExtension(kExt);

    cout << String().Printf
    (
        "b: file %s %d [%d]\n", 
        fname.GetPath().c_str(), 
        uint32_t(fname.flags), 
        x
    );
}

void TestEnviron()
{
    cout << "\nEnviron Test" << endl;
    cout << "------------\n" << endl;

    cout << SubstituteEnvVars("shell = $SHELL, path = $HOME/bob") << endl;
}

#ifdef BROKEN
class Printer : public IntHashIter
{
public:
    void ProcessItem(StrConst s, int a)
    { cout << s << " -> " << a << endl; };
};

void TestHash()
{
    cout << "\nHash Test" << endl;
    cout << "---------\n" << endl;

    hash_map<const char*, int> hash;
    Printer     printer;
    
    hash["bob"]   = 20;
    hash["mary"]  = 33;
    hash["peter"] = 21;
    hash["jane"]  = 10;

    hash["mary"] += 1;
    
    hash.Iterate(printer);

    cout << "bob exists: " << hash.ItemExists("bob") << endl;
    cout << "robert exists: " << hash.ItemExists("robert") << endl;
}
#endif

#define NEW_CL

#if 0
void TestHeap()
{
#ifdef NEW_CL
    priority_queue

    vector<float> heap;
    int           i, n = 100;
    
    cout << "\nHeap Test" << endl;
    cout << "-----------\n" << endl;

    srand(20);
    
    for (i = 0; i < n; i++)
    {
        heap.push_back(rand());
        push_heap(heap);
    }

    // update random entries
    for (i = 0; i < n / 5; i++)
    {
        int idx = rand() % heap.NumItems();
        cout << "updating " << idx << endl;

        heap[idx] = cl_rand();
        if (idx > 0 && ((idx - 1) / 2)
        {
            // heapify up
            heap_push(heap.begin(), heap.begin() + idx);
            Assert(isheap(heap.begin(), heap.end()), "bad heap");
        }
        else
        {
            // heapify down
            // grrr... dance around STL's limitations
            // we rely here on them doing the
            // standard *first = *last, heapify_down(...) thing.
            heap.push_back(heap[idx]);
            heap_pop(heap.begin(), heap.end());
        }
        
        Assert(isheap(heap.begin(), heap.end()), "bad heap");
    }

    // delete random entries
    for (i = 0; i < n / 5; i++)
    {
        he = heap.heap[rand() % heap.NumItems()];
        cout << "removing " << he->heapIdx << endl;
        heap.Delete(he);
    }
    
//  for (i = 0; i < heap.NumItems(); i++)
//      if (heap.heap[i]->heapIdx != i)
//          cout << "bad heapIdx at " << i << " : " << heap.heap[i]->heapIdx << endl;

    cout << heap.NumItems() << " items in heap:" << endl;
    i = 0;
        
    while (heap.size() > 0)
    {
        
        cout << i++ << " > " << heap.front() << endl;
        pop_heap(heap.begin(), heap.end());
    }
#else
    vector<float> heap;
    int         i, n = 100;
    
    cout << "\nHeap Test" << endl;
    cout << "-----------\n" << endl;

    srand(20);
    
    for (i = 0; i < n; i++)
    {
        heap.push_back(rand());
        he->cost = rand(); // XXX cl_rand();
        heap.Insert(he);
    }

    for (i = 0; i < n / 5; i++)
    {
        he = heap.heap[rand() % n];
        cout << "updating " << he->heapIdx << endl;
        he->cost = rand();
        heap.Update(he);
    }
    
    for (i = 0; i < n / 5; i++)
    {
        he = heap.heap[rand() % heap.NumItems()];
        cout << "removing " << he->heapIdx << endl;
        heap.Delete(he);
    }
    
    for (i = 0; i < heap.NumItems(); i++)
        if (heap.heap[i]->heapIdx != i)
            cout << "bad heapIdx at " << i << " : " << heap.heap[i]->heapIdx << endl;

    cout << heap.NumItems() << " items in heap:" << endl;
    i = 0;
        
    while (he = heap.RemoveMax())
    {
        cout << i++ << " > " << he->cost << endl;
        delete he;
    }
#endif
}
#endif


void TestObjArray()
{


}

#ifdef UNFINISHED
void TestSplayTree()
{
    /* A sample use of these functions.  Start with the empty tree,         */
    /* insert some stuff into it, and then delete it                        */
    Tree * root;
    int i;
    root = NULL;               /* the empty tree */
    size = 0;
    for (i = 0; i < 1024; i++)
    {
        root = insert((541 * i) & (1023), root);
    }
    for (i = 0; i < 1024; i++)
    {
        root = delete((541 * i) & (1023), root);
    }
    printf("size = %d\n", size);
}
#endif

void TestMessages()
{
    CL_ASSERT(false);
    CL_EXPECT(false);
    CL_RANGE(0, 1, 2);

    CLAssert(false, "bob", "bob", 10);
    CLTrace("hello %d\n", 88);
    CLError("oh %d\n", 99);
}

int main(int argc, char *argv[])
{
    cFileSpec test;
    
    TestEnviron();
    TestString();
    TestFileSpec();
    //TestMessages();
    //TestHeap();
        
    return 0;
}
