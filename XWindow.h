#ifndef XWindow_H
#define XWindow_H

#include "Vector.h"
#include "Matrix.h"

class XWindow
{
protected:
	static XWindow * s_table[256];

	Display * _dpy;
	Window _w;
	XWindow * _next;
	XWindow * _parent;
	XWindow * _sibling;
	int _nchildren;
	XWindow * _children;
	char * _name;
	GLuint _texture;
	int _x;
	int _y;
	int _width;
	int _height;
	int _hdepth;

	int _event_mask;

	bool _textured;
	bool _mapped;

	Matrix _matrix;

	bool Initialize();

public:

	struct Hit
	{
		Vector4 _pos;
		Vector4 _dir;
		float _t;
		XWindow * _w;
		Matrix _matrix;
		float _x;
		float _y;
		Hit()
		{
			_t = 1000000.f;
			_w = NULL;
		}
		Hit(const Vector3 &pos, const Vector3 &dir)
		{
			_t = 1000000.f;
			_w = NULL;
			_pos = pos;
			_dir = dir;
		}
	};

	struct Nearest
	{
		Vector4 _pos;
		float _radius;
		float _distance;
		XWindow * _frame;
		float _framex;
		float _framey;
		XWindow * _w;
		int _x;
		int _y;
		int _zone;
		Nearest() {}
		Nearest(const Vector3 & pos, float radius)
		{
			_pos = pos;
			_radius = radius;
			_distance = 1000000.f;
		}
	};

	XWindow(Display * dpy, Window w, XWindow * next = NULL);
	~XWindow();

	void Add(XWindow * child);
	void Remove(XWindow * child);

	void UpdateHierarchy();

	bool Update(int x, int y, int width, int height);
	void Unmap();

	void Draw();

	XWindow * GetEventWindow(int event_mask, int &x, int &y);

	static bool GetNearest(Nearest &nearest, int event_mask);
	static bool HitTest(Hit &hit, int event_mask); 
	static XWindow * GetWindow(Display * dpy, Window w);
	static bool RemoveWindow(Window w);

	Window w() { return _w; }
	int width() { return _width; }
	int height() { return _height; }
	Matrix & matrix() { return _matrix; }
	bool mapped() { return _mapped; }
	int event_mask() { return _event_mask; }
	int x() { return _x; }
	int y() { return _y; }

	XWindow * sibling() { return _sibling; }
	int nchildren() { return _nchildren; }
	XWindow * children() { return _children; }

	void SendMotionEvent(Window root, int x, int y, int state);
	void SendCrossingEvent(Window root, int x, int y, int state, bool enter);
	void SendButtonEvent(Window root, int x, int y, int button, int state, bool press);
	void SendKeyEvent(Window root, int key, int state, bool press);
};

#endif//XWindow_H

