#include <math.h>
#include "Vector.h"

const float Vector3::zero[4] = {0.f, 0.f, 0.f, 0.f};
const float Vector3::left[4] = {-1.f, 0.f, 0.f, 0.f};
const float Vector3::right[4] = {1.f, 0.f, 0.f, 0.f};
const float Vector3::up[4] = {0.f, 1.f, 0.f, 0.f};
const float Vector3::down[4] = {0.f, -1.f, 0.f, 0.f};
const float Vector3::forward[4] = {0.f, 0.f, -1.f, 0.f};
const float Vector3::back[4] = {0.f, 0.f, 1.f, 0.f};

float Vector3::length()
{
	float d = Dot(*this, *this);
	return sqrtf(d);
}

void Vector3::normalize()
{
	float d = length();
	if (d == 0.f)
	{
		return;
	}
	_x /= d;
	_y /= d;
	_z /= d;
}

Vector3 normalize(const Vector3 &v)
{
	float d = Dot(v, v);
	if (d == 0.f)
	{
		return Vector3(Vector3::zero);
	}
	d = sqrtf(d);
	return v / d;
}



