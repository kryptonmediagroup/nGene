// SceneGraph.cpp: implementation of the CSceneGraph class.
//
//////////////////////////////////////////////////////////////////////

#include "sceneGraph.h"
#include <fstream>

enum {	ST_LOADOBJECTLAYER=100, ST_OBJECTMOTION, ST_SHOWOBJECT, ST_AMBIENTCOLOR, 
ST_AMBIENTINTENSITY, ST_ADDLIGHT, ST_LIGHTNAME, ST_SHOWLIGHT,
ST_LIGHTMOTION, ST_LIGHTCOLOR, ST_LIGHTINTENSITY, ST_AFFECTCAUSTICS, ST_CURRENTLIGHT,
ST_LIGHTTYPE, ST_AFFECTDIFFUSE, ST_AFFECTSPECULAR, ST_SCHEMATICPOSITION, ST_ADDCAMERA, ST_CAMERANAME,
ST_SHOWCAMERA, ST_CAMERAMOTION, ST_ZOOMFACTOR, ST_FRAMESIZE, ST_CURRENTCAMERA,
ST_PIXELASPECT, ST_MASKPOSITION, ST_APERTUREHEIGHT, ST_ANTIALIASING,
ST_SOLIDBACKDROP, ST_BACKDROPCOLOR, ST_ZENITHCOLOR, ST_SKYCOLOR,
ST_GROUNDCOLOR, ST_NADIRCOLOR, ST_FOGTYPE, ST_DATAOVERLAYLABEL, 
ST_SHADOWOPTIONS, ST_ENABLESHADOWMAPS, ST_ENABLEVOLUMETRICLIGHTS, ST_SHADOWTYPE,
ST_NUMCHANNELS, ST_CHANNEL, ST_ENVELOPE, ST_KEY, ST_BEHAVIORS, 
ST_PARENTITEM, ST_FIRSTFRAME, ST_LASTFRAME, ST_FRAMESTEP, 
ST_PREVIEWFIRSTFRAME, ST_PREVIEWLASTFRAME, ST_PREVIEWFRAMESTEP, 
ST_CURRENTFRAME, ST_FRAMESPERSECOND, 
ST_BONEFALLOFFTYPE,	ST_ADDBONE,	ST_BONENAME, ST_SHOWBONE,			
ST_BONEACTIVE,	ST_BONERESTPOSITION,	ST_BONERESTDIRECTION,	
ST_BONERESTLENGTH,	ST_BONESTRENGTH,	ST_SCALEBONESTRENGTH,	
ST_BONEMOTION,	
ST_LEFTBRACE, ST_RIGHTBRACE,	
ST_UNKNOWN
} enumSceneTokens;


enum { LW_OBJECT=1, LW_LIGHT, LW_CAMERA, LW_BONE, LW_UNDEFINED } enumItemTypes;

struct structSceneTokens tokenMap[] =
{
	// For stripping useless punctuation
	{ST_LEFTBRACE, "{"},
	{ST_RIGHTBRACE, "}"},

	// These pertain to objects
	{ST_LOADOBJECTLAYER, "LoadObjectLayer"},
	{ST_OBJECTMOTION,"ObjectMotion"},
	{ST_SHOWOBJECT,"ShowObject"},
	
	// These pertain to lights
	{ST_AMBIENTCOLOR, "AmbientColor"}, 
	{ST_AMBIENTINTENSITY,"AmbientIntensity"},
	{ST_ADDLIGHT, "AddLight"},
	{ST_LIGHTNAME, "LightName"},
	{ST_SHOWLIGHT, "ShowLight"},
	{ST_LIGHTMOTION,"LightMotion"},
	{ST_LIGHTCOLOR,"LightColor"},
	{ST_LIGHTINTENSITY, "LightIntensity"},
	{ST_AFFECTCAUSTICS,"AffectCaustics"},
	{ST_LIGHTTYPE,"LightType"},	
	{ST_SCHEMATICPOSITION,"SchematicPosition"},
	{ST_AFFECTSPECULAR, "AffectSpecular"},
	{ST_AFFECTDIFFUSE, "AffectDiffuse"},
	{ST_CURRENTLIGHT, "CurrentLight"},

	// These affect shadows (which I don't implement yet at all)
	{ST_SHADOWOPTIONS, "ShadowOptions"},
	{ST_ENABLESHADOWMAPS, "EnableShadowMaps"},
	{ST_ENABLEVOLUMETRICLIGHTS, "EnableVolumetricLights"},
	{ST_SHADOWTYPE, "ShadowType"},

	// These pertain to cameras
	{ST_ADDCAMERA,"AddCamera"},
	{ST_CAMERANAME,"CameraName"},
	{ST_SHOWCAMERA,"ShowCamera"},
	{ST_CURRENTCAMERA, "CurrentCamera"},
	{ST_CAMERAMOTION,"CameraMotion"},
	{ST_ZOOMFACTOR, "ZoomFactor"},
	{ST_FRAMESIZE,"FrameSize"},
	{ST_PIXELASPECT,"PixelAspect"}, 
	{ST_MASKPOSITION,"MaskPosition"},
	{ST_APERTUREHEIGHT, "ApertureHeight"}, 
	{ST_ANTIALIASING,"Antialiasing"},
	{ST_SOLIDBACKDROP,"SolidBackdrop"},
	{ST_BACKDROPCOLOR,"BackdropColor"},
	{ST_ZENITHCOLOR, "ZenithColor"},
	{ST_SKYCOLOR,"SkyColor"},
	{ST_GROUNDCOLOR,"GroundColor"},
	{ST_NADIRCOLOR,"NadirColor"},
	{ST_FOGTYPE,"FogType"},
	{ST_DATAOVERLAYLABEL,"DataOverlayLabel"},
	
	// These pertain to bones.  Bones always directly follow the item to which
	// they're parented in the scene file.
	{ST_BONEFALLOFFTYPE,	"BoneFalloffType"},
	{ST_ADDBONE,			"AddBone"},
	{ST_BONENAME,			"BoneName"},
	{ST_SHOWBONE,			"ShowBone"},
	{ST_BONEACTIVE,			"BoneActive"},
	{ST_BONERESTPOSITION,	"BoneRestPosition"},
	{ST_BONERESTDIRECTION,	"BoneRestDirection"},
	{ST_BONERESTLENGTH,		"BoneRestLength"},
	{ST_BONESTRENGTH,		"BoneStrength"},
	{ST_SCALEBONESTRENGTH,	"ScaleBoneStrength"},
	{ST_BONEMOTION,			"BoneMotion"},
	
	
	// These are common to all entity types
	{ST_NUMCHANNELS,"NumChannels"}, 
	{ST_CHANNEL,"Channel"},
	{ST_ENVELOPE,"Envelope"},
	{ST_KEY,"Key"},
	{ST_BEHAVIORS,"Behaviors"},
	{ST_PARENTITEM,"ParentItem"},
	
	// These are for previews
	{ST_PREVIEWFIRSTFRAME, "PreviewFirstFrame"},
	{ST_PREVIEWLASTFRAME, "PreviewLastFrame"},
	{ST_PREVIEWFRAMESTEP, "PreviewFrameStep"},
	
	{ST_FIRSTFRAME, "FirstFrame"},
	{ST_LASTFRAME, "LastFrame"},
	{ST_FRAMESTEP, "FrameStep"},
	
	{ST_CURRENTFRAME, "CurrentFrame"},
	{ST_FRAMESPERSECOND, "FramesPerSecond"}
	
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSceneGraph::CSceneGraph()
{
	m_SceneName = NULL;
	m_bBuildingLight = false;
	m_bBuildingCamera = false;
	m_bBuildingObject = false;
	m_bBuildingEnvironment = false;	// mostly used for global light settings
	m_TopNode = new CNode;
	m_TopNode->m_strName = new char[strlen("Top Node")];
	strcpy(m_TopNode->m_strName,"Top Node");
	m_TextureManager = new CTextureManager;
}

CSceneGraph::~CSceneGraph()
{
	if (m_SceneName)
		delete m_SceneName;
	// Trash the rest of the scene, element by element.
	if (m_TopNode)
		delete m_TopNode;

}

// Read a Lightwave scene file, parse it and built the scene graph from its contents
bool CSceneGraph::ParseScene(char *sceneName)
{
	char textline[256];							// should be long enough for anything
	ifstream sceneFile;
	float f;	// another generally useful variable;
	
	m_NodeUnderConstruction = NULL;
	int handyCounter = 0;
	int iExpectedChannels, iCurrentChannel;
	
	int iCurrentObjectID = -1, 
		iCurrentLightID = -1, 
		iCurrentCameraID = -1, 
		iCurrentBoneID = -1;	// No support for bones yet.
	
	CModel *mdl			= NULL;


	
	// First, if it doesn't end in LWS, it's not a Lightwave scene.
	// I may lift this restriction later. - GT
	
	if (!strstr(sceneName,".LWS") && !strstr(sceneName,".lws"))
		return false;
	
	sceneFile.open(sceneName,ios::nocreate);
	if (!sceneFile.is_open())	
		return false;
	
	while(!sceneFile.eof())
	{
		sceneFile>>textline;
		
		switch(TokenLookup(textline))
		{
			// I think these are Layout display commands
			
		case ST_SHOWLIGHT:	// Always a pair of integers, meaningless to us at the moment.
			sceneFile >> textline;
			sceneFile >> textline;
			break;
			
		case ST_SHOWOBJECT:	// Always a pair of integers, meaningless to us at the moment.
			sceneFile >> textline;
			sceneFile >> textline;
			break;
			
		case ST_SHOWCAMERA:	// Always a pair of integers, meaningless to us at the moment.
			sceneFile >> textline;
			sceneFile >> textline;
			break;
			
		case ST_SCHEMATICPOSITION:
		case ST_DATAOVERLAYLABEL:
			break;
			
			// I think these refer to nonrealtime rendering
		case ST_ANTIALIASING:
		case ST_FRAMESIZE:
		case ST_PIXELASPECT:
		case ST_MASKPOSITION:
		case ST_APERTUREHEIGHT:	// This might control viewing angle and/or image size
		case ST_AFFECTCAUSTICS:
		case ST_FOGTYPE:
			break;
			
			// These describe the equivalent of a "limbo cyc" for rendering
		case ST_SOLIDBACKDROP:
		case ST_BACKDROPCOLOR:
		case ST_ZENITHCOLOR:
		case ST_SKYCOLOR:
		case ST_GROUNDCOLOR:
		case ST_NADIRCOLOR:
			break;
			
			// Now we get to something we can act on.  These appear to describe the
			// scene itself.
			
			// These are some special commands that describe global lighting conditions
		case ST_AMBIENTCOLOR:
			break;
			
		case ST_AMBIENTINTENSITY:
			break;
			
			// Here are a couple of commands that affect lights
		case ST_AFFECTDIFFUSE:
			sceneFile >> textline;
			if (m_bBuildingLight)
				((CLight *)m_NodeUnderConstruction)->m_bAffectDiffuse = (bool)atoi(textline);
			break;
			
		case ST_AFFECTSPECULAR:
			sceneFile >> textline;
			if (m_bBuildingLight)
				((CLight *)m_NodeUnderConstruction)->m_bAffectSpecular = (bool)atoi(textline);
			break;
			
		case ST_LOADOBJECTLAYER:
			CommitSceneElement();	// finish doing whatever we were doing.
			iCurrentObjectID = GenerateItemNumber(LW_OBJECT,iCurrentObjectID);
			m_bBuildingObject = true;
			m_NodeUnderConstruction = new CModel;				
			m_NodeUnderConstruction->m_lNodeID = iCurrentObjectID;
			sceneFile >> textline;	// Eat the layer ID for now.  The next thing will be the filename of the object.
			sceneFile >> textline;	// Filename of the object!
			mdl = (CModel *)m_NodeUnderConstruction;
			mdl->Load(ExtractActualFilename(textline), m_TextureManager);	
			break;
			
		case ST_ADDLIGHT:
			CommitSceneElement();	// finish doing whatever we were doing.
			iCurrentLightID = GenerateItemNumber(LW_LIGHT,iCurrentLightID);
			m_bBuildingLight = true;
			m_NodeUnderConstruction = new CLight;
			m_NodeUnderConstruction->m_lNodeID = iCurrentLightID;
			break;
			
		case ST_CURRENTLIGHT:		// Only meaningful in Layout
			sceneFile >> textline;	// Eat the parameter and toss it.
			break;

		case ST_ADDCAMERA:
			CommitSceneElement();	// finish doing whatever we were doing.
			iCurrentCameraID = GenerateItemNumber(LW_CAMERA,iCurrentCameraID);
			m_bBuildingCamera = true;
			m_NodeUnderConstruction = new CCamera;
			m_NodeUnderConstruction->m_lNodeID = iCurrentCameraID;
			break;

		case ST_CURRENTCAMERA:
			sceneFile >> textline;
			m_iActiveCameraID = atoi(textline);
			break;
			
		case ST_NUMCHANNELS:
			sceneFile >> textline;	// should be channel count
			iExpectedChannels = atoi(textline);
			break;
			
		case ST_CHANNEL:
			sceneFile >> textline;
			iCurrentChannel = atoi(textline);
			break;
			
		case ST_ENVELOPE:	// Don't know what to do to for this yet.
			break;
			
		case ST_KEY:		
			// Depending on which channel this is and what motion type we're 
			// processing, the rest of this line will be rotation in radians or
			// translation in meters.  Each channel represents a separate axis.
			
			// Lights and Objects have 9 channels.  Cameras have only 6.
			
			// Read the rest of the line this was on.
			sceneFile >> textline;	// gets the first value in the line.  There should be eight more values after this one.
			f = atof(textline);
			
			switch(iCurrentChannel)
			{
			case 0:
				m_NodeUnderConstruction->m_CurrentTranslation.x = f;
				break;

			case 1:
				m_NodeUnderConstruction->m_CurrentTranslation.y = f;
				break;

			case 2:
				m_NodeUnderConstruction->m_CurrentTranslation.z = f;
				break;

			case 3:
				m_NodeUnderConstruction->m_CurrentRotation.y = f * RADS2DEGREESMULTIPLIER;	// heading

				break;

			case 4:
				m_NodeUnderConstruction->m_CurrentRotation.x = f * RADS2DEGREESMULTIPLIER;	// pitch
				break;

			case 5:
				m_NodeUnderConstruction->m_CurrentRotation.z = f * RADS2DEGREESMULTIPLIER;	// bank
				break;

			case 6:
				m_NodeUnderConstruction->m_CurrentScale.x = f;
				break;

			case 7:
				m_NodeUnderConstruction->m_CurrentScale.y = f;
				break;

			case 8:
				m_NodeUnderConstruction->m_CurrentScale.z = f;
				break;
			}
			// The next eight parameters are coefficients in the animation spline for this
			// node.  For now we'll just eat them and dispose of the evidence.
			sceneFile >> textline;	// time
			sceneFile >> textline;	// smoothing	- 0 TCB (Kochanek-Bartels)
			//				- 1 Hermite
			//				- 2 Bezier
			//				- 3 Linear
			//				- 4 Stepped
			
			// Curve parameters.  tension, continuity, bias, incoming tangent, outgoing tangent, and 
			// the last one is apparently always zero.
			for(handyCounter = 0; handyCounter < 6; handyCounter++)
				sceneFile >> textline;
			
			break;
			
			case ST_BEHAVIORS:	
				// Eat the parameters.  They refer to animation in-out properties.
				sceneFile >> textline;	// pre
				sceneFile >> textline;	// post
				// 0 - reset
				// 1 - constant
				// 2 - repeat
				// 3 - oscillate
				// 4 - offset repeat
				// 5 - linear
				break;
				
			case ST_PARENTITEM:
				{
					sceneFile >> textline;
					// The text line is in hex, so I can't do this the normal way.
					char fragment[5];
					memset(fragment,0,sizeof(fragment));
					strncpy(fragment,textline,4);

					// Endian hazard here..
					m_NodeUnderConstruction->m_lParentNodeID = axtoi(fragment)<<16;
					memset(fragment,0,sizeof(fragment));
					strncpy(fragment,&textline[4],4);
					m_NodeUnderConstruction->m_lParentNodeID += axtoi(fragment);				
				}
				break;		
				
				// Objects are named like their filenames.  Since cameras and 
				// lights aren't loaded but created, they have to be arbitrarily named.

				// Fortunately, names for both lights and cameras work identically.			
			case ST_CAMERANAME:
			case ST_LIGHTNAME:
				sceneFile >> textline;
				m_NodeUnderConstruction->m_strName = new char[strlen(textline)];
				strcpy(m_NodeUnderConstruction->m_strName,textline);
				break;
				
				// Shadow parameters, which I don't do anything with at the moment.
			case ST_SHADOWOPTIONS:
			case ST_ENABLESHADOWMAPS:
			case ST_ENABLEVOLUMETRICLIGHTS:
				break;

			case ST_SHADOWTYPE:
				sceneFile >> textline;				
				break;
				// These appear to be parsing state flags; since we're acquiring
				// the data by context, we don't need to do anything with these tokens.
			case ST_CAMERAMOTION:
			case ST_OBJECTMOTION:
			case ST_LIGHTMOTION:
				break;
				
			case ST_ZOOMFACTOR:
				break;
				
			case ST_LIGHTCOLOR:
				sceneFile >> textline;
				((CLight *)m_NodeUnderConstruction)->m_LightColor.r = atof(textline);
				sceneFile >> textline;
				((CLight *)m_NodeUnderConstruction)->m_LightColor.g = atof(textline);
				sceneFile >> textline;
				((CLight *)m_NodeUnderConstruction)->m_LightColor.b = atof(textline);
				break;
				
			case ST_LIGHTINTENSITY:
				sceneFile >> textline;
				((CLight *)m_NodeUnderConstruction)->m_fLightIntensity = atof(textline);
				break;
				
			case ST_LIGHTTYPE:
				sceneFile >> textline;
				((CLight *)m_NodeUnderConstruction)->m_iLightType = atoi(textline);
				break;
				
				// Some tokens specific to animation settings.  Each has one parameter,
				// which we'll silently eat.
			case ST_FIRSTFRAME:
			case ST_LASTFRAME:
			case ST_FRAMESTEP:
			case ST_PREVIEWFIRSTFRAME:
			case ST_PREVIEWLASTFRAME:
			case ST_PREVIEWFRAMESTEP:
			case ST_CURRENTFRAME:
			case ST_FRAMESPERSECOND:
				sceneFile >> textline;
				break;
				
			case ST_BONEFALLOFFTYPE:
			case ST_ADDBONE:
			case ST_BONENAME:
			case ST_SHOWBONE:
			case ST_BONEACTIVE:
			case ST_BONERESTPOSITION:
			case ST_BONERESTDIRECTION:
			case ST_BONERESTLENGTH:
			case ST_BONESTRENGTH:
			case ST_SCALEBONESTRENGTH:
			case ST_BONEMOTION:
#if defined(_WIN32)
				OutputDebugString("No bone support yet.\n");
#else
				printf("No bone support yet.\n");
#endif				
				break;	// do nothing with any of this for now..
				
			case ST_UNKNOWN:
#if defined(_WIN32)			
				OutputDebugString("Parsing noise: ");
				OutputDebugString(textline);
				OutputDebugString("\n");
#else
				printf("Parsing noise: %s\n",textline);
#endif
		}
	}
	
	// Close whatever we were working on and get the flock outa here.
	CommitSceneElement();	
	m_ActiveCamera = (CCamera *)(m_TopNode->FindItemByID(0x30000000+m_iActiveCameraID));
	

	return true;
	
	
	
}

CSceneGraph::CSceneGraph(char *sceneName)
{
	m_bBuildingLight = false;
	m_bBuildingCamera = false;
	m_bBuildingObject = false;
	m_bBuildingEnvironment = false;	// mostly used for global light settings

	m_TopNode = false;
	m_TextureManager = new CTextureManager;

	ParseScene(sceneName);
	// If successful, m_TopNode will be !NULL.

	
}

int CSceneGraph::TokenLookup(char *s)
{
	int max = sizeof(tokenMap)/sizeof(struct structSceneTokens);
	for (int ii = 0; ii < max; ii++)
	{
		if (strstr(s, tokenMap[ii].token))
			return tokenMap[ii].tokenID;
	}
	return ST_UNKNOWN;
}

bool CSceneGraph::CommitSceneElement()
{
	CNode *aNode = NULL;

	// The camera appears to be upside down and backwards, whereas everything else appears to load
	// correctly.
	if (m_bBuildingCamera)
	{
		if (m_NodeUnderConstruction->m_iNodeType != NODE_CAMERA)
			return false;

		// Make some rotational adjustments

		float fAxisValue = m_NodeUnderConstruction->m_CurrentRotation.y + 180.0f;
		while(fAxisValue>360.0f)
			fAxisValue -= 360.0f;
		m_NodeUnderConstruction->m_CurrentRotation.y = fAxisValue;

		fAxisValue = m_NodeUnderConstruction->m_CurrentRotation.x + 125.0f;
		while(fAxisValue>360.0f)
			fAxisValue -= 360.0f;
		m_NodeUnderConstruction->m_CurrentRotation.x = fAxisValue;


		m_bBuildingCamera = false;
	}
	
	if (m_bBuildingObject)
	{	
		if (m_NodeUnderConstruction->m_iNodeType != NODE_MODEL)
			return false;
		m_bBuildingObject = false;
	}
	
	if (m_bBuildingLight)
	{
		if (m_NodeUnderConstruction->m_iNodeType != NODE_LIGHT)
			return false;
		m_bBuildingLight = false;
	}
	
	if (m_bBuildingEnvironment)
	{
		if (m_NodeUnderConstruction->m_iNodeType != NODE_ENVIRONMENT)
			return false;
		m_bBuildingEnvironment = false;
	}
	
	if (m_bBuildingBone)
		// Do nothing for now, as we don't support bones yet.
		//if (m_NodeUnderConstruction->m_iNodeType != NODE_BONE)
		//	return false;
		m_bBuildingBone = false;
	
	if (m_NodeUnderConstruction)
	{
		// Find where this node is supposed to go and 
		// and attach it there.
		
		// This appears not to be working correctly - GET WIP
		
		aNode = m_TopNode->FindItemByID(m_NodeUnderConstruction->m_lParentNodeID);
		if (aNode)
			aNode->AddChild(m_NodeUnderConstruction);
		else
			m_TopNode->AddChild(m_NodeUnderConstruction);
		
		m_NodeUnderConstruction->m_CurrentTranslation.z *= -1.0f;
		
		if (m_bBuildingCamera)
			InvertVertexCoordinates(&(m_NodeUnderConstruction->m_CurrentRotation));
		
	}
	
	return true;
}

char * CSceneGraph::ExtractActualFilename(char *fullpath)
{
	return (strrchr(fullpath,'/')+1);
}

// The first 4 significat bits describe the item type (1 = object, 2 = light, 3 = camera, 4 = bone).
// The last four hex digits (bits 0-15) are the item number.
// In the case of bones, it looks like this:

//	Item type	(1 hex digit)	bone number (3 hex digits) item number (4 hex digits)
//		T B B B I I I I 

int CSceneGraph::GenerateItemNumber(int itemType, int lastItemNumber)
{
	int basetype = itemType * 0x10000000;
	
	
	if (itemType == LW_BONE)
		return -1;	// no support for bones yet.
	
	if (lastItemNumber == -1)
		return basetype;
	else
		return lastItemNumber+1;
	
	
	
}



void CSceneGraph::RenderScene()
{
	// Set up the viewpoint using the active camera

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (!m_ActiveCamera)
	{
		SelectCamera();
		if (!m_ActiveCamera)
			return;
	}
	else
		m_ActiveCamera->Update();
	

	if (m_TopNode)
	{
		m_TopNode->TransformAndLight();		// Since I'm using OpenGL to do my matrix math,
		//m_TopNode->TransformAndRender();	// I have to do the transforms twice.  Really stupid, but it's a shortcut.
		//m_TopNode->JustLight();
		m_TopNode->TransformAndRender();
	}	
	
	//((CModel *)m_TopNode->FindItemByName("table"))->Render();
	//((CModel *)m_TopNode->FindItemByName("Aaron"))->Render();
	//((CModel *)m_TopNode->FindItemByName("flipper"))->Render();
	//m_TopNode->TransformAndRender();

}

// There is always at least one camera, but let's select which one we want to use
// for our user's viewpoint.  If we can't find the indicated camera, return false,
// else if successful, return true.

bool CSceneGraph::SelectCamera(int id)
{
	// Cameras have an item type of 3, so 
	// we take the ID and construct the expect ID from that.
	m_ActiveCamera = (CCamera *)m_TopNode->FindItemByID(0x30000000+id);
	if (!m_ActiveCamera)
		return false;
	else
		return true;
	
}


int CSceneGraph::axtoi(char *hexStr)
{
	int n = 0;         // position in string
	int m = 0;         // position in digit[] to shift
	int count;         // loop index
	int intValue = 0;  // integer value of hex string
	int digit[5];      // hold values to convert
	while (n < 4) 
	{
		if (hexStr[n]=='\0')
			break;
		
		if (hexStr[n] > 0x29 && hexStr[n] < 0x40 ) //if 0 to 9
			digit[n] = hexStr[n] & 0x0f;            //convert to int
		
		else if (hexStr[n] >='a' && hexStr[n] <= 'f') //if a to f
			digit[n] = (hexStr[n] & 0x0f) + 9;      //convert to int
		
		else if (hexStr[n] >='A' && hexStr[n] <= 'F') //if A to F
			digit[n] = (hexStr[n] & 0x0f) + 9;      //convert to int
		
		else break;
		n++;
	}
	
	count = n;
	m = n - 1;
	n = 0;
	
	while(n < count) 
	{
		// digit[n] is value of hex digit at position n
		// (m << 2) is the number of positions to shift
		// OR the bits into return value
		intValue = intValue | (digit[n] << (m << 2));
		m--;   // adjust the position to set
		n++;   // next digit to process
	}
	return (intValue);
}


