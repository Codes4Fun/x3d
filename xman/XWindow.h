#ifndef XWindow_H
#define XWindow_H

#include "Vector.h"
#include "Matrix.h"

class XDisplay;

class XWindow
{
protected:
	friend class XDisplay;

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
	XWindow(Display * dpy, Window w, XWindow * next = NULL);
	~XWindow();

	void Add(XWindow * child);
	void Remove(XWindow * child);

	void UpdateHierarchy();

	bool Update(int x, int y, int width, int height);
	void Unmap();

	void Draw();

	XWindow * GetEventWindow(int event_mask, int &x, int &y);

	Window w() { return _w; }
	int width() { return _width; }
	int height() { return _height; }
	Matrix & matrix() { return _matrix; }
	bool mapped() { return _mapped; }
	int event_mask() { return _event_mask; }
	int x() { return _x; }
	int y() { return _y; }

	XWindow * parent() { return _parent; }
	XWindow * sibling() { return _sibling; }
	int nchildren() { return _nchildren; }
	XWindow * children() { return _children; }

	bool IsParent(XWindow * w);

	void SendMotionEvent(Window root, int x, int y, int state);
	void SendCrossingEvent(Window root, int x, int y, int state, int detail, Window child, bool enter);
	void SendButtonEvent(Window root, int x, int y, int button, int state, bool press);
	void SendKeyEvent(Window root, int key, int state, bool press);
};

#endif//XWindow_H

