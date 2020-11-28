// Camera.cpp: implementation of the CCamera class.
//
//////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
#include <windows.h>
#endif

#include "camera.h"
#include <GL/gl.h>
#include <GL/glu.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCamera::CCamera()
{
	m_iNodeType = NODE_CAMERA;
}

CCamera::~CCamera()
{

}

// This must be called by the scene graph in the render step before it does anything
// else.  Surface culling, frustrum culling and lighting are all dependent on the camera's
// position and facing vector.

void CCamera::Update()
{
	// Until I get this figured out, just return.
	
	glMatrixMode(GL_MODELVIEW);

	// Camera motion is relative to any parents it may have.
	// We would walk backwards through the scene graph, finding
	// all the ancestors to this camera and doing the transforms
	// needed to compute the final transform on this camera.

	// In a perfect world.

	// In this world, cameras can't be parented to anything, so I 
	// can neatly sidestep the whole issue.

	glLoadIdentity();

	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glRotatef(-m_CurrentRotation.x+180.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(-m_CurrentRotation.y+180.0f, 0.0f, 1.0f, 0.0f);
	glRotatef(-m_CurrentRotation.z, 0.0f, 0.0f, 1.0f);
	
	glTranslatef(-m_CurrentTranslation.x, -m_CurrentTranslation.y, -m_CurrentTranslation.z);

	glMatrixMode(GL_MODELVIEW);
	

	
}
