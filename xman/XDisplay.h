#ifndef XDISPLAY_H
#define XDISPLAY_H

class XWindow;

class XDisplay
{
protected:
	static XWindow * s_table[1024];

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

	class Cross
	{
	public:
		int _count;
		int _base;
		XWindow ** _w;

		Cross();
		~Cross();

		void Add(XWindow * w);
	};

	static bool GetNearest(Nearest &nearest, int event_mask);
	static bool HitTest(Hit &hit, int event_mask); 
	static XWindow * GetWindow(Display * dpy, Window w);
	static bool RemoveWindow(Window w);
	static void GetCross(XWindow * a, XWindow * b, Cross & cross);
};

#endif//XDISPLAY_H
