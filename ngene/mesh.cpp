/***************************************************************************
                          mesh.cpp  -  description
                             -------------------
    begin                : Mon Mar 12 2001

 ***************************************************************************/
#include <iostream.h>
#include "mesh.h"
#include "texturemanager.h"
#include <assert.h>

CMesh::CMesh(){
	m_Strips = NULL;
}
CMesh::~CMesh(){
	// Delete strips and textures.

}
/** Calls CLWO2Loader::Load(), then 
constructs strips by material and sorts 
them (again, by material). */

int CMesh::Load(char *fName, CTextureManager *textureManager)
{

	CLWO2Loader::Load(fName);	
#ifdef _DATADUMP	
	cout << "File " << fName;
#endif
	assert(m_iErrCode==LWOLOADER_SUCCESS);

	switch(m_iErrCode)
	{
	case LWOLOADER_NOTFOUND:
#ifdef _DATADUMP
		cout << " not found." << endl;
		exit(-1);
#endif
		break;

	case LWOLOADER_NOTANIFF:
#ifdef _DATADUMP
		cout << " is not in standard IFF format. " << endl;
		exit( -1 );
#endif
		break;

	case LWOLOADER_NOTANOBJECT:
#ifdef _DATADUMP
		cout << " is not an object. " << endl;
		exit( -1 );
#endif
		break;

	case LWOLOADER_SUCCESS:
#ifdef _DATADUMP
		cout << " was loaded successfully." << endl;
#endif
		break;
		
	}
	if (textureManager)
		LoadAllRequiredTextures(textureManager);

	return m_iErrCode;
}

// Loads all the textures required for this object.  (Don't worry about duplicate loads - the CTexture
// class watches that problem for you.
int CMesh::LoadAllRequiredTextures( CTextureManager *textureManager )
{
	int unsigned idx = 0;

	while( idx < m_NumSurfaces)									
	{
		// Scan through the subchunks for this texture (if there ARE any)
		// and pick out the filenames for the texture maps that have
		// been applied to the COLR channel in the texture.
		//
		// The rest is trash, for now.
		//
		//
		SurfaceBlock *s = m_Surfaces[idx].surfaceBlocks;
		if (s)
		{
			while(s)
			{
				if (s->imap)
				{
					// Derive the name of the bitmap from the clipindex
					// and tell the semi-intelligent collection of textures 
					// to load the thing up and return us a handle to it.

					// But only for color maps.  The rest can go hang for the moment.


					// We may have to modify the texture class so it can 
					// accept more detailed input than this.

					if (!strcmp(s->textureChannel,"COLR"))
					{
						GLuint result = textureManager->LoadImage(GetClipName(s->imap->clipIndex));
						m_Surfaces[idx].colormapHandle = result;
					}
				}
				s = s->next;
			}
		}
		idx++;
	}
	return true;
}



