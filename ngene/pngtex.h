/*
   Copyright (C) 2000 Xavier Hosxe <xhosxe@free.fr>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#if defined(_WIN32)
#include <windows.h>
#endif

#include "png.h"
#include <GL/gl.h>		 
#include <GL/glu.h>		 
#include <GL/glut.h>

class PngTex {
public:
    PngTex(const char *file);
    PngTex();
    ~PngTex();
    void bindToTexture(GLuint &texture);

    char *getTextureForOpenGl()
	{
	    return openGL_tex;
	}
    int getWidth()
	{
	    return (int)width;
	}
    int getHeight()
	{
	    return (int)height;
	}
	void setAll(int w, int h, char * tex) {
		width = w;
		height = h;
		openGL_tex = tex;
	}

protected:
    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    int interlace_type;
    int compression_type;
    int filter_type;
    char *openGL_tex;
};
