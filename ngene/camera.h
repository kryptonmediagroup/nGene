// Camera.h: interface for the CCamera class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_CAMERA_H)
#define _CAMERA_H


#include "node.h"

class CCamera : public CNode 
{
public:
	void Update( void );
	CCamera();
	virtual ~CCamera();

};

#endif
