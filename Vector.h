#ifndef VECTOR_H
#define VECTOR_H

struct Vector3
{
	float _x, _y, _z;

	static const float zero[4];
	static const float left[4];
	static const float right[4];
	static const float up[4];
	static const float down[4];
	static const float forward[4];
	static const float back[4];

	Vector3() {}

	Vector3(const Vector3 &v)
	{
		_x = v._x; _y = v._y; _z = v._z;
	}
	
	Vector3(float x, float y, float z)
	{
		_x = x; _y = y; _z = z;
	}

	Vector3(const float * v)
	{
		_x = v[0]; _y = v[1]; _z = v[2];
	}

	Vector3 & operator = (const Vector3 &v)
	{
		_x = v._x; _y = v._y; _z = v._z;
		return *this;
	}

	Vector3 & operator += (const Vector3 &v)
	{
		_x += v._x; _y += v._y; _z += v._z;
		return *this;
	}

	Vector3 & operator -= (const Vector3 &v)
	{
		_x -= v._x; _y -= v._y; _z -= v._z;
		return *this;
	}

	Vector3 & operator *= (const Vector3 &v)
	{
		_x *= v._x; _y *= v._y; _z *= v._z;
		return *this;
	}

	Vector3 & operator /= (const Vector3 &v)
	{
		_x /= v._x; _y /= v._y; _z /= v._z;
		return *this;
	}

	Vector3 & operator *= (float s)
	{
		_x *= s; _y *= s; _z *= s;
		return *this;
	}

	Vector3 & operator /= (float d)
	{
		_x /= d; _y /= d; _z /= d;
		return *this;
	}

	float length();

	void normalize();
};

static inline float Dot(const Vector3 &l, const Vector3 &r)
{
	return l._x * r._x + l._y * r._y + l._z * r._z;
}

static inline Vector3 operator + (const Vector3 &l, const Vector3 &r)
{
	return Vector3(l._x + r._x, l._y + r._y, l._z + r._z);
}

static inline Vector3 operator - (const Vector3 &l, const Vector3 &r)
{
	return Vector3(l._x - r._x, l._y - r._y, l._z - r._z);
}

static inline Vector3 operator * (const Vector3 &l, const Vector3 &r)
{
	return Vector3(l._x * r._x, l._y * r._y, l._z * r._z);
}

static inline Vector3 operator / (const Vector3 &l, const Vector3 &r)
{
	return Vector3(l._x / r._x, l._y / r._y, l._z / r._z);
}

static inline Vector3 operator * (const Vector3 &l, float r)
{
	return Vector3(l._x * r, l._y * r, l._z * r);
}

static inline Vector3 operator / (const Vector3 &l, float r)
{
	return Vector3(l._x / r, l._y / r, l._z / r);
}

Vector3 normalize(const Vector3 &v);

struct Vector4 : public Vector3
{
	float _w;
	Vector4() {}

	Vector4(const Vector3 & v) : Vector3(v)
	{
		_w = 1.f;
	}

	Vector4(const Vector3 & v, float w) : Vector3(v)
	{
		_w = w;
	}

	Vector4(float x, float y, float z, float w)
	{
		_x = x; _y = y; _z = z; _w = w;
	}
};

#endif//VECTOR_H

