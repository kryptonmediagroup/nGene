// texturemanager.cpp: implementation of the texturemanager class.
//
//////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
#include <windows.h>
#else
#include <string.h>
#endif

#include "texturemanager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTextureManager::CTextureManager()
{
	m_AllTheTexturesInTheUniverse = NULL;

}

CTextureManager::~CTextureManager()
{
	FlushAll();
}

void CTextureManager::FlushAll()
{
	CTexture *a = m_AllTheTexturesInTheUniverse;

	while(a)
	{
		if (a->next)
		{
			a = a->next;
			delete a->prev;
			a->prev = NULL;
		}
		else
		{
			delete a;
			a = NULL;
		}
	}			

}

GLuint CTextureManager::LoadImage(char *filename)
{
	GLuint glTexID = GetTextureByName(filename);

	if (glTexID==0)	// Then we don't know about this texture, and need to load a copy
	{
		CTexture *a = m_AllTheTexturesInTheUniverse;
		if (a)
		{
			while(a->next)
				a = a->next;
			a->next = new CTexture;
			a->next->prev = a;
			a = a->next;
		}
		else
		{
			a = new CTexture;
			m_AllTheTexturesInTheUniverse = a;
		}
		glTexID = a->LoadImage(filename);	// glTexID gets filled in with the texID of the texture (returned by reference)
		return glTexID;
	}
	else
		return glTexID;

}

GLuint CTextureManager::GetTextureByName(char *filename)
{
	CTexture *p = m_AllTheTexturesInTheUniverse;
	while(p)
	{
		if (strstr(p->m_Filename,filename))
			return p->m_texID;
		else
			p = p->next;
	}
	return 0;	// There's no such thing as an OpenGL texture name of 0 (OpenGL reserves it and you can't assign it to anything)
				// so this should be safe as a return value.

}


