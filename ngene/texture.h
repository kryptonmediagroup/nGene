/***************************************************************************
                          texture.h  -  description

 ***************************************************************************/

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "GL/gl.h"
#include "GL/glu.h"

class CTexture
{
public:
	char * m_Filename;
  CTexture();
	virtual ~CTexture();

  GLuint LoadImage(char *filename, GLuint texnr = 0, char mm = 0);  // Generic loading funtion - returns actual GL texture name

  GLuint LoadTGA(char *filename,GLuint texnr);    // TGA loader - returns actual GL texture name
  GLuint LoadPCX(char *filename,GLuint texnr);    // PCX loader - returns actual GL texture name
  GLuint LoadPNG(char *filename, GLuint texnr);		// PNG loader - returns actual GL texture name

  GLuint m_bpp;
  GLuint m_width;
  GLuint m_height;
  GLuint m_texID;
  /** This static field gives all instances of CTexture 
access to a valuable bit of information, i.e., the 
OpenGL texture ID of the last texture created. */
	CTexture * prev;
	CTexture * next;

protected:
	char m_mipmap;
	GLubyte *m_imageData;
};


#endif
