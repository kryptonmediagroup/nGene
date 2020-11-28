// ----------------------


#include <stdio.h>   // Always a good idea.
#include <time.h>    // For our FPS stats.


#if defined(_WIN32)
#include <windows.h>
#endif

#include <GL/gl.h>   // OpenGL itself.
#include <GL/glu.h>  // GLU support library.
#include <GL/glut.h> // GLUT support library.

#include "sceneGraph.h"



#define PROGRAM_TITLE "Gene Turnbow's nGENE"

// Some global variables.

CMesh *theModel = NULL;
CTextureManager *g_singleModelTextureManager;
CSceneGraph *theScene = NULL;

// Window and ID, window width and height.

int Window_ID;
int Window_Width = 800;
int Window_Height = 600;

// Our display mode settings.
int Light_On = 1;
int Blend_On = 0;
int Texture_On = 0;
int Filtering_On = 0;
int Alpha_Add = 0;

int Curr_TexMode = 0;
char *TexModesStr[] = {"GL_DECAL","GL_MODULATE","GL_BLEND","GL_REPLACE"};
GLint TexModes[] = {GL_DECAL,GL_MODULATE,GL_BLEND,GL_REPLACE};

// Object and scene global variables.

// position and rotation speed variables.
float X_Rot   = 0.0f;
float Y_Rot   = 0.0f;
float X_Speed = 0.0f;
float Y_Speed = 0.0f;
float Z_Off   = -5.0f;
float Z_OffsetSpeed = 0.0f;


float g_fFlipperOneAngle;
float g_fFlipperTwoAngle;
float g_fPlungerPosition;
clock_t g_lFlipperOneDelay, g_lFlipperTwoDelay, g_lPlungerDelay;

// Settings for our light.  Try playing with these (or add more lights).
float Light_Ambient[]=  { 0.2f, 0.2f, 0.2f, 1.0f };
float Light_Diffuse[]=  { 0.7f, 0.7f, 0.7f, 1.0f };
float Light_Position[]= { 2.0f, 3.0f, 5.0f, 1.0f };
float Light_Specular[]= { 0.7f, 0.7f, 0.7f, 1.0f };



// A few things specific to the HyperTilt tester program
CModel 
	*g_ptrFlipperOne, 
	*g_ptrFlipperTwo, 
	*g_ptrTable, 
	*g_ptrPlunger, 
	*g_ptrAaronHead, 
	*g_ptrAaronHeadSatellite;

// ------
// Frames per second (FPS) statistic variables and routine.

#define FRAME_RATE_SAMPLES 50
int FrameCount=0;
float FrameRate=0;

void helpme(void);

static void ourDoFPS() 
{
	static clock_t last=0;
	clock_t now;
	float delta;
	
	if (++FrameCount >= FRAME_RATE_SAMPLES) 
	{
		now  = clock();
		delta= (now - last) / (float) CLOCKS_PER_SEC;
		last = now;
		
		FrameRate = FRAME_RATE_SAMPLES / delta;
		FrameCount = 0;
	}
}


// ------
// String rendering routine; leverages on GLUT routine.

static void ourPrintString(void *font,char *str)
{
	int i,l=strlen(str);
	
	for(i=0;i<l;i++)
		glutBitmapCharacter(font,*str++);
}


void helpme(void)
{
	printf("\nlwoviewer\n\n"
		"Usage:	lwoviewer <Lightwave 6.5 model filename>\n\n"
		"Use arrow keys to rotate, 'R' to reverse, 'S' to stop.\n"
		"Page up/down will move the model away from/towards camera.\n\n"
		"Use first letter of shown display mode settings to alter.\n\n"
		"Q or [Esc] to quit; OpenGL window must have focus for input.\n");
}

// ------
// Routine which actually does the drawing

void cbRenderScene(void )
{
	
	char buf[80]; // For our strings.
	int lastMaterialTagID = -1;	// We use this to help us keep track of materials changes.  
	GLuint activeTexture = 0;

	// Since LWO2Loader sorts the polygons it loads by material,
	// we can issue a minimum of materials settings instructions to
	// OpenGL by only doing it when the material changes.
	
	// Enables, disables or otherwise adjusts as 
	// appropriate for our current settings.
	
	glShadeModel(GL_SMOOTH);
	
//	if (Texture_On)
		glEnable(GL_TEXTURE_2D);
//	else
//		glDisable(GL_TEXTURE_2D);
	
	if (Light_On) 
		glEnable(GL_LIGHTING);
	else 
		glDisable(GL_LIGHTING);
	
	if (Alpha_Add)
		glBlendFunc(GL_SRC_ALPHA,GL_ONE); 
	else
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
    // If we're blending, we don't want z-buffering.
	if (Blend_On)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}
	
	/*if (Filtering_On)
	{
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	}
	else
    {
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    }
	*/
	
	if (theModel)	// We're looking at a single model, and no scene has been loaded.
	{
		
		// Need to manipulate the ModelView matrix to move our model around.
		glMatrixMode(GL_MODELVIEW);
		glFrontFace(GL_CW);
		// Reset to 0,0,0; no rotation, no scaling.
		glLoadIdentity(); 
		
		// Clear the color and depth buffers.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Move the object back from the screen.
		glTranslatef(0.0f,0.0f,Z_Off);
		
		// Rotate the calculated amount.
		glRotatef(X_Rot,1.0f,0.0f,0.0f);
		glRotatef(Y_Rot,0.0f,1.0f,0.0f);
		
		// OK, let's start drawing our planer quads.
		
		for (unsigned int polyCount = 0; polyCount < theModel->m_NumPolygons; polyCount++)
		{
			activeTexture = theModel->m_Surfaces[theModel->m_Polygons[polyCount]->tag].colormapHandle;

			if (activeTexture)
				glEnable(GL_TEXTURE_2D);
			else
				glDisable(GL_TEXTURE_2D);
			

			glBegin(GL_TRIANGLES); 
			Vector3 *v3 = &(theModel->m_Polygons[polyCount]->polyNormal);  	 	 		
			
			glNormal3f( v3->x, v3->y, v3->z );
			int tagID = theModel->m_Polygons[polyCount]->tag;
			
			// Now that we've got all the setup for this poly taken care of,
			// let's express the polygon list.  For the moment, we're
			// accepting any number of vertices, because OpenGL can.  Later
			// I'll chop things up into tri's during the preprocessing phase.

			VMAP *vMap = NULL;
			
			unsigned int vertCount = theModel->m_Polygons[polyCount]->numVertices;
			float *vf = &(theModel->m_Surfaces[tagID].colr[0]);
			glColor3f( *(vf), *(vf+1), *(vf+2));	
				
			activeTexture = theModel->m_Surfaces[theModel->m_Polygons[polyCount]->tag].colormapHandle;
			glBindTexture(GL_TEXTURE_2D,activeTexture);	// if the active texture is 0, OpenGL unbinds all textures.

			vMap = theModel->m_Surfaces[theModel->m_Polygons[polyCount]->tag].colormapVMAP;
			for(unsigned int vCount = 0; vCount < vertCount; vCount++)
			{
				// No texture coordinates yet, because I'm not doing textures yet.
				// That would be a glTexCoord2f() call..

				if (vMap)
				{
					VMAPElement *el = NULL;
					float *coord = NULL;

					el = theModel->GetVMAPForVertex(vMap,theModel->m_Polygons[polyCount]->vertices[vCount]);					
					if (el)
					{
						coord = el->values;
						if (coord)
							glTexCoord2f(coord[0],coord[1]);
					}
				}

				Vector3 *v = &(theModel->m_Vertices[theModel->m_Polygons[polyCount]->vertices[vCount]]);
				glVertex3f( v->x, v->y, v->z );			
			}
			glEnd();
		}
		
		// All polygons have been drawn.
		
	}
	
	else
		
	{
		// Need to manipulate the ModelView matrix to move our model around.
		glMatrixMode(GL_MODELVIEW);
		
		// Reset to 0,0,0; no rotation, no scaling.
		glLoadIdentity(); 
		glFrontFace(GL_CW);
		// Clear the color and depth buffers.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Move the object back from the screen.
		structVector3 *pos = &(theScene->m_ActiveCamera->m_CurrentTranslation);
		structVector3 *rot = &(theScene->m_ActiveCamera->m_CurrentRotation);
		
		pos->z += Z_OffsetSpeed;
		
		// Rotate the calculated amount.
		rot->x+= X_Speed;
		rot->y+= Y_Speed;
		// leave rot->z alone for now
		
		
		// Tell the first Aaron head to spin a little
		if (g_ptrAaronHead)
		{
			pos = &(g_ptrAaronHead->m_CurrentRotation);
			g_ptrAaronHead->SetRotationf(pos->x, pos->y - 1.5f, pos->z);
			if (g_ptrAaronHeadSatellite)
				g_ptrAaronHeadSatellite->SetRotationf(pos->x, pos->y + 5.0f, pos->z);
		}
		
		if (g_ptrFlipperOne)
		if ((clock()-g_lFlipperOneDelay)/(CLOCKS_PER_SEC/6)>0)
			g_ptrFlipperOne->m_CurrentRotation.y = g_fFlipperOneAngle;
		
		if(g_ptrFlipperTwo)

		if ((clock()-g_lFlipperTwoDelay)/(CLOCKS_PER_SEC/6)>0)
			g_ptrFlipperTwo->m_CurrentRotation.y = g_fFlipperTwoAngle;

		if (g_ptrPlunger)
		if ((clock()-g_lPlungerDelay)/(CLOCKS_PER_SEC/6)>0)
			g_ptrPlunger->m_CurrentTranslation.z = g_fPlungerPosition;
		
		// Draw the scene from the scene graph
		if (theScene)
			theScene->RenderScene();
	}
	// Everything all done at once.
	//glCallList(1);
	
	// Move back to the origin (for the text, below).
	glLoadIdentity();
	
	// We need to change the projection matrix for the text rendering.  
	glMatrixMode(GL_PROJECTION);
	
	// But we like our current view too; so we save it here.
	glPushMatrix();
	
	// Now we set up a new projection for the text.
	glLoadIdentity();
	glOrtho(0,Window_Width,0,Window_Height,-1.0,1.0);
	
	// Lit or textured text looks awful.
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	
	// We don't want depth-testing either.
	glDisable(GL_DEPTH_TEST); 
	
	// But, for fun, let's make the text partially transparent too.
	glColor4f(0.6,1.0,0.6,.75);
	
	// Render our various display mode settings.
	
	if (!theScene)
	{
		sprintf(buf," Mode: %s", TexModesStr[Curr_TexMode]);
	}
	else
	{
		if (g_ptrFlipperOne && g_ptrFlipperTwo)
		sprintf(buf, "Flippers: %3.2f %3.2f",g_ptrFlipperOne->m_CurrentRotation.y,g_ptrFlipperTwo->m_CurrentRotation.y);
	}
	glRasterPos2i(2,2); ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	
	sprintf(buf,"  Add: %d", Alpha_Add);
	glRasterPos2i(2,14); ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	
	sprintf(buf,"Blend: %d", Blend_On);
	glRasterPos2i(2,26); ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	
	sprintf(buf,"Light: %d", Light_On);
	glRasterPos2i(2,38); ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	
	//sprintf(buf,"Tex: %d", Texture_On);
	//glRasterPos2i(2,50); ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	
	//sprintf(buf,"Filt: %d", Filtering_On);
	//glRasterPos2i(2,62); ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	
	
	// Now we want to render the calulated FPS at the top.
	
	// To ease, simply translate up.  Note we're working in screen
	// pixels in this projection.
	
	glTranslatef(6.0f,Window_Height - 16,0.0f);
	
	// Make sure we can read the FPS section by first placing a 
	// dark, mostly opaque backdrop rectangle.
	glColor4f(0.2,0.2,0.2,0.75);
	
	glBegin(GL_QUADS);   
	glVertex3f(  0.0f, -2.0f, 0.0f);
	glVertex3f(  0.0f, 14.0f, 0.0f);
	glVertex3f(140.0f, 14.0f, 0.0f);
	glVertex3f(140.0f, -2.0f, 0.0f);
	glEnd();
	
	glColor4f(0.9,0.2,0.2,.75);
	sprintf(buf,"FPS: %f F: %2d", FrameRate, FrameCount);
	glRasterPos2i(6,0);
	ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	if (theModel)
		sprintf(buf,"POLYGONS: %d",theModel->m_NumPolygons);
	else
	{
		structVector3 rot = theScene->m_ActiveCamera->m_CurrentRotation;
		sprintf(buf,"ROT X: %4.4f Y: %4.4f Z: %4.4f Z OFFST:%f", rot.x, rot.y, rot.z, Z_Off);
	}
	
	glRasterPos2i(6,-14);
	ourPrintString(GLUT_BITMAP_HELVETICA_12,buf);
	
	// Done with this special projection matrix.  Throw it away.
	glPopMatrix();
	
	// All done drawing.  Let's show it.
	glutSwapBuffers();
	
	// Now let's do the motion calculations.  Note that these
	// are updated but not used by CSceneGraph::RenderScene().
	X_Rot+=X_Speed; 
	Y_Rot+=Y_Speed; 
	
	// And collect our statistics.
	ourDoFPS();
}


// ------
// Callback function called when a normal key is pressed.

void cbKeyPressed(
				  unsigned char key, 
				  int x, int y
				  )
{
	switch (key) {
	case 113: case 81: case 27: // Q (Escape) - We're outta here.
		glutDestroyWindow(Window_ID);
		exit(1);
		break; // exit doesn't return, but anyway...
		
	case 130: case 98: // B - Blending.
		Blend_On = Blend_On ? 0 : 1; 
		if (!Blend_On)
			glDisable(GL_BLEND); 
		else
			glEnable(GL_BLEND);
		break;
		
	case 108: case 76:  // L - Lighting
		Light_On = Light_On ? 0 : 1; 
		break;
		
		//	case 109: case 77:  // M - Mode of Blending
		//		if ( ++ Curr_TexMode > 3 )
		//			Curr_TexMode=0;
		//		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,TexModes[Curr_TexMode]);
		//		break;
		
		//	case 116: case 84: // T - Texturing.
		//		Texture_On = Texture_On ? 0 : 1; 
		//		break;
		
	case 97: case 65:  // A - Alpha-blending hack.
		Alpha_Add = Alpha_Add ? 0 : 1; 
		break;
		
		//	case 102: case 70:  // F - Filtering.
		//		Filtering_On = Filtering_On ? 0 : 1; 
		//		break;
		
	case 'S': case 's':   // S/s = Stop!
		X_Speed=Y_Speed=Z_OffsetSpeed=0;
		break;

	case ' ':
		if (g_ptrPlunger)
		{
			if (g_ptrPlunger->m_CurrentTranslation.z == g_fPlungerPosition)
			{
				g_ptrPlunger->m_CurrentTranslation.z += 1.0f;
				g_lPlungerDelay = clock();
			}
		}
		break;
		
	case 114: case 82:  // R - Reverse.
		X_Speed=-X_Speed;
		Y_Speed=-Y_Speed;
		Z_OffsetSpeed=-Z_OffsetSpeed;
		break;
	case '?': case '/':	// right flipper
		
		if (g_ptrFlipperTwo)
		{
			if (g_ptrFlipperTwo->m_CurrentRotation.y == g_fFlipperTwoAngle)
				g_ptrFlipperTwo->m_CurrentRotation.y -= 45.0f;
			g_lFlipperTwoDelay = clock();
		}
		break;
	case 'Z': case 'z':	// left flipper
		
		if (g_ptrFlipperOne)
		{
			if (g_ptrFlipperOne->m_CurrentRotation.y == g_fFlipperOneAngle)
				g_ptrFlipperOne->m_CurrentRotation.y += 45.0f;
			g_lFlipperOneDelay = clock();
		}
		break;
		
	default:
		printf ("KP: No action for %d.\n", key);
		break;
    }
}


// ------
// Callback Function called when a special key is pressed.

void cbSpecialKeyPressed(
						 int key,
						 int x, 
						 int y
						 )
{
	switch (key) {
	case GLUT_KEY_PAGE_UP: // move the model into the distance.
		Z_Off -= 0.1f;
		Z_OffsetSpeed -= 0.01f;	// used only for scene view
		break;
		
	case GLUT_KEY_PAGE_DOWN: // move the model closer.
		Z_Off += 0.1f;
		Z_OffsetSpeed += 0.01f; // used only for scene view
		break;
		
	case GLUT_KEY_UP: // decrease x rotation speed;
		X_Speed -= 0.05f;
		break;
		
	case GLUT_KEY_DOWN: // increase x rotation speed;
		X_Speed += 0.05f;
		break;
		
	case GLUT_KEY_LEFT: // decrease y rotation speed;
		Y_Speed -= 0.05f;
		break;
		
	case GLUT_KEY_RIGHT: // increase y rotation speed;
		Y_Speed += 0.05f;
		break;
		
	default:
		printf ("SKP: No action for %d.\n", key);
		break;
    }
}


// ------
// Function to build a simple full-color texture with alpha channel,
// and then create mipmaps.  This could instead load textures from
// graphics files from disk, or render textures based on external
// input.
/*
void ourBuildTextures(void )
{
	GLenum gluerr;
	GLubyte tex[128][128][4];
	int x,y,t;
	int hole_size = 3300; // ~ == 57.45 ^ 2.

	// Generate a texture index, then bind it for future operations.
	glGenTextures(1,&Texture_ID);
	glBindTexture(GL_TEXTURE_2D,Texture_ID);
  
	// Iterate across the texture array.
	
	for(y=0;y<128;y++) 
	{
		for(x=0;x<128;x++) 
		{
	  
			// A simple repeating squares pattern.
			// Dark blue on white.
		
			if ( ( (x+4)%32 < 8 ) && ( (y+4)%32 < 8)) 
			{
				tex[x][y][0]=tex[x][y][1]=0; tex[x][y][2]=120;
			} 
			else 
			{
				tex[x][y][0]=tex[x][y][1]=tex[x][y][2]=240;
			}
		  
			// Make a round dot in the texture's alpha-channel.
			
			// Calculate distance to center (squared).
			t = (x-64)*(x-64) + (y-64)*(y-64) ;
			  
			if ( t < hole_size) // Don't take square root; compare squared.
			{
				tex[x][y][3]=255; // The dot itself is opaque.
			}
			else 
			{
				if (t < hole_size + 100)
				{
					tex[x][y][3]=128; // Give our dot an anti-aliased edge.
				}
				else
				{
					tex[x][y][3]=0;   // Outside of the dot, it's transparent.
				}		
			}
		}
	}
	
	// The GLU library helps us build MipMaps for our texture.
					
	if ((gluerr=gluBuild2DMipmaps(GL_TEXTURE_2D, 4, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, (void *)tex))) 
	{
		fprintf(stderr,"GLULib%s\n",gluErrorString(gluerr));
		exit(-1);
	}

	// Some pretty standard settings for wrapping and filtering.
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

	// We start with GL_DECAL mode.
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
}
*/
							
// ------
// Callback routine executed whenever our window is resized.  Lets us
// request the newly appropriate perspective projection matrix for 
// our needs.  Try removing the gluPerspective() call to see what happens.
							
void cbResizeScene( int Width, int Height )
{
	// Let's not core dump, no matter what.
	if (Height == 0)
		Height = 1;
	
	glViewport(0, 0, Width,Height );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,100.0f);
	
	glMatrixMode(GL_MODELVIEW);
	
	Window_Width  = Width;
	Window_Height = Height;
}
							
extern CSceneGraph *theScene;



// ------
// Does everything needed before losing control to the main
// OpenGL event loop.  

void initScene( int Width, int Height, char *filename )
{
	//ourBuildTextures();
	if (!theScene)
		theScene = new CSceneGraph();
	
	// Let the scene loader have this first.
	if (!theScene->ParseScene(filename))
	{
		// Make a texture manager for use by this model alone.  (If a scene loads, it makes its own,
		// so with a scene file that actually exists, this isn't necessary).

		
		g_singleModelTextureManager = new CTextureManager;
		// Load the model requested
		theModel = new CMesh;
		if (!theModel->Load(filename, g_singleModelTextureManager))
		{
			printf("Couldn't find %s\n",filename);
			exit(1);	
		}
		else
		{
			// Set up for a single model
			glShadeModel(GL_SMOOTH);
			
			// Color to clear color buffer to.
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

			// Depth to clear depth buffer to; type of test.
			glClearDepth(1.0);
			glDepthFunc(GL_LESS);
			
			// Windows does blits from the bottom up, not the top down.
			// For Windows systems, we need to flip the screen with respect to
			// Windows.			

			// Enables Smooth Color Shading; try GL_FLAT for (lack of) fun.
			
			// Load up the correct perspective matrix; using a callback directly.
			cbResizeScene(Width,Height);
			
			// Set up a light, turn it on.
			glLightfv(GL_LIGHT1, GL_POSITION, Light_Position);
			glLightfv(GL_LIGHT1, GL_AMBIENT,  Light_Ambient);
			glLightfv(GL_LIGHT1, GL_DIFFUSE,  Light_Diffuse);
			glLightfv(GL_LIGHT1, GL_SPECULAR, Light_Specular);
			glEnable (GL_LIGHT1); 
			
			glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
			// A handy trick -- have surface material mirror the color.
			glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
			glEnable(GL_COLOR_MATERIAL);
			
		}
		
	}
	else
	{
		// A scene was successfully loaded. Select a camera (in this case, the first one in the scene).
		theScene->SelectCamera();
		CCamera *cam = theScene->m_ActiveCamera;
		Z_Off = cam->m_CurrentTranslation.x;
		X_Rot = cam->m_CurrentRotation.x;
		Y_Rot = cam->m_CurrentRotation.y;
		
		// Set up for a scene
		
		// Color to clear color buffer to.
		glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
		
		// Depth to clear depth buffer to; type of test.
		glClearDepth(1.0);
		glDepthFunc(GL_LESS); 
		//glFrontFace(GL_CW);
		

		// Enables Smooth Color Shading; try GL_FLAT for (lack of) fun.

		glShadeModel(GL_SMOOTH);
		
		// Load up the correct perspective matrix; using a callback directly.
		cbResizeScene(Width,Height);
		
		// Set up a light, turn it on.
		//glLightfv(GL_LIGHT1, GL_POSITION, Light_Position);
		//glLightfv(GL_LIGHT1, GL_AMBIENT,  Light_Ambient);
		//glLightfv(GL_LIGHT1, GL_DIFFUSE,  Light_Diffuse);
		//glLightfv(GL_LIGHT1, GL_SPECULAR, Light_Specular);
		//glEnable (GL_LIGHT1); 
		
		// A handy trick -- have surface material mirror the color.
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

	}
}

// Find named elements within the scene and attach them to global pointers,
// so they can be manipulated.
void attachControls( void )
{
	g_ptrFlipperTwo = (CModel *)theScene->m_TopNode->FindItemByName("flipper");
	g_ptrFlipperTwo->SetName("flippertwo");
	g_ptrFlipperOne = (CModel *)theScene->m_TopNode->FindItemByName("flipper");
	g_ptrFlipperOne->SetName("flipperone");
	g_ptrTable = (CModel *)theScene->m_TopNode->FindItemByName("table");
	g_ptrAaronHead = (CModel *)theScene->m_TopNode->FindItemByName("Aaron");
	strcpy(g_ptrAaronHead->m_strName,"Aarin");	// So we don't get the same model again on the next search.
	g_ptrAaronHeadSatellite = (CModel *)theScene->m_TopNode->FindItemByName("Aaron");
	g_ptrPlunger = (CModel *)theScene->m_TopNode->FindItemByName("plunger");
	
	g_fFlipperTwoAngle = g_ptrFlipperTwo->m_CurrentRotation.y;
	g_fFlipperOneAngle = g_ptrFlipperOne->m_CurrentRotation.y;
	g_fPlungerPosition = g_ptrPlunger->m_CurrentTranslation.z;
}


// ------
// The main() function.  Inits OpenGL.  Calls our own init function,
// then passes control onto OpenGL.

int main( int argc, char **argv )
{
	if (argc==1)
	{
		helpme();
		exit(-1);
	}
	
	
	
	glutInit(&argc, argv);
	
	// To see OpenGL drawing, take out the GLUT_DOUBLE request.
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	
	//glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	
	glutInitWindowSize(Window_Width, Window_Height);
	
	// Open a window 
	Window_ID = glutCreateWindow( PROGRAM_TITLE );
	
	// Register the callback function to do the drawing. 
	glutDisplayFunc(&cbRenderScene);
	
	// If there's nothing to do, draw.
	glutIdleFunc(&cbRenderScene);
	
	// It's a good idea to know when our window's resized.
	glutReshapeFunc(&cbResizeScene);
	
	// And let's get some keyboard input.
	glutKeyboardFunc(&cbKeyPressed);
	glutSpecialFunc(&cbSpecialKeyPressed);
	
	// OK, OpenGL's ready to go.  Let's call our own init function.
	// This will also load up the Lightwave object and prepare it for viewing.
	initScene(Window_Width, Window_Height, argv[1]);
	
	// Attach some controls to specific elements in the scene we loaded.
	
	if (strstr(argv[1],"lws") && strstr(argv[1],"zen"))
		attachControls();
	
	// Print out a bit of help dialog.
	printf("\nLightwave Viewer\n\n"
	 "In Model mode, use arrow keys to rotate, 'R' to reverse, 'S' to stop.\n"
	 "Page up/down will move the Lightwave model away from/towards camera.\n\n"
	 "In Scene mode, use arrow keys to pivot camera, PgUp/PgDn to dolly,\n"
	 "Z/z for left flipper, ?/ for right flipper.\n\n"
	 "Use first letter of shown display mode settings to alter.\n\n"
	 "Q or [Esc] to quit; OpenGL window must have focus for input.\n");
	
	// Pass off control to OpenGL.
	// Above functions are called as appropriate.
	glutMainLoop();
	
	
	return 1;
}

/*
// Make a GL list of operations necessary to draw the object.
glNewList(1,GL_COMPILE);

  // OK, let's start drawing our planer quads.
  glBegin(GL_TRIANGLES);
  
	
	  //If I were doing this for real, I'd be doing backface culling
	  //and such before feeding the data to OpenGL, but for right
	  //now I just wanna get the thing working.  I'll play with that
	  //other stuff later.
	  for (unsigned int polyCount = 0; polyCount < theModel->m_NumPolygons; polyCount++)
	  {
	  Vector3 v3;	
	  
		
		  memcpy(&v3,&(theModel->m_Polygons[polyCount].polyNormal),sizeof(Vector3));  	 	 		
		  
			glNormal3f( v3.x, v3.y, v3.z );
			int tagID = theModel->m_Polygons[polyCount].tag;
			
			  memcpy(&v3, &(theModel->m_Surfaces[tagID].colr), sizeof(Vector3));
			  glColor3f( v3.x, v3.y, v3.z);
			  
				// Now that we've got all the setup for this poly taken care of,
				// let's express the polygon list.  For the moment, we're
				// accepting any number of vertices, because OpenGL can.  Later
				// I'll chop things up into tri's during the preprocessing phase.
				
				  unsigned int vertCount = theModel->m_Polygons[polyCount].numVertices;
				  for(unsigned int vCount = 0; vCount < vertCount; vCount++)
				  {
				  // No texture coordinates yet, because I'm not doing textures yet.
				  // That would be a glTexCoord2f() call..
				  Vector3 v;
				  memcpy(&v,&(theModel->m_Polygons[polyCount].vertices[vCount]),sizeof(Vector3));
				  glVertex3f( v.x, v.y, v.z );			
				  }
				  }
				  
					// All polygons have been drawn.
					glEnd();
					
					  glEndList();
					  */
					
					
					
