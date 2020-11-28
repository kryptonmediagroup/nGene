// Node.h: interface for the CNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NODE_H__9709C96C_915D_4EF5_9D8F_B40A5BA7EC48__INCLUDED_)
#define AFX_NODE_H__9709C96C_915D_4EF5_9D8F_B40A5BA7EC48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "vector.h"

enum NODE_TYPES {NODE_MODEL, NODE_TRANSFORM, NODE_LIGHT, NODE_CAMERA, NODE_BONE, NODE_ENVIRONMENT, NODE_RAWNODE};
 
class CNode  
{
protected:
public:
	void SetName(char *name);
	void SetRotationf(float x, float y, float z);
	void SetTranslationf(float x, float y, float z);
	void JustRender(void);
	void JustLight(void);
	void TransformAndRender(void);
	void TransformAndLight(void);
	void AddChild(CNode *adoptee);
	CNode * FindItemByName(char *soughtName);
	CNode * FindItemByID(long itemID);
	CNode(CNode &n);
	char * m_strName;
	CNode();
	virtual ~CNode();

	long m_lNodeID;	// Node ID's are used mainly during construction of the scene graph from parts
					// as they're read in from the Lightwave scene file.  They're hex numbers, 
					// having the format TXXXNNNN or TBBBNNNN, where T = type, N = item number, 
					// and B = bone number if T = 4.  Item types are 1 = object, 2 = light, 
					// 3 = camera, 4 = bone.

	long m_lParentNodeID;	// ID of parent node, used only during scene construction.

	// Apparently all nodes, regardless of type, need this in order to participate in the scene graph.
	structVector3 m_CurrentRotation;
	structVector3 m_CurrentTranslation;
	structVector3 m_CurrentScale;	

	enum NODE_TYPES m_iNodeType;
	CNode *m_Children, *m_Next, *m_Prev, *m_Parent;

};

#endif // !defined(AFX_NODE_H__9709C96C_915D_4EF5_9D8F_B40A5BA7EC48__INCLUDED_)
