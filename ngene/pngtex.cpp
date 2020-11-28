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

/* 
 * Png algorithm from the libpng....
 *
 */

#include "pngtex.h"

PngTex::PngTex()
{
	width=0;
	height=0;
	openGL_tex = NULL;
}

PngTex::PngTex(const char* file_name)
{
    FILE *fp = fopen(file_name, "rb");
    png_byte header[4];
    png_structp png_ptr;

    if (!fp)
    {
		throw "Cannot open texture";
    }
    
    if (!png_sig_cmp(header, (png_size_t)0, 4))
		throw "not png type";

    
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
									 NULL,
									 NULL, 
									 NULL);
    if (!png_ptr)
        throw "error while reading png_structp";

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr,
								(png_infopp)NULL, (png_infopp)NULL);
		throw "error while reading info_struct";
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr,
								(png_infopp)NULL);
		throw "error while reading info_struct";
    }

    
    png_init_io(png_ptr, fp);

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height,
				 &bit_depth, &color_type, &interlace_type,
				 &compression_type, &filter_type);

    
    png_bytep *row_pointers = new png_bytep[height];
    unsigned int row;
    for (row = 0; row < height; row++)
    {
		int size;
		row_pointers[row] = new png_byte[(size=png_get_rowbytes(png_ptr, info_ptr))];
    }
   
    png_read_image(png_ptr, row_pointers);
    
    openGL_tex= new char[height*width*4];
    unsigned int i,j;
    
    for(j = 0; j < height; j++) {
		for (i=0; i < width;i++)
		{
			int scale = 3 + (PNG_COLOR_TYPE_RGB_ALPHA==color_type);
			openGL_tex[(j*width+i)*4 ] = (char)*(char*)( row_pointers[j] +i*scale) ;
			openGL_tex[(j*width+i)*4 +1 ] = (char)*(char*)( row_pointers[j] +i*scale +1 ) ;
			openGL_tex[(j*width+i)*4 +2 ] = (char)*(char*)( row_pointers[j] +i*scale +2 ) ;
			if (color_type==PNG_COLOR_TYPE_RGB)
				openGL_tex[(j*width+i)*4 +3 ] = 255 ;
			else
				openGL_tex[(j*width+i)*4 +3 ] = (char)*(char*)( row_pointers[j] +i*scale +3 ) ;
		}
    }
    
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);

    for (row = 0; row < height; row++)
    {
		delete row_pointers[row];
    }


}

// This passes the texture "name" back via reference
void PngTex::bindToTexture(GLuint &texture)
{
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, & texture );
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
     width, 
     height, 
     0, GL_RGBA,
     GL_UNSIGNED_BYTE, 
     openGL_tex);
  /*gluBuild2DMipmaps(GL_TEXTURE_2D,  GL_RGBA16, 
		    width, 
		    height, GL_RGBA,
		    GL_UNSIGNED_BYTE, 
		    openGL_tex); */
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}


PngTex::~PngTex()
{
    delete openGL_tex;
}

