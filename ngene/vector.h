/***************************************************************************
                          vector.h  -  description
 ***************************************************************************/

#ifndef CVECTOR_H
#define CVECTOR_H

#include <math.h>

/**Vector class, including all pertinent matrix operations

  *@author Gene Turnbow
  */
typedef struct structVector3
{	union 
	{
		float x;
		float r;
	};
	union
	{
		float y;
		float g;
	};
	union
	{
		float z;
		float b;
	};
} Vector3; 


	
typedef struct structVectorColor4
{	float r, g, b, a; } VectorColor4;


typedef struct structVertex
{
	Vector3 vec;	// coordinates
} Vertex;

Vector3 &operator*(const Vector3 &a, const Vector3 &b);
Vector3 &normalize(Vector3 &a);
bool operator==(const Vector3 &a, const Vector3 &b);
void crossproduct(Vector3 &target, const Vector3 &a, const Vector3 &b);
Vector3 &operator*=(Vector3 &a, const Vector3 &b);
Vector3 &operator-=(Vector3 &a, const Vector3 &b);
Vector3 &operator/=(Vector3 &a, const Vector3 &b);

Vector3 &operator/=(Vector3 &a, float b);
Vector3 &operator*=(Vector3 &a, float b);

VectorColor4 &assign(VectorColor4 &a, VectorColor4 &b);

void InvertVertexCoordinates(Vector3 *v);
void MirrorOnYAxis(Vector3 *v);

#endif
