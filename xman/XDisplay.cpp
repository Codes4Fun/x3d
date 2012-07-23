#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdamage.h>
#include <GL/gl.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "XWindow.h"
#include "XDisplay.h"

XWindow * XDisplay::s_table[1024];

bool XDisplay::GetNearest(Nearest &nearest, int event_mask)
{
	if (nearest._distance > nearest._radius)
	{
		nearest._w = NULL;
		nearest._distance = nearest._radius;
	}
	for (int i = 0; i < 1024; i++)
	{
		XWindow * w = s_table[i];
		for (; w; w = w->_next)
		{
			if (!w->_mapped /*|| !(w->_event_mask & event_mask)*/ || w->_hdepth != 1)
			{
				continue;
			}
			Matrix inv = w->_matrix;
			inv.FastInverse();
			Vector3 local = inv * nearest._pos;
			if (local._z < -nearest._distance)
			{
				//printf("%s:%d\n", __FILE__, __LINE__);
				continue;
			}
			if (local._z > nearest._distance)
			{
				//printf("%s:%d   %f > %f\n", __FILE__, __LINE__, local._z, nearest._distance);
				continue;
			}
			int zone;
			if (local._x < -nearest._distance)
			{
				//printf("%s:%d\n", __FILE__, __LINE__);
				continue;
			}
			else if (local._x < 0.f)
			{
				zone = 0;
			}
			else if (local._x < w->_width)
			{
				zone = 1;
			}
			else if (local._x <= w->_width + nearest._distance)
			{
				zone = 2;
			}
			else
			{
				//printf("%s:%d\n", __FILE__, __LINE__);
				continue;
			}
			if (local._y > nearest._distance)
			{
				//printf("%s:%d\n", __FILE__, __LINE__);
				continue;
			}
			else if (local._y > 0.f)
			{
				//zone += 0;
			}
			else if (local._y > -w->_height)
			{
				zone += 3;
			}
			else if (local._y >= -w->_height - nearest._distance)
			{
				zone += 6;
			}
			else
			{
				//printf("%s:%d\n", __FILE__, __LINE__);
				continue;
			}

			Vector3 pos;
			float distance;
			switch (zone)
			{
			case 0:
				pos = local;
				break;
			case 1:
				pos = Vector3(0.f, local._y, local._z);
				break;
			case 2:
				pos = local - Vector3(w->_width, 0.f, 0.f);
				break;
			case 3:
				pos = Vector3(local._x, 0, local._z);
				break;
			case 4:
				if (local._z > nearest._distance)
				{
					if (nearest._zone == 4)
					{
						//printf("%s:%d\n", __FILE__, __LINE__);
						continue;
					}
				}
				nearest._distance = local._z;
				nearest._framex = local._x;
				nearest._framey = local._y;
				nearest._w = w;
				nearest._zone = zone;
				continue;
			case 5:
				pos = local - Vector3(w->_width, local._y, 0.f);
				break;
			case 6:
				pos = local - Vector3(0.f, -w->_height, 0.f);
				break;
			case 7:
				pos = local - Vector3(local._x, -w->_height, 0.f);
				break;
			case 8:
				pos = local - Vector3(w->_width, -w->_height, 0.f);
				break;
			}
			distance = pos.length();
			if (distance > nearest._distance)
			{
				//printf("%s:%d\n", __FILE__, __LINE__);
				continue;
			}
			nearest._distance = distance;
			nearest._framex = local._x;
			nearest._framey = local._y;
			nearest._w = w;
			nearest._zone = zone;
		}
	}
	if (nearest._w)
	{
		nearest._frame = nearest._w;
		nearest._x = nearest._framex;
		nearest._y = -nearest._framey;
		nearest._w = nearest._w->GetEventWindow(event_mask, nearest._x, nearest._y);
	}
	return nearest._w != NULL;
}

bool XDisplay::HitTest(Hit &hit, int event_mask)
{
	for (int i = 0; i < 1024; i++)
	{
		XWindow * w = s_table[i];
		for (; w; w = w->_next)
		{
			if (!w->_mapped || !(w->_event_mask & event_mask) || w->_hdepth != 1)
			{
				continue;
			}
			Vector3 pos = hit._pos - w->matrix().translation();
			float d1 = Dot(pos, w->matrix().back());
			float d2 = Dot(hit._dir, w->matrix().back());
			if (d2 * d1 >= 0.f)
			{
				continue;
			}
			float t = fabs(d1 / d2);
			if (t > hit._t)
			{
				continue;
			}
			Vector3 intersection = pos + hit._dir * t;
			float x = Dot(intersection, w->matrix().right());
			if (x < 0 || x > w->width())
			{
				continue;
			}
			float y = Dot(intersection, w->matrix().up());
			if (y > 0 || y < -w->height())
			{
				continue;
			}
			hit._t = t;
			hit._w = w;
			hit._matrix = w->matrix();
			hit._matrix.translation() = intersection - pos;
			hit._x = x;
			hit._y = y;
		}
	}

	return hit._w != NULL;
}

XWindow * XDisplay::GetWindow(Display * dpy, Window w)
{
	int index = ((w & 0xf0000000) >> 26) | (w & 0x3f);
	XWindow * xw = s_table[index];
	while (xw && !(xw->_dpy == dpy && xw->_w == w))
	{
		xw = xw->_next;
	}
	if (!xw)
	{
		xw = new XWindow(dpy, w, s_table[index]);
		if (!xw->Initialize())
		{
			delete xw;
			return NULL;
		}
		xw->_next = s_table[index];
		s_table[index] = xw;
	}
	return xw;
}

bool XDisplay::RemoveWindow(Window w)
{
}

void XDisplay::GetCross(XWindow * a, XWindow * b, Cross &cross)
{
	if (!a || !b || a == b) return;

	XWindow * c = a;
	while (c != b && !b->IsParent(c))
	{
		cross.Add(c);
		c = c->_parent;
		assert(c);
	}
	cross.Add(c);
	cross._base = cross._count - 1;
	while (c != b)
	{
		cross.Add(b);
		b = b->_parent;
		assert(b);
	}
}


#define CROSS_BLOCKS(x) (((x) + 7) & ~7)
XDisplay::Cross::Cross()
{
	_count = 0;
	_base = 0;
	_w = NULL;
}

XDisplay::Cross::~Cross()
{
	free(_w);
}

void XDisplay::Cross::Add(XWindow * w)
{
	if ((_count & 7) == 0)
	{
		if (!_w)
			_w = (XWindow**)malloc(sizeof(XWindow*) * CROSS_BLOCKS(_count + 1));
		else
			_w = (XWindow**)realloc(_w, sizeof(XWindow*) * CROSS_BLOCKS(_count + 1));
	}
	assert(w);
	_w[_count++] = w;
}


