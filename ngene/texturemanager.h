// texturemanager.h: interface for the texturemanager class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_TEXTUREMANAGER_H)
#define _TEXTUREMANAGER_H

#include "texture.h"


class CTextureManager  
{
public:
	GLuint GetTextureByName(char *filename);
	GLuint LoadImage(char *filename);
	void FlushAll(void);
	CTextureManager();
	virtual ~CTextureManager();
	CTexture *m_AllTheTexturesInTheUniverse;
	
};

#endif 
