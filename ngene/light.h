// Light.h: interface for the CLight class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(_LIGHT_H)
#define _LIGHT_H

#include "node.h"
#include "vector.h"	// Added by ClassView


// Lightwave's light types are:
//	0 = Distant
//	1 = Point
//	2 = Spotlight
//	3 = Linear
//  4 = Area
//
// I'm pretty certain OpenGL only supports the first three of these types.
//

class CLight : public CNode
{
public:
	bool m_bAffectDiffuse;
	bool m_bAffectSpecular;
	void Update(void);
	bool m_bEnabled;
	int m_iLightType; 
	float m_fLightIntensity;
	structVector3 m_LightColor;
	CLight();
	virtual ~CLight();

};

#endif 


