#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

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

void usage(const char * program)
{
	printf("usage: %s [-d display]\n", program);
}

int main(int argc, char **argv)
{
	char * display_name = NULL;

	for (int i = 1; i < argc; i++)
	{
		char *arg = argv[i];

		if (!strcmp (arg, "-d"))
		{
			if (++i >= argc)
			{
				usage (argv[0]);
				return 1;
			}

			display_name = argv[i];
			continue;
		}
	}

	Display * dpy = XOpenDisplay(display_name);

	usage (argv[0]);

	if (!display_name)
	{
		printf("~= dumping default display =~\n");
	}

	Window root = DefaultRootWindow(dpy);
	DumpWindow(dpy, root, 1);

	return 0;
}

