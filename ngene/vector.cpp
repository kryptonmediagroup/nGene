/***************************************************************************
                          vector.cpp  -  description
                             -------------------
    begin                : Mon Mar 12 2001

 ***************************************************************************/

#include "vector.h"

Vector3 &operator*(const Vector3 &a, const Vector3 &b)
{
	static Vector3 c;	// unfortunately, this makes this routine not threadsafe.
	c.x = a.x * b.x;
	c.y = a.y * b.y;
	c.z = a.z * b.z;
	return c;
}

Vector3 &normalize(Vector3 &a)
{
	float d = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
	a.x /= d;
	a.y /= d;
	a.z /= d;
	return a;
}

bool operator==(const Vector3 &a, const Vector3 &b)
{
	return  (a.x == b.z && a.y == b.y && a.z == b.z);
}

void crossproduct(Vector3 &target, const Vector3 &a, const Vector3 &b)
{
	target.x = (a.y * b.z) - (a.z * b.y);
	target.y = (a.z * b.x) - (a.x * b.z);
	target.z = (a.x * b.y) - (a.y * b.x);		
}

Vector3 &operator*=(Vector3 &a, const Vector3 &b)
{
	a.x *= b.x;
	a.y *= b.y;
	a.x *= b.z;
	return a;
}

Vector3 &operator-=(Vector3 &a, const Vector3 &b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	return a;
}

Vector3 &operator/=(Vector3 &a, const Vector3 &b)
{
	a.x /= b.x;
	a.y /= b.y;
	a.z /= b.z;
	return a;
}




Vector3 &operator*=(Vector3 &a, float b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;
	return a;
}


void InvertVertexCoordinates(Vector3 *v)
{
	v->x *= -1.0f;
	v->y *= -1.0f;
	v->z *= -1.0f;
}

void MirrorOnYAxis(Vector3 *v)
{
	v->x *= -1.0f;
}
