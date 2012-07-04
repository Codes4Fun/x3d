#ifndef MATRIX_H
#define MATRIX_H

#include "Vector.h"

struct Matrix
{
	float _m[16];

	static const float identity[16];

	Matrix() {}

	Matrix(const Matrix &m);

	Matrix(const float * m);

	Vector4 & right()
	{
		return *((Vector4*)_m);
	}

	Vector4 & up()
	{
		return *((Vector4*)(_m + 4));
	}

	Vector4 & back()
	{
		return *((Vector4*)(_m + 8));
	}

	Vector4 & translation()
	{
		return *((Vector4*)(_m + 12));
	}

	Vector4 & right() const
	{
		return *((Vector4*)_m);
	}

	Vector4 & up() const
	{
		return *((Vector4*)(_m + 4));
	}

	Vector4 & back() const
	{
		return *((Vector4*)(_m + 8));
	}

	Vector4 & translation() const
	{
		return *((Vector4*)(_m + 12));
	}

	void PrependTranslate(float x, float y, float z);
	void PrependScale(float x, float y, float z);
	void PrependRotate(float angle, float x, float y, float z);

	void AppendTranslate(float x, float y, float z);
	void AppendScale(float x, float y, float z);
	void AppendRotate(float angle, float x, float y, float z);

	inline void Translate(float x, float y, float z) { PrependTranslate(x,y,z); }
	inline void Scale(float x, float y, float z) { PrependScale(x,y,z); }
	inline void Rotate(float angle, float x, float y, float z)  { PrependRotate(angle, x,y,z); }

	void FastInverse();
};

static inline Vector3 operator * (const Matrix &m, const Vector3 &v)
{
	return Vector3(
		m._m[0] * v._x + m._m[4] * v._y + m._m[8] * v._z + m._m[12],
		m._m[1] * v._x + m._m[5] * v._y + m._m[9] * v._z + m._m[13],
		m._m[2] * v._x + m._m[6] * v._y + m._m[10] * v._z + m._m[14]);
}

static inline Vector4 operator * (const Matrix &m, const Vector4 &v)
{
	return Vector4(
		m._m[0] * v._x + m._m[4] * v._y + m._m[8] * v._z + m._m[12] * v._w,
		m._m[1] * v._x + m._m[5] * v._y + m._m[9] * v._z + m._m[13] * v._w,
		m._m[2] * v._x + m._m[6] * v._y + m._m[10] * v._z + m._m[14] * v._w,
		m._m[3] * v._x + m._m[7] * v._y + m._m[11] * v._z + m._m[15] * v._w);
}

static inline Matrix operator * (const Matrix &l, const Matrix &r)
{
	Matrix m;
	m.right() = l * r.right();
	m.up() = l * r.up();
	m.back() = l * r.back();
	m.translation() = l * r.translation();
	return m;
}

#endif//MATRIX_H
