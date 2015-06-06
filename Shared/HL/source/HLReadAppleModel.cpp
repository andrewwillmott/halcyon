/*
     File: modelUtil.c
 Abstract: Functions for loading a model file for vertex arrays.
  Version: 1.0
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2010~2011 Apple Inc. All Rights Reserved.
 
 */

#include <HLReadAppleModel.h>

#include <CLFileSpec.h>

using namespace nHL;
using namespace nCL;

typedef struct modelHeaderRec
{
	char fileIdentifier[30];
	unsigned int majorVersion;
	unsigned int minorVersion;
} modelHeader;

typedef struct modelTOCRec
{
	unsigned int attribHeaderSize;
	unsigned int byteElementOffset;
	unsigned int bytePositionOffset;
	unsigned int byteTexcoordOffset;
	unsigned int byteNormalOffset;
} modelTOC;

typedef struct modelAttribRec
{
	unsigned int byteSize;
	GLenum datatype;
	GLenum primType; //If index data
	unsigned int sizePerElement;
	unsigned int numElements;
} modelAttrib;

demoModel* mdlLoadModel(const char* filepathname)
{
	if(NULL == filepathname)
	{
		return NULL;
	}
	
	demoModel* model = (demoModel*) calloc(sizeof(demoModel), 1);
	
	if(NULL == model)
	{
		return NULL;
	}
						
	
	size_t sizeRead;
	int error;
	FILE* curFile = fopen(filepathname, "r");
	
	if(!curFile)
	{	
		mdlDestroyModel(model);	
		return NULL;
	}
	
	modelHeader header;
	
	sizeRead = fread(&header, 1, sizeof(modelHeader), curFile);
	
	if(sizeRead != sizeof(modelHeader))
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	if(strncmp(header.fileIdentifier, "AppleOpenGLDemoModelWWDC2010", sizeof(header.fileIdentifier)))
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	if(header.majorVersion != 0 && header.minorVersion != 1)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	modelTOC toc;
	
	sizeRead = fread(&toc, 1, sizeof(modelTOC), curFile);
	
	if(sizeRead != sizeof(modelTOC))
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	if(toc.attribHeaderSize > sizeof(modelAttrib))
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	modelAttrib attrib;
	
	error = fseek(curFile, toc.byteElementOffset, SEEK_SET);
	
	if(error < 0)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	sizeRead = fread(&attrib, 1, toc.attribHeaderSize, curFile);
	
	if(sizeRead != toc.attribHeaderSize)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	model->elementArraySize = attrib.byteSize;
	model->elementType = attrib.datatype;
	model->numElements = attrib.numElements;
	
	// OpenGL ES cannot use UNSIGNED_INT elements
	// So if the model has UI element...
	if(GL_UNSIGNED_INT == model->elementType)
	{
		//...Load the UI elements and convert to UNSIGNED_SHORT
		
		GLubyte* uiElements = (GLubyte*) malloc(model->elementArraySize);
		model->elements = (GLubyte*)malloc(model->numElements * sizeof(GLushort)); 
		
		sizeRead = fread(uiElements, 1, model->elementArraySize, curFile);
		
		if(sizeRead != model->elementArraySize)
		{
			fclose(curFile);
			mdlDestroyModel(model);		
			return NULL;
		}
		
		GLuint elemNum = 0;
		for(elemNum = 0; elemNum < model->numElements; elemNum++)
		{
			//We can't handle this model if an element is out of the UNSIGNED_INT range
			if(((GLuint*)uiElements)[elemNum] >= 0xFFFF)
			{
				fclose(curFile);
				mdlDestroyModel(model);		
				return NULL;
			}
			
			((GLushort*)model->elements)[elemNum] = ((GLuint*)uiElements)[elemNum];
		}
		
		free(uiElements);
	
		
		model->elementType = GL_UNSIGNED_SHORT;
		model->elementArraySize = model->numElements * sizeof(GLushort);
	}
	else 
	{	
		model->elements = (GLubyte*)malloc(model->elementArraySize);
		
		sizeRead = fread(model->elements, 1, model->elementArraySize, curFile);
		
		if(sizeRead != model->elementArraySize)
		{
			fclose(curFile);
			mdlDestroyModel(model);		
			return NULL;
		}
	}

	fseek(curFile, toc.bytePositionOffset, SEEK_SET);
	
	sizeRead = fread(&attrib, 1, toc.attribHeaderSize, curFile);
	
	if(sizeRead != toc.attribHeaderSize)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	model->positionArraySize = attrib.byteSize;
	model->positionType = attrib.datatype;
	model->positionSize = attrib.sizePerElement;
	model->numVertcies = attrib.numElements;
	model->positions = (GLubyte*) malloc(model->positionArraySize);
	
	sizeRead = fread(model->positions, 1, model->positionArraySize, curFile);
	
	if(sizeRead != model->positionArraySize)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	error = fseek(curFile, toc.byteTexcoordOffset, SEEK_SET);
	
	if(error < 0)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	sizeRead = fread(&attrib, 1, toc.attribHeaderSize, curFile);
	
	if(sizeRead != toc.attribHeaderSize)
	{	
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	model->texcoordArraySize = attrib.byteSize;
	model->texcoordType = attrib.datatype;
	model->texcoordSize = attrib.sizePerElement;
	
	//Must have the same number of texcoords as positions
	if(model->numVertcies != attrib.numElements)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	model->texcoords = (GLubyte*) malloc(model->texcoordArraySize);
	
	sizeRead = fread(model->texcoords, 1, model->texcoordArraySize, curFile);
	
	if(sizeRead != model->texcoordArraySize)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	error = fseek(curFile, toc.byteNormalOffset, SEEK_SET);
	
	if(error < 0)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	sizeRead = fread(&attrib, 1, toc.attribHeaderSize, curFile);
	
	if(sizeRead !=  toc.attribHeaderSize)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	model->normalArraySize = attrib.byteSize;
	model->normalType = attrib.datatype;
	model->normalSize = attrib.sizePerElement;

	//Must have the same number of normals as positions
	if(model->numVertcies != attrib.numElements)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
		
	model->normals = (GLubyte*) malloc(model->normalArraySize );
	
	sizeRead =  fread(model->normals, 1, model->normalArraySize , curFile);
	
	if(sizeRead != model->normalArraySize)
	{
		fclose(curFile);
		mdlDestroyModel(model);		
		return NULL;
	}
	
	
	fclose(curFile);
	
	return model;
	
}

demoModel* mdlLoadQuadModel()
{
	GLfloat posArray[] = {
		-200.0f, 0.0f, -200.0f,
		 200.0f, 0.0f, -200.0f,
		 200.0f, 0.0f,  200.0f,
		-200.0f, 0.0f,  200.0f
	};
		
	GLfloat texcoordArray[] = { 
		0.0f,  1.0f,
		1.0f,  1.0f,
		1.0f,  0.0f,
		0.0f,  0.0f
	};
	
	GLfloat normalArray[] = {
		0.0f, 0.0f, 1.0,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
	};
	
	GLushort elementArray[] =
	{
		0, 2, 1,
		0, 3, 2
	};
	
	demoModel* model = (demoModel*) calloc(sizeof(demoModel), 1);
	
	if(NULL == model)
	{
		return NULL;
	}
	
	model->positionType = GL_FLOAT;
	model->positionSize = 3;
	model->positionArraySize = sizeof(posArray);
	model->positions = (GLubyte*)malloc(model->positionArraySize);
	memcpy(model->positions, posArray, model->positionArraySize);
	
	model->texcoordType = GL_FLOAT;
	model->texcoordSize = 2;
	model->texcoordArraySize = sizeof(texcoordArray);
	model->texcoords = (GLubyte*)malloc(model->texcoordArraySize);
	memcpy(model->texcoords, texcoordArray, model->texcoordArraySize );

	model->normalType = GL_FLOAT;
	model->normalSize = 3;
	model->normalArraySize = sizeof(normalArray);
	model->normals = (GLubyte*)malloc(model->normalArraySize);
	memcpy(model->normals, normalArray, model->normalArraySize);
	
	model->elementArraySize = sizeof(elementArray);
	model->elements	= (GLubyte*)malloc(model->elementArraySize);
	memcpy(model->elements, elementArray, model->elementArraySize);
	
	model->primType = GL_TRIANGLES;
	
	
	model->numElements = sizeof(elementArray) / sizeof(GLushort);
	model->elementType = GL_UNSIGNED_SHORT;
	model->numVertcies = model->positionArraySize / (model->positionSize * sizeof(GLfloat));
	
	return model;
}

void mdlDestroyModel(demoModel* model)
{
	if(NULL == model)
	{
		return;
	}
	
	free(model->elements);
	free(model->positions);
	free(model->normals);
	free(model->texcoords);
	
	free(model);
}


GLuint BuildMesh(demoModel* model)
{
    GLuint meshName;
    
    // Create a vertex array object (VAO) to cache model parameters
    glGenVertexArrays(1, &meshName);
    glBindVertexArray(meshName);
    
    GLuint posBufferName;
    
    // Create a vertex buffer object (VBO) to store positions
    glGenBuffers(1, &posBufferName);
    glBindBuffer(GL_ARRAY_BUFFER, posBufferName);
    
    // Allocate and load position data into the VBO
    glBufferData(GL_ARRAY_BUFFER, model->positionArraySize, model->positions, GL_STATIC_DRAW);
    
    // Enable the position attribute for this VAO
    glEnableVertexAttribArray(kVBPositions);
    
    // Get the size of the position type so we can set the stride properly
    GLsizei posTypeSize = GetGLTypeSize(model->positionType);

    // Set up parmeters for position attribute in the VAO including,
    //  size, type, stride, and offset in the currenly bound VAO
    // This also attaches the position VBO to the VAO
    glVertexAttribPointer
    (
        kVBPositions,       // What attibute index will this array feed in the vertex shader (see buildProgram)
        model->positionSize,    // How many elements are there per position?
        model->positionType,    // What is the type of this data?
        GL_FALSE,               // Do we want to normalize this data (0-1 range for fixed-pont types)
        model->positionSize * posTypeSize, // What is the stride (i.e. bytes between positions)?
        0
    );  // What is the offset in the VBO to the position data?
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (model->normals)
    {
        GLuint normalBufferName;
        
        // Create a vertex buffer object (VBO) to store positions
        glGenBuffers(1, &normalBufferName);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferName);
        {
            // Allocate and load normal data into the VBO
            glBufferData(GL_ARRAY_BUFFER, model->normalArraySize, model->normals, GL_STATIC_DRAW);
            
            // Enable the normal attribute for this VAO
            glEnableVertexAttribArray(kVBNormals);
            
            // Get the size of the normal type so we can set the stride properly
            GLsizei normalTypeSize = GetGLTypeSize(model->normalType);
            
            // Set up parmeters for position attribute in the VAO including,
            //   size, type, stride, and offset in the currenly bound VAO
            // This also attaches the position VBO to the VAO
            glVertexAttribPointer
            (
                kVBNormals,    // What attibute index will this array feed in the vertex shader (see buildProgram)
                model->normalSize, // How many elements are there per normal?
                model->normalType, // What is the type of this data?
                GL_FALSE,              // Do we want to normalize this data (0-1 range for fixed-pont types)
                model->normalSize*normalTypeSize, // What is the stride (i.e. bytes between normals)?
                0
             ); // What is the offset in the VBO to the normal data?
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    if (model->texcoords)
    {
        GLuint texcoordBufferName;
        
        // Create a VBO to store texcoords
        glGenBuffers(1, &texcoordBufferName);
        glBindBuffer(GL_ARRAY_BUFFER, texcoordBufferName);
        {
            // Allocate and load texcoord data into the VBO
            glBufferData(GL_ARRAY_BUFFER, model->texcoordArraySize, model->texcoords, GL_STATIC_DRAW);
            
            // Enable the texcoord attribute for this VAO
            glEnableVertexAttribArray(kVBTexCoords);
            
            // Get the size of the texcoord type so we can set the stride properly
            GLsizei texcoordTypeSize = GetGLTypeSize(model->texcoordType);
            
            // Set up parmeters for texcoord attribute in the VAO including,
            //   size, type, stride, and offset in the currenly bound VAO
            // This also attaches the texcoord VBO to VAO
            glVertexAttribPointer
            (
                kVBTexCoords,         // What attibute index will this array feed in the vertex shader (see buildProgram)
                model->texcoordSize,  // How many elements are there per texture coord?
                model->texcoordType,  // What is the type of this data in the array?
                GL_TRUE,              // Do we want to normalize this data (0-1 range for fixed-point types)
                model->texcoordSize*texcoordTypeSize,  // What is the stride (i.e. bytes between texcoords)?
                0
            );   // What is the offset in the VBO to the texcoord data?
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    GLuint elementBufferName;
    
    // Create a VBO to vertex array elements
    // This also attaches the element array buffer to the VAO
    glGenBuffers(1, &elementBufferName);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferName);
    
    // Allocate and load vertex array element data into VBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->elementArraySize, model->elements, GL_STATIC_DRAW);

    GL_CHECK;

    glBindVertexArray(0);

    return meshName;
}

bool LoadMDLMesh(cGLMeshInfo* info, const char* modelName, const char* textureName)
{
    //////////////////////////////
    // Load our character model //
    //////////////////////////////
    
    demoModel* model = mdlLoadModel(modelName);
    if (!model)
        return false;
    
    // Build Vertex Buffer Objects (VBOs) and Vertex Array Object (VAOs) with our model data
    info->mMesh = BuildMesh(model);
    
    // Cache the number of element and primType to use later in our glDrawElements calls
    info->mNumElts  = model->numElements;
    info->mEltType  = model->elementType;
    
    mdlDestroyModel(model);

    if (textureName)
        info->mTextures[kTextureDiffuseMap] = LoadTexture32(cFileSpec(textureName));

    return true;
}

