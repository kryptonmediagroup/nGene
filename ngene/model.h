// Model.h: interface for the CModel class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_MODEL_H)
#define _MODEL_H

#include "node.h"
#include "mesh.h"
#include "texturemanager.h"

class CModel : public CNode, CMesh
{
public:
	void Render(void);
	virtual int Load(char *filename, CTextureManager *textureManager = NULL);
	CModel();
	virtual ~CModel();

};

#endif 


