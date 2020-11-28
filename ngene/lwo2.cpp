/***************************************************************************
                          lwo2.cpp  -  description
                             -------------------
    begin                : Fri Dec 29 2000

 ***************************************************************************/
/* 

 * LWOB2.H  - written by Gene Turnbow of Hyper Entertainment, Inc.
 * 
 * $Log$
 *
 * A loader class for loading a Lightwave 6.5 model file into memory, 
 * then parse it and expose its structure.  The ability to accept a 
 * string stream instead of loading it directly from the disk makes 
 * it usable in a variety of development environments, including gaming.
 *
 * This was based on a C program written by Yoshiaki Tazaki of 
 * D-Storm, Inc. in 1999, which scanned an LWO2 file and 
 * dumped text to STDOUT.
 *
 *
 */


#include "lwo2.h"

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#endif




// Constructor/Destructor
CLWO2Loader::CLWO2Loader(void)
{
	m_cFileBuf = NULL;
	m_pFileCursor = NULL;
	
	for (int i = 0; i < 6; i++)
		m_BoundingBox[i] = 0;
	
	m_NumVertices		= 0;
	m_NumPolygons		= 0;
	m_NumSurfaces		= 0;
	m_NumStrips			= 0;
	
	m_Polygons 			= NULL;
	m_Surfaces			= NULL;
	m_LinkedSurfaces	= NULL;	// Used only during parsing
	m_Vertices			= NULL;
	m_Strips 			= NULL;
	m_VMAPS				= NULL;
	
	m_iErrCode 			= 0;
	m_stObjectName		= NULL;
	m_Tags				= NULL;	// Used only during parsing
	m_CLIPS				= NULL;	

	m_bBuildingSurface	= false;
	m_bBuildingSurfaceBlock = false;
	
}

CLWO2Loader::~CLWO2Loader()
{
	// We may have been handed a strstream from the caller, but if we didn't,
	// we have some clean-up to do.
	if (m_cFileBuf)
	{
		delete m_cFileBuf;
		m_cFileBuf = NULL;

	}
	if (m_cFilename)
		free(m_cFilename);	// was malloced, not newed

	// Trash all the VMAPS.
	if (m_LinkedSurfaces)
		int i = 1;
		;

	if (m_Polygons)
	{
		structPolygon *aPoly = *m_Polygons;

		for (unsigned int ii = 0; ii < m_NumPolygons; ii++)
			delete aPoly->vertices;

		delete[] m_Polygons;
	}
			

	int slug;

	if (m_Strips)
		slug = 1;	// slug code
		

	if (m_Tags)
		slug = 1;	// slug code

	if (m_Vertices)
		slug = 1;	// slug code

	if (m_VMAPS)
	{
		structVMAP* aMap = m_VMAPS;

		while(aMap)
		{
			for (int ii = 0; ii < aMap->numElements; ii++)			
				delete aMap->elements[ii].values;
				
			delete[] aMap->elements;

			structVMAP *oldMap = aMap;
			aMap = aMap->next;
			delete oldMap;
		}
	}
	if (m_CLIPS)
	{
		CLIP *aClip = m_CLIPS;
		while(aClip->next)
		{
			delete aClip->clipName;
			aClip = aClip->next;
			delete aClip->prev;
		}
		delete aClip->clipName;
		delete aClip;
	}
			

}

/*
*  Swap byte order for WIN32
*/
unsigned short CLWO2Loader::_SwapTwoBytes (unsigned short w)
{
	unsigned short tmp;
	tmp =  (w & 0x00ff);
	tmp = ((w & 0xff00) >> 0x08) | (tmp << 0x08);
	return tmp;
}

unsigned long CLWO2Loader::_SwapFourBytes (unsigned long w)
{
	unsigned long tmp;
	tmp =  (w & 0x000000ff);
	tmp = ((w & 0x0000ff00) >> 0x08) | (tmp << 0x08);
	tmp = ((w & 0x00ff0000) >> 0x10) | (tmp << 0x08);
	tmp = ((w & 0xff000000) >> 0x18) | (tmp << 0x08);
	return tmp;
}

int CLWO2Loader::seek_pad( int size )
{
//	if (size>0)
//		m_ifsLWOFile->seekg(size,std::ios_base::cur);
	for (int cnt = 0; cnt < size; cnt++)
		m_pFileCursor++;
	return size;
}

int CLWO2Loader::read_u1( U1 *vals, int num )
{
	int s = sizeof(U1)*num;

	memcpy(vals,m_pFileCursor,s);
	seek_pad(s);
	return s;
}

int CLWO2Loader::read_u2( U2 *vals, int num )
{
	int   i, bytesread;
	int readsize = sizeof(U2) * num;
	memcpy(vals,m_pFileCursor,readsize);
	seek_pad(readsize);
	bytesread = num * sizeof(U2);
	for (i = 0; i < num; i++) 
		vals[i] = MSB2(vals[i]);
	return bytesread;
}


int CLWO2Loader::read_u4( U4 *vals, int num )
{
	int   i;
	int readsize = num * sizeof(U4);

	memset(vals,0,readsize);
	memcpy(vals,m_pFileCursor,readsize);
	seek_pad(readsize);
	for (i = 0; i < num; i++) 
		vals[i] = MSB4(vals[i]);
	return readsize;
}


int CLWO2Loader::read_i1( I1 *vals, int num )
{
	return read_u1((U1 *)vals, num);
}


int CLWO2Loader::read_i2( I2 *vals, int num )
{
	return read_u2((U2 *)vals, num);
}


int CLWO2Loader::read_f4( F4 *vals, int num )
{
	return read_u4((U4 *)vals, num);
}


int CLWO2Loader::read_col12( COL12 *col, int num )
{
	return read_f4((F4 *)col, 3 * num);
}


int CLWO2Loader::read_vec12( Vector3 *vec, int num )
{
	return read_f4((F4 *)vec, 3 * num);
}


int CLWO2Loader::read_vx( VX *vx )
{
	unsigned char ch,  bytesread = 0;
	
	*vx = 0L;

	ch = *m_pFileCursor++;
	if (ch == 0xff) 
	{
		ch = *m_pFileCursor++;
		*vx |= ch << 16;
		ch = *m_pFileCursor++;
		*vx |= ch << 8;
		ch = *m_pFileCursor++;
		*vx |= ch;
		bytesread = 4;
	} 
	else 
	{
		*vx |= ch << 8;
		ch = *m_pFileCursor++;
		*vx |= ch;
		bytesread = 2;
	}
	return bytesread;
}


int CLWO2Loader::read_name( S0 name )
{
	int   ch, bytesread = 0;
	
    do  
	{
        ch = *m_pFileCursor++;
        name[bytesread++] = ch;
    } while (ch);
	
	if (bytesread & 1) 
	{
        ch = *m_pFileCursor++;
		bytesread++;
	}
	return bytesread;
}


int CLWO2Loader::read_id4( ID4 id )
{
	memset(id,0,sizeof(ID4));
	memcpy(id,m_pFileCursor,sizeof(ID4)-1);
    for (int cnt = 0; cnt < 4; cnt++)
		m_pFileCursor++;
	return 4;
}


/*
* Prints an error message to stderr and exits.
*/

void CLWO2Loader::error(const char *message)
{
	FILE *log = fopen("LWO2Loader.log","w+");
#ifdef _DATADUMP	
	fprintf(log,"Error while parsing %s: %s\n", m_cFilename , message);
#endif
	fflush(log);
	fclose(log);
	exit(1);
}


/*
* Reads vertices, and returns bytes read.
*/

U4 CLWO2Loader::read_pnts(U4 nbytes)
{
		unsigned bytesread = 0;

    m_NumVertices = nbytes/sizeof(Vector3);
#ifdef _DATADUMP
    printf("PNTS [%d] nPts [%d]\n", nbytes, m_NumVertices);
#endif		
    m_Vertices = new Vector3[sizeof(Vector3) * m_NumVertices];
		bytesread = read_vec12(m_Vertices, m_NumVertices);

	for(unsigned int q=0; q < m_NumVertices; q++)
		m_Vertices[q].z = -m_Vertices[q].z;		// Lightwave uses a left-handed coordinate
																					// system.  OpenGL (and just about everybody else)
																					// seems to want a right-handed one.  As handed-ness
																					// is defined by the direction of the Z-axis,
																					// flipping this one value changes the handedness
																					// of the model.
																					

#ifdef _DATADUMP	
		// We'll keep this around for now, since it's useful for debugging.
    for (unsigned int i = 0; i < m_NumVertices; i++)
		printf("\t[%3d] [%f,%f,%f]\n", i, m_Vertices[i].x , m_Vertices[i].y, m_Vertices[i].z);
#endif	
    return(bytesread);
}


/*
* Reads Bounding Box.
* Returns the bytes to read.
*/

U4 CLWO2Loader::read_bbox(U4 nbytes)
{
		read_f4(m_BoundingBox, 6);
#ifdef _DATADUMP		
    printf("BBOX [%d]\n", nbytes);
    printf("\tMIN [%f,%f,%f]\n", m_BoundingBox[0], m_BoundingBox[1], m_BoundingBox[2]);
    printf("\tMAX [%f,%f,%f]\n", m_BoundingBox[3], m_BoundingBox[4], m_BoundingBox[5] );
#endif	
    return(nbytes);
}


/*
* Reads polygons, and returns bytes read.
*/

U4 CLWO2Loader::read_pols(U4 nbytes)
{
  U2	  numvert, flags;
  U4	  nPols, bytesread;
	VX    vx;
	ID4   id;
	int   n;
	struct structLinkedPolygon *polyPtr = NULL;
	m_ParsedPolygons = NULL;

	// Create enough polygon objects to handle the data we're about to read.
#ifdef _DATADUMP	
	printf("POLS [ bytes %ld]", nbytes);
#endif
	
	bytesread = 0L;
	nPols = 0L;
	
	bytesread += read_id4( id );
#ifdef _DATADUMP	
	printf(" [%s]\n", id);
#endif	
	
	while (bytesread < nbytes)
	{
		bytesread += read_u2(&numvert, 1);
		flags      = (0xfc00 & numvert) >> 10;
		numvert    =  0x03ff & numvert;
#ifdef _DATADUMP		
		printf("\t[%3u] NVERT[%3u] FLAG[%02x] <", nPols, numvert, flags);
#endif		
		nPols++;

		// Now, let's create a polygon, stick it on the end of the list, 
		// and fill in the blanks.
		polyPtr = m_ParsedPolygons;
		if (!polyPtr)
		{
			polyPtr = new structLinkedPolygon;
			memset(polyPtr,0,sizeof(structLinkedPolygon));
			polyPtr->p = new structPolygon;
			memset(polyPtr->p,0,sizeof(structPolygon));
			m_ParsedPolygons = polyPtr;	// Very important - this anchors the list so it can be walked again as the last step.
		}
		else
		{
			while(polyPtr)
			{
				if (polyPtr->next)
					polyPtr = polyPtr->next;
				else
				{
					polyPtr->next = new structLinkedPolygon; // Found the end..
					polyPtr->next->prev = polyPtr;
					polyPtr->next->next = NULL;
					polyPtr = polyPtr->next;
					polyPtr->p = new structPolygon;
					memset(polyPtr->p,0,sizeof(structPolygon));
					break;
				}
			}
		}
		
		// We've got a polygon, and it's attached to the end of the 
		// polygons list.  Now to populate it.
		polyPtr->p->numVertices = numvert;
		polyPtr->p->vertices = new VX[numvert];
		polyPtr->p->flags = flags; // not sure what flags do yet.
		polyPtr->p->tag = 0;   // don't know what material this polygon has yet.
		
		for (n = 0; n < (int)numvert; n++)
		{
			vx = 0;
			bytesread += read_vx( &vx );
			if (n+1 == numvert) 
			{
#ifdef _DATADUMP			
				printf("%u>\n", vx);
#endif			
				polyPtr->p->vertices[n] = vx;
			}
			else
			{
#ifdef _DATADUMP			
				printf("%u, ", vx);
#endif			
				polyPtr->p->vertices[n] = vx;
			}
		}
		// Special case: what happens if Lightwave feeds us a polygon with more than 3 sides, or
		// less than two?  Do we do bozo correction, or let it go?
		
		// Compute the surface normal for this polygon.
		// Only the first two vertices are used.
		
		// First, get a copy of the first three vertices in the polygon.
		// (Since we're dealing with triangles, this should be all of them.)
		ComputeSurfaceNormalAndPlaneConstant(polyPtr);
	}
	
	
	if (bytesread != nbytes) 
	{
		char msg[256];
		sprintf(msg,"??? %ld != %ld\n", bytesread, nbytes);
		error(msg);
	}
	
	// Moving this code to a point AFTER the poly-tag relationships have been
	// read and AFTER they've been sorted by tag ID.
	m_NumPolygons = nPols;

#if 0
	// As a last step, convert the polygon linked list into an array.  That way we can
	// refer to polygons by their index instead of having to walk the chain every time
	// we need to access one. I'm not sure what the ramifications of this will be later
	// on.  I don't know enough about what I'm doing yet, but this seems to be the right 
	// approach.
	m_Polygons = new structPolygon[nPols];
	memset(m_Polygons,0,sizeof(structPolygon));
	
	// Since structArrayedPolygon is an element inside structPolygon and has an identical structure,
	// I can just copy them from the linked list to the array without even looking at them.
	n = 0;
	polyPtr = l_ParsedPolygons;
	
	while(polyPtr)
	{
		memcpy((void *)&m_Polygons[n],(void *)&(polyPtr->p),sizeof(struct structPolygon));
		polyPtr = polyPtr->next;
		n++;
		if (n>nPols)
		{
#ifdef _DATADUMP
			printf("Error, processed more polygons than I thought I had.\n");
#endif
			exit(-1);
		}
	}
	m_NumPolygons = nPols;
	
	// Now I can destroy the array of parsed polygons.  We no longer need it.
	// This is going to leak like a bitch until I figure it out..
	// FIX THIS RIGHT AWAY.
	/*
	polyPtr = l_ParsedPolygons;
	while(polyPtr)
	{
		structPolygon *head = polyPtr->next;
		delete polyPtr;
		if (head)
			polyPtr = head;
	}
	*/
#endif
    return(bytesread);
    // That should do it.  I may have to write a dump routine later to verify that it's working.
}


/*
* Reads polygon tags, and returns bytes read.
*/

U4 CLWO2Loader::read_ptag(U4 nbytes)
{
    U2	 tag;
    U4	 nTags, bytesread;
	VX   vx;
	ID4  id;
#ifdef _DATADUMP	
    printf("PTAG [%3d]", nbytes);
#endif
	
	bytesread = 0L;
    nTags = 0L;
	
	bytesread += read_id4( id );
#ifdef _DATADUMP
	printf(" [%s]\n", id);
#endif

	structLinkedPolygon *currentPoly = m_ParsedPolygons;

	while (bytesread < nbytes)
	{
		bytesread += read_vx( &vx );
		bytesread += read_u2( &tag, 1 );
		// Apply the tag to the indicated polygon.  Now that the polygons are arrayed and indexed,
		// this is a simple task.
		if (!currentPoly)
		{
#ifdef _DATADUMP
		    printf("ERROR: polygon index exceeds available polygons.\n");
#endif
		    exit(-1);
		}

		currentPoly->p->tag = tag;
		currentPoly = currentPoly->next;

#ifdef _DATADUMP					
		printf("\tPOLY[%3u] TAG[%3d]\n", vx, tag);
#endif		
		//nTags++;
	}
	
#ifdef _DATADUMP	
	if (bytesread != nbytes) 
	{
		char msg[256];
		sprintf(msg,"??? %ld != %3d\n", bytesread, nbytes);
		error(msg);
	}
#endif
	
    return(bytesread);
}


/*
* Reads vertex mapping, and returns bytes read.
*/
// How many types of VMAP are there?  Let's just do TXUV's for right now.

U4 CLWO2Loader::read_vmap(U4 nbytes)
{
	U2	 dim;
	U4	 bytesread;
	VX   vx;
	F4   value;
	S0   name;
	ID4  id;
	int  vmaps_read = 0;
	VMAP *aVMAP = NULL;
	int  n;
	bool bSafeToBuild = false;

#ifdef _DATADUMP				
	printf("VMAP [%ld]", nbytes);	// This is the size of the entire VMAP block, but
    															// because the name of the block can be pretty much
    															// any size, it doesn't give us much of a clue as to
    															// how many elements to expect.
#endif
	
	bytesread = 0L;
	bytesread += read_id4( id );
	
	// Handle the various sub-chunks.
	int type = MAKE_ID(id[0],id[1],id[2],id[3]);

	if (type==ID_TXUV)			// We can't handle any other type of VMAP right now
		bSafeToBuild = true;

#ifdef _DATADUMP				
	printf(" [%s]", id);	// This'll be things like TXUV, and whatever
#endif	
	
	bytesread += read_u2  ( &dim, 1 );	// This is the number of floats per record
	bytesread += read_name( name );		// This is the string name of the VMAP.
	
#ifdef _DATADUMP	
	printf(" DIM [%ld] NAME [%s]\n", dim, name);
#endif	
	
	// Here's where we can take a guess at how large a block of UV pairs to make
	// UV Pairs count = (nbytes - bytesread)/sizeof(F4)*dim
	if (bSafeToBuild)
	{
	if (dim == 0)
		aVMAP = CreateNewVMAP(name,type,dim);
	else
		aVMAP = CreateNewVMAP(name, type, (nbytes - bytesread)/(sizeof(F4)*dim));

	if (!aVMAP)
		return false;	// If making the VMAP failed, we're out of memory or the subchunk type is unknown.
	}
						// Either way, we're toast.
						
	while (bytesread < nbytes)
	{
		bytesread += read_vx( &vx );
		
#ifdef _DATADUMP					
		printf("\tVERT[%ld] VALS[", vx);
#endif			
		if (bSafeToBuild)
			aVMAP->elements[vmaps_read].idx = vx;

		for (n = 0; n < (int) dim; n++)
		{
			bytesread += read_f4(&value, 1);
			if (bSafeToBuild)
				aVMAP->elements[vmaps_read].values[n] = value;
			
#ifdef _DATADUMP
			if (bSafeToBuild)
			{
			if (n+1 == dim) 
				printf("%f]\n", aVMAP->elements[vmaps_read].values[n]);
			else
				printf("%f, " , aVMAP->elements[vmaps_read].values[n]);
			if (n>1)
				printf(" (ignored) ");
			}
			else
				printf(" (ignored, not safe to build) ");
#endif		
		}
		vmaps_read++;
	}
	if (bytesread != nbytes) 
	{
		char msg[256];
		sprintf(msg,"??? %d != %d\n", bytesread, nbytes);
		error(msg);
	}
	
    return(bytesread);
}


/*
* Reads a BLOK header chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_head(U4 nbytes, SurfaceBlock* block)
{
    U4	    bytesread = 0, type, byteshold;
	U2      size, u2[4];
	VX      vx[4];
	F4      f4[4];
    ID4     id;
    S0		name;
	
	bytesread += read_name(name);
#ifdef _DATADUMP	
	printf("<%s>\n", name);
#endif	
	
	// Man, I thought I'd already done this.

	
    while (bytesread < nbytes) 
    {
		
		/* Handle the various sub-chunks. */
		bytesread += read_id4( id );
		bytesread += read_u2( &size, 1 );
		type = MAKE_ID(id[0],id[1],id[2],id[3]);
		
		byteshold = bytesread;
#ifdef _DATADUMP					
		printf("\t\t\t[%s] (%ld) ", id, size);
#endif		
		
		switch (type)
		{
		case ID_CHAN:
			bytesread += read_id4(id);
#ifdef _DATADUMP						
			printf("<%s>\n", id);
#endif			
			memcpy(&(block->textureChannel),id, sizeof(char) * 4);
			break;
		case ID_NAME:
			bytesread += read_name(name);	// I don't think this is ever called.
			break;
#ifdef _DATADUMP			
			printf("(Surprise..)<%s>\n", name);			// I'm pretty sure this isn't called here either.
#endif			

		case ID_OREF:
			bytesread += read_name(name);
#ifdef _DATADUMP			
			printf("(Surprise..)<%s>\n", name);			// I'm pretty sure this isn't called here either.
#endif			

			break;
		case ID_ENAB:
			bytesread += read_u2(u2, 1);
#ifdef _DATADUMP			
			printf("<%d>\n", u2[0]);
#endif			
			break;

		case ID_AXIS:
		case ID_NEGA:
			bytesread += read_u2(u2, 1);
#ifdef _DATADUMP			
			printf("<%d>\n", u2[0]);
#endif			
			break;
		case ID_OPAC:
			bytesread += read_u2(u2, 1);
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%d> <%f> <%d>\n", u2[0], f4[0], vx[0]);
#endif			
			break;
		default:
			bytesread += seek_pad(size);
#ifdef _DATADUMP						
			printf("(%d bytes)\n", size);
#endif			
			break;
		}
		if ((size - bytesread + byteshold) > 0) 
			bytesread += seek_pad((size - bytesread + byteshold));
		
    }
	
    return(bytesread);
}


/*
* Reads a TMAP chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_tmap(U4 nbytes)
{
    U4	    bytesread = 0, type, byteshold;
	U2      size, u2;
	VX      vx[4];
    ID4     id;
    S0		name;
	Vector3 vec;
	// If we're here, we can assume, a priori, that we're reading
	// data for a PROC or an IMAP.
	
	// Since we weren't passed a pointer to it, we'll have to 
	// find the surface block ourselves and figure it out.
	LinkedSurface *lastSurface;
	SurfaceBlock *lastSurfaceBlock;

	lastSurface = m_LinkedSurfaces;
	if (!lastSurface)	// Big booboo happened
	{
#if defined(_DATADUMP)
		printf("No surface while reading TMAP.\n");
#endif
		exit(-1);
	}
	// Walk to the end of the list.
	while(lastSurface->next)
		lastSurface = lastSurface->next;

	lastSurfaceBlock = lastSurface->surf.surfaceBlocks;
	if (!lastSurfaceBlock)
	{
#if defined(_DATADUMP)
		printf("No IMAP or PROC subchunk while reading TMAP.\n");
#endif
		exit(-1);
	}
	while(lastSurfaceBlock->next)
		lastSurfaceBlock = lastSurfaceBlock->next;


#ifdef _DATADUMP				
    printf("\n");
#endif
    while (bytesread < nbytes) 
    {
		
		/* Handle the various sub-chunks. */
		
		bytesread += read_id4( id );
		bytesread += read_u2( &size, 1);
		type = MAKE_ID(id[0],id[1],id[2],id[3]);
		
		byteshold = bytesread;
		
#ifdef _DATADUMP					
		printf("\t\t\t[%s] (%ld) ", id, size);
#endif

		switch (type)
		{

		case ID_CNTR:	// centering offset
			bytesread += read_vec12(&vec,1);
			bytesread += read_vx(vx);	// a bit of padding
			if (lastSurfaceBlock->imap)
				lastSurfaceBlock->imap->tMap.cntr = vec;			
			else if (lastSurfaceBlock->proc)
				lastSurfaceBlock->proc->tMap.cntr = vec;
			else
			{
#if defined(_DATADUMP)
				printf("Read centering offset for nonexistent TMAP (no IMAP, no PROC)\n");
#endif
				exit(-1);
			}
#if defined(_DATADUMP)
			printf("<%f, %f, %f>\n",vec.x, vec.y, vec.z);
#endif
			break;
			

		case ID_SIZE:
			bytesread += read_vec12(&vec,1);
			bytesread += read_vx(vx);	// a bit of padding
			if (lastSurfaceBlock->imap)
				lastSurfaceBlock->imap->tMap.size = vec;			
			else if (lastSurfaceBlock->proc)
				lastSurfaceBlock->proc->tMap.size = vec;
			else
			{
#if defined(_DATADUMP)
				printf("Read size for nonexistent TMAP (no IMAP, no PROC)\n");
#endif
				exit(-1);
			}
#if defined(_DATADUMP)
			printf("<%f, %f, %f>\n",vec.x, vec.y, vec.z);
#endif
			break;

		case ID_ROTA:	// rotation
			bytesread += read_vec12(&vec,1);
			bytesread += read_vx(vx);	// a bit of padding
			if (lastSurfaceBlock->imap)
				lastSurfaceBlock->imap->tMap.rota = vec;			
			else if (lastSurfaceBlock->proc)
				lastSurfaceBlock->proc->tMap.rota = vec;			
			else
			{
#if defined(_DATADUMP)
				printf("Read rotation for nonexistent TMAP (no IMAP, no PROC)\n");
#endif
				exit(-1);
			}			
#if defined(_DATADUMP)
			printf("<%f, %f, %f>\n",vec.x, vec.y, vec.z);
#endif
			break;

		case ID_FALL:	// falloff
			bytesread += read_u2(&u2, 1);
			bytesread += read_vec12(&vec, 1);
			bytesread += read_vx(&vx[0]);
			if (lastSurfaceBlock->imap)
			{
				lastSurfaceBlock->imap->tMap.falloff_type = u2;
				lastSurfaceBlock->imap->tMap.falloff_combinedAxesVector = vec;
				lastSurfaceBlock->imap->tMap.falloff_envelope = vx[0];
			}
			else if (lastSurfaceBlock->proc)
			{
				lastSurfaceBlock->proc->tMap.falloff_type = u2;
				lastSurfaceBlock->proc->tMap.falloff_combinedAxesVector = vec;
				lastSurfaceBlock->proc->tMap.falloff_envelope = vx[0];
			}
			else
			{
#if defined(_DATADUMP)
				printf("Read falloff data for nonexistent TMAP (no IMAP, no PROC)\n");
#endif
				exit(-1);
			}
#if defined(_DATADUMP)
			printf("<%d: %f, %f, %f: %d>\n",u2, vec.x, vec.y, vec.z, vx);
#endif
			break;

		case ID_OREF:	// Reference object - we're storing this for now, but we're not processing it. 
						// It would be a real bitch to support in the engine.
			bytesread += read_name(name);
			if (lastSurfaceBlock->imap)
			{
				lastSurfaceBlock->imap->tMap.oref = new char[sizeof(name)+1];
				strcpy(lastSurfaceBlock->imap->tMap.oref,name);
			}
			else if (lastSurfaceBlock->proc)
			{
				lastSurfaceBlock->proc->tMap.oref = new char[sizeof(name)+1];
				strcpy(lastSurfaceBlock->proc->tMap.oref, name);
			}
			else
			{
#if defined(_DATADUMP)
				printf("Read object reference name for nonexistent TMAP (no IMAP, no PROC)\n");
#endif
				exit(-1);
			}
#if defined(_DATADUMP)
			printf("<\"%s\">\n",name);
#endif
			break;

		case ID_CSYS:	// specifies the coordinate system.
			bytesread += read_u2(&u2, 1);
			if (lastSurfaceBlock->imap)
				lastSurfaceBlock->imap->tMap.csys = u2;
			else if (lastSurfaceBlock->proc)
				lastSurfaceBlock->proc->tMap.csys = u2;
			else
			{
#if defined(_DATADUMP)
				printf("Read coordinate system type for nonexistent TMAP (no IMAP, no PROC)\n");
#endif
				exit(-1);
			}
#if defined(_DATADUMP)
			{
				char translation[60];
				switch(u2)
				{
				case 0:
					sprintf(translation,"object coordinates");
					break;
				case 1:
					sprintf(translation,"world coordinates");
					break;
				}
			printf("<%d (%s)>\n",u2,translation);
			}
#endif
			break;
		default:
			bytesread += seek_pad(size);
#ifdef _DATADUMP						
			printf("(Unknown field)(%ld bytes)\n", size);
#endif
			break;
		}
		if ((size - bytesread + byteshold) > 0) 
			bytesread += seek_pad((size - bytesread + byteshold));
		
    }
	
    return(bytesread);
}


/*
* Reads a BLOK chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_blok(U4 nbytes)
{
    U4	    bytesread = 0, type, byteshold;
	U2      size, u2[4];
	U1		u1[10];
	S0      name;
	F4      f4[256];
	I2      i2[256];
	VX      vx;

    ID4     id;
	COL12   col;
	int     i, n;

	// Whatever kind of block it is, first we need to make a new structure to hold the data
	// we're about to read.

	LinkedSurface *lastLinkedSurface;
	SurfaceBlock *lastSurfaceBlock;

	// Find the last surface we were working on (and if we're reading a block, it's a given that
	// we're working on a surface.
	lastLinkedSurface = m_LinkedSurfaces;

	while(lastLinkedSurface->next)
		lastLinkedSurface=lastLinkedSurface->next;

	
    while (bytesread < nbytes) 
    {
		
		/* Handle the various sub-chunks. */
		bytesread += read_id4( id );
		bytesread += read_u2( &size, 1 );
		type = MAKE_ID(id[0],id[1],id[2],id[3]);
		
		byteshold = bytesread;
#ifdef _DATADUMP
		printf("\t\t[%s] (%ld) ", id, size);
#endif

		
		// Under certain circumstances, we'll want to make a new surface block.
		if (type == ID_IMAP || type == ID_PROC || type == ID_GRAD || type == ID_SHDR)
		{
			if (!lastLinkedSurface->surf.surfaceBlocks)
			{
				lastLinkedSurface->surf.surfaceBlocks = new SurfaceBlock;
				lastSurfaceBlock = lastLinkedSurface->surf.surfaceBlocks;
				memset(lastSurfaceBlock,0,sizeof(SurfaceBlock));
			}
			else
			{
				lastSurfaceBlock = lastLinkedSurface->surf.surfaceBlocks;
				while(lastSurfaceBlock->next)
					lastSurfaceBlock = lastSurfaceBlock->next;
				lastSurfaceBlock->next = new SurfaceBlock;
				memset(lastSurfaceBlock->next,0,sizeof(SurfaceBlock));
				lastSurfaceBlock = lastSurfaceBlock->next;
			}

			// While we're about things, fill in its type and read the header.
			lastSurfaceBlock->blockType = type;	// PROC, IMAP, GRAD or SHDR
			bytesread += read_head(size, lastSurfaceBlock);	// Some block processing happens in this call
		}

		switch (type)
		{
		case ID_IMAP:
			lastSurfaceBlock->imap = new IMAP;			
			memset(lastSurfaceBlock->imap,0,sizeof(IMAP));			
			break;

		case ID_PROC:	// This is for procedural texturing.  We're probably not doing that.
			lastSurfaceBlock->proc = new structPROC;
			memset(lastSurfaceBlock->proc,0,sizeof(structPROC));
			break;

		case ID_GRAD:	// This is for gradiants.  We're probably not doing that.
			lastSurfaceBlock->grad = new GRAD;
			memset(lastSurfaceBlock->grad,0,sizeof(GRAD));
			break;

		case ID_SHDR:	// This is for shaders.  We're probably not going to support those either.	
			lastSurfaceBlock->shdr = new SHDR;
			memset(lastSurfaceBlock->shdr,0,sizeof(SHDR));
			break;

		case ID_VMAP:		// In this case, VMAP is used as a subchunk, identifying the VMAP by name
			bytesread += read_name(name);
#ifdef _DATADUMP
			printf("<%s>\n", name);
#endif
			lastSurfaceBlock->imap->vmapName = new char[strlen(name)+1];
			strcpy(lastSurfaceBlock->imap->vmapName,name);
			break;

		case ID_FLAG:	//	No idea what this does at the moment.  In fact, it seems to be missing from the documentation.
			bytesread += read_u2(u2, 1);	// Gee, is this ever even called??
#ifdef _DATADUMP			
			printf("<%ld>\n", u2[0]);
#endif
			break;

		case ID_AXIS:	// projection axis
			bytesread += read_u2(&(u2[0]), 1);
#ifdef _DATADUMP			
			printf("<%ld>\n", u2[0]);
#endif
			switch(lastSurfaceBlock->blockType)
			{
			case ID_IMAP:
				lastSurfaceBlock->imap->textureAxis = u2[0];
				break;
			case ID_PROC:
				lastSurfaceBlock->proc->axis = u2[0];
				break;
#if defined(_DATADUMP)
				printf("Read axis for nonexistent PROC or IMAP.\n");
#endif
				exit(-1);
				break;
			}
			break;
			
			

		case ID_PROJ:	// projection type
			bytesread += read_u2(&u2[0], 1);
#ifdef _DATADUMP			
			printf("<%ld>\n", u2[0]);
#endif
			lastSurfaceBlock->imap->projectionMode = u2[0];
			break;

		case ID_PIXB:	// pixel blending flags
			bytesread += read_u2(&(u2[0]), 1);
#ifdef _DATADUMP			
			printf("<%ld>\n", u2[0]);
#endif
			lastSurfaceBlock->imap->pixelBlendingFlags = u2[0];			
			break;

		case ID_TMAP:	// This'll fill in the texture map for this IMAP block.  It has a lot of
						// noodly little variables, so it makes sense to make this a separate routine.
			bytesread += read_tmap(size);
			break;

		case ID_IMAG:	// Index of the clip map to use.
			bytesread += read_vx(&vx);
#ifdef _DATADUMP			
			printf("<%ld>\n", vx);
#endif
			lastSurfaceBlock->imap->clipIndex = vx;
			break;

		case ID_WRAP:
			bytesread += read_u2(u2, 2);
#ifdef _DATADUMP			
			printf("<%ld, %ld>\n", u2[0], u2[1]);
#endif
			lastSurfaceBlock->imap->wrapWidthMethod = u2[0];
			lastSurfaceBlock->imap->wrapHeightMethod = u2[1];
			break;

		case ID_WRPW:
			bytesread += read_f4(lastSurfaceBlock->imap->wrapWidthCycles, 1);
			bytesread += read_vx(&(lastSurfaceBlock->imap->wrapWidthEnvelope));
#ifdef _DATADUMP			
			printf("<%f> <%ld>\n", lastSurfaceBlock->imap->wrapWidthCycles[0], 
				lastSurfaceBlock->imap->wrapWidthEnvelope );
#endif
			break;

		case ID_WRPH:
			bytesread += read_f4(lastSurfaceBlock->imap->wrapHeightCycles, 1);
			bytesread += read_vx(&(lastSurfaceBlock->imap->wrapHeightEnvelope));
#ifdef _DATADUMP			
			printf("<%f> <%ld>\n", lastSurfaceBlock->imap->wrapHeightCycles[0], 
				lastSurfaceBlock->imap->wrapHeightEnvelope );
#endif
			break;

		case ID_TAMP:	// texture amplitude, used for bump mapping
			bytesread += read_f4(lastSurfaceBlock->imap->textureAmplitude, 1);
			bytesread += read_vx(&(lastSurfaceBlock->imap->textureAmplitudeEnvelope));
#ifdef _DATADUMP			
			printf("<%f> <%ld>\n", lastSurfaceBlock->imap->textureAmplitude[0], 
				lastSurfaceBlock->imap->textureAmplitudeEnvelope );
#endif
			break;

		case ID_VALU: // used only for procedural textures
			bytesread += read_f4(f4, size / sizeof(F4));
#ifdef _DATADUMP			
			for (i = 0; i < (int)(size / sizeof(F4)); i++) 
				printf("<%f> ", f4[i] );
			printf("\n");
#endif			
			lastSurfaceBlock->proc->value = new F4[size];
			memcpy(lastSurfaceBlock->proc->value,f4,size*sizeof(F4));
			break;

		case ID_AAST:	//	This refers to antialiasing.
			bytesread += read_u2(&(lastSurfaceBlock->imap->antialiasingFlags), 1);
			bytesread += read_f4(lastSurfaceBlock->imap->antialiasingStrength, 1);
#ifdef _DATADUMP						
			printf("<%ld> <%f>\n", lastSurfaceBlock->imap->antialiasingFlags, 
				lastSurfaceBlock->imap->antialiasingStrength );
#endif
			break;

		case ID_STCK:	// I'm pretty sure this doesn't do ANYTHING right now.
			bytesread += read_u2(u2, 1);
			bytesread += read_f4(f4, 1);
#ifdef _DATADUMP						
			printf("<%ld> <%f>\n", u2[0], f4[0] );
#endif
			break;

		case ID_GRST:	// These only affect the gradient in the user interface.  They don't affect rendering.
			bytesread += read_f4(f4,1);
			lastSurfaceBlock->grad->grst = f4[0];
#ifdef _DATADUMP						
			printf("GRST <%f>\n", f4[0] );
#endif
			break;

		case ID_GREN:
			bytesread += read_f4(f4, 1);
			lastSurfaceBlock->grad->grst = f4[0];
#ifdef _DATADUMP						
			printf("GREN <%f>\n", f4[0] );
#endif
			break;

		case ID_COLR:	// Surface base color.  This is what you see on a textureless object.
			bytesread += read_col12(&col, 1);
			bytesread += read_vx(&vx);
#ifdef _DATADUMP						
			printf("<%f,%f,%f> <%ld>\n", col[0], col[1], col[2], vx);
#endif		
			memcpy(lastLinkedSurface->surf.colr,col,sizeof(float) * 3);					
			break;

		case ID_FUNC:	// Procedural texture algorithm (often the name of a plugin.
			bytesread += n = read_name(name);

			if (lastSurfaceBlock->proc)
			{
			lastSurfaceBlock->proc->funcstring = new char[strlen(name)+1];
			strcpy(lastSurfaceBlock->proc->funcstring,name);	// And that's probably all we'll do with this for now.
			}
			if (lastSurfaceBlock->shdr)
			{
			lastSurfaceBlock->shdr->funcstring = new char[strlen(name)+1];
			strcpy(lastSurfaceBlock->shdr->funcstring,name);	// And that's probably all we'll do with this for now.
			}

#ifdef _DATADUMP
			printf("<%s> ", name);
#endif
			for (i = 0; i < (size -n); i++) 
			{
				bytesread += read_u1(u1, 1);
#ifdef _DATADUMP							
				printf("<0x%02x> ", u1[0]);
#endif
			}
#ifdef _DATADUMP
			printf("\n");
#endif
			break;

		case ID_FTPS:	// I don't recognize this tag at all.  No clue.
			n = size / sizeof(F4);
			bytesread += read_f4(f4, n);
#ifdef _DATADUMP						
			for (i = 0; i < n; i++)
				printf("<%f> ", f4[i]);
			printf("\n");
#endif
			break;

		case ID_ITPS:	// I'm kinda clueless on this one too.
			n = size / sizeof(I2);
			bytesread += read_i2(i2, n );
#ifdef _DATADUMP						
			for (i = 0; i < n; i++) 
				printf("<%ld> ", i2[i]);
			printf("\n");
#endif
			break;

		case ID_ETPS:	// Oh, come on, this is getting monotonous.
			while (size > 0) 
			{
				n = read_vx(&vx);
				bytesread += n;
				size -= n;
#ifdef _DATADUMP							
				printf("<%ld> ", vx);
#endif
			}
#ifdef _DATADUMP						
			printf("\n");
#endif
			break;

		default:
			bytesread += seek_pad(size);
#ifdef _DATADUMP
			printf("(%ld bytes) (unrecognized data)\n", size);
#endif
			break;
		}
		if ((size - bytesread + byteshold) > 0) 
			bytesread += seek_pad((size - bytesread + byteshold));
		
    }
	
    return(bytesread);
}


/*
* Reads a SURF chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_surf(U4 nbytes)
{
    U4	    bytesread = 0, type, byteshold;
    U2      size, u2[4];
    F4      f4[4];
	VX      vx[4];
    S0      name, source, s0;
	ID4     id;
	COL12   col;
	
	m_bBuildingSurface = true;

#ifdef _DATADUMP				
    printf("SURF [%ld]\n", nbytes);
#endif
	
	bytesread += read_name( name );
	bytesread += read_name( source );
	
#ifdef _DATADUMP				
    printf("[%s] [%s]\n", name, source);
#endif
	
    // Find the end of the surfaces chain and add a surface
    if (!m_LinkedSurfaces)
	{
		m_LinkedSurfaces = new structLinkedSurface;
		memset(m_LinkedSurfaces,0,sizeof(structLinkedSurface));
		m_LinkedSurfaces->prev = NULL;
		m_LinkedSurfaces->next = NULL;
		m_stSurfaceUnderConstruction = m_LinkedSurfaces;
	}
    else
	{
		// Find the tail of the chain
		m_stSurfaceUnderConstruction = m_LinkedSurfaces;
		while(m_stSurfaceUnderConstruction)
		{
			if (!m_stSurfaceUnderConstruction->next)	// At the tail? Extend the chain by one link.
			{
				m_stSurfaceUnderConstruction->next = new structLinkedSurface;
				memset(m_stSurfaceUnderConstruction->next,0,sizeof(structLinkedSurface));
				(m_stSurfaceUnderConstruction->next)->prev = m_stSurfaceUnderConstruction;
				m_stSurfaceUnderConstruction = m_stSurfaceUnderConstruction->next;
				break;
			}
			else
				m_stSurfaceUnderConstruction = m_stSurfaceUnderConstruction->next;
		}
	}
	
    // Now whatever just happened, surf points to the newly created surface.
    m_stSurfaceUnderConstruction->surf.surfaceName = new char[strlen(name)+1]; // I didn't use strdup because I'd have had to use free instead of delete to get rid of this.
    strcpy(m_stSurfaceUnderConstruction->surf.surfaceName,name);
	
    while (bytesread < nbytes) 
    {
		if ((nbytes - bytesread) < 6) 
		{
			bytesread += seek_pad((nbytes - bytesread));
			return(bytesread);
		}
		
		/* Handle the various sub-chunks. */
		bytesread += read_id4( id );
		bytesread += read_u2( &size, 1 );
		type = MAKE_ID(id[0],id[1],id[2],id[3]);
		
		byteshold = bytesread;
#ifdef _DATADUMP					
		printf("\t[%s] (%ld) ", id, size);
#endif
		
		switch (type)
		{
		case ID_COLR:
			bytesread += read_col12(&col, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f,%f,%f> <%ld>\n", col[0], col[1], col[2], vx[0]);
#endif
			m_stSurfaceUnderConstruction->surf.colr[0] = col[0];
			m_stSurfaceUnderConstruction->surf.colr[1] = col[1];
			m_stSurfaceUnderConstruction->surf.colr[2] = col[2];
			// Don't know what to do with VX..
			break;
		case ID_LCOL:
			bytesread += read_col12(&col, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f,%f,%f> <%ld>\n", col[0], col[1], col[2], vx[0]);
#endif
			m_stSurfaceUnderConstruction->surf.lcol[0] = col[0];
			m_stSurfaceUnderConstruction->surf.lcol[1] = col[1];
			m_stSurfaceUnderConstruction->surf.lcol[2] = col[2];
			// There's that vx value again..
			break;
		case ID_DIFF:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.diff = f4[0];
			break;		
		case ID_LUMI:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.lumi = f4[0];
			break;		
		case ID_SPEC:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.spec = f4[0];
			break;		
		case ID_REFL:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.refl = f4[0];
			break;		
		case ID_TRAN:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.refl = f4[0];
			break;		
		case ID_TRNL:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.trnl = f4[0];
			break;		
		case ID_GLOS:

			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP			
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.gloss = f4[0];
			break;		
		case ID_SHRP:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP			
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.shrp = f4[0];
			break;		
		case ID_BUMP:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.bump = f4[0];
			break;		
		case ID_RSAN:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.rsan = f4[0];
			break;		
		case ID_RIND:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.rind = f4[0];
			break;		
		case ID_CLRH:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.clrh = f4[0];
			break;
		case ID_CLRF:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.clrf = f4[0];
			break;
		case ID_ADTR:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.adtr = f4[0];
			break;		
		case ID_GVAL:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.gval = f4[0];
			break;
		case ID_LSIZ:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%f> <%ld>\n", f4[0], vx[0]);
#endif			
			m_stSurfaceUnderConstruction->surf.lsiz = f4[0];
			break;
		case ID_SIDE:
		case ID_RFOP:
		case ID_TROP:
			bytesread += read_u2(u2, 1);
			bytesread += seek_pad(size-sizeof(U2));
#ifdef _DATADUMP			
			printf("<%ld>\n", u2[0]);
#endif			
			break;
		case ID_SMAN:
			bytesread += read_f4(f4, 1);
#ifdef _DATADUMP						
			printf("<%f>\n", f4[0]);
#endif			
			break;
		case ID_RIMG:
		case ID_TIMG:
			bytesread += read_vx(vx);
#ifdef _DATADUMP						
			printf("<%ld>\n", vx[0]);
#endif			
			break;
		case ID_GLOW:
			bytesread += read_u2(u2  , 1);
			bytesread += read_f4(f4  , 1);
			bytesread += read_vx(vx);
			bytesread += read_f4(f4+1, 1);
			bytesread += read_vx(vx+1);
#ifdef _DATADUMP						
			printf("<%ld> <%f> <%ld> <%f> <%ld>\n", u2[0], f4[0], vx[0], f4[1], vx[1]);
#endif			
			break;
		case ID_LINE:
			bytesread += read_u2(u2, 1);
			if ( size > 2 ) 
			{
				bytesread += read_f4(f4, 1);
				bytesread += read_vx(vx);
				if (size > 8) 
				{
					bytesread += read_col12(&col, 1);
					bytesread += read_vx(vx+1);
#ifdef _DATADUMP						
					printf("<%ld> <%f> <%ld> <%f,%f,%f> <%ld>\n",
						u2[0], f4[0], vx[0], col[0], col[1], col[2], vx[1]);
#endif						
				}
#ifdef _DATADUMP										
				else 
					printf("<%ld> <%f> <%d>\n", u2[0], f4[0], vx[0]);
#endif					
			}
#ifdef _DATADUMP									
			else
				printf("<%ld>\n", u2[0]);
#endif				
			break;
		case ID_ALPH:
			bytesread += read_u2(u2, 1);
			bytesread += read_f4(f4, 1);
#ifdef _DATADUMP			
			printf("<%ld> <%f>\n", u2[0], f4[0]);
#endif			
			break;
		case ID_AVAL:
			bytesread += read_f4(f4, 1);
#ifdef _DATADUMP			
			printf("<%f>\n", f4[0]);
#endif			
			break;
		case ID_BLOK:	// This is the critical one.  It signifies the beginning of a new block of image mapping data
#ifdef _DATADUMP		
			printf("\n");
#endif			
			bytesread += read_blok(size);
			break;
		case ID_CMNT:
			memset( s0, 0x00, sizeof(s0) );
			bytesread += read_u1((unsigned char *)&s0[0], size);
#ifdef _DATADUMP			
			printf("<%s>\n", s0);
#endif			
			break;
		default:
			bytesread += seek_pad(size);
#ifdef _DATADUMP			
			printf("(%ld bytes)\n", size);
#endif			
		}
		if ((size - bytesread + byteshold) > 0) 
			bytesread += seek_pad((size - bytesread + byteshold));
		
    }
	
    return(bytesread);
}


/*
* Reads a CLIP chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_clip(U4 nbytes)
{
    U4	  bytesread = 0, bytes, type, index, byteshold;
    U2	  size, u2[4];
	U1    u1[4];
	I2    i2[5];
    F4    f4[4];
	VX    vx[4];
    S0    name, ext, server;
	ID4   id;
	
	bytesread += read_u4( &index, 1 );
	printf("CLIP [%ld] [%ld]\n",nbytes,index);	
	
    while (bytesread < nbytes) 
    {
		if ((nbytes - bytesread) < 6) 
		{
			bytesread += seek_pad((nbytes - bytesread));
			return(bytesread);
		}
		// _open(
		/* Handle the various sub-chunks. */
		bytesread += read_id4( id );
		bytesread += read_u2( &size, 1 );
		type = MAKE_ID(id[0],id[1],id[2],id[3]);
		byteshold = bytesread;
#ifdef _DATADUMP								
		printf("\t[%s] (%ld) ", id, size);
#endif		
		
		switch (type)

		{
		case ID_STIL:
			bytesread += read_name(name);
#ifdef _DATADUMP									
			printf("%s\n",name);
#endif			
			// Find the end of the clip list and tack this one on.
			{
				CLIP *aClip;

				if (!m_CLIPS)
				{
					m_CLIPS = new CLIP;
					aClip = m_CLIPS;
					memset(aClip,0,sizeof(CLIP));
				}
				else
				{
					aClip = m_CLIPS;
					while(aClip->next)
						aClip = aClip->next;
					aClip->next = new CLIP;
					memset(aClip->next,0,sizeof(CLIP));
					aClip->next->prev = aClip;
					aClip = aClip->next;
				}
				aClip->clipIndex = index;
				aClip->clipName = new char[strlen(name)+1];
				strcpy(aClip->clipName,name);
			}
			break;

		case ID_ISEQ:
			bytesread += read_u1( u1, 2 );
			bytesread += read_i2(i2, 2);
			bytesread += read_name(name);
			bytesread += read_name(ext);
#ifdef _DATADUMP									
			printf("<%ud> <%ud> <%ud> <%ud> <%s> <%s>\n", u1[0], u1[1],u2[0],u2[1],name,ext);
#endif			
			break;

		case ID_ANIM:
			bytesread += read_name(name);
			bytesread += read_name(server);
#ifdef _DATADUMP									
			printf("<%s> <%s>\n",name,server);
#endif			
			break;

		case ID_XREF:
			bytesread += read_u4(&index, 1);
			bytesread += read_name(name);
#ifdef _DATADUMP									
			printf("<%ud> <%s>\n", index, name );
#endif			
			break;

		case ID_ALPH:
			bytesread += read_vx(vx);
#ifdef _DATADUMP									
			printf("<%ud>\n",vx[0]);			
#endif			
			break;

		case ID_STCC:
			bytesread += read_i2(i2, 2);
			bytesread += read_name(name);
#ifdef _DATADUMP									
			printf("<%u> <%u> <%s>\n",i2[0], i2[1], name);			
#endif			
			break;

		case ID_CONT:
		case ID_BRIT:
		case ID_SATR:
		case ID_HUE:
		case ID_GAMM:
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx);
#ifdef _DATADUMP									
			printf("<%f><%u>\n",f4[0],vx[0]);
#endif			
			break;

		case ID_NEGA:
			bytesread += read_u2(u2, 1);	
#ifdef _DATADUMP									
			printf("<%u>\n",u2[0]);			
#endif			
			break;

		case ID_CROP:
			bytesread += read_f4(f4, 4);
#ifdef _DATADUMP									
			printf("<%f> <%f> <%f> <%f>\n",f4[0],f4[1],f4[2],f4[3]);
#endif			
			break;

		case ID_COMP:
			bytesread += read_vx(vx);
			bytesread += read_f4(f4, 1);
			bytesread += read_vx(vx+1);
#ifdef _DATADUMP									
			printf("<%u> <%f> <%u>\n", vx[0],f4[0],vx[1]);
#endif			
			break;

		case ID_IFLT:
		case ID_PFLT:
			bytes = bytesread;
			bytesread += read_name(name);
			bytesread += read_i2(i2, 1);
			bytesread += seek_pad(size-(bytesread-bytes));			
#ifdef _DATADUMP									

			printf("<%s> <%u>\n",name,i2[0]);
#endif			
			break;

		default:
			bytesread += seek_pad(size);
#ifdef _DATADUMP									
			printf("(%u bytes)\n",bytes);			
#endif			
		}
		if ((size - bytesread + byteshold) > 0) 
			bytesread += seek_pad((size - bytesread + byteshold));
		
    }
	
    return(bytesread);
}


/*
* Reads a ENVL chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_envl(U4 nbytes)
{
    U4   bytesread = 0, bytes, type, byteshold;
    U2	 size, u2[4], n, count;
	U1	 u1[4];
	I2	 index;
    F4	 f4[4];
    S0   name;
	ID4  id;
	
	bytesread += read_i2( &index, 1 );
    printf("ENVL [%d] [%d]\n", nbytes, index);
	
    while (bytesread < nbytes) 
    {
		if ((nbytes - bytesread) < 6) 
		{
			bytesread += seek_pad((nbytes - bytesread));
			return(bytesread);
		}
		
		/* Handle the various sub-chunks. */
		bytesread += read_id4( id );
		bytesread += read_u2( &size, 1 );
		type = MAKE_ID(id[0],id[1],id[2],id[3]);
		
		byteshold = bytesread;
#ifdef _DATADUMP								
		printf("\t[%s] (%d) ", id, size);
#endif		
		
		switch (type)
		{
		case ID_PRE:
		case ID_POST:
			bytesread += read_u2(u2, 1);
#ifdef _DATADUMP									
			printf("<%d>\n", u2[0]);
#endif			
			break;
		case ID_TYPE:
			bytesread += read_u2(u2, 1);
#ifdef _DATADUMP									
			printf("<%04x>\n", u2[0]);
#endif			
			break;
		case ID_KEY:
			bytesread += read_f4(f4, 2);
#ifdef _DATADUMP			
			printf("<%f> <%f>\n", f4[0], f4[1]);
#endif			
			if (size != 8) 
				size = 8;				// Apparently the Surface Editor has a bug that causes this.
			break;
		case ID_SPAN:
			bytes = bytesread;
			bytesread += read_id4(id);
#ifdef _DATADUMP									
			printf("<%s>", id);
#endif			
			count = (size - bytesread + bytes) / sizeof(F4);
			for (n = 0; n < count; n++) 
			{
				bytesread += read_f4(f4, 1);
#ifdef _DATADUMP										
				printf(" <%f>", f4[0]);
#endif				
			}
#ifdef _DATADUMP									
			printf("\n");
#endif			
			break;
		case ID_CHAN:
			bytes = bytesread;
			bytesread += read_name(name);
			bytesread += read_u2(u2, 1);
#ifdef _DATADUMP									
			printf("<%s> <%d>\n", name, u2[0]);
#endif			
			for (n = 0; n < (size-bytesread+bytes); n++) 
			{
				bytesread += read_u1(u1, 1);
#ifdef _DATADUMP										
				if (!(n % 8)) 
					printf("\t");

				printf("<0x%02x> ", u1[0]);

				if (!((n+1) % 8)) 
					printf("\n");
#endif
			}
#ifdef _DATADUMP													
			printf("\n");
#endif			
			break;
		case ID_NAME:
			bytesread += read_name(name);
#ifdef _DATADUMP													
			printf("<%s>\n", name);
#endif			
			break;
		default:
			bytesread += seek_pad(size);
#ifdef _DATADUMP													
			printf("(%d bytes)\n", size);
#endif			
		}
		if ((size - bytesread + byteshold) > 0) 	  
			bytesread += seek_pad((size - bytesread + byteshold));	  
    }
	
    return(bytesread);
}


/*
* Reads the TAGS chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_tags(U4 nbytes)
{
	U4    bytesread = 0L;
    S0    name;
#ifdef _DATADUMP	

	U4 n = 0;
    printf("TAGS [%d]\n", nbytes);
#endif
	
    while (bytesread < nbytes) 
    {
		bytesread += read_name(name);
#ifdef _DATADUMP												
		printf("\t[%u] [%s]\n",n++,name);		
#endif	
		structTag *thisTag;
		// Add to the linked list of tags
		if (!m_Tags)
		{
			m_Tags = new structTag;
			thisTag = m_Tags;
			thisTag->next = NULL;
			thisTag->prev = NULL;
		}
		else
		{
			thisTag = m_Tags;
			while(thisTag->next)
				thisTag = thisTag->next;
			thisTag->next = new structTag;
			thisTag->next->prev = thisTag;
			thisTag->next->next = NULL;
			thisTag = thisTag->next;
		}
		thisTag->tagName = strdup(name);
	}
    return(bytesread);
}


/*
* Reads the LAYR chunk, and returns bytes read.
*/

U4 CLWO2Loader::read_layr(U4 nbytes)
{
	U4       bytesread = 0L;
	U2       flags[2], parent[1];
	Vector3    pivot;
    S0       name;
#ifdef _DATADUMP											
    printf("\nLAYR [%d]", nbytes);
#endif
	
	/*  Layer no. and flags  */
	bytesread += read_u2(flags, 2);
	
	/*  Pivot point  */
	bytesread += read_vec12(&pivot, 1);
	bytesread += read_name(name);
#ifdef _DATADUMP											
	printf(" NO [%d] NAME [%s]\n", flags[0], name);
	printf("\tFLAGS [0x%04x] PIVOT [%f,%f,%f]\n", flags[1], pivot.x, pivot.y, pivot.z);
#endif	
	if ((nbytes-bytesread) == sizeof(U2)) 
	{
		bytesread += read_u2(parent, 1);
#ifdef _DATADUMP												
		printf("\tPARENT [%u]\n", parent[0]);
#endif		
	}
	
    return (bytesread);
}



int CLWO2Loader::Load(char *fName)
{
//	using namespace std;

	int fileHandle;
	unsigned long fileLength;



    U4	  datasize, bytesread = 0L;
    U4	  type, size;
    ID4   id;
	
	m_bBuildingSurface = false;

	m_cFilename = strdup(fName);

#if defined(_WIN32)
	if( (fileHandle = _open( fName, _O_RDONLY | _O_BINARY ))  <= 0 )   
	{
		_close(fileHandle);
		m_iErrCode = LWOLOADER_FILEIOERROR;
		return m_iErrCode;
	}
	
	fileLength = _filelength( fileHandle );
	m_cFileBuf = new char[fileLength];	// Allocate new space			
	memset(m_cFileBuf,0,fileLength);
	
	_read(fileHandle, m_cFileBuf, fileLength);	// buffer = all the data, p points to a pointer to our data.
	_close(fileHandle);
#else
	if( (fileHandle = open( fName, O_RDONLY ))  <= 0 )   
	{
		close(fileHandle);
		m_iErrCode = LWOLOADER_FILEIOERROR;
		return m_iErrCode;
	}

	struct stat stats;

	fstat(fileHandle, &stats);
	fileLength = stats.st_size;

	m_cFileBuf = new char[fileLength];	// Allocate new space			
	memset(m_cFileBuf,0,fileLength);	
	read(fileHandle, m_cFileBuf, fileLength);	// buffer = all the data, p points to a pointer to our data.
	close(fileHandle);
#endif
	// Set the file cursor to the beginning of the file image in memory.
	m_pFileCursor = m_cFileBuf;

    /* Make sure the Lightwave file is an IFF file. */
	read_id4(id);
	type = MAKE_ID(id[0],id[1],id[2],id[3]);
    if (type != ID_FORM)
	{
		error("Not an IFF file (Missing FORM tag)");
		m_iErrCode = LWOLOADER_NOTANIFF;
		return m_iErrCode;
	}
	
	if (!read_u4(&datasize, 1))
		error("Unable to get valid data size.");
#ifdef _DATADUMP											
    printf("FORM [%u]\n", datasize);
#endif
	
    /* Make sure the IFF file has a LWO2 form type. */
	bytesread += read_id4(id);
	type = MAKE_ID(id[0],id[1],id[2],id[3]);
    if (type != ID_LWO2)
	{
		error("Not a lightwave object (Missing LWO2 tag)");
		m_iErrCode = LWOLOADER_NOTANOBJECT;
		return m_iErrCode;
	}
#ifdef _DATADUMP											
    printf("LWO2\n");
#endif
	
    /* Read all Lightwave chunks. */
    while (bytesread < datasize) 
	{

		
		bytesread += read_id4(id);
		bytesread += read_u4(&size, 1);
		
		type = MAKE_ID(id[0],id[1],id[2],id[3]);
		
		switch (type) 
		{
        case ID_TAGS:	
			read_tags(size); 
			break;

        case ID_CLIP:	
			read_clip(size); 
			break;

        case ID_ENVL:
			read_envl(size); 
			break;

        case ID_LAYR:	
			read_layr(size); 
			break;

        case ID_PNTS:
			read_pnts(size); 
			break;

        case ID_BBOX:
			read_bbox(size); 
			break;

        case ID_POLS:
	  read_pols(size); 
	  break;

        case ID_PTAG:
			read_ptag(size); 
			break;

        case ID_VMAP:
			read_vmap(size); 
			break;

        case ID_SURF:
			read_surf(size); 
			break;

        default:
			seek_pad(size);
			printf("%s [%d]\n", id, size);
			break;
		}
		bytesread += size;		
    }
	m_iErrCode = LWOLOADER_SUCCESS;

	// Another closing step, convert the linked list of surfaces into an array of them.
	ConvertSurfaceListToArray();

	

	// One last bit of falderal: We need to sort the list of polygons by material.  We couldn't do this before,
	// because in Lightwave files the polygons are read first, then the materials are assigned per polygon via
	// a polygon-tag assignment list.
 
	// Sorting the list does two things.  First, it speeds up the rendering by not thrashing the cache as the 
	// textures swap in and out.  Second, it makes it easier to build strips of polygons when I get to the point
	// where that's important to think about.

	SortPolygonsBySurface();

	ConvertToTrianglesOnly();

	// Time to throw away the linked list wrapper.  Now that we know for certain how many polygons we're 
	// dealing with, we no longer need the conceit of the linked list of polygons.
	ConvertPolyListToArray();



	return m_iErrCode;
}



// First pass - make a guess at where each polygon goes in the list 
// by dividing the list space approximately evenly by the number of 
// surfaces represented.

// After that, finishing off with a simple bubble sort should get 
// the whole list sorted in three or four passes.

// Note that ConvertSurfaceListToArray() must be called FIRST, or this will bomb out.
void CLWO2Loader::SortPolygonsBySurface()
{

	structLinkedPolygon *curPoly = m_ParsedPolygons;

	bool swapped = true;

	// Simple bubble sort for now.  Fairly slow, but very easy to code.  I'll optimize this later.
	do 
	{
		swapped = false;
		if (curPoly)
		{
			if (curPoly->next)
			{
				if (curPoly->p->tag > curPoly->next->p->tag)
				{
					structPolygon *p = curPoly->p;
					curPoly->p = curPoly->next->p;
					curPoly->next->p = p;
					swapped = true;
				}
			}
		}
	} while(swapped);
}

void CLWO2Loader::ConvertPolyListToArray()
{
	// As a last step, convert the polygon linked list into an array.  That way we can
	// refer to polygons by their index instead of having to walk the chain every time
	// we need to access one. I'm not sure what the ramifications of this will be later
	// on.  I don't know enough about what I'm doing yet, but this seems to be the right 
	// approach.
	ptrLinkedPolygon polyPtr = m_ParsedPolygons;

	m_Polygons = new ptrPolygon[m_NumPolygons];
	memset(m_Polygons,0,sizeof(ptrPolygon)*m_NumPolygons);
	
	// Since structArrayedPolygon is an element inside structPolygon and has an identical structure,
	// I can just copy them from the linked list to the array without even looking at them.
	unsigned int n = 0;
	polyPtr = m_ParsedPolygons;
	

	while(polyPtr)
	{
		m_Polygons[n] = polyPtr->p;
		polyPtr = polyPtr->next;

		
		// Trash as we go
		if (polyPtr)	// There may not have been a next one
			if (polyPtr->prev)
				delete polyPtr->prev;
		n++;
		if (n>m_NumPolygons)
		{
			printf("Error, processed more polygons than I thought I had.\n");
//			exit(-1);
		}
	}
	
	
}


// Another post-parse process, moved here to reduce clutter in the CLWO2Loader::Load() member.
void CLWO2Loader::ConvertSurfaceListToArray()
{
// Find the end of the list, keeping count as we go.

	m_NumSurfaces = 0;
	structLinkedSurface *s = m_LinkedSurfaces;
	
	while(s)
	{
		m_NumSurfaces++;
		s = s->next;
	}


	// Now let's make the array of final surfaces
	m_Surfaces = new structSurface[m_NumSurfaces];

	// Okay, now let's copy them over.  But they need to be copied in the same
	// order in which their names are listed in the tags list.  That way
	// the surface indices will match the sequence of surfaces in the surface vector.

	int surfaceCount = 0;

	structTag *tag = m_Tags;

	while(tag)
	{
		s = m_LinkedSurfaces;

		// Find the surface with the same name
		while(s)
		{
			structSurface *source = &(s->surf);
			if (!strcmp(tag->tagName,source->surfaceName))
			{
				structSurface *t = &m_Surfaces[surfaceCount];
				memcpy(t,source,sizeof(structSurface));
				surfaceCount++;
				break;
			}		
			else
				s = s->next;
		}
		tag = tag->next;		
	}

	// Now trash the linked surfaces and the linked tag list.  We'll never need them again.
	s = m_LinkedSurfaces;
	while(s)
	{
		s = s->next;
		if (s)
		{
			m_LinkedSurfaces = s->prev;
			delete m_LinkedSurfaces;
		}
	}

	tag = m_Tags;
	while(tag)
	{
		tag = tag->next;
		if (tag)
		{
			m_Tags = tag->prev;
			delete m_Tags->tagName;
			delete m_Tags;
		}
	}

	m_Tags				= NULL;
	m_LinkedSurfaces	= NULL;	// Never to be used again.

	// Yet another closing step.  Go do all the VMAP lookups by name and assign pointers
	// to the found VMAPS to each IMAP subchunk, so we don't have to do this at render time.
	AttachVMAPStoIMAPS();

}


bool CLWO2Loader::BuildStripsByMaterial( void )
{
	unsigned int surfCnt;
	typedef structLinkedPolygon * ptrStructPolygon;

	structLinkedPolygon **ptrPolyListBySurface;

	// Create a strip for each material used in this object.
	m_Strips = new structStrip[m_NumSurfaces];
	for (surfCnt = 0; surfCnt < m_NumSurfaces; surfCnt++)
		m_Strips[surfCnt].surfaceIDX = surfCnt;	// Since they're declared in the same order as the surfaces appear in the surface list, this'll work.

	// Walk through the list of all the polygons and toss each polygon into a list.  They'll be sorted into strips later.
	ptrPolyListBySurface = new ptrStructPolygon[m_NumPolygons];
	memset(ptrPolyListBySurface,0,sizeof(ptrStructPolygon) * m_NumPolygons);

	// Stick polygons onto poly list pointers
	return true;
}

// This routine finds polygons with the wrong number of sides and makes them three-sided, either
// by vertex doubling or by chopping sets of three vertices off and making new polygons out of them 
// until it gets the correct number.

// I toyed with a few other names for this routine before deciding on this one.  Just be happy
// you didn't get BozoProofTheGeometry(void).
//

// BTW, this bit still isn't working right.  I think it's not reconstructing the poly list correctly.
//
void CLWO2Loader::ConvertToTrianglesOnly()
{
	structLinkedPolygon *lp = this->m_ParsedPolygons;

	while(lp)
	{
		int vCount = lp->p->numVertices;
		// One-sided and two-sided polygons are useful.  
		// However, polygons with more than three aren't.
		if (vCount > 3)
		{
			VX *vtxList;

			structLinkedPolygon *newLinkedPolygon = new structLinkedPolygon;
			
			// Tell the poly in front of us there's a new guy in line behind him
			if (lp->prev)

			{
				lp->prev->next = newLinkedPolygon;
				newLinkedPolygon->prev = lp->prev;
			}

			// Tell the current poly to fall in line behind us
			newLinkedPolygon->next = lp;
			lp->prev = newLinkedPolygon;

			// Now we build a triangle into the new polygon using vertex data from the original,
			//	A	,	A	,	A
			//	 0		 n-2	 n-1
			// and prune the original down by one vertices (A   , the last one).
			//											     n-1
			newLinkedPolygon->p = new structPolygon;
			memcpy(newLinkedPolygon->p,lp->p,sizeof(structPolygon));

			newLinkedPolygon->p->vertices = new VX[3];
			vtxList = newLinkedPolygon->p->vertices;

			//vtxList[2]=lp->p->vertices[0];
			//vtxList[1]=lp->p->vertices[lp->p->numVertices-1];
			//vtxList[0]=lp->p->vertices[lp->p->numVertices-2];
			vtxList[0]=lp->p->vertices[0];
			vtxList[1]=lp->p->vertices[1];
			vtxList[2]=lp->p->vertices[2];
			
			newLinkedPolygon->p->numVertices=3;

			// Now do the pruning
			lp->p->numVertices--;
			vtxList = new VX[lp->p->numVertices];

			// We could have many more than four vertices left over, so we do this by memcpy instead of 
			// direct assignment.

			memcpy(vtxList,lp->p->vertices,lp->p->numVertices*sizeof(VX));
			delete lp->p->vertices;
			lp->p->vertices = vtxList;

			// Recompute the surface normal
			ComputeSurfaceNormalAndPlaneConstant(newLinkedPolygon);
			ComputeSurfaceNormalAndPlaneConstant(lp);
			
			lp = lp->prev->prev;
		}
		lp = lp->next;
	}
}

void CLWO2Loader::ComputeSurfaceNormalAndPlaneConstant(structLinkedPolygon *polyPtr)
{
		Vector3 sA, sB, sC;
		sA = m_Vertices[polyPtr->p->vertices[2]];	
		sB = m_Vertices[polyPtr->p->vertices[1]];	
		sC = m_Vertices[polyPtr->p->vertices[0]];
		
		// find the midpoint of the first segment (our first vector)
		Vector3 mA = sB;
		mA -= sA;
		
		// find the midpoint of the second segment (our second vector)
		Vector3 mB = sC;
		mB -= sB;
		
		// get the crossproduct of the two midpoint (our normal vector)
		// normalize it and store it.
		Vector3 normal;
		crossproduct(normal, mA, mB);
#ifdef _DATADUMP
		printf("Normal of NEXT POLY prior to normalization: %6.9f, %6.9f, %6.9f\n",
			normal.x, normal.y, normal.z);
#endif
		normalize(normal);
		
		// This will need testing to be certain that the
		// cross product is being computed and stored correctly.
		structPolygon *gon = polyPtr->p;
		gon->polyNormal = normal;
		
		// Compute the plane constant (one vertex dotted with the normal vector)
		gon->planeConstant = 
			-(normal.x * m_Vertices[gon->vertices[0]].x +
			normal.y * m_Vertices[gon->vertices[0]].y +
			normal.z * m_Vertices[gon->vertices[0]].z);
#if 0		
//#ifdef _DATADUMP
		printf("Vertex one:   %6.9f, %6.9f, %6.9f\n",sA.x,sA.y,sA.z);
		printf("Vertex two:   %6.9f, %6.9f, %6.9f\n",sB.x,sB.y,sB.z);
		printf("Vertex three: %6.9f, %6.9f, %6.9f\n",sC.x,sC.y,sC.z);
		printf("Midpoint of segment one: %6.9f, %6.9f, %6.9f\n",mA.x,mA.y,mA.z);
		printf("Midpoint of segment two: %6.9f, %6.9f, %6.9f\n",mB.x,mB.y,mB.z);
		printf("Normalized cross product: %6.9f, %6.9f, %6.9f\n\n",normal.x,normal.y,normal.z);
		printf("Normalized cross product from the stored polyNormal: %6.9f, %6.9f, %6.9f\n",
			gon->polyNormal.x,gon->polyNormal.y,gon->polyNormal.z);
		printf("Plane Constant is %6.9f\n",gon->planeConstant);		
		printf("----------------------------------------------\n\n");
#endif	
		

}
/** Creates a new VMAP array of the desired
length having elements of the specified size */
structVMAP * CLWO2Loader::CreateNewVMAP(char *name, int mapType, int elementCount)
{
	int makeThisManyFloats = 0;
	switch(mapType)
	{
		case ID_TXUV:	// 2 floats - texture UV's
			makeThisManyFloats = 2;
			break;
		case ID_PICK: // 0 floats
			makeThisManyFloats = 0;	
			break;
		case ID_MNVW: // 1 float	- subpatch weights
			makeThisManyFloats = 1;
			break;
		case ID_MORF: // 3 floats	- morph relative displacement
			makeThisManyFloats = 3;
			break;
		case ID_SPOT: // 3 floats	- spot absolute displacement
			makeThisManyFloats = 3;
			break;
		case ID_RGB:	// 3 floats	- vertex color
			makeThisManyFloats = 3;
			break;
		case ID_RGBA: // 4 floats - vertex color with alpha
			makeThisManyFloats = 4;
			break;
		case ID_WGHT:	// 1 float	-	weight map
			makeThisManyFloats = 1;
			break;
		default:			// If we got here, I dunno what the hell kind of VMAP it is.
			break;
	}
	
	if (!m_VMAPS)
	{
		m_VMAPS = new structVMAP;
		m_VMAPS->name = new char[strlen(name)+1];
		m_VMAPS->mapType = mapType;
		strcpy(m_VMAPS->name,name);
		m_VMAPS->next = 0;	// no next or prev, as we're the first
		m_VMAPS->prev = 0;
		m_VMAPS->elements = new structVMAPElement[elementCount];
		m_VMAPS->numElements = elementCount;
		m_VMAPS->floatsPerElement = makeThisManyFloats;
		for(int ii = 0; ii < elementCount; ii++)
		{
			if (makeThisManyFloats == 0)
				m_VMAPS->elements[ii].values = 0; // NULL
			else
			{
				m_VMAPS->elements[ii].values = new float[makeThisManyFloats];
				memset(m_VMAPS->elements[ii].values,0,sizeof(float)*makeThisManyFloats);
			}
		}
		return m_VMAPS;
	}
	else
	{
		structVMAP *aMap = m_VMAPS;
		while(aMap->next)
			aMap = aMap->next;
		aMap->next = new structVMAP;
		aMap->name = new char[strlen(name)+1];
		aMap->mapType = mapType;
		strcpy(aMap->name,name);
		aMap->next->prev = aMap;
		aMap->next->next = 0;
		aMap = aMap->next;
		aMap->elements = new structVMAPElement[elementCount];
		aMap->numElements = elementCount;
		aMap->floatsPerElement = makeThisManyFloats;
		for(int ii = 0; ii < elementCount; ii++)
		{
			if (makeThisManyFloats == 0)
				m_VMAPS->elements[ii].values = 0; // NULL
			else
				m_VMAPS->elements[ii].values = new float[makeThisManyFloats];
		}
		return aMap;
	}
}
		
// Given the clip index from a surface subchunk, find the associated filename and return it as a filename.
char * CLWO2Loader::GetFullPathClipName(int idx)
{
	CLIP *aClip;

	if (!m_CLIPS)
		return NULL;

	aClip = m_CLIPS;
	while(aClip)
	{
		if (aClip->clipIndex == idx)
			return aClip->clipName;
		else
			aClip = aClip->next;
	}
	return NULL;
}

char * CLWO2Loader::GetClipName(int idx)
{
	CLIP *aClip;

	if (!m_CLIPS)
		return NULL;

	aClip = m_CLIPS;
	while(aClip)
	{
		if (aClip->clipIndex == idx)
		{
			if (strchr(aClip->clipName,'/'))
				return (strrchr(aClip->clipName,'/')+1);
			else
				return aClip->clipName;
		}
		else
			aClip = aClip->next;
	}
	return NULL;

}

void CLWO2Loader::AttachVMAPStoIMAPS()
{
	for (unsigned int ii = 0; ii < m_NumSurfaces; ii++ )
	{
		SurfaceBlock *b = m_Surfaces[ii].surfaceBlocks;
		while(b)
		{
			if (b->imap)
			{
				if (b->imap->vmapName)	// if it's null, there's no VMAP.
				{
					VMAP *v = m_VMAPS;
					while(v)
					{
						if (!strcmp(b->imap->vmapName,v->name))
						{
							b->imap->ptrVMAP = v;
							// And here's a hack to put the VMAP at the top level of the
							// surface so we don't have to hunt through the subchunks
							// at render time.  I'll think of a smoother way to do this
							// later.  I agree that this is kind of redundant, but it's just
							// a hack.
							m_Surfaces[ii].colormapVMAP = v;
							break;
						}
						v = v->next;
					}
				}

			}
			b = b->next;
		}
	}
}

VMAP * CLWO2Loader::GetVMAPByName(char *name)
{
	if (!m_VMAPS)
		return NULL;

	VMAP *v = m_VMAPS;
	while(v)
	{
		if (!strcmp(v->name,name))
			return v;
		else
			v = v->next;
	}
	return NULL;	// Couldn't find it.
}

VMAPElement * CLWO2Loader::GetVMAPForVertex(VMAP *vmap, int vidx)
{
	VMAP *v = vmap;
	
	for (int cnt = 0; cnt < v->numElements; cnt++)
	{
//		char msg[512];
//		sprintf(msg,"v->elements[cnt].idx = %d, match check to %d\n",v->elements[cnt].idx,vidx);
//		OutputDebugString(msg);

		if (v->elements[cnt].idx == vidx && v->mapType == ID_TXUV)
			return &(v->elements[cnt]);
	}
	return NULL;	// No matching VMAP for specified index found.

}
