
// use with: Xephyr :9 +bs -wm -screen 1280x720
// then: phasetest -display :9

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdamage.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <sys/time.h>

#include "Hydra.h"
#include "XWindow.h"

#define ESCAPE 9

Display * g_gldpy;
Window g_glwin;
int g_glscreen;
XVisualInfo *g_glvisinfo;
GLXContext g_glctx;

int g_width = 640;
int g_height = 480;
float g_scale = 1.f;

float g_ppi = 117.f;
Vector3 g_pos(0,0,30.0f);

Matrix g_camera(Matrix::identity);

XWindow * xw;
Display * g_dpy;
Window g_root;
Window g_mouse_focus;
Window g_kb_focus;
int g_button_state = 0;

Hydra * g_hydra;

unsigned int GetTime()
{
	timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * 1000 + time.tv_usec / 1000;
}


void SetupProjection2D(int Width, int Height)
{
   glViewport(0, 0, Width, Height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0.f, (GLfloat)Width, (GLfloat)Height, 0.f, -100.f, 100.f);

   glMatrixMode(GL_MODELVIEW);
}

void SetupProjection3D(int Width, int Height)
{
   if (Height==0)				// Prevent A Divide By Zero If The Window Is Too Small
	  Height=1;
   glViewport(0, 0, Width, Height);		// Reset The Current Viewport And Perspective Transformation

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   float near = 0.1f;
   float yscale = Height / (float)Width;
   glFrustum(-near, near, -near * yscale, near * yscale, near, 1000.f);

   glMatrixMode(GL_MODELVIEW);
}

/* A general OpenGL initialization function.  Sets all of the initial parameters. */
void InitGL(int Width, int Height)			// We call this right after our OpenGL window is created.
{
	g_width = Width;
	g_height = Height;

	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_BLEND);
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//glClearColor(1.f, 1.f, 1.f, 1.f);
	glClearColor(96.f / 255.f, 118.f / 255.f, 98.f / 255.f, 1.f);
}

/* The function called when our window is resized (which shouldn't happen, because we're fullscreen) */
void ReSizeGLScene(int Width, int Height)
{
	g_width = Width;
	g_height = Height;
}

int prev_frame = -1;
int frame = 0;

void DrawCursor()
{
	float verts[] =
	{
		 0.0f, 0.0f,

		-0.5f,-1.5f,
		-0.2f,-1.2f,
		-0.2f,-2.0f,

		 0.2f,-2.0f,
		 0.2f,-1.2f,
		 0.5f,-1.5f,
	};
	float verts2[] =
	{
		 0.0f, 0.3f,
		-0.2f, 0.2f,

		-0.8f,-1.3f,
		-0.7f,-1.7f,
		-0.5f,-1.7f,

		-0.4f,-1.5f,
		-0.4f,-2.3f,
		 0.4f,-2.3f,
		 0.4f,-1.5f,

		 0.5f,-1.7f,
		 0.7f,-1.7f,
		 0.8f,-1.3f,

		 0.2f, 0.2f,
	};

	glDisable(GL_TEXTURE_2D);
	glVertexPointer(2, GL_FLOAT, 0, verts );

	glColor4f(1.f, 1.f, 1.f, 1.f);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 7);

	//glTranslatef(0.f, 0.25f, -0.01f);
	//glScalef(1.25f, 1.25f, 1.25f);
	//glColor4f(0.f, 0.f, 0.f, 1.f);
	//glDrawArrays(GL_TRIANGLE_FAN, 0, 13);

	glTranslatef(0.f, 0.f, -0.01f);
	glVertexPointer(2, GL_FLOAT, 0, verts2 );
	glColor4f(0.f, 0.f, 0.f, 1.f);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 13);
}

void DrawCursorShadow(float closeness)
{
	float verts2[] =
	{
		 0.0f,-0.5f,
		 0.0f, 0.3f,
		-0.2f, 0.2f,

		-0.8f,-1.3f,
		-0.7f,-1.7f,
		-0.5f,-1.7f,

		-0.4f,-1.5f,
		-0.4f,-2.3f,
		 0.4f,-2.3f,
		 0.4f,-1.5f,

		 0.5f,-1.7f,
		 0.7f,-1.7f,
		 0.8f,-1.3f,

		 0.2f, 0.2f,
		 0.0f, 0.3f,
	};

	closeness = 1.f - closeness * closeness;

	float colors[] =
	{
		 1.0f, 1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,

		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,

		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,

		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,

		 1.0f, 1.0f, 1.0f, closeness,
		 1.0f, 1.0f, 1.0f, closeness,
	};

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glVertexPointer(2, GL_FLOAT, 0, verts2 );
	glColorPointer(4, GL_FLOAT, 0, colors);
	glColor4f(0.f, 0.f, 0.f, closeness);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 15);
}

void DrawGLScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer

	SetupProjection3D(g_width, g_height);

	glPushMatrix();
	glLoadIdentity();

	float screen = xw->width();

	//xw->matrix() = *(Matrix*)Matrix::identity;
	//xw->matrix().Translate(0, 0, screen);
	//xw->matrix().Rotate(0.1f * frame, 0, 1, 0);

#if 0
	int nchildren = 0;
	int max_width = 0;
	int max_height = 0;
	for (XWindow * child = xw->children(); child; child = child->sibling())
	{
		if (!child->mapped())
		{
			continue;
		}
		nchildren++;
		if (child->width() > max_width) max_width = child->width();
		if (child->height() > max_height) max_height = child->height();
	}

	float start_x = -960.f;
	float start_y = 540.f;
	int i = 0;
	float left = start_x;
	float top = start_y;
	max_width = 0;
	for (XWindow * child = xw->children(); child; child = child->sibling())
	{
		if (!child->mapped())
		{
			continue;
		}
		if ((top - child->height()) < -start_y)
		{
			left += max_width + 16.f;
			top = start_y;
			max_width = 0;
		}
		child->matrix() = *(Matrix*)Matrix::identity;
		child->matrix().Translate(left, top, 0);
		float x = left + child->width() *0.5f;
		child->matrix().Rotate(20.f * x / start_x, 0.f ,1.f, 0.f);
		//child->matrix().Rotate(20.f - 40.f * (i + 0.5f) / nchildren, 0.f ,1.f, 0.f);
		top -= child->height() + 16.f;
		if (child->width() > max_width) max_width = child->width();
		i++;
	}
#endif

	Matrix left, right;
	static int controli = 0;
	static float controls[2][20];
#if 1
	int hands = getHands(g_hydra, left, right, controls[controli]);
#endif
	controli = 1 - controli;
	static bool active = false;
	static bool tracking = false;
	static Matrix relativeMat;
	if (!active)
	{
		if (hands)
		{
			printf("-= hands activated =-\n");
			active = true;
		}
	}
	else
	{
		if (!hands)
		{
			printf("-= hands deactivated =-\n");
			active = false;
		}
	}
	if (hands & 1)
	{
		//for (int i = 0; i < 10; i++)
		//	printf("%f ", controls[1-controli][i]);
		//printf("\n");
		if (!tracking)
		{
			if (controls[1-controli][4] > 0.75f && controls[controli][4] < 0.25f )
			{
				Matrix inv = left;
				inv.FastInverse();
				relativeMat = g_camera * inv;
				tracking = true;
			}
		}
		else
		{
			if (controls[1-controli][4] > 0.75f && controls[controli][4] < 0.25f )
			{
				tracking = false;
			}
			else
			{
				g_camera = relativeMat * left;
				//printf("%f %f %f\n", g_camera.translation()._x, g_camera.translation()._y, g_camera.translation()._z);
			}
		}
		//printf("hand %f %f %f, %f %f %f\n", left.translation()._x, left.translation()._y, left.translation()._z, left.back()._x, left.back()._y, left.back()._z);
	}

	Matrix inv = g_camera;
	inv.FastInverse();
	glMultMatrixf(inv._m);

	glTranslatef( -g_pos._x, -g_pos._y, -g_pos._z);

	g_scale = g_ppi / 2.56f;//960.f;

	static Matrix grabMat;
	static Matrix grabMat2;
	static bool grabbing = false;
	static XWindow::Nearest nearest;
	if (hands & 2)
	{
#if 0
		for (int i = 10; i < 20; i++)
			printf("%f ", controls[1-controli][i]);
		printf("\n");
#endif
		if (!grabbing)
		{
			nearest._pos = (right.translation() + Vector3(0, -12.f * 2.56f, 0)) * g_scale;
			nearest._radius = 64.f;
			nearest._distance = 100000.f;
			nearest._w = NULL;
			bool found = XWindow::GetNearest(nearest, 0);
			if (nearest._w)
			{
				printf("nearest %08x\n", (int)nearest._w->w());
			}

#if 0
			Matrix pixMat = right;
			pixMat.AppendTranslate(0, -12.f * 2.56f, 0);
			pixMat.AppendScale(g_scale, g_scale, g_scale);

			printf(" %f %f %f ", nearest._pos._x, nearest._pos._y, nearest._pos._z);
			printf(" %f %f %f ", pixMat.translation()._x, pixMat.translation()._y, pixMat.translation()._z);
			if (found)
				printf("near %08x\n %f %f\n", (int)nearest._w->w(), nearest._x, nearest._y);
			else
				printf("near None\n");
#endif
			if (!found)
			{
				if (g_mouse_focus != None)
				{
					XWindow * w = XWindow::GetWindow(g_dpy, g_mouse_focus);
					w->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyAncestor, None, false);
					XWindow * child = w;
					for (w = w->parent(); w != xw; w = w->parent())
					{
						w->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyVirtual, child->w(), false);
						child = w;
					}
					g_mouse_focus = None;
				}
			}
			else
			{
				if (g_mouse_focus != nearest._w->w())
				{
					if (g_mouse_focus != None)
					{
						XWindow * w = XWindow::GetWindow(g_dpy, g_mouse_focus);
						XWindow::Cross cross;
						XWindow::GetCross(w, nearest._w, cross);
						if (cross._base == 0)
						{
							cross._w[0]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyInferior, None, false);
							for (int i = cross._count - 1; i > 1; i--)
							{
								cross._w[i]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyVirtual, cross._w[i - 1]->w(), false);
							}
							cross._w[cross._base + 1]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyAncestor, None, true);
						}
						else if (cross._base == cross._count - 1)
						{
							cross._w[0]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyAncestor, None, false);
							for (int i = 1; i < cross._base; i++)
							{
								cross._w[i]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyVirtual, cross._w[i - 1]->w(), true);
							}
							cross._w[cross._base]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyInferior, None, true);
						}
						else
						{
							cross._w[0]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyNonlinear, None, false);
							for (int i = 1; i < cross._base; i++)
							{
								cross._w[i]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyNonlinearVirtual, cross._w[i - 1]->w(), false);
							}
							for (int i = cross._count - 1; i > cross._base + 1; i--)
							{
								cross._w[i]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyNonlinearVirtual, cross._w[i - 1]->w(), true);
							}
							cross._w[cross._base + 1]->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyNonlinear, None, true);
						}
						g_mouse_focus = nearest._w->w();
					}
					else
					{
						g_mouse_focus = nearest._w->w();
						XWindow * parent = nearest._frame;
						while (parent != nearest._w)
						{
							XWindow * child = nearest._w;
							for (; child->parent() != parent; child = child->parent());
							parent->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyVirtual, child->w(), true);
							parent = child;
						}
						nearest._w->SendCrossingEvent(g_root, nearest._x, nearest._y, g_button_state, NotifyAncestor, None, true);
					}
				}
				if (controls[1-controli][18] > 0.75f && controls[controli][18] < 0.25f )
				{
					if (nearest._w->w() != g_kb_focus)
					{
						g_kb_focus = nearest._w->w();
						XSetInputFocus(g_dpy, g_kb_focus, RevertToParent, CurrentTime);
					}
					nearest._w->SendButtonEvent(g_root, nearest._x, nearest._y, Button1, g_button_state, true);
					g_button_state |= Button1Mask;
				}
				else if (controls[controli][18] > 0.75f && controls[1-controli][18] < 0.25f )
				{
					nearest._w->SendButtonEvent(g_root, nearest._x, nearest._y, Button1, g_button_state, false);
					g_button_state &= ~Button1Mask;
				}
				else if (controls[1-controli][16] > 0.75f && controls[controli][16] < 0.25f )
				{
				}
				else if (controls[1-controli][14] > 0.75f && controls[controli][14] < 0.25f )
				{
					Matrix pixMat = right;
					pixMat.AppendTranslate(0, -12.f * 2.56f, 0);
					pixMat.AppendScale(g_scale, g_scale, g_scale);
					pixMat.FastInverse();
					grabMat = pixMat * nearest._frame->matrix();

					grabbing = true;
				}
				else
				{
					nearest._w->SendMotionEvent(g_root, nearest._x, nearest._y, g_button_state);
				}
			}
		}
		else
		{
			if (controls[1-controli][14] < 0.25f )
			{
				grabbing = false;
			}
			else
			{
				Matrix pixMat = right;
				pixMat.AppendTranslate(0, -12.f * 2.56f, 0);
				pixMat.AppendScale(g_scale, g_scale, g_scale);
				nearest._frame->matrix() = pixMat * grabMat;
				//printf("%f %f %f\n", g_camera.translation()._x, g_camera.translation()._y, g_camera.translation()._z);
			}
		}
		glPushMatrix();
		glTranslatef( 0, -12.f * 2.56f, 0);
		glMultMatrixf(right._m);
		glScalef(0.3f, 0.3f, 0.3f);
		DrawCursor();
		glPopMatrix();
		//printf("hand %f %f %f\n", right.translation()._x, right.translation()._y, right.translation()._z);
	}

	glScalef(1.f / g_scale, 1.f / g_scale, 1.f / g_scale);

	xw->Draw();


	if (nearest._frame)
	{
		glPushMatrix();
		glMultMatrixf(nearest._frame->matrix()._m);
		glTranslatef(nearest._framex, nearest._framey, 1.0f);
		glScalef(0.3f * g_scale, 0.3f * g_scale, 1.f);
		DrawCursorShadow(nearest._distance / 64.f);
		glPopMatrix();
	}

	glPopMatrix();

	glFinish();

	glXSwapBuffers(g_gldpy, g_glwin);

	frame++;
}

void keyPressed(unsigned char key, int x, int y) 
{
	if (key == ESCAPE) 
	{
		exitHydra();
		exit(0);
	}
}

static void usage(char * program_name)
{
	fprintf (stderr, "usage: %s [-display host:dpy]", program_name);
	exit(1);
}


static const char s_tabs[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
#define TABS(x) (&s_tabs[sizeof(s_tabs) - (x)])

void DumpWindow(Display * dpy, Window w, int depth)
{
	Window *children, dummy;
	unsigned int nchildren;
	int i;
	char *window_name;
	XWindowAttributes attrib;
	
	if (!XFetchName(dpy, w, &window_name))
	{
		window_name = NULL;
	}

	XGetWindowAttributes(dpy, w, &attrib);

	printf("%s%08x (%d, %d) \"%s\" %08x %d %d %d %d\n", TABS(depth),
		(unsigned int)w, attrib.c_class, attrib.map_state, window_name, (int)attrib.all_event_masks,
		attrib.x, attrib.y, attrib.width, attrib.height);

	if (!XQueryTree(dpy, w, &dummy, &dummy, &children, &nchildren))
	{
		return;
	}

	for (i=0; i<nchildren; i++)
	{
		DumpWindow(dpy, children[i], depth + 1);
	}

	if (children)
	{
		XFree ((char *)children);
	}
}


Window createWindow(const char * name, int width, int height)
{
   int attrib[] = { GLX_RGBA,
			GLX_RED_SIZE, 1,
			GLX_GREEN_SIZE, 1,
			GLX_BLUE_SIZE, 1,
			GLX_DOUBLEBUFFER,
			GLX_DEPTH_SIZE, 1,
			None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;

   g_gldpy = XOpenDisplay(NULL);
   if (!g_gldpy)
   {
	  printf("Error: couldn't open display %d\n", 0);
	  return -1;
   }

   scrnum = DefaultScreen( g_gldpy );
   root = RootWindow( g_gldpy, scrnum );

   g_glvisinfo = glXChooseVisual( g_gldpy, scrnum, attrib );
   if (!g_glvisinfo) {
	  printf("Error: couldn't get an RGB, Double-buffered visual\n");
	  exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( g_gldpy, root, g_glvisinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	attr.event_mask |= PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( g_gldpy, root, 0, 0, width, height,
				0, g_glvisinfo->depth, InputOutput,
				g_glvisinfo->visual, mask, &attr );

   /* set hints and properties */
   {
	  XSizeHints sizehints;
	  sizehints.x = 0;
	  sizehints.y = 0;
	  sizehints.width  = width;
	  sizehints.height = height;
	  sizehints.flags = USSize | USPosition;
	  XSetNormalHints(g_gldpy, win, &sizehints);
	  XSetStandardProperties(g_gldpy, win, name, name,
							  None, (char **)NULL, 0, &sizehints);
   }

   return win;
}





void initializeWindows(Display * dpy)
{
	Window *children, dummy;
	unsigned int nchildren;
	Window root = DefaultRootWindow(dpy);

	if (!XQueryTree(dpy, root, &dummy, &dummy, &children, &nchildren))
	{
		return;
	}

	for (int i=0; i<nchildren; i++)
	{
		//DumpWindow(children[i], depth + 1);
	}

	if (children)
	{
		XFree ((char *)children);
	}
}


int OnXErrorEvent(Display * dpy, XErrorEvent * error)
{
	printf("had an error\n");
}


int main(int argc, char **argv) 
{
	int i;

	Display * dpy;
	char * display_name = NULL;
	for (i = 1; i < argc; i++)
	{
		char *arg = argv[i];

		if (!strcmp (arg, "-display") || !strcmp (arg, "-d"))
		{
			if (++i >= argc)
			{
				usage (argv[0]);
			}

			display_name = argv[i];
			continue;
		}
	}

	if (!display_name)
	{
		usage(argv[0]);
	}

	dpy = XOpenDisplay(display_name);
	g_dpy = dpy;
	if (!dpy)
	{
		fprintf (stderr, "%s:  unable to open display '%s'\n",
					argv[0], XDisplayName (display_name));
		usage (argv[0]);
	}

	int damageError, damageEvent;
	if (!XDamageQueryExtension (dpy, &damageEvent, &damageError))
	{
		fprintf (stderr, "%s: No damage extension\n", argv[0]);
		return 1;
	}

	g_glwin = createWindow("test", 640, 480);
	g_glctx = glXCreateContext( g_gldpy, g_glvisinfo, NULL, True );
	if (!g_glctx)
	{
		printf("Error: glXCreateContext failed\n");
		exit(1);
	}
	XMapWindow(g_gldpy, g_glwin);
	glXMakeCurrent(g_gldpy, g_glwin, g_glctx);

	InitGL(640, 480);

	//clickMouse();

	Window root = DefaultRootWindow(dpy);
	g_root = root;
	DumpWindow(dpy, root, 1);
#if 0
	for (int i = 0; i < 256; i++)
	{
		int x = 100 * cos(i * 3.141593 / 64.0) + 200;
		int y = 100 * sin(i * 3.141593 / 64.0) + 200;
		XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
		XFlush(dpy);
		usleep(10000);
	}
#endif

	XSetErrorHandler(OnXErrorEvent);

	int revert_to;
	//XGetInputFocus(dpy, &g_kb_focus, &revert_to);
	//g_mouse_focus = g_kb_focus;
	g_mouse_focus = None;
	g_kb_focus = None;

	XSelectInput (dpy, XRootWindow (dpy, 0), StructureNotifyMask | SubstructureNotifyMask | FocusChangeMask);
	xw = XWindow::GetWindow(dpy, root);
	xw->UpdateHierarchy();

	for (XWindow * child = xw->children(); child; child = child->sibling())
	{
		child->Update(0,0,0,0);
		if (child->mapped())
		{
			printf("focus %08x\n", (int)child->w());
			XSetInputFocus(dpy, child->w(), RevertToParent, CurrentTime);
			g_mouse_focus = child->w();
			g_kb_focus = child->w();
		}
		child->matrix().translation() -= Vector3(0.5f * child->width(), -0.5f * child->height(), 0);
	}

	g_hydra = initHydra();

	while (1)
	{
		XEvent event;
		while (XPending(dpy) > 0)
		{
			XNextEvent(dpy, &event);
			switch (event.type)
			{
			case ConfigureNotify:
				printf("ConfigureNotify %08x\n", (int)event.xconfigure.window);
				{
					Window wabove;
					XWindow * w = XWindow::GetWindow(dpy, event.xconfigure.window);
					xw->UpdateHierarchy();

					XWindow * above = NULL;
					if (XGetTransientForHint(dpy, w->w(), &wabove) && wabove != None)
					{
						printf("   transient for %08x\n", (int)wabove);
						above = XWindow::GetWindow(dpy, wabove);
					}
					if ((!above || !above->mapped()) && event.xconfigure.above != None)
					{
						printf("   above %08x\n", (int)event.xconfigure.above);
						above = XWindow::GetWindow(dpy, event.xconfigure.above);
					}
					if (!above) printf("no above\n");
					else if (!above->mapped()) printf("above not mapped\n");

					if (above && above->mapped())
					{
						float x = w->x() - above->x();
						float y = w->y() - above->y();
						w->matrix() = above->matrix();
						//w->matrix().translation() += Vector3(x, y, 1.f);
						w->matrix().translation() += w->matrix().right() * x - w->matrix().up() * y + w->matrix().back() * 0.1f;
						printf("   above %08x %f %f\n", (int)above->w(), above->matrix().translation()._x, above->matrix().translation()._y);
						printf("   windo %08x %f %f\n", (int)w->w(), w->matrix().translation()._x, w->matrix().translation()._y);
					}
				}
				break;
			case Expose:
				printf("Expose %08x\n", (int)event.xexpose.window);
				{
					XWindow * w = XWindow::GetWindow(dpy, event.xexpose.window);
					if (w)
					{
						XDamageCreate (dpy, event.xcreatewindow.window, XDamageReportRawRectangles);
						w->Update(0,0,0,0);
					}
				}
				break;
			case MapNotify:
				printf("Map %08x\n", (int)event.xmap.window);
				{
					XWindow * w = XWindow::GetWindow(dpy, event.xmap.window);
					xw->UpdateHierarchy();
					if (w)
					{
						XDamageCreate (dpy, event.xmap.window, XDamageReportRawRectangles);
						w->Update(0,0,0,0);
					}
				}
				break;
			case UnmapNotify:
				printf("Unmap %08x\n", (int)event.xunmap.window);
				{
					XWindow * w = XWindow::GetWindow(dpy, event.xunmap.window);
					if (w)
					{
						w->Unmap();
						for (XWindow * child = xw->children(); child; child = child->sibling())
						{
							if (child->mapped())
							{
								printf("focus %08x\n", (int)child->w());
								XSetInputFocus(dpy, child->w(), RevertToParent, CurrentTime);
							}
						}
					}
				}
				break;
			case FocusIn:
				printf("focus in %08x\n", (int)event.xfocus.window);
				break;
			case FocusOut:
				printf("focus out %08x\n", (int)event.xfocus.window);
				break;
			default:
				if (event.type == damageEvent + XDamageNotify)
				{
					XDamageNotifyEvent *de = (XDamageNotifyEvent *) &event;
					XWindow * w = XWindow::GetWindow(dpy, de->drawable);
					if (w)
					{
						/*printf ("damage %08x %08x %d %d %d %d, %d %d %d %d\n", de->drawable, w->w(),
								de->area.x, de->area.y, de->area.width, de->area.height,
								de->geometry.x, de->geometry.y, de->geometry.width, de->geometry.height);*/
						w->Update(de->area.x, de->area.y, de->area.width, de->area.height);
					}
				}
			}
		}
		while (XPending(g_gldpy) > 0)
		{
			XNextEvent(g_gldpy, &event);
			switch (event.type)
			{
			case Expose:
				break;
			case ConfigureNotify:
				ReSizeGLScene(event.xconfigure.width, event.xconfigure.height);
				break;
			case KeyPress:
				printf("key %08x %d \n", (int)g_kb_focus, event.xkey.keycode);
				keyPressed(event.xkey.keycode, 0, 0);
				{
					XWindow * w = XWindow::GetWindow(dpy, g_kb_focus);
					w->SendKeyEvent(root, event.xkey.keycode, event.xkey.state, true);
				}
				break;
			case KeyRelease:
				{
					XWindow * w = XWindow::GetWindow(dpy, g_kb_focus);
					w->SendKeyEvent(root, event.xkey.keycode, event.xkey.state, false);
				}
				break;
#if 0
			case MotionNotify:
				{
					Vector3 ray(event.xbutton.x * 2.f / g_width - 1.f, -(event.xbutton.y * 2.f - g_height) / g_width, -1.f);
					//printf ("ray %f %f %f\n", ray._x, ray._y, ray._z);
					ray.normalize();
					XWindow::Hit hit(g_pos * g_scale, ray);
					if (!XWindow::HitTest(hit, PointerMotionMask))
					{
						if (g_mouse_focus != None)
						{
							XWindow * w = XWindow::GetWindow(dpy, g_kb_focus);
							w->SendCrossingEvent(root, hit._x, -hit._y, g_button_state, false);
							g_mouse_focus = None;
						}
						//printf ("miss\n");
						break;
					}
					if (g_mouse_focus != hit._w->w())
					{
						if (g_mouse_focus != None)
						{
							XWindow * w = XWindow::GetWindow(dpy, g_kb_focus);
							w->SendCrossingEvent(root, hit._x, -hit._y, g_button_state, false);
						}
						g_mouse_focus = hit._w->w();
						hit._w->SendCrossingEvent(root, hit._x, -hit._y, g_button_state, true);
					}
					//printf ("hit %08x %f %f\n", focus, hit._x, -hit._y);
					hit._w->SendMotionEvent(root, hit._x, -hit._y, g_button_state);
				}
				break;
			case ButtonPress:
				{
					Vector3 ray(event.xbutton.x * 2.f / g_width - 1.f, -(event.xbutton.y * 2.f - g_height) / g_width, -1.f);
					printf ("ray %f %f %f\n", ray._x, ray._y, ray._z);
					ray.normalize();
					XWindow::Hit hit(g_pos * g_scale, ray);
					if (!XWindow::HitTest(hit, ButtonPressMask))
					{
						printf ("miss\n");
						break;
					}
					if (hit._w->w() != g_kb_focus)
					{
						g_kb_focus = hit._w->w();
						XSetInputFocus(dpy, g_kb_focus, RevertToParent, CurrentTime);
					}
					printf ("hit %08x %f %f %f, %f %f %f\n", (int)g_kb_focus, hit._x, -hit._y, hit._t,
							hit._matrix.translation()._x, hit._matrix.translation()._y, hit._matrix.translation()._z);
					hit._w->SendButtonEvent(root, hit._x, -hit._y, event.xbutton.button, event.xbutton.state, true);
				}
				break;
			case ButtonRelease:
				{
					Vector3 ray(event.xbutton.x * 2.f / g_width - 1.f, -(event.xbutton.y * 2.f - g_height) / g_width, -1.f);
					printf ("ray %f %f %f\n", ray._x, ray._y, ray._z);
					ray.normalize();
					XWindow::Hit hit(g_pos * g_scale, ray);
					if (!XWindow::HitTest(hit, ButtonReleaseMask))
					{
						printf ("miss\n");
						break;
					}
					if (hit._w->w() != g_kb_focus)
					{
						g_kb_focus = hit._w->w();
						XSetInputFocus(dpy, g_kb_focus, RevertToParent, CurrentTime);
					}
					printf ("hit %08x %f %f %f, %f %f %f\n", (int)g_kb_focus, hit._x, -hit._y, hit._t,
							hit._matrix.translation()._x, hit._matrix.translation()._y, hit._matrix.translation()._z);
					hit._w->SendButtonEvent(root, hit._x, -hit._y, event.xbutton.button, event.xbutton.state, false);
				}
				break;
#endif
			}
		}
		DrawGLScene();
	}

	return 1;
}



