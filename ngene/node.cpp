// Node.cpp: implementation of the CNode class.
//
//////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
#include <windows.h>
#endif

#include "model.h"
#include "light.h"

#include <string.h>

#include <GL/gl.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef NULL
#define NULL 0
#endif

CNode::CNode()
{
	m_Children = NULL;
	m_Next = NULL;
	m_Prev = NULL;
	m_Parent = NULL;	// May never need this...

	m_iNodeType = NODE_RAWNODE;
	m_strName = NULL;
	m_lNodeID = 0;
	m_lParentNodeID = 0;
	memset(&m_CurrentRotation,0,sizeof(structVector3));
	memset(&m_CurrentScale,0,sizeof(structVector3));
	memset(&m_CurrentTranslation,0,sizeof(structVector3));

}

CNode::~CNode()
{
	if (m_strName)
		delete m_strName;
	// Should we delete all the children recursively? Hmm..probably.
	// If we all do this, the result of deleting the top node of a tree
	// should be an extirpated tree.
	
	if (m_Next)
		delete m_Next;
	if (m_Children)
		delete m_Children;
}

CNode::CNode(CNode &n)
{
	m_Children = NULL;
	m_Next = NULL;
	m_Prev = NULL;
	m_iNodeType = n.m_iNodeType;
	m_strName = new char[strlen(n.m_strName)];
	strcpy(m_strName,n.m_strName);
}


// A recursive routine, this function drills down through the heirarchy looking for the item with
// the matching ID.  If no match is found, it returns -1.  Note that the caller will have to recast
// the pointer to something a little less generic before attempting to use it for much.
CNode * CNode::FindItemByID(long itemID)
{
	CNode *foundNode = NULL;
	CNode *aNode = NULL;

	// Is it us?
	if (m_lNodeID == itemID)
		return this;

	// It's not us.  Query all our siblings, then all our children.
	// If we're at the head of a list of siblings, we've probably been asked to query 
	// all our siblings by our parent.
	if (m_Next && !m_Prev)
	{
		aNode = m_Next;
		while(aNode)
		{
			foundNode = aNode->FindItemByID(itemID);
			if (foundNode != NULL)
				return foundNode;
			else
				aNode = aNode->m_Next;
		}
	}

	// If we were head of a list of items, all siblings have now been queried.
	// Now let's talk to the kids.  They'll either come up with the goods, or they won't.
	if (m_Children)
		return m_Children->FindItemByID(itemID);
	else
		return NULL;	// If none of our siblings had it, and we have no children either, we came up empty.

}

// A recursive routine, this function drills down through the heirarchy looking for the item with
// the matching item name (Everything in the scene graph is assumed to have a unique name.  If this
// isn't the case, this routine will simply return the first match it finds.)
//  If no match is found, it returns -1.  Note that the caller will have to recast
// the pointer to something a little less generic before attempting to use it for much.

CNode * CNode::FindItemByName(char *soughtName)
{
	CNode *foundNode = NULL;
	CNode *aNode;

	// Is it us? Match the string, then check the string length to make sure they're the
	// same. This gets us a guaranteed exact match.
	if (strstr(m_strName, soughtName)!=NULL && strlen(soughtName)==strlen(m_strName))
		return this;

	// It's not us.  Query all our siblings, then all our children.
	// If we're at the head of a list of siblings, we've probably been asked to query 
	// all our siblings by our parent.
	if (m_Next && !m_Prev)
	{
		aNode = m_Next;
		while(aNode)
		{
			foundNode = aNode->FindItemByName(soughtName);
			if (foundNode != NULL)
				return foundNode;
			else
				aNode = aNode->m_Next;
		}
	}

	// If we were head of a list of items, all siblings have now been queried.
	// Now let's talk to the kids.  They'll either come up with the goods, or they won't.
	if (m_Children)
		return m_Children->FindItemByName(soughtName);
	else
		return NULL;	// If none of our siblings had it, and we have no children either, we came up empty.

}

// Adds a child to the current node, by walking the chain of kids until it finds the end of it 
// and attaching the adoptee there.
void CNode::AddChild(CNode *adoptee)
{
	CNode *aChild = NULL;

	if (!m_Children)	// No kids? Instant family! :)
	{
		m_Children = adoptee;
		m_Children->m_Prev = NULL;
		m_Children->m_Next = NULL;
	}
	else
	{
		aChild = m_Children;
		while(aChild)
		{
			if (aChild->m_Next == NULL)	// Found the end!
			{
				aChild->m_Next = adoptee;
				adoptee->m_Prev = aChild;
				return;
			}
			else
				aChild = aChild->m_Next;	// otherwise, keep looking...
		}
	}

}


void CNode::TransformAndLight(void)
{
	// Now it's safe to translate
	glTranslatef(m_CurrentTranslation.x, m_CurrentTranslation.y, m_CurrentTranslation.z);

	glRotatef(-m_CurrentRotation.x, 1.0f, 0.0f, 0.0f);	// Rotate n radians on the x axis
	glRotatef(-m_CurrentRotation.y, 0.0f, 1.0f, 0.0f);	// Rotate n radians on the y axis
	glRotatef(-m_CurrentRotation.z, 0.0f, 0.0f, 1.0f);	// Rotate n radians on the z axis

//	glScalef(m_CurrentScale.x, m_CurrentScale.y, m_CurrentScale.z);	// These had better be 1's if no scaling is desired..

	// Now that we've done all that, let's tell any kids we might have to do the same thing.
	if (m_Children)
	{
		CNode *aNode = m_Children;

		while(aNode)
		{
			glPushMatrix();
			aNode->TransformAndLight();
			glPopMatrix();
			aNode = aNode->m_Next;
		}
	}

	// If we're a light, then light up and adjust as needed.
	if (m_iNodeType == NODE_LIGHT)
		((CLight *)this)->Update();

}

void CNode::TransformAndRender()
{
	glTranslatef(m_CurrentTranslation.x, m_CurrentTranslation.y, m_CurrentTranslation.z);

//	glScalef(m_CurrentScale.x, m_CurrentScale.y, m_CurrentScale.z);	// These had better be 1's if no scaling is desired..

	glRotatef(-m_CurrentRotation.x, 1.0f, 0.0f, 0.0f);	// Rotate n degrees on the x axis
	glRotatef(-m_CurrentRotation.y, 0.0f, 1.0f, 0.0f);	// Rotate n degrees on the y axis
	glRotatef(-m_CurrentRotation.z, 0.0f, 0.0f, 1.0f);	// Rotate n degrees on the z axis


	// Now that we've done all that, let's tell any kids we might have to do the same thing.
	if (m_Children)
	{
		CNode *aNode = m_Children;

		while(aNode)
		{
			glPushMatrix();
			aNode->TransformAndRender();
			glPopMatrix();
			aNode = aNode->m_Next;
		}
	}

	// Are we a model?  If so, time to draw ourselves.
	if (m_iNodeType == NODE_MODEL)
		((CModel *)this)->Render();

}

void CNode::JustLight()
{

	// Now that we've done all that, let's tell any kids we might have to do the same thing.
	if (m_Children)
	{
		CNode *aNode = m_Children;

		while(aNode)
		{
			aNode->JustLight();
			aNode = aNode->m_Next;
		}
	}

	// If we're a light, then light up and adjust as needed.
	if (m_iNodeType == NODE_LIGHT)
		((CLight *)this)->Update();
}

void CNode::JustRender()
{
	// Now that we've done all that, let's tell any kids we might have to do the same thing.
	if (m_Children)
	{
		CNode *aNode = m_Children;

		while(aNode)
		{
			aNode->JustRender();
			aNode = aNode->m_Next;
		}
	}

	// Are we a model?  If so, time to draw ourselves.
	if (m_iNodeType == NODE_MODEL)
		((CModel *)this)->Render();

}

void CNode::SetTranslationf(float x, float y, float z)
{
	m_CurrentTranslation.x = x;
	m_CurrentTranslation.y = y;
	m_CurrentTranslation.z = z;
}

void CNode::SetRotationf(float x, float y, float z)
{
	m_CurrentRotation.x = x;
	m_CurrentRotation.y = y;
	m_CurrentRotation.z = z;
}

void CNode::SetName(char *name)
{
	if (m_strName)
		delete m_strName;

	m_strName = new char[strlen(name)+1];
	strcpy(m_strName,name);
}
