//
//  File:       HLReadObj.cpp
//
//  Function:   <Description>
//
//  Author(s):  Andrew Willmott
//
//  Copyright:  2013
//

#include <HLReadObj.h>

#include <CLFileSpec.h>
#include <VL234f.h>

#include <HLGLUtilities.h>

using namespace nHL;
using namespace nCL;

/*
    OBJ Format is: 'function args' on each line.
    
    v x y z             add vertex
    vt u v [w]          add texture vert.
    vn i j k            add normal
    f 1 5 3 2           define polygon
                        indices are of the form v[/vt[/vn]] 
                        indices are 1-based.
    g group1 group2...  set group names for following f's.
    o                   object name for following f's
    s n                 set smoothing group.
    usemtl material     set material name for following f's

    Extensions:

    vc r g b            add colour
    
*/

/*
    Example material file:
    
    newmtl Cube_0
        Ns 100
        d 1
        illum 2
        Ka .2 .2 .2
        Kd 0.8 0.8 0.8
        Ks 0 0 0
        map_Kd finishedx0001.jpg
        map_Ka finishedx0001.jpg
*/

namespace
{
    struct cMesh
    {
        vector<Vec3f> mPositions;
        vector<Vec3f> mNormals;
        vector<Vec2f> mUVs;

        vector<int32_t> mPositionIndices;
        vector<int32_t> mNormalIndices;
        vector<int32_t> mUVIndices;

        vector<int> mVertexIndices;
    };
}

namespace
{
    void InPlaceTriangulate(int numVerts, vector<int>& indices)
    {
        // Assume polygon of numVerts indices at the end of 'indices', and triangulate it in place.

        int baseVertexIndex = indices.size() - numVerts;
        int baseEltIndex = indices[baseVertexIndex];

        for (int i = 1; i < numVerts - 2; i++)
        {
            indices.insert(indices.begin() + baseVertexIndex + 3 * i, indices[baseVertexIndex + 3 * i - 1]);
            indices.insert(indices.begin() + baseVertexIndex + 3 * i, baseEltIndex);
        }
    }

    bool FaceCommand(cMesh* mesh, int argc, const char* va[])
    {
        argc--;
        va++;

        char* end;

        for (int i = 0; i < argc; i++)
        {
            long ip = strtol(va[i], &end, 10);

            if (end != va[i])
                mesh->mPositionIndices.push_back(int(ip) - 1);

            if (end[0] == '/')
            {
                const char* next = end + 1;
                long it = strtol(next, &end, 10);

                if (end != next)
                    mesh->mUVIndices.push_back(int(it) - 1);
            }

            if (end[0] == '/')
            {
                const char* next = end + 1;
                long in = strtol(end + 1, &end, 10);

                if (end != next)
                    mesh->mNormalIndices.push_back(int(in) - 1);
            }
        }

        // quick and dirty in-place triangulation.
        if (argc > 3)
        {
            InPlaceTriangulate(argc, mesh->mPositionIndices);

            if (!mesh->mNormalIndices.empty())
                InPlaceTriangulate(argc, mesh->mNormalIndices);

            if (!mesh->mUVIndices.empty())
                InPlaceTriangulate(argc, mesh->mUVIndices);
        }

        return true;
    }

    bool PositionCommand(cMesh* mesh, int argc, const char* va[])
    {
        if (argc < 4)
            return false;

        Vec3f p;
        p[0] = atof(va[1]);
        p[1] = atof(va[2]);
        p[2] = atof(va[3]);

        mesh->mPositions.push_back(p);

        return true;
    }

    bool NormalCommand(cMesh* mesh, int argc, const char* va[])
    {
        if (argc < 4)
            return false;

        Vec3f p;
        p[0] = atof(va[1]);
        p[1]= atof(va[2]);
        p[2] = atof(va[3]);

        mesh->mNormals.push_back(p);

        return true;
    }

    bool TexCoordCommand(cMesh* mesh, int argc, const char* va[])
    {
        if (argc < 3)
            return false;

        Vec2f p;
        p[0] = atof(va[1]);
        p[1] = atof(va[2]);

        mesh->mUVs.push_back(p);

        return true;
    }

    bool ObjectCommand(cMesh* mesh, int argc, const char* va[])
    {
        return true;
    }

    bool GroupCommand(cMesh* mesh, int argc, const char* va[])
    {
        return true;
    }

    bool SmoothingGroupCommand(cMesh* mesh, int argc, const char* va[])
    {
        return true;
    }

    bool MaterialCommand(cMesh* mesh, int argc, const char* va[])
    {
        return true;
    }
    bool MaterialLibraryCommand(cMesh* mesh, int argc, const char* va[])
    {
        return true;
    }


    bool ProcessObjCommand(cMesh* mesh, int argc, const char* argv[])
    {
        assert(argc > 0);

        switch (argv[0][0])
        {
            case 'f':
                return FaceCommand(mesh, argc, argv);
            case 'v':
                if (argv[0][1] == 'n')
                    return NormalCommand(mesh, argc, argv);
                else if (argv[0][1] == 't')
                    return TexCoordCommand(mesh, argc, argv);
                else if (argv[0][1] == 0)
                    return PositionCommand(mesh, argc, argv);
                break;
            case 'o':
                return ObjectCommand(mesh, argc, argv);
            case 'g':
                return GroupCommand(mesh, argc, argv);
            case 's':
                return SmoothingGroupCommand(mesh, argc, argv);
            case 'u':
                if (strcasecmp(argv[0], "usemtl") == 0)
                    return MaterialCommand(mesh, argc, argv);
                break;
            case 'm':
                if (strcasecmp(argv[0], "mtllib") == 0)
                    return MaterialLibraryCommand(mesh, argc, argv);
                break;
            case '#':
                return true;
        }
        
        return false;
    }


#ifdef TODO
    // ancient GCL code...
    int ParseMaterialFile(StrConst filename)
    {
        String      	token;
        ifstream    	s;
        cColour      	c;
        cBlinnMaterial*	curMaterial = 0;

        DBG_COUT << "parsing material file " << filename << endl;
        s.open(filename);
        if (!s)
        {
            cerr << "(ParseMaterialFile) Cannot access " << filename << endl;
            return(-1);
        }

        while (s)
        {
            if (token.ReadWord(s))
            {
                if (token[0] == '#')
                    ;
                else if (token == "newmtl")
                {
                    curMaterial = new cBlinnMaterial;
                    
                    token.ReadString(s);
                    DBG_COUT << "new material " << token << endl;
                    slGetMaterialLibrary()->AddMaterial(curMaterial, token);
                }
                else if (!curMaterial)
                    cerr << "(ParseMaterialFile) *** No material, ignoring token: " << token << endl;
                else if (token == "Kd")
                {
                    s >> c[0] >> c[1] >> c[2];
                    curMaterial->rdClr = c;
                    curMaterial->rd = 1.0;
                    curMaterial->mFlags.Set(cBlinnMaterial::kHasRd);
                }
                else if (token == "Ks")
                {
                    s >> c[0] >> c[1] >> c[2];
                    curMaterial->rsClr = c;
                    curMaterial->rs = 1.0;
                    curMaterial->mFlags.Set(cBlinnMaterial::kHasRs);
                }
                else if (token == "Ns")
                    s >> curMaterial->rn;
                else if (token == "Ke")
                {
                    s >> c[0] >> c[1] >> c[2];
                    curMaterial->edClr = c;
                    curMaterial->ed = 1.0;
                    curMaterial->mFlags.Set(cBlinnMaterial::kHasEd);
                }
                else if (token == "map_Kd")
                {
                    String texFile;
                    
                    texFile.ReadWord(s);
                    SubstituteEnvVars(texFile);

                    DBG_COUT << "texture file " << texFile << endl;
                    curMaterial->mFlags.Set(cBlinnMaterial::kHasTexture);
                    curMaterial->mTextureFile = texFile;
                }
                else
                    cerr << "(ParseMaterialFile) *** Ignoring unknown token: " << token << endl;

                ReadLine(s, &token); // ignore rest of the line
            }
        }
        return(0);
    }
#endif

    int Split(char* buffer, int maxArgs, const char** argv)
    {
        maxArgs--;  // always reserve the last spot for 0 terminator
        
        for (int argc = 0; argc < maxArgs; argc++)
        {
            argv[argc] = strsep(&buffer, " \t\n\r");

            if (!argv[argc] || !argv[argc][0])
                return argc;
        }

        fprintf(stderr, "Warning: ignored arguments past %d\n", maxArgs - 1);
        argv[maxArgs] = 0;
        return maxArgs;
    }

    bool ReadObjFile(FILE* file, cMesh* mesh)
    {
        *mesh = cMesh();

        const int kMaxArgs = 256;
        const char* argv[kMaxArgs];

        char* lineBuffer = 0;
        size_t lineBufferSize = 0;
        ssize_t lineSize;

        while ((lineSize = getline(&lineBuffer, &lineBufferSize, file)) > 0)
        {
            int argc = Split(lineBuffer, kMaxArgs, argv);

            if (argc > 0)
            {

                if (!ProcessObjCommand(mesh, argc, argv))
                {
                    fprintf(stderr, "Can't parse command: '%s'\n", argv[0]);
                    return false;
                }
            }
        }

        printf("Read %zu positions, %zu normals, %zu uvs\n",
            mesh->mPositions.size(),
            mesh->mNormals.size(),
            mesh->mUVs.size()
        );

        printf("     %zd position indices, %zd normal indices, %zd uv indices\n",
            mesh->mPositionIndices.size(),
            mesh->mNormalIndices.size(),
            mesh->mUVIndices.size()
        );

        return true;
    }

    bool WriteObjFile(const cMesh* mesh, FILE* file)
    {
        for (auto vp: mesh->mPositions)
            fprintf(file, "v %g %g %g\n", vp[0], vp[1], vp[2]);

        for (auto vp: mesh->mNormals)
            fprintf(file, "vn %g %g %g\n", vp[0], vp[1], vp[2]);

        for (auto vp: mesh->mUVs)
            fprintf(file, "vt %g %g %g\n", vp[0], vp[1], vp[2]);

        auto const& ip = mesh->mPositionIndices;
        auto const& it = mesh->mUVIndices;
        auto const& in = mesh->mNormalIndices;

        int faceType = 0;

        if (it.size() == ip.size())
            faceType += 1;
        if (in.size() == ip.size())
            faceType += 2;

        switch (faceType)
        {
        case 0:
            for (int i = 0, n = ip.size(); i < n; i += 3)
                fprintf(file, "f %d %d %d\n", ip[i + 0] + 1, ip[i + 1] + 1, ip[i + 2] + 1);
            break;
        case 1:
            for (int i = 0, n = ip.size(); i < n; i += 3)
                fprintf(file, "f %d/%d %d/%d %d/%d\n",
                    ip[i + 0] + 1, it[i + 0] + 1,
                    ip[i + 1] + 1, it[i + 1] + 1,
                    ip[i + 2] + 1, it[i + 2 + 1]
                );
            break;
        case 2:
            for (int i = 0, n = ip.size(); i < n; i += 3)
                fprintf(file, "f %d//%d %d//%d %d//%d\n",
                    ip[i + 0] + 1, in[i + 0] + 1,
                    ip[i + 1] + 1, in[i + 1] + 1,
                    ip[i + 2] + 1, in[i + 2] + 1
                );
            break;
        case 3:
            for (int i = 0, n = ip.size(); i < n; i += 3)
                fprintf(file, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                    ip[i + 0] + 1, it[i + 0] + 1, in[i + 0] + 1,
                    ip[i + 1] + 1, it[i + 1] + 1, in[i + 1] + 1,
                    ip[i + 2] + 1, it[i + 2] + 1, in[i + 2] + 1
                );
            break;
        }

        return true;
    }

    void BuildGLMesh(const cMesh* mesh, cGLMeshInfo* meshInfo)
    {
        GLuint meshName;
        
        // Create a vertex array object (VAO) to cache mesh parameters
        glGenVertexArrays(1, &meshName);
        glBindVertexArray(meshName);

        GLuint posBufferName;
        
        // Create a vertex buffer object (VBO) to store positions
        glGenBuffers(1, &posBufferName);
        glBindBuffer(GL_ARRAY_BUFFER, posBufferName);
        {
            glBufferData(GL_ARRAY_BUFFER, mesh->mPositions.size() * 12, mesh->mPositions.data(), GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(kVBPositions);
            
            glVertexAttribPointer
            (
                kVBPositions,
                3,
                GL_FLOAT,
                GL_FALSE,
                12,
                0
            );
            GL_CHECK;

            glEnableVertexAttribArray(kVBColours);
            
            glVertexAttribPointer
            (
                kVBColours,
                3,
                GL_FLOAT,
                GL_FALSE,
                12,
                0
            );
            GL_CHECK;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if (!mesh->mNormals.empty())
        {
            GLuint normalBufferName;
            
            glGenBuffers(1, &normalBufferName);
            glBindBuffer(GL_ARRAY_BUFFER, normalBufferName);
            {
                glBufferData(GL_ARRAY_BUFFER, mesh->mNormals.size() * 12, mesh->mNormals.data(), GL_STATIC_DRAW);
                
                glEnableVertexAttribArray(kVBNormals);

                glVertexAttribPointer
                (
                    kVBNormals,
                    3,
                    GL_FLOAT,
                    GL_FALSE,   // Do we want to normalize this data (0-1 range for fixed-pont types)
                    12,
                    0
                );

                GL_CHECK;
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        
        if (!mesh->mUVs.empty())
        {
            GLuint texcoordBufferName;
            
            // Create a VBO to store texcoords
            glGenBuffers(1, &texcoordBufferName);
            glBindBuffer(GL_ARRAY_BUFFER, texcoordBufferName);
            {
                glBufferData(GL_ARRAY_BUFFER, mesh->mUVs.size() * 8, mesh->mUVs.data(), GL_STATIC_DRAW);
                
                glEnableVertexAttribArray(kVBTexCoords);

                glVertexAttribPointer
                (
                    kVBTexCoords,
                    2,
                    GL_FLOAT,
                    GL_TRUE,
                    8,
                    0
                );

                GL_CHECK;
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        
        GLuint elementBufferName;
        glGenBuffers(1, &elementBufferName);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferName);
        
        // Allocate and load vertex array element data into VBO
        CL_ASSERT(sizeof(mesh->mPositionIndices[0]) == sizeof(uint32_t));
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->mPositionIndices.size() * sizeof(uint32_t), mesh->mPositionIndices.data(), GL_STATIC_DRAW);

        GL_CHECK;

        glBindVertexArray(0);

        meshInfo->mMesh    = meshName;
        meshInfo->mNumElts = mesh->mPositionIndices.size();
        meshInfo->mEltType = GL_UNSIGNED_INT;
    }

}


bool nHL::LoadObj(cGLMeshInfo* meshInfo, const nCL::cFileSpec& spec)
{
    cMesh mesh;

    FILE* file = spec.FOpen("r");

    bool success = file && ReadObjFile(file, &mesh);

    fclose(file);

    if (success)
    {
        BuildGLMesh(&mesh, meshInfo);
    }

    return success;
}
