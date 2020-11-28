// Model.cpp: implementation of the CModel class.
//
//////////////////////////////////////////////////////////////////////

#include "model.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModel::CModel()
{
	m_iNodeType = NODE_MODEL;	
}

CModel::~CModel()
{
	// If children, do we delete them?
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
			

	if (m_Strips)
		;

	if (m_Tags)
		;

	if (m_Vertices)
		;

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

}


int CModel::Load(char *filename, CTextureManager *textureManager)
{
		int fn = strlen(filename);
		CNode::m_strName = new char[fn-4+1];
		strncpy(CNode::m_strName,filename,fn-3);
		CNode::m_strName[fn-4]=0;
		// Chuck the extension and use what's left as the model name
		return CMesh::Load(filename, textureManager);
}

void CModel::Render()
{
	int lastMaterialTagID = -1;
	
	glScalef(m_CurrentScale.x, m_CurrentScale.y, m_CurrentScale.z);
	
	// OK, let's start drawing our planer quads.
	GLuint activeTexture = 0;

	for (unsigned int polyCount = 0; polyCount <  m_NumPolygons; polyCount++)
	{
		activeTexture =  m_Surfaces[ m_Polygons[polyCount]->tag].colormapHandle;
		VMAP *vMap = NULL;
		
		if (activeTexture)
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,activeTexture);
			vMap =  m_Surfaces[ m_Polygons[polyCount]->tag].colormapVMAP;
		}
		else
			glDisable(GL_TEXTURE_2D);
		
		
		glBegin(GL_TRIANGLES); 

		Vector3 *v3 = &( m_Polygons[polyCount]->polyNormal);  	 	 				
		glNormal3f( v3->x, v3->y, v3->z );

		int tagID =  m_Polygons[polyCount]->tag;
		
		float *vf = &( m_Surfaces[tagID].colr[0]);
		glColor3f( *(vf), *(vf+1), *(vf+2));	

		// Now that we've got all the setup for this poly taken care of,
		// let's express the polygon list.  For the moment, we're
		// accepting any number of vertices, because OpenGL can.  Later
		// I'll chop things up into tri's during the preprocessing phase.
		
		
		unsigned int vertCount =  m_Polygons[polyCount]->numVertices;
		
		for(unsigned int vCount = 0; vCount < vertCount; vCount++)
		{
			// No texture coordinates yet, because I'm not doing textures yet.
			// That would be a glTexCoord2f() call..
			unsigned int vertexIndex = 	m_Polygons[polyCount]->vertices[vCount];
			if (vMap)
			{
				VMAPElement *el = NULL;
				float *coord = NULL;
				
				el =  GetVMAPForVertex(vMap, vertexIndex);					
				if (el)
				{
					coord = el->values;
					if (coord)
						glTexCoord2f(coord[0],coord[1]);
				}
			}
			
			Vector3 *v = &( m_Vertices[ vertexIndex]);
			glVertex3f( v->x, v->y, v->z );			
		}
		glEnd();
	}
	
	// All polygons have been drawn.
	
}





