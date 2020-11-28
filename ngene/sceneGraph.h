// SceneGraph.h: interface for the CSceneGraph class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __SCENEGRAPH_H
#define __SCENEGRAPH_H


#ifndef NULL
#define NULL 0
#endif

#include "node.h"
#include "model.h"
#include "light.h"
#include "camera.h"

#include <fstream.h>

#define RADS2DEGREESMULTIPLIER 57.29578


typedef struct structSceneTokens
{
	int tokenID; 

	char *token;

}structSceneTokens;



		

class CSceneGraph  
{
public:	
	int m_iActiveCameraID;
	int axtoi(char *hexStr);
	bool SelectCamera(int id = 0);
	void RenderScene( void );
	bool m_bSceneLoadSuccessful;
	int GenerateItemNumber(int itemType, int lastItemNumber);
	CNode * m_TopNode;
	CSceneGraph(char *sceneName);
	bool ParseScene(char *sceneName);
	char *m_SceneName;
	CSceneGraph();
	virtual ~CSceneGraph();
	CCamera * m_ActiveCamera;

protected:
	CTextureManager * m_TextureManager;
	char * ExtractActualFilename(char *fullpath);
	CNode * m_NodeUnderConstruction;
	bool CommitSceneElement(void);
	int TokenLookup(char *s);

	// Since Lightwave scene files are flat as a pancake, the parser has to be just as flat.
	// These keep track of what the parser's currently working on.  When a new 'Load' token
	// is encountered, it stops building whatever it was working on, puts it away and 
	// starts making the next entity.

	bool m_bBuildingLight, 
		m_bBuildingCamera, 
		m_bBuildingObject,
		m_bBuildingBone,
		m_bBuildingEnvironment;	// mostly used for global light settings
};

#endif 
