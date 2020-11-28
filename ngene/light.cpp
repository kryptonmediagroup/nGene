// Light.cpp: implementation of the CLight class.
//
//////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
#include <windows.h>
#endif

#include "light.h"
#include <GL/gl.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLight::CLight() 
{
	m_fLightIntensity = 0.85f;
	m_iLightType = 0;		// Don't know what the defaults are yet.
	m_LightColor.r = 1.0f;	// White light
	m_LightColor.g = 1.0f;
	m_LightColor.b = 1.0f;
	m_bAffectDiffuse = true;
	m_bAffectSpecular = true;
	m_bEnabled = false;
	m_iNodeType = NODE_LIGHT;
	
}

CLight::~CLight()
{
	
}

// OpenGL actually supports more lighting features than Lightwave does, so
// we might not get everything.
void CLight::Update()
{
	int glLightID = (m_lNodeID & 0xffff) + GL_LIGHT0;
	float v[4];
	

	// Set up a light, turn it on.
				// Set up a light, turn it on.
	
	// Position
	v[0] = m_CurrentTranslation.x;
	v[1] = m_CurrentTranslation.y;
	v[2] = m_CurrentTranslation.z;
	v[3] = 1.0f;

	glLightfv(glLightID, GL_POSITION, v);
	
	// Ambient - all lights do this
	v[0] = m_fLightIntensity * m_LightColor.r;
	v[1] = m_fLightIntensity * m_LightColor.g;
	v[2] = m_fLightIntensity * m_LightColor.b;
	//glLightfv(glLightID, GL_AMBIENT, v);
	
	// Reuse the same settings for diffusion
	if (m_bAffectDiffuse)
		glLightfv(glLightID, GL_DIFFUSE, v );
	
	if (m_bAffectSpecular)
		glLightfv(glLightID, GL_SPECULAR, v);

	if (!m_bEnabled)
	{
		glEnable(glLightID); 
		m_bEnabled = true;
	}
//	else
//		glEnable(glLightID);
			// A handy trick -- have surface material mirror the color.
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	
}
