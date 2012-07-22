#include <string.h>
#include <math.h>
#include "Matrix.h"

const float Matrix::identity[16] = 
{
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

Matrix::Matrix(const Matrix &m)
{
	memcpy(_m, m._m, sizeof(_m));
}

Matrix::Matrix(const float * m)
{
	memcpy(_m, m, sizeof(_m));
}


void Matrix::PrependTranslate(float x, float y, float z)
{
	Matrix m(Matrix::identity);
	m._m[12] = x;
	m._m[13] = y;
	m._m[14] = z;
	(*this) = (*this) * m;
}

void Matrix::PrependScale(float x, float y, float z)
{
	Matrix m(Matrix::identity);
	m._m[0] = x;
	m._m[5] = y;
	m._m[10] = z;
	(*this) = (*this) * m;
}

void Matrix::PrependRotate(float angle, float x, float y, float z)
{
	Vector3 n(x, y, z);
	n.normalize();
	float rad = angle * 3.141593f / 180.f;
	float c = cos(rad);
	float s = sin(rad);
	float ic = 1 - c;
	float xy = x * y;
	float xz = x * z;
	float yz = y * z;
	float xs = x * s;
	float ys = y * s;
	float zs = z * s;
	Matrix m(Matrix::identity);
	m._m[0] = x * x * ic + c;
	m._m[1] = xy * ic + zs;
	m._m[2] = xz * ic - ys;

	m._m[4] = xy * ic - zs;
	m._m[5] = y * y * ic + c;
	m._m[6] = yz * ic + xs;

	m._m[8] = xz * ic + ys;
	m._m[9] = yz * ic - xs;
	m._m[10] = z * z * ic + c;
	(*this) = (*this) * m;
}


void Matrix::AppendTranslate(float x, float y, float z)
{
	Matrix m(Matrix::identity);
	m._m[12] = x;
	m._m[13] = y;
	m._m[14] = z;
	(*this) = m * (*this);
}

void Matrix::AppendScale(float x, float y, float z)
{
	Matrix m(Matrix::identity);
	m._m[0] = x;
	m._m[5] = y;
	m._m[10] = z;
	(*this) = m * (*this);
}

void Matrix::AppendRotate(float angle, float x, float y, float z)
{
	Vector3 n(x, y, z);
	n.normalize();
	float rad = angle * 3.141593f / 180.f;
	float c = cos(rad);
	float s = sin(rad);
	float ic = 1 - c;
	float xy = x * y;
	float xz = x * z;
	float yz = y * z;
	float xs = x * s;
	float ys = y * s;
	float zs = z * s;
	Matrix m(Matrix::identity);
	m._m[0] = x * x * ic + c;
	m._m[1] = xy * ic + zs;
	m._m[2] = xz * ic - ys;

	m._m[4] = xy * ic - zs;
	m._m[5] = y * y * ic + c;
	m._m[6] = yz * ic + xs;

	m._m[8] = xz * ic + ys;
	m._m[9] = yz * ic - xs;
	m._m[10] = z * z * ic + c;
	(*this) = m * (*this);
}

#if 1
void Matrix::FastInverse()
{
	float temp1, temp2, temp3;
	float tx, ty, tz, tw;
	float sx, sy, sz;

	// get the scale
	sx = right().length();
	sy = up().length();
	sz = back().length();

	// normalize
	right() /= sx;
	up() /= sy;
	back() /= sz;

	// inverse rotation
	temp1 = _m[1];
	temp2 = _m[2];
	temp3 = _m[6];
	_m[1] = _m[4];
	_m[2] = _m[8];
	_m[6] = _m[9];
	_m[4] = temp1;
	_m[8] = temp2;
	_m[9] = temp3;

	// inverse scale
	right() /= sx;
	up() /= sy;
	back() /= sz;

	// inverse translation
	tx = _m[12] * _m[0]
	   + _m[13] * _m[4]
	   + _m[14] * _m[8];
	ty = _m[12] * _m[1]
	   + _m[13] * _m[5]
	   + _m[14] * _m[9];
	tz = _m[12] * _m[2]
	   + _m[13] * _m[6]
	   + _m[14] * _m[10];
	tw = _m[12] * _m[3]
	   + _m[13] * _m[7]
	   + _m[14] * _m[11]
	   + _m[15];

	_m[12] = -tx;
	_m[13] = -ty;
	_m[14] = -tz;
	_m[15] = tw;
}
#else
void Matrix::FastInverse()
{
	float temp, x, y, z, w, tx, ty, tz, tw;
	x = _m[12];
	y = _m[13];
	z = _m[14];
	w = _m[15];
	_m[12] = 0;
	_m[13] = 0;
	_m[14] = 0;
	_m[15] = 1;

	temp = _m[1];
	_m[1] = _m[4];
	_m[4] = temp;
	temp = _m[2];
	_m[2] = _m[8];
	_m[8] = temp;
	temp = _m[6];
	_m[6] = _m[9];
	_m[9] = temp;

	tx = x * _m[0]
	   + y * _m[4]
	   + z * _m[8]
	   + w * _m[12];
	ty = x * _m[1]
	   + y * _m[5]
	   + z * _m[9]
	   + w * _m[13];
	tz = x * _m[2]
	   + y * _m[6]
	   + z * _m[10]
	   + w * _m[14];
	tw = x * _m[3]
	   + y * _m[7]
	   + z * _m[11]
	   + w * _m[15];

	_m[12] = -tx;
	_m[13] = -ty;
	_m[14] = -tz;
	_m[15] = tw;
}
#endif


