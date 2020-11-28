/***************************************************************************
texture.cpp  -  description
***************************************************************************/


// There was a reference to SDL here, but it was only needed for loading
// Windows BMP files.  Who cares?  TIFF's, Targa files, JPEG's, PCX's and
// PNG's should do just fine.  (For the moment, I don't have TIFF or JPEG
// support.  I prolly oughta fix that soon, but I want to check to see if
// I'm doing this right at all before I start adding stuff. - GT 03/12/200

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#include "texture.h"
#include "png.h"	// libPNG


typedef struct _pcxHeader
{
	short id[2];
	short offset[2];
	short size[2];
} pcxHeader;



// mm is the mipmapping level, unused for now.
// texnr is the texture number needed by OpenGL.
GLuint CTexture::LoadImage(char *filename, GLuint texnr, char mm )      // Choose, which loader to use
{
	GLuint resultingTextureName;

	int namesize=strlen(filename);
	char *ext = (filename+namesize-3);
	
	if (strstr(ext,"TGA") || strstr(ext,"tga"))
	{
		resultingTextureName = LoadTGA(filename,texnr);
		if (resultingTextureName!=0)
			goto ADDTOLIST;
	}
		
	if (strstr(ext,"PCX") || strstr(ext,"pcx"))
	{
		resultingTextureName = LoadPCX(filename,texnr);
		if (resultingTextureName!=0)
			goto ADDTOLIST;
	}
			
	if (strstr(ext,"PNG") || strstr(ext,"png"))
	{
		resultingTextureName = LoadPNG(filename,texnr);
		if (resultingTextureName!=0)
			goto ADDTOLIST;
	}
				
	return 0; // NULL
				
ADDTOLIST:
	
	m_Filename = new char[strlen(filename)];
	strcpy(m_Filename,filename);
	return resultingTextureName;
				
}



GLuint CTexture::LoadTGA(char *filename, GLuint texnr)
{
	GLubyte TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0}; // Uncompressed TGA Header
	GLubyte TGAcompare[12];                        // Used To Compare TGA Header
	GLubyte header[6];                             // First 6 Useful Bytes From The Header
	GLuint  bytesPerPixel;                         // Holds Number Of Bytes Per Pixel Used In The TGA File
	GLuint  imageSize;                             // Used To Store The Image Size When Setting Aside Ram
	GLuint  temp;                                  // Temporary Variable
	GLenum  type=GL_RGBA;                          // Set The Default GL Mode To RBGA (32 BPP)
	long  i;
	
	m_texID = 0;	// BUG: What if there's already a valid GL texture in this object? If m_texID is nonzero, we should clean house before loading another one.

	FILE *file = fopen(filename, "rb");            // Open The TGA File
	
	if(file==NULL ||                               // Does File Even Exist?
		fread(TGAcompare,1,sizeof(TGAcompare),file)!=sizeof(TGAcompare) || // Are There 12 Bytes To Read?
		memcmp(TGAheader,TGAcompare,sizeof(TGAheader))!=0 || // Does The Header Match What We Want?
		fread(header,1,sizeof(header),file)!=sizeof(header)) // If So Read Next 6 Header Bytes
	{
		//fclose(file); // This produced an error.  Apparently fclose() can't accept a NULL as a handle and not poop.
#if defined(_DATADUMP)
		printf("Texture not found: %s\n",filename);
#endif
		return false;
	}
	
	m_width  = header[1] * 256 + header[0]; // Determine The TGA Width (highbyte*256+lowbyte)
	m_height = header[3] * 256 + header[2]; // Determine The TGA Height (highbyte*256+lowbyte)
	
	if(m_width <=0  ||                       // Is The Width Less Than Or Equal To Zero
		m_height <=0 ||                      // Is The Height Less Than Or Equal To Zero
		(header[4]!=24 && header[4]!=32))           // Is The TGA 24 or 32 Bit?
	{
		fclose(file);                                // If Anything Failed, Close The File
		return false;                                // Return False
	}
	
	m_bpp  = header[4];                     // Grab The TGA's Bits Per Pixel (24 or 32)
	bytesPerPixel = m_bpp/8;                // Divide By 8 To Get The Bytes Per Pixel
	imageSize     = m_width*m_height*bytesPerPixel; // Calculate The Memory Required For The TGA Data
	
	m_imageData=(GLubyte *)malloc(imageSize); // Reserve Memory To Hold The TGA Data
	
	if(m_imageData==NULL ||                 // Does The Storage Memory Exist?
		fread(m_imageData, 1, imageSize, file)!=imageSize) // Does The Image Size Match The Memory Reserved?
	{
		if(m_imageData!=NULL)                 // Was Image Data Loaded
			free(m_imageData);                  // If So, Release The Image Data
		
		fclose(file);                                // Close The File
		return false;                                // Return False
	}
	
	for(i=0; i<(long)imageSize; i+=bytesPerPixel)   // Loop Through The Image Data
	{                                              // Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
		temp=m_imageData[i];                  // Temporarily Store The Value At Image Data 'i'
		m_imageData[i] = m_imageData[i + 2]; // Set The 1st Byte To The Value Of The 3rd Byte
		m_imageData[i + 2] = temp;            // Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
	}
	
	fclose (file);                                 // Close The File
	
	// Build A Texture From The Data
	if(texnr!=0)
	{
		glBindTexture(GL_TEXTURE_2D, texnr);
		m_texID = texnr;
	}
	else
	{
		glGenTextures(1, &m_texID);             // Generate OpenGL texture IDs
		glBindTexture(GL_TEXTURE_2D, m_texID);  // Bind Our Texture
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Linear Filtered
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Linear Filtered
	
	if (m_bpp==24)                          // Was The TGA 24 Bits
	{
		type=GL_RGB;                                 // If So Set The 'type' To GL_RGB
	}
	if (m_bpp==32)
	{
		type=GL_RGBA;
	}

	if(!m_mipmap)
		glTexImage2D(GL_TEXTURE_2D, 0, type, m_width, m_height, 0, type, GL_UNSIGNED_BYTE, m_imageData);
	else
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, m_width, m_height, type, GL_UNSIGNED_BYTE, m_imageData);
	
	texnr = m_texID;								// feeds it back to the caller
	return texnr;                                   // Texture Building Went Ok, Return True
	
	
}


GLuint CTexture::LoadPCX(char *filename, GLuint texnr)
{
	FILE *hTexFile = fopen( filename, "rb" );

	// BUG: What if this texture object already has a GL texture?  We should flush it out, but for the 
	// moment that's not happening.

	m_texID = 0;

	/* check the file open command */
	if( hTexFile != NULL )
	{
		int imgWidth, imgHeight, texFileLen, imgBufferPtr, i;
		pcxHeader *pcxPtr;
		unsigned char *imgBuffer, *texBuffer, *pcxBufferPtr, *paletteBuffer;
		
		/* find length of file */
		fseek( hTexFile, 0, SEEK_END );
		texFileLen = ftell( hTexFile );
		fseek( hTexFile, 0, SEEK_SET );
		
		/* read in file */
		texBuffer = (unsigned char *)malloc( texFileLen+1 );
		fread( texBuffer, sizeof( char ), texFileLen, hTexFile );
		
		/* get the image dimensions */
		pcxPtr = (pcxHeader *)texBuffer;
		imgWidth = pcxPtr->size[0] - pcxPtr->offset[0] + 1;
		imgHeight = pcxPtr->size[1] - pcxPtr->offset[1] + 1;
		
		/* image starts at 128 from the beginning of the buffer */
		imgBuffer = (unsigned char *)malloc( imgWidth * imgHeight );
		imgBufferPtr = 0;
		pcxBufferPtr = &texBuffer[128];
		/* decode the pcx image */
		while( imgBufferPtr < (imgWidth * imgHeight) )
		{
			if( *pcxBufferPtr > 0xbf )
			{
				int repeat = *pcxBufferPtr++ & 0x3f;
				for( i=0; i<repeat; i++ )
					imgBuffer[imgBufferPtr++] = *pcxBufferPtr;
			} else {
				imgBuffer[imgBufferPtr++] = *pcxBufferPtr;
			}
			pcxBufferPtr++;
		}
		/* read in the image palette */
		paletteBuffer = (unsigned char *)malloc( 768 );
		for( i=0; i<768; i++ )
			paletteBuffer[i] = texBuffer[ texFileLen-768+i ];
		
		/* find the nearest greater power of 2 for each dimension */
		m_width=imgWidth;
		m_height=imgHeight;
		{
			int imageWidth = imgWidth, imageHeight = imgHeight;
			i = 0;
			while( imageWidth )
			{
				imageWidth /= 2;
				i++;
			}
			m_width = (int)pow( 2, i );
			i = 0;
			while( imageHeight )
			{
				imageHeight /= 2;
				i++;
			}
			m_height = (int)pow( 2, i );
		}
		/* now create the OpenGL texture */
		{
			int i, j;
			m_imageData = (unsigned char *)malloc( m_width * m_height * 3 );
			for (j = 0; j < imgHeight; j++)
			{
				for (i = 0; i < imgWidth; i++)
				{
					m_imageData[3*(j * m_width + i)+0]
						= paletteBuffer[ 3*imgBuffer[j*imgWidth+i]+0 ];
					m_imageData[3*(j * m_width + i)+1]
						= paletteBuffer[ 3*imgBuffer[j*imgWidth+i]+1 ];
					m_imageData[3*(j * m_width + i)+2]
						= paletteBuffer[ 3*imgBuffer[j*imgWidth+i]+2 ];
				}
			}
		}
		/* cleanup */
		free( paletteBuffer );
		free( imgBuffer );
		
		if(texnr>0)
		{
			glBindTexture(GL_TEXTURE_2D, texnr);
			m_texID = texnr;
		}
		else
		{
			glGenTextures(1, &m_texID);
			glBindTexture(GL_TEXTURE_2D, m_texID);
		}
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		if(!m_mipmap)
			glTexImage2D( GL_TEXTURE_2D, 0, 3, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_imageData);
		else
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, m_imageData);
		
	} else {
		/* skip the texture setup functions */
		return m_texID;
	}
	return m_texID;
}

/** PNG loader */
GLuint CTexture::LoadPNG(char *filename, GLuint texnr)
{
    png_uint_32 width;
    png_uint_32 height;
    GLuint bit_depth;
    GLuint color_type;
    GLuint interlace_type;
    GLuint compression_type;
    GLuint filter_type;
	
	unsigned char *imageData;
	
	m_texID = 0;		// Hopefully we'll get a good value for this somewhere in here..

	FILE *hTexFile = fopen(filename,"rb");
	if (hTexFile)
	{
		png_byte header[4];
		png_structp png_ptr;
		
		if (!png_sig_cmp(header, (png_size_t)0, 4))
			return false;	// Not ping type
		
		
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
			return false; // error while reading png_structp
		
		
		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			png_destroy_read_struct(&png_ptr,
				(png_infopp)NULL, (png_infopp)NULL);
			return false;	//	error while reading info_struct
		}
		
		png_infop end_info = png_create_info_struct(png_ptr);
		if (!end_info)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr,
				(png_infopp)NULL); // error while reading info_struct
		}
		
		png_init_io(png_ptr, hTexFile);
		png_read_info(png_ptr, info_ptr);
		png_get_IHDR(png_ptr, info_ptr, &width, &height,
			(int *)&bit_depth, (int *)&color_type, (int *)&interlace_type,
			(int *)&compression_type, (int *)&filter_type);
		
		
		png_bytep *row_pointers= new png_bytep[height];
		memset(row_pointers,0,height*sizeof(png_bytep));
		
		unsigned int row;
		for (row = 0; row < height; row++)
		{
			int size;
			row_pointers[row] = new png_byte[(size=png_get_rowbytes(png_ptr, info_ptr))];
		}
		
		png_read_image(png_ptr, row_pointers);
		imageData = new unsigned char[height*width*4];
		unsigned int i,j;
		
		for(j = 0; j < height; j++)
		{
			for (i=0; i < width;i++)
			{
				int scale = 3 + (PNG_COLOR_TYPE_RGB_ALPHA==color_type);
				imageData[(j*width+i)*4 ] = (char)*(char*)( row_pointers[j] +i*scale) ;
				imageData[(j*width+i)*4 +1 ] = (char)*(char*)( row_pointers[j] +i*scale +1 ) ;
				imageData[(j*width+i)*4 +2 ] = (char)*(char*)( row_pointers[j] +i*scale +2 ) ;
				if (color_type==PNG_COLOR_TYPE_RGB)
					imageData[(j*width+i)*4 +3 ] = 255 ;
				else
					imageData[(j*width+i)*4 +3 ] = (char)*(char*)( row_pointers[j] +i*scale +3 ) ;
			}
		}
		
		
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		fclose(hTexFile);
		
		for (row = 0; row < height; row++)
			delete row_pointers[row];
		
		// It's loaded, or we died before we got here.  Now bind the texture
		// to an OpenGL handle.  Or whatever it actually does.
		if(texnr>0)
		{
			glBindTexture(GL_TEXTURE_2D, texnr);
			m_texID = texnr;
		}
		else
		{
			glGenTextures(1, &m_texID);
			glBindTexture(GL_TEXTURE_2D, m_texID);
		}
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		if(!m_mipmap)
			glTexImage2D( GL_TEXTURE_2D, 0, 3, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
		else
			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, imageData);
		
	}
	else
	{
		// If we couldn't find the file, just skip the texture setup routines.
		return m_texID;	// It might not have been set to anything, in which case it sends out a zero (the error code)
	}
	return m_texID;
}	

CTexture::CTexture(void)
{
	m_bpp = 0;
	m_height = 0;
	m_imageData = 0;
	m_mipmap = 0;
	m_texID = 0;	// I think 0 is a valid number here
	m_width = 0;
	next = 0;
	prev = 0;
	m_imageData = 0;
	m_Filename = 0;
}

CTexture::~CTexture()
{
	if (m_imageData)
		free(m_imageData);
	if (m_Filename)
		delete m_Filename;

}

