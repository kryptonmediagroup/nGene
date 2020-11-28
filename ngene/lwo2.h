/***************************************************************************/
//                          lwo2.h  -  description
//                             -------------------
//    begin                : Fri Dec 29 2000
  
/* 
 * LWOB2.H  - written by Gene Turnbow Hyper Entertainment, Inc.
 * 
 * $Log$
 *
 * A loader class for loading a Lightwave 6.5 model file into memory, then parse it 
 *				and expose its structure.  The ability to accept a string stream
 *				instead of loading it directly from the disk makes it usable
 *				in a variety of development environments, including gaming.
 *
 * This was based on a C program written by Yoshiaki Tazaki of D-Storm, Inc. in 1999,
 * which scanned an LWO2 file and dumped text to STDOUT.
 *
 * This is the core of the loader.  Anything specific to a particular API (i.e., OpenGL,
 * Direct3D, et cetera) should go in a class derived from this one.  I've included the class Mesh,
 * which makes the assumption that you're working with OpenGL.
 *
 */

#ifndef __LWOB2_H
#define __LWOB2_H

// If you're not going to be running this on an Intel chip, comment out the following line:
#define _INTEL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

#if !defined(_WIN32)
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "vector.h"


#define MAKE_ID(a,b,c,d)	\
	((unsigned long) (a)<<24 | (unsigned long) (b)<<16 | \
	 (unsigned long) (c)<<8 | (unsigned long) (d))

/* Universal IFF identifiers */

#define ID_FORM		MAKE_ID('F','O','R','M')
#define ID_LWO2		MAKE_ID('L','W','O','2')

/**  PRIMARY CHUNK ID  **/
#define ID_LAYR		MAKE_ID('L','A','Y','R')
#define ID_PNTS		MAKE_ID('P','N','T','S')
#define ID_VMAP		MAKE_ID('V','M','A','P')
#define ID_POLS		MAKE_ID('P','O','L','S')
#define ID_TAGS		MAKE_ID('T','A','G','S')
#define ID_PTAG		MAKE_ID('P','T','A','G')
#define ID_ENVL		MAKE_ID('E','N','V','L')
#define ID_CLIP		MAKE_ID('C','L','I','P')
#define ID_SURF		MAKE_ID('S','U','R','F')
#define ID_BBOX		MAKE_ID('B','B','O','X')
#define ID_DESC		MAKE_ID('D','E','S','C')
#define ID_TEXT		MAKE_ID('T','E','X','T')

#define ID_ICON		MAKE_ID('I','C','O','N')

/**  POLS TYPE  **/
#define ID_FACE		MAKE_ID('F','A','C','E')
#define ID_CRVS		MAKE_ID('C','U','R','V')
#define ID_PCHS		MAKE_ID('P','T','C','H')
#define ID_MBAL		MAKE_ID('M','B','A','L')
#define ID_BONE		MAKE_ID('B','O','N','E')

/**  PTAG TYPE  **/
#define ID_SURF		MAKE_ID('S','U','R','F')
#define ID_BNID		MAKE_ID('B','N','I','D')
#define ID_SGMP		MAKE_ID('S','G','M','P')
#define ID_PART		MAKE_ID('P','A','R','T')

/**  IMAGE SUB-CHUNK ID  */
#define ID_STIL		MAKE_ID('S','T','I','L')
#define ID_ISEQ		MAKE_ID('I','S','E','Q')
#define ID_ANIM		MAKE_ID('A','N','I','M')
#define ID_XREF		MAKE_ID('X','R','E','F')
#define ID_STCC		MAKE_ID('S','T','C','C')
#define ID_CONT		MAKE_ID('C','O','N','T')
#define ID_BRIT		MAKE_ID('B','R','I','T')
#define ID_SATR		MAKE_ID('S','A','T','R')
#define ID_HUE		MAKE_ID('H','U','E',' ')
#define ID_GAMM		MAKE_ID('G','A','M','M')
#define ID_NEGA		MAKE_ID('N','E','G','A')
#define ID_CROP		MAKE_ID('C','R','O','P')
#define ID_ALPH		MAKE_ID('A','L','P','H')
#define ID_COMP		MAKE_ID('C','O','M','P')
#define ID_IFLT		MAKE_ID('I','F','L','T')
#define ID_PFLT		MAKE_ID('P','F','L','T')

/**  ENVELOPE SUB-CHUNK  **/
#define ID_PRE		MAKE_ID('P','R','E',' ')
#define ID_POST		MAKE_ID('P','O','S','T')
#define ID_KEY		MAKE_ID('K','E','Y',' ')
#define ID_SPAN		MAKE_ID('S','P','A','N')
#define ID_CHAN		MAKE_ID('C','H','A','N')

/**  SURFACE SUB-CHUNK ID  */
#define ID_COLR		MAKE_ID('C','O','L','R')
#define ID_DIFF		MAKE_ID('D','I','F','F')
#define ID_LUMI		MAKE_ID('L','U','M','I')
#define ID_SPEC		MAKE_ID('S','P','E','C')
#define ID_REFL		MAKE_ID('R','E','F','L')
#define ID_TRAN		MAKE_ID('T','R','A','N')
#define ID_TRNL		MAKE_ID('T','R','N','L')
#define ID_GLOS		MAKE_ID('G','L','O','S')
#define ID_SHRP		MAKE_ID('S','H','R','P')
#define ID_BUMP		MAKE_ID('B','U','M','P')
#define ID_SIDE		MAKE_ID('S','I','D','E')
#define ID_SMAN		MAKE_ID('S','M','A','N')
#define ID_RFOP		MAKE_ID('R','F','O','P')
#define ID_RIMG		MAKE_ID('R','I','M','G')
#define ID_RSAN		MAKE_ID('R','S','A','N')
#define ID_RIND		MAKE_ID('R','I','N','D')
#define ID_CLRH		MAKE_ID('C','L','R','H')
#define ID_TROP		MAKE_ID('T','R','O','P')
#define ID_TIMG		MAKE_ID('T','I','M','G')
#define ID_CLRF		MAKE_ID('C','L','R','F')
#define ID_ADTR		MAKE_ID('A','D','T','R')
#define ID_GLOW		MAKE_ID('G','L','O','W')
#define ID_LINE		MAKE_ID('L','I','N','E')
#define ID_ALPH		MAKE_ID('A','L','P','H')
#define ID_AVAL		MAKE_ID('A','V','A','L')
#define ID_GVAL		MAKE_ID('G','V','A','L')
#define ID_BLOK		MAKE_ID('B','L','O','K')
#define ID_LCOL		MAKE_ID('L','C','O','L')
#define ID_LSIZ		MAKE_ID('L','S','I','Z')
#define ID_CMNT		MAKE_ID('C','M','N','T')

/**  TEXTURE LAYER  **/
#define ID_CHAN		MAKE_ID('C','H','A','N')
#define ID_TYPE		MAKE_ID('T','Y','P','E')
#define ID_NAME		MAKE_ID('N','A','M','E')
#define ID_ENAB		MAKE_ID('E','N','A','B')
#define ID_OPAC		MAKE_ID('O','P','A','C')
#define ID_FLAG		MAKE_ID('F','L','A','G')
#define ID_PROJ		MAKE_ID('P','R','O','J')
#define ID_STCK		MAKE_ID('S','T','C','K')
#define ID_TAMP		MAKE_ID('T','A','M','P')

/**  TEXTURE MAPPING  **/
#define ID_TMAP		MAKE_ID('T','M','A','P')
#define ID_AXIS		MAKE_ID('A','X','I','S')
#define ID_CNTR		MAKE_ID('C','N','T','R')
#define ID_SIZE		MAKE_ID('S','I','Z','E')
#define ID_ROTA		MAKE_ID('R','O','T','A')
#define ID_OREF		MAKE_ID('O','R','E','F')
#define ID_FALL		MAKE_ID('F','A','L','L')
#define ID_CSYS		MAKE_ID('C','S','Y','S')

/**  IMAGE MAP  **/
#define ID_IMAP		MAKE_ID('I','M','A','P')
#define ID_IMAG		MAKE_ID('I','M','A','G')
#define ID_WRAP		MAKE_ID('W','R','A','P')
#define ID_WRPW		MAKE_ID('W','R','P','W')
#define ID_WRPH		MAKE_ID('W','R','P','H')
#define ID_VMAP		MAKE_ID('V','M','A','P')
#define ID_AAST		MAKE_ID('A','A','S','T')
#define ID_PIXB		MAKE_ID('P','I','X','B')

/**  PROCEDURAL TEXTURE  **/
#define ID_PROC		MAKE_ID('P','R','O','C')
#define ID_COLR		MAKE_ID('C','O','L','R')
#define ID_VALU		MAKE_ID('V','A','L','U')
#define ID_FUNC		MAKE_ID('F','U','N','C')
#define ID_FTPS		MAKE_ID('F','T','P','S')
#define ID_ITPS		MAKE_ID('I','T','P','S')
#define ID_ETPS		MAKE_ID('E','T','P','S')

/**  GRADIENT **/
#define ID_GRAD		MAKE_ID('G','R','A','D')
#define ID_GRST		MAKE_ID('G','R','S','T')
#define ID_GREN		MAKE_ID('G','R','E','N')

/**  SHADER PLUGIN  */
#define ID_SHDR		MAKE_ID('S','H','D','R')
#define ID_DATA		MAKE_ID('D','A','T','A')

/**  VMAP TYPES */
#define ID_TXUV   MAKE_ID('T','X','U','V')	/* 2 floats */
#define ID_PICK   MAKE_ID('P','I','C','K')  /* 0 floats */
#define ID_MNVW   MAKE_ID('M','N','V','W')  /* 1 float */
#define ID_MORF   MAKE_ID('M','O','R','F')  /* 3 floats */
#define ID_SPOT   MAKE_ID('S','P','O','T')  /* 3 floats */
#define ID_RGB    MAKE_ID('R','G','B', 0)		/* Not sure about this one */ /* 3 floats */
#define ID_RGBA   MAKE_ID('R','G','B','A')	/* 4 floats */
#define ID_WGHT   MAKE_ID('W','G','H','T')	/* 1 float */

// Stupid programmer trick.  Just because it's not _WIN32 doesn't mean it's not an Intel chip
// we're compiling for.

#ifdef _INTEL
#define MSB2			_SwapTwoBytes
#define MSB4			_SwapFourBytes
#define LSB2(w)			(w)
#define LSB4(w)			(w)
#else
#define MSB2(w)			(w)
#define MSB4(w)			(w)
#define LSB2			_SwapTwoBytes
#define LSB4			_SwapFourBytes
#endif


// These are Newtek-style data types.
typedef char            I1;
typedef short           I2;
typedef int             I4;
typedef unsigned char   U1;
typedef unsigned short  U2;
typedef unsigned int    U4;
typedef float           F4;
typedef unsigned int    VX;
typedef float           FP4;
typedef float           ANG4;
typedef float           VEC12[3];
typedef float           COL12[3];
typedef char            ID4[5];
typedef char            S0[255];
typedef char            FNAM0[255];

typedef struct structVMAPElement
{
	VX idx;					// All VMAPS apply to a vertex.  This tells you which one this VMAP applies to.
	float *values;// This is the list of floats in this VMAP.  The parent structure tells you how many floats
									// to expect in this element.
} VMAPElement;

typedef struct structVMAP
{
	char *name;
	int mapType;
	int numElements;
	int floatsPerElement;
	VMAPElement *elements;
	struct structVMAP *next, *prev;
} VMAP;

typedef struct structPolygon
{
  unsigned long numVertices;
  VX *vertices;
  unsigned long flags;
  Vector3 polyNormal;	// surface normal for the polygon.  (I'm just learning this, but
			// I know that this isn't the same as vertex normals. It's the
			// unit vector cross-product of all the vertices in the list.
  float planeConstant;  									
  unsigned int tag; // identifies which surface material this polygon has 
} structPolygon;

typedef struct structLinkedPolygon
{
	structLinkedPolygon *next, *prev;
	structPolygon *p;
} LinkedPolygon;


typedef structPolygon * ptrPolygon;
typedef LinkedPolygon * ptrLinkedPolygon;

typedef struct structTMAP	// texture mapping
{
	struct structVector3 cntr,	// CNTR, SIZE, ROTA: center, size, rotation and falloff
		size,
		rota;
	U2 falloff_type;	// FALL gets three variables in a gulp
	struct structVector3 falloff_combinedAxesVector;
	VX falloff_envelope;	// 0 - cubic, 1-sphere, 2-Linear x, 3-Linear y, 4-Linearz
		
	char *oref;	// reference object (by name) (default is "(none)")
	unsigned short	csys;	// coordinate system.  0 for object coordinates (default if missing) or 1 for world coordinates
} TMAP;

typedef struct structPROC
{
	U2 axis;
	FP4 *value;
	char *funcstring;	// the command string and parameters of the function.  Which we'll never use.
	TMAP tMap;
} structPROC;

typedef struct structCLIP
{
	int clipIndex;
	char *clipName;
	structCLIP *prev, *next;
} CLIP;

typedef struct structGRAD
{
	char *inputParameter;	// Possible values are: 
							//		"Previous Layer"
							//		"Bump"
							//		"Slope"
							//		"Incidence Angle"
							//		"Light Incidence"
							//		"Distance to Camera"
							//		"Distance to Object"
							//		"X Distance to Object"
							//		"Y Distance to Object"
							//		"Z Distance to Object"
							//		"Weight Map"
	char *itemName;			//	Name of a scene item.  This is used when the input parameter is derived from
							//		a property of an item in the scene
	FP4 grst, gren;			// start and end of input range
	U2 grpt;				//	repeat mode (undefined)
	unsigned char *keys;
	U2 interpolation;		// 0 - linear, 1 - spline, 2 - step
} GRAD;

typedef struct structSHDR
{
	char stub;
	char *funcstring;
} SHDR;

typedef struct structIMAP	// Image mapping
{
	U2 projectionMode;	// PROJ: 0-planar, 1-cyl, 2-sphere, 3-cubic, 4-front proj, 5-UV
	U2 textureAxis;		// AXIS: 0-X, 1-Y, 2-Z
	VX clipIndex;		// IMAG: index of clip to use (which will be a text string describing the file it comes from)
	U2 wrapWidthMethod, wrapHeightMethod;	// WRAP: yields two variables on read
						// values are:
						//		0 - reset	areas outside image are assumed to be black.
						//		1 - repeat	image is repeated or tiled
						//		2 - mirror	like repeat, but alternate images are reversed
						//		3 - edge	the color is taken from the image's nearest edge pixel.
	float wrapWidthCycles[4];
	VX wrapWidthEnvelope;
	float wrapHeightCycles[4];
	VX wrapHeightEnvelope;
	U2 antialiasingFlags;
	float antialiasingStrength[4];
	U2 pixelBlendingFlags;
	// And there's a STCK variable, but it's reserved for future use and doesn't do anything right now.
	float textureAmplitude[4];
	VX textureAmplitudeEnvelope;
	char *vmapName;
	VMAP *ptrVMAP;
	TMAP tMap;
} IMAP;


	
typedef struct structSurfaceBlock	// These are surface subchunks
{
	U4	blockType;		// This will be GRAD, PROC, SHDR or IMAP
	char *ordinalString;	// used for identifying the processing sequence of surface blocks
	ID4 textureChannel;	// identifies which surface attribute this surface block modifies
	U2 enabled;			// True if the texture layer or shader should be evaluated during rendering.  Assumed true if missing during parse.
	U2 opacityType;		// 0-additive, 1-subtractive, 2-difference, 3-multiply, 4-divide, 5-alpha, 6-texture displacement
	FP4 opacity;
	VX opacityEnvelope;	// index of envelope to use for opacity

	IMAP *imap;
	GRAD *grad;
	structPROC *proc;
	SHDR *shdr;
	structSurfaceBlock *prev, *next;
	
}SurfaceBlock;


// TagID refers to the index of the surface in the array of surfaces
typedef struct structSurface
{
	char *surfaceName;
	char *textureName;	// there is no support for more than one texture per surface yet
	COL12 colr,				// base color
		lcol;				// undocumented, but might be a separate luminosity color
	F4 diff,				// diffusion
		lumi,				// luminosity
		spec,				// specularity
		refl,				// reflectivity
		tran,				// transparentcy
		trnl,				// translucency
		gloss,				// specular glossiness - controls the fallof of specular highlights
		shrp,				// diffuse sharpness
		bump,				// bump intensity
		rsan,				// 
		rind,
		clrh,
		clrf,
		adtr,
		gval,
		lsiz ;	
	unsigned int tagID;
	unsigned int colormapHandle;	// This'll be the handle for the color channel.  Later I'll expand this 
						// so we can have textures for each channel.
	VMAP *colormapVMAP;

	SurfaceBlock *surfaceBlocks;
} Surface;

typedef struct structTag
{
	structTag *prev, *next;
	char *tagName;
} Tag;

typedef struct structStrip
{
	Vertex *vertexList;
	int numVertices;
	
	Vector3 *normalsList;
	int numNormals;
	int surfaceIDX;	
} Strip;

// Required during object parsing.  The linked list is converted to an array once parsing is completed.
typedef struct structLinkedSurface
{
	structLinkedSurface *prev, *next;
	structSurface surf;
} LinkedSurface;

class CLWO2Loader
{

protected:
	void AttachVMAPStoIMAPS( void );	// Assumes that surfaces have already been converted to an array
	struct structLinkedSurface * m_stSurfaceUnderConstruction;

	void ComputeSurfaceNormalAndPlaneConstant(structLinkedPolygon *p);
	void ConvertToTrianglesOnly(void);	// Quads or worse? Break 'em down into triangles.
	void ConvertSurfaceListToArray(void);	// After this, the polygon array is nonsortable.
	bool BuildStripsByMaterial( void );
	ptrLinkedPolygon m_ParsedPolygons;

	void ConvertPolyListToArray(void);
	structTag * m_Tags;
	char *m_cFileBuf, *m_pFileCursor;	

	Strip *m_Strips;	// for processing the polygon list into strips by surface, later
	VMAP *m_VMAPS;	// Used for all sorts of things VMAPS are good for.
	CLIP *m_CLIPS;	// List of image map filenames with index numbers.  Stored for post-load processing for identifying which texture maps go with which surface subchunks.
	int m_NumStrips;
	
	unsigned short _SwapTwoBytes (unsigned short w);
	unsigned long _SwapFourBytes (unsigned long w);

	int seek_pad( int size );
	int read_u1( U1 *vals, int num );
	int read_u2( U2 *vals, int num );
	int read_u4( U4 *vals, int num );
	int read_i1( I1 *vals, int num );	// Apparently not used
	int read_i2( I2 *vals, int num );
	int read_f4( F4 *vals, int num );
	int read_col12( COL12 *col, int num);
	int read_vec12( Vector3 *vec, int num );
	int read_vx( VX *vx );
	int read_name( S0 name );
	int read_id4( ID4 id );
	void error(const char *message);
	U4 read_pnts(U4 nbytes);
	U4 read_bbox(U4 nbytes);
	U4 read_pols(U4 nbytes);
	U4 read_ptag(U4 nbytes);
	U4 read_vmap(U4 nbytes);
	U4 read_head(U4 nbytes, SurfaceBlock *block);
	U4 read_tmap(U4 nbytes);
	U4 read_blok(U4 nbytes);
	U4 read_surf(U4 nbytes);
	U4 read_clip(U4 nbytes);
	U4 read_envl(U4 nbytes);
	U4 read_tags(U4 nbytes);
	U4 read_layr(U4 nbytes);
	
	  /** Read the given number of vertices. */
  int readVertices(Vector3 *v, int numVertices);
  void SortPolygonsBySurface(void);
  /** Creates a new VMAP array of the desired
length having elements of the specified size */
  VMAP * CreateNewVMAP(char *name, int mapType, int mapSize);

  bool m_bBuildingSurface, m_bBuildingSurfaceBlock;


public:
	VMAPElement * GetVMAPForVertex(VMAP *vmap, int vidx);
	VMAP * GetVMAPByName(char *name);
	char * GetClipName(int idx);
	char * GetFullPathClipName(int idx);	// Given the clip index from a surface subchunk, return the filename it refers to.
	char * m_cFilename;
	int m_iErrCode;
	virtual int Load(char *fName);
	CLWO2Loader(void);
	virtual ~CLWO2Loader();
	
	//*************************************************************
	// These are for holding the raw data as read from the LWO file
	Vector3 *m_Vertices;

	unsigned int m_NumVertices;
	
	F4 m_BoundingBox[6];			// basically, six floats. This describes opposing corners of the bounding box.
	structLinkedSurface *m_LinkedSurfaces;		// This isn't an array, it's a linked list.
	structSurface *m_Surfaces;			// This is the finished array of surfaces.  
	unsigned int m_NumSurfaces;

	
	ptrPolygon *m_Polygons;       // used as final polygon list
	unsigned int m_NumPolygons;
  /** This is the name of the object as listed
within the LW6.5 model file. */
  char * m_stObjectName;
	//*************************************************************

};

enum {
	LWOLOADER_FILEIOERROR,
	LWOLOADER_NOTFOUND, 
	LWOLOADER_NOTANIFF, 
	LWOLOADER_NOTANOBJECT, 
	LWOLOADER_SUCCESS

	};



#endif
