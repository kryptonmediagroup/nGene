/***************************************************************************
                          mesh.h  -  description
                             -------------------
    begin                : Mon Mar 12 2001
 ***************************************************************************/

#ifndef MESH_H
#define MESH_H

#if defined(_WIN32)
#include <windows.h>
#endif

#include "lwo2.h"
#include "texturemanager.h" // An OpenGL texture handling class

/**With CLWO2Loader at its core, this class loads the geometry 
and constructs useful OpenGL strips of polygons, sorted by
material.  

This was split out into a separate class because different 
engines need to be able to do this differently; doing it this way 
gives you options as to how best to do it without having to figure 
out the particulars of how the loader code itself works.
  *@author Gene Turnbow
*/

// I can't imagine needing more than 256 textures for a single object.
// It would grind the game engine to a halt to have all those loaded at once.

#define  MAX_TEXTURES_PER_MESH 256


class CMesh : public CLWO2Loader  
{
protected:
	structStrip *m_Strips;
	
public: 	
	virtual int LoadAllRequiredTextures( CTextureManager *textureManager );
	CMesh();
	virtual ~CMesh();	
	virtual int Load(char *fName, CTextureManager *textureManager = NULL);
};

#endif
