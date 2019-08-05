
// use with: Xephyr :9 +bs -wm -screen 1280x720
// then: phasetest -display :9

#include <x3dConfig.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdamage.h>

#include <GL/glew.h>
#include <GL/glx.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <sys/time.h>

#if defined(USE_HYDRA)
#include "Hydra.h"
#elif defined(USE_OPENVR)
#include <openvr.h>
#endif
#include "XWindow.h"
#include "XDisplay.h"

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

Matrix g_camera = Matrix::identity;

XWindow * xw;
Display * g_dpy;
Window g_root;
Window g_mouse_focus;
Window g_kb_focus;
int g_button_state = 0;

#if defined(USE_HYDRA)
Hydra * g_hydra;
#elif defined(USE_OPENVR)
vr::IVRSystem * pVR;

void ConvertMatrix34(float * mat, const vr::HmdMatrix34_t &mat34)
{
	mat[0] = mat34.m[0][0];
	mat[1] = mat34.m[1][0];
	mat[2] = mat34.m[2][0];
	mat[3] = 0;
	mat[4] = mat34.m[0][1];
	mat[5] = mat34.m[1][1];
	mat[6] = mat34.m[2][1];
	mat[7] = 0;
	mat[8] = mat34.m[0][2];
	mat[9] = mat34.m[1][2];
	mat[10] = mat34.m[2][2];
	mat[11] = 0;
	mat[12] = mat34.m[0][3];
	mat[13] = mat34.m[1][3];
	mat[14] = mat34.m[2][3];
	mat[15] = 1;
}

void ConvertMatrix44(float * mat, const vr::HmdMatrix44_t &mat44)
{
	mat[0] = mat44.m[0][0];
	mat[1] = mat44.m[1][0];
	mat[2] = mat44.m[2][0];
	mat[3] = mat44.m[3][0];
	mat[4] = mat44.m[0][1];
	mat[5] = mat44.m[1][1];
	mat[6] = mat44.m[2][1];
	mat[7] = mat44.m[3][1];
	mat[8] = mat44.m[0][2];
	mat[9] = mat44.m[1][2];
	mat[10] = mat44.m[2][2];
	mat[11] = mat44.m[3][2];
	mat[12] = mat44.m[0][3];
	mat[13] = mat44.m[1][3];
	mat[14] = mat44.m[2][3];
	mat[15] = mat44.m[3][3];
}
#endif

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

int renderTargets = 2;
uint32_t renderTargetSize_w;
uint32_t renderTargetSize_h;
GLuint frameBuffer[2];
GLuint texture[2];
GLuint renderBuffer[2];
bool useRenderTarget = false;

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

	renderTargetSize_w = Width;
	renderTargetSize_h = Height;
#if defined(USE_OPENVR)
	pVR->GetRecommendedRenderTargetSize( &renderTargetSize_w, &renderTargetSize_h );
#endif

	glGenFramebuffers(2, frameBuffer);
	glGenTextures(2, texture);
	glGenRenderbuffers(2, renderBuffer);

	for (int i = 0; i < renderTargets; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer[i]);
		glBindTexture(GL_TEXTURE_2D, texture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderTargetSize_w, renderTargetSize_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture[i], 0);

		glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer[i]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, renderTargetSize_w, renderTargetSize_h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer[i]);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			glDeleteFramebuffers(2, frameBuffer);
			glDeleteTextures(2, texture);
			glDeleteRenderbuffers(2, renderBuffer);

			fprintf(stderr, "failed to create framebuffers\n");

			return;
		}
	}

	useRenderTarget = true;
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

void SetMouseFocus(XWindow * focus, int x, int y)
{
	if (!focus)
	{
		if (g_mouse_focus == None)
			return;

		XWindow * w = XDisplay::GetWindow(g_dpy, g_mouse_focus);
		w->SendCrossingEvent(g_root, x, y, g_button_state, NotifyAncestor, None, false);
		XWindow * child = w;
		for (w = w->parent(); w != xw; w = w->parent())
		{
			w->SendCrossingEvent(g_root, x, y, g_button_state, NotifyVirtual, child->w(), false);
			child = w;
		}
		g_mouse_focus = None;
		return;
	}

	if (focus->w() == g_mouse_focus)
		return;

	if (g_mouse_focus != None)
	{
		XWindow * w = XDisplay::GetWindow(g_dpy, g_mouse_focus);
		XDisplay::Cross cross;
		XDisplay::GetCross(w, focus, cross);
		if (cross._base == 0)
		{
			cross._w[0]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyInferior, None, false);
			for (int i = cross._count - 1; i > 1; i--)
			{
				cross._w[i]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyVirtual, cross._w[i - 1]->w(), false);
			}
			cross._w[cross._base + 1]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyAncestor, None, true);
		}
		else if (cross._base == cross._count - 1)
		{
			cross._w[0]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyAncestor, None, false);
			for (int i = 1; i < cross._base; i++)
			{
				cross._w[i]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyVirtual, cross._w[i - 1]->w(), true);
			}
			cross._w[cross._base]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyInferior, None, true);
		}
		else
		{
			cross._w[0]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyNonlinear, None, false);
			for (int i = 1; i < cross._base; i++)
			{
				cross._w[i]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyNonlinearVirtual, cross._w[i - 1]->w(), false);
			}
			for (int i = cross._count - 1; i > cross._base + 1; i--)
			{
				cross._w[i]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyNonlinearVirtual, cross._w[i - 1]->w(), true);
			}
			cross._w[cross._base + 1]->SendCrossingEvent(g_root, x, y, g_button_state, NotifyNonlinear, None, true);
		}
		g_mouse_focus = focus->w();
	}
	else
	{
		g_mouse_focus = focus->w();
		XWindow * parent = focus->parent();//nearest._frame;
		while (parent != focus)
		{
			XWindow * child = focus;
			for (; child->parent() != parent; child = child->parent());
			parent->SendCrossingEvent(g_root, x, y, g_button_state, NotifyVirtual, child->w(), true);
			parent = child;
		}
		focus->SendCrossingEvent(g_root, x, y, g_button_state, NotifyAncestor, None, true);
	}
}

#if defined(USE_HYDRA) || defined(USE_OPENVR)
typedef float VRControls[2][20];
#define VR_LEFT_X 0
#define VR_LEFT_Y 1
#define VR_LEFT_TRIGGER 2
#define VR_LEFT_BUMPER 3
#define VR_LEFT_MENU 4
#define VR_LEFT_STICK 5
#define VR_LEFT_1 6
#define VR_LEFT_2 7
#define VR_LEFT_3 8
#define VR_LEFT_4 9
#define VR_RIGHT_X 10
#define VR_RIGHT_Y 11
#define VR_RIGHT_TRIGGER 12
#define VR_RIGHT_BUMPER 13
#define VR_RIGHT_MENU 14
#define VR_RIGHT_STICK 15
#define VR_RIGHT_1 16
#define VR_RIGHT_2 17
#define VR_RIGHT_3 18
#define VR_RIGHT_4 19

struct VRInputState {
	Matrix left, right;
	int controli;
	VRControls controls;
	int hands;
	bool active;
	bool tracking;
	Matrix relativeMat;
} vrInputState;

Matrix cursorMat;
XDisplay::Nearest nearest;

bool VRInputPressed(int control)
{
	int prev = vrInputState.controli;
	int next = 1 - prev;
	VRControls &controls = vrInputState.controls;
	return (controls[next][control] > 0.75f && controls[prev][control] < 0.25f );
}

bool VRInputReleased(int control)
{
	int prev = vrInputState.controli;
	int next = 1 - prev;
	VRControls &controls = vrInputState.controls;
	return (controls[prev][control] > 0.75f && controls[next][control] < 0.25f );
}

#if defined(USE_OPENVR)
vr::TrackedDevicePose_t g_rTrackedDevicePose[ vr::k_unMaxTrackedDeviceCount ];
int g_hmdIndex;
Matrix projMat[2];
Matrix eyeMat[2];
Matrix hmdMat = Matrix::identity;

const char * sInputError[17] = {
	"None",
	"NameNotFound",
	"WrongType",
	"InvalidHandle",
	"InvalidParam",
	"NoSteam",
	"MaxCapacityReached",
	"IPCError",
	"NoActiveActionSet",
	"InvalidDevice",
	"InvalidSkeleton",
	"InvalidBoneCount",
	"InvalidCompressedData",
	"NoData",
	"BufferTooSmall",
	"MismatchedActionManifest",
	"MissingSkeletonData"
};

struct Action {
	const char * path;
	vr::VRActionHandle_t handle;
	vr::InputDigitalActionData_t data;
};

enum action_e {
	ACTION_LEFT_A,
	ACTION_RIGHT_A,
	ACTION_LEFT_TRIGGER,
	ACTION_RIGHT_TRIGGER,
	ACTION_LEFT_GRIP,
	ACTION_RIGHT_GRIP,
	ACTION_COUNT
};
Action actions[ACTION_COUNT] = {
	{"/actions/default/in/a_left", vr::k_ulInvalidActionHandle},
	{"/actions/default/in/a_right", vr::k_ulInvalidActionHandle},
	{"/actions/default/in/trigger_left", vr::k_ulInvalidActionHandle},
	{"/actions/default/in/trigger_right", vr::k_ulInvalidActionHandle},
	{"/actions/default/in/grip_left", vr::k_ulInvalidActionHandle},
	{"/actions/default/in/grip_right", vr::k_ulInvalidActionHandle},
};
vr::VRActionSetHandle_t defaultActionSet = vr::k_ulInvalidActionSetHandle;
vr::VRActionHandle_t leftActionPose = vr::k_ulInvalidActionHandle;
vr::VRActionHandle_t rightActionPose = vr::k_ulInvalidActionHandle;
bool buttonActive = false;
bool failedGetDigitalActionData = false;
#endif

bool vrInit() {
#if defined(USE_HYDRA)
	g_hydra = initHydra();
	return g_hydra? true : false;
#elif defined(USE_OPENVR)
	float m_fNearClip = 0.01f;
 	float m_fFarClip = 100.0f;

	ConvertMatrix44(projMat[0]._m, pVR->GetProjectionMatrix( vr::Eye_Left, m_fNearClip, m_fFarClip));
	ConvertMatrix44(projMat[1]._m, pVR->GetProjectionMatrix( vr::Eye_Right, m_fNearClip, m_fFarClip));
	ConvertMatrix34(eyeMat[0]._m, pVR->GetEyeToHeadTransform( vr::Eye_Left ));
	ConvertMatrix34(eyeMat[1]._m, pVR->GetEyeToHeadTransform( vr::Eye_Right ));
	g_hmdIndex = vr::k_unTrackedDeviceIndex_Hmd;

	//vr::IVRInput * vrInput = vr::VRInput();
	#define vrInput (vr::VRInput())
	if (!vrInput) {
		fprintf(stderr, "no vr input\n");
		return false;
	}

	const char am_filename[] = "action_manifest.json";
	char am_path[ 4096 + sizeof(am_filename)];
	ssize_t count = readlink( "/proc/self/exe", am_path, 4096 );
	if (!count)
	{
		perror("readlink(\"/proc/self/exe\"):");
		return false;
	}
	am_path[count] = '\0';
	char * last_char = strrchr(am_path, '/');
	if (!last_char)
	{
		fprintf(stderr, "failed to find path, found this instead: %s\n", am_path);
		return false;
	}
	last_char[1] = '\0';
	strcat(am_path, am_filename);

	vr::EVRInputError eInError;
	//eInError = vrInput->SetActionManifestPath("/data/projects/openvr_console/src/action_manifest.json");
	//eInError = vrInput->SetActionManifestPath("C:\\Projects\\openvr_logger\\openvr_logger\\src\\action_manifest.json");
	//eInError = vrInput->SetActionManifestPath("C:\\Projects\\openvr_cookbook\\vrinput\\src\\action_manifest.json");
	eInError = vrInput->SetActionManifestPath(am_path);
	if (eInError != vr::VRInputError_None)
	{
		fprintf(stderr, "SetActoinManifestPath failed with: %d\n", eInError);
		return false;
	}

	for (int i = 0; i < ACTION_COUNT; i++)
	{
		eInError = vrInput->GetActionHandle(actions[i].path, &actions[i].handle);
		if (eInError != vr::VRInputError_None)
		{
			fprintf(stderr, "GetActionHandle failed with: %d\n", eInError);
			return false;
		}
	}

	eInError = vrInput->GetActionHandle("/actions/default/in/hand_left", &leftActionPose);
	if (eInError != vr::VRInputError_None)
	{
		fprintf(stderr, "GetActionHandle failed with: %d\n", eInError);
		return false;
	}

	eInError = vrInput->GetActionHandle("/actions/default/in/hand_right", &rightActionPose);
	if (eInError != vr::VRInputError_None)
	{
		fprintf(stderr, "GetActionHandle failed with: %d\n", eInError);
		return false;
	}

	eInError = vrInput->GetActionSetHandle("/actions/default", &defaultActionSet);
	if (eInError != vr::VRInputError_None)
	{
		fprintf(stderr, "GetActionSetHandle failed with: %d\n", eInError);
		return false;
	}

	return true;
#endif
}

int getHands(Matrix &left, Matrix &right, float * controls)
{
#if defined(USE_HYDRA)
	return getHands(g_hydra, left, right, controls);
#elif defined(USE_OPENVR)
	vr::VREvent_t e;
	while( pVR->PollNextEvent( &e, sizeof( e ) ) ) { }
	vr::VRCompositor()->WaitGetPoses(g_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );
	if ( g_rTrackedDevicePose[g_hmdIndex].bPoseIsValid )
	{
		ConvertMatrix34(hmdMat._m, g_rTrackedDevicePose[g_hmdIndex].mDeviceToAbsoluteTracking);
		//printf("%f %f %f\n", hmdMat.translation()._x, hmdMat.translation()._y, hmdMat.translation()._z);
	}
	vr::VRActiveActionSet_t activeActionSet;
	activeActionSet.ulActionSet = defaultActionSet;
	//activeActionSet.ulActionSet = m_actionsetDemo;
	activeActionSet.ulRestrictedToDevice = vr::k_ulInvalidInputValueHandle;
	activeActionSet.nPriority = 0;

	vr::EVRInputError eInError;
	eInError = vrInput->UpdateActionState(&activeActionSet, sizeof(activeActionSet), 1);
	if (eInError != vr::VRInputError_None)
	{
		fprintf(stderr, "UpdateActionState failed with: %d\n", eInError);
		return 0;
	}

	for (int i = 0; i < ACTION_COUNT; i++)
	{
		eInError = vrInput->GetDigitalActionData(actions[i].handle, &actions[i].data, sizeof(actions[i].data), vr::k_ulInvalidInputValueHandle);
		if (eInError != vr::VRInputError_None)
		{
			if (!failedGetDigitalActionData)
			{
				failedGetDigitalActionData = true;
				printf("GetDigitalActionData failed with: %d \"%s\"\n", eInError, sInputError[eInError]);
			}
			return 0;
		}
	}

	int hands = 0;
	vr::InputPoseActionData_t poseData;

	eInError = vr::VRInput()->GetPoseActionData( leftActionPose, vr::TrackingUniverseStanding, 0, &poseData, sizeof( poseData ), vr::k_ulInvalidInputValueHandle );
	if ( eInError == vr::VRInputError_None
		&& poseData.bActive && poseData.pose.bPoseIsValid )
	{
		hands |= 1;
		ConvertMatrix34(left._m, poseData.pose.mDeviceToAbsoluteTracking);
		left.PrependScale(1.f/100.f, 1.f/100.f, 1.f/100.f);
		left.AppendTranslate(0, -1, 0);
		left.AppendScale(100, 100, 100);
	} else {
		if (eInError != vr::VRInputError_None)
			printf("left pose error %d %s\n", eInError, sInputError[eInError]);
		else
			printf("left pose error %d %d\n", poseData.bActive, poseData.pose.bPoseIsValid);
	}

	eInError = vr::VRInput()->GetPoseActionData( rightActionPose, vr::TrackingUniverseStanding, 0, &poseData, sizeof( poseData ), vr::k_ulInvalidInputValueHandle );
	if ( eInError == vr::VRInputError_None
		&& poseData.bActive && poseData.pose.bPoseIsValid )
	{
		hands |= 2;
		ConvertMatrix34(right._m, poseData.pose.mDeviceToAbsoluteTracking);
		right.PrependRotate(-90.f, 1.f, 0.f, 0.f);
		right.PrependScale(1.f/100.f, 1.f/100.f, 1.f/100.f);
		right.AppendTranslate(0, -1, 0);
		right.AppendScale(100, 100, 100);
		//printf("%f %f %f\n", right.translation()._x, right.translation()._y, right.translation()._z);
	} else {
		if (eInError != vr::VRInputError_None)
			printf("right pose error %d %s\n", eInError, sInputError[eInError]);
		else
			printf("right pose error %d %d\n", poseData.bActive, poseData.pose.bPoseIsValid);
	}

	memset(controls, 0, sizeof(float)*20);
	controls[VR_RIGHT_MENU] = (actions[ACTION_RIGHT_GRIP].data.bActive && actions[ACTION_RIGHT_GRIP].data.bState)? 1.f : 0.f;
	controls[VR_RIGHT_3] = (actions[ACTION_RIGHT_TRIGGER].data.bActive && actions[ACTION_RIGHT_TRIGGER].data.bState)? 1.f : 0.f;
	controls[VR_RIGHT_1] = (actions[ACTION_RIGHT_A].data.bActive && actions[ACTION_RIGHT_A].data.bState)? 1.f : 0.f;

	return hands;
#endif
}


void vrInputUpdate()
{
	Matrix &left = vrInputState.left;
	Matrix &right = vrInputState.right;
	int &controli = vrInputState.controli;
	VRControls &controls = vrInputState.controls;
	int &hands = vrInputState.hands;
	bool &active = vrInputState.active;
	bool &tracking = vrInputState.tracking;
	Matrix &relativeMat = vrInputState.relativeMat;

	hands = getHands(left, right, controls[controli]);
	controli = 1 - controli;
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
#if 0
		for (int i = 0; i < 10; i++)
			printf("%f ", controls[1-controli][i]);
		printf("\n");
#endif
		if (!tracking)
		{
			if (VRInputPressed(VR_LEFT_MENU))
			{
				Matrix inv = left;
				inv.FastInverse();
				//relativeMat = g_camera * inv;
				relativeMat = inv * g_camera;
				tracking = true;
			}
		}
		else
		{
			if (VRInputPressed(VR_LEFT_MENU))
			{
				tracking = false;
			}
			else
			{
				//g_camera = relativeMat * left;
				g_camera = left * relativeMat;
				//printf("%f %f %f\n", g_camera.translation()._x, g_camera.translation()._y, g_camera.translation()._z);
			}
		}
		//printf("hand %f %f %f, %f %f %f\n", left.translation()._x, left.translation()._y, left.translation()._z, left.back()._x, left.back()._y, left.back()._z);
	}

	static Matrix grabMat;
	static Matrix grabMat2;
	static bool grabbing = false;
	if (hands & 2)
	{
#if 0
		for (int i = 10; i < 20; i++)
			printf("%f ", controls[1-controli][i]);
		printf("\n");
#endif
		if (!grabbing)
		{
#if defined(USE_HYDRA)
			nearest._pos = (right.translation() + Vector3(0, -12.f * 2.56f, 0)) * g_scale;
#else
			nearest._pos = right.translation() * g_scale;
#endif
			nearest._radius = 64.f;
			nearest._distance = 100000.f;
			nearest._w = NULL;
			bool found = XDisplay::GetNearest(nearest, 0);
			if (nearest._w)
			{
				//printf("nearest %08x\n", (int)nearest._w->w());
			}

#if 0
			Matrix pixMat = right;
#if defined(USE_HYDRA)
			pixMat.AppendTranslate(0, -12.f * 2.56f, 0);
#endif
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
				SetMouseFocus(NULL, nearest._x, nearest._y);
			}
			else
			{
				SetMouseFocus(nearest._w, nearest._x, nearest._y);

				if (VRInputPressed(VR_RIGHT_3))
				{
					if (nearest._w->w() != g_kb_focus)
					{
						g_kb_focus = nearest._w->w();
						XSetInputFocus(g_dpy, g_kb_focus, RevertToParent, CurrentTime);
					}
					nearest._w->SendButtonEvent(g_root, nearest._x, nearest._y, Button1, g_button_state, true);
					g_button_state |= Button1Mask;
				}
				else if (VRInputReleased(VR_RIGHT_3))
				{
					nearest._w->SendButtonEvent(g_root, nearest._x, nearest._y, Button1, g_button_state, false);
					g_button_state &= ~Button1Mask;
				}
				else if (VRInputPressed(VR_RIGHT_1))
				{
					if (nearest._w->w() != g_kb_focus)
					{
						g_kb_focus = nearest._w->w();
						XSetInputFocus(g_dpy, g_kb_focus, RevertToParent, CurrentTime);
					}
					nearest._w->SendButtonEvent(g_root, nearest._x, nearest._y, Button3, g_button_state, true);
					g_button_state |= Button3Mask;
				}
				else if (VRInputReleased(VR_RIGHT_1))
				{
					nearest._w->SendButtonEvent(g_root, nearest._x, nearest._y, Button3, g_button_state, false);
					g_button_state &= ~Button3Mask;
				}
				else if (VRInputPressed(VR_RIGHT_MENU))
				{
					Matrix pixMat = right;
#if defined(USE_HYDRA)
					pixMat.AppendTranslate(0, -12.f * 2.56f, 0);
#endif
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
			if (controls[1-controli][VR_RIGHT_MENU] < 0.25f )
			{
				grabbing = false;
			}
			else
			{
				Matrix pixMat = right;
#if defined(USE_HYDRA)
				pixMat.AppendTranslate(0, -12.f * 2.56f, 0);
#endif
				pixMat.AppendScale(g_scale, g_scale, g_scale);
				nearest._frame->matrix() = pixMat * grabMat;
				//printf("%f %f %f\n", g_camera.translation()._x, g_camera.translation()._y, g_camera.translation()._z);
			}
		}
		cursorMat = Matrix::identity;
#if defined(USE_HYDRA)
		cursorMat.Translate(0, -12.f * 2.56f, 0);
#endif
		cursorMat = right * cursorMat;
		cursorMat.Scale(0.3f, 0.3f, 0.3f);

		//printf("hand %f %f %f\n", right.translation()._x, right.translation()._y, right.translation()._z);
	}
}
#endif

void DrawGLScene(const Matrix &camera)
{
	glPushMatrix();

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

	Matrix inv = camera;
	inv.FastInverse();
	glMultMatrixf(inv._m);

#if defined(USE_HYDRA)
	glTranslatef( -g_pos._x, -g_pos._y, -g_pos._z);
#endif

#if defined(USE_HYDRA) || defined(USE_OPENVR)
	glPushMatrix();
	glMultMatrixf(cursorMat._m);
	DrawCursor();
	glPopMatrix();
#endif

	glScalef(1.f / g_scale, 1.f / g_scale, 1.f / g_scale);

	xw->Draw();

#if defined(USE_HYDRA) || defined(USE_OPENVR)
	if (nearest._frame)
	{
		glPushMatrix();
		glMultMatrixf(nearest._frame->matrix()._m);
		glTranslatef(nearest._framex, nearest._framey, 1.0f);
		glScalef(0.3f * g_scale, 0.3f * g_scale, 1.f);
		DrawCursorShadow(nearest._distance / 64.f);
		glPopMatrix();
	}
#endif

	glPopMatrix();

	glFinish();
}

void keyPressed(unsigned char key, int x, int y)
{
	if (key == ESCAPE)
	{
#if defined(USE_HYDRA)
		exitHydra();
#elif defined (USE_OPENVR)
		if( pVR )
		{
			vr::VR_Shutdown();
			pVR = NULL;
		}
#endif
		exit(0);
	}
}

static void usage(char * program_name)
{
	fprintf (stderr, "usage: %s [-display host:dpy]", program_name);
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
	attr.override_redirect = False;//True;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

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

	printf("version %d.%d\n", x3d_VERSION_MAJOR, x3d_VERSION_MINOR);
#if defined(USE_HYDRA)
	printf("  using hydra module\n");
#endif

	Display * dpy;
	const char * display_name = NULL;
	for (i = 1; i < argc; i++)
	{
		char *arg = argv[i];

		if (!strcmp (arg, "-display") || !strcmp (arg, "-d"))
		{
			if (++i >= argc)
			{
				usage(argv[0]);
				exit(0);
			}

			display_name = argv[i];
			continue;
		}
	}

	if (!display_name)
	{
		usage(argv[0]);
		printf(" attempting default of :9\n");
		display_name = ":9";
	}

	dpy = XOpenDisplay(display_name);
	g_dpy = dpy;
	if (!dpy)
	{
		fprintf (stderr, "%s:  unable to open display '%s'\n",
					argv[0], XDisplayName (display_name));
		usage(argv[0]);
		exit(0);
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
	//XMapWindow(g_gldpy, g_glwin);
	XMapRaised(g_gldpy, g_glwin);
	//XGrabKeyboard(g_gldpy, g_glwin, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	glXMakeCurrent(g_gldpy, g_glwin, g_glctx);
	glewExperimental = GL_TRUE;
	glewInit();

#if defined(USE_OPENVR)
	vr::EVRInitError eError = vr::VRInitError_None;
	pVR = vr::VR_Init( &eError, vr::VRApplication_Scene );
	if ( eError != vr::VRInitError_None )
	{
		fprintf(stderr, "%d %s", eError, vr::VR_GetVRInitErrorAsEnglishDescription( eError ));
		return 1;
	}
#endif

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
	xw = XDisplay::GetWindow(dpy, root);
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

#if defined(USE_HYDRA) || defined(USE_OPENVR)
	if (!vrInit())
	{
		return 1;
	}
#endif

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
					XWindow * w = XDisplay::GetWindow(dpy, event.xconfigure.window);
					xw->UpdateHierarchy();

					XWindow * above = NULL;
					if (XGetTransientForHint(dpy, w->w(), &wabove) && wabove != None)
					{
						printf("   transient for %08x\n", (int)wabove);
						above = XDisplay::GetWindow(dpy, wabove);
					}
					if ((!above || !above->mapped()) && event.xconfigure.above != None)
					{
						printf("   above %08x\n", (int)event.xconfigure.above);
						above = XDisplay::GetWindow(dpy, event.xconfigure.above);
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
					XWindow * w = XDisplay::GetWindow(dpy, event.xexpose.window);
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
					XWindow * w = XDisplay::GetWindow(dpy, event.xmap.window);
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
					XWindow * w = XDisplay::GetWindow(dpy, event.xunmap.window);
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
					XWindow * w = XDisplay::GetWindow(dpy, de->drawable);
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
			default:
				//printf("unhandled event %d\n", event.type);
				break;
			case Expose:
				break;
			case ConfigureNotify:
				ReSizeGLScene(event.xconfigure.width, event.xconfigure.height);
				break;
			case KeyPress:
				printf("key %08x %d \n", (int)g_kb_focus, event.xkey.keycode);
				keyPressed(event.xkey.keycode, 0, 0);
				if (g_kb_focus != None)
				{
					XWindow * w = XDisplay::GetWindow(dpy, g_kb_focus);
					w->SendKeyEvent(root, event.xkey.keycode, event.xkey.state, true);
				}
				break;
			case KeyRelease:
				if (g_kb_focus != None)
				{
					XWindow * w = XDisplay::GetWindow(dpy, g_kb_focus);
					w->SendKeyEvent(root, event.xkey.keycode, event.xkey.state, false);
				}
				break;
#if !defined(USE_HYDRA) && !defined(USE_OPENVR)
			case MotionNotify:
				{
					Vector3 ray(event.xbutton.x * 2.f / g_width - 1.f, -(event.xbutton.y * 2.f - g_height) / g_width, -1.f);
					//printf ("ray %f %f %f\n", ray._x, ray._y, ray._z);
					ray.normalize();
					XDisplay::Hit hit(g_pos * g_scale, ray);
					if (!XDisplay::HitTest(hit, PointerMotionMask))
					{
						SetMouseFocus(NULL, (int)hit._x, (int)-hit._y);
						//printf ("miss\n");
						break;
					}
					SetMouseFocus(hit._w, (int)hit._x, (int)-hit._y);
					//printf ("hit %08x %f %f\n", focus, hit._x, -hit._y);
					hit._w->SendMotionEvent(root, hit._x, -hit._y, g_button_state);
				}
				break;
			case ButtonPress:
				{
					Vector3 ray(event.xbutton.x * 2.f / g_width - 1.f, -(event.xbutton.y * 2.f - g_height) / g_width, -1.f);
					printf ("ray %f %f %f\n", ray._x, ray._y, ray._z);
					ray.normalize();
					XDisplay::Hit hit(g_pos * g_scale, ray);
					if (!XDisplay::HitTest(hit, ButtonPressMask))
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
					XDisplay::Hit hit(g_pos * g_scale, ray);
					if (!XDisplay::HitTest(hit, ButtonReleaseMask))
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

		g_scale = g_ppi / 2.56f;//960.f;

#if defined(USE_HYDRA) || defined(USE_OPENVR)
		vrInputUpdate();
#endif

		//float screen = xw->width();
		//xw->matrix() = *(Matrix*)Matrix::identity;
		//xw->matrix().Translate(0, 0, screen);
		//xw->matrix().Rotate(0.1f * frame, 0, 1, 0);
		//xw->matrix().Rotate(45.f, 0, 1, 0);

		if (useRenderTarget) {
			for (int eyeIndex = 0; eyeIndex < 2; eyeIndex++)
			{
				glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer[eyeIndex]);
				glEnable(GL_DEPTH_TEST);

				glClearColor(96.f / 255.f, 118.f / 255.f, 98.f / 255.f, 1.f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer

#if defined(USE_OPENVR)
				glViewport( 0, 0, renderTargetSize_w, renderTargetSize_h);
				glMatrixMode(GL_PROJECTION);
				glLoadMatrixf(projMat[eyeIndex]._m);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				Matrix viewMat = hmdMat * eyeMat[eyeIndex];
				viewMat.AppendTranslate(0, -1, 0);
				viewMat.AppendScale(100, 100, 100);
#else
				SetupProjection3D(g_width, g_height);
				glLoadIdentity();

				Matrix viewMat = g_camera;
				viewMat.PrependTranslate(eyeIndex*4.0f - 2.f, 0, 0);
#endif

				DrawGLScene(viewMat);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, g_width, g_height);
			glClearColor(1, 0, 1, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, 2, 0, 1, -100.f, 100.f);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);

			//glDisable(GL_TEXTURE_2D);
			glEnable(GL_TEXTURE_2D);
			glClientActiveTexture(GL_TEXTURE0);
			glActiveTexture(GL_TEXTURE0);

			float verts2[] = {
				0, 1,
				1, 1,
				1, 0,
				0, 0,
			};
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts2);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, verts2 );
			glColor4f(1.f, 1.f, 1.f, 1.f);

			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glTranslatef(1,0,0);
			glBindTexture(GL_TEXTURE_2D, texture[1]);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glFinish();

#if defined(USE_OPENVR)
			vr::Texture_t leftEyeTexture = {(void*)(int64_t)texture[0], vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
			vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture );
			vr::Texture_t rightEyeTexture = {(void*)(int64_t)texture[1], vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
			vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture );
#endif

		} else {
			glClearColor(96.f / 255.f, 118.f / 255.f, 98.f / 255.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer

			SetupProjection3D(g_width, g_height);

			glLoadIdentity();
			DrawGLScene(g_camera);
		}

		glXSwapBuffers(g_gldpy, g_glwin);

		frame++;
	}

	return 1;
}



