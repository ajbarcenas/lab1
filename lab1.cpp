//
//modified by: Alexisis Barcenas
//date: 09/03/2019
//
//3350 Spring 2019 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
// .general animation framework
// .animation loop
// .object definition and movement
// .collision detection
// .mouse/keyboard interaction
// .object constructor
// .coding style
// .defined constants
// .use of static variables
// .dynamic memory allocation
// .simple opengl components
// .git
//
//elements we will add to program...
//   .Game constructor
//   .multiple particles
//   .gravity
//   .collision detection
//   .more objects
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"
#include <string.h>

const int MAX_PARTICLES = 1500;
const float GRAVITY = 0.1;

//some structures

struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
	Shape() {
		//define a box shape
		width = 100;
		height = 10;
	}
};

struct Particle {
	Shape s;
	Vec velocity;
};

class Global {
public:
	int xres, yres;
	Shape box[4];
	Particle particle[MAX_PARTICLES];
	int n;
	Global();
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
} x11;

class Image {
public:
	int width, height;
	unsigned char* data;
	~Image() { delete[] data; }
	Image(const char* fname) {
		if (fname[0] == '\0')
			return;
		//printf("fname **%s**\n", fname);
		int ppmFlag = 0;
		char name[40];
		strcpy(name, fname);
		int slen = strlen(name);
		char ppmname[80];
		if (strncmp(name + (slen - 4), ".ppm", 4) == 0)
			ppmFlag = 1;
		if (ppmFlag) {
			strcpy(ppmname, name);
		}
		else {
			name[slen - 4] = '\0';
			//printf("name **%s**\n", name);
			sprintf(ppmname, "%s.ppm", name);
			//printf("ppmname **%s**\n", ppmname);
			char ts[100];
			//system("convert eball.jpg eball.ppm");
			sprintf(ts, "convert %s %s", fname, ppmname);
			system(ts);
		}
		//sprintf(ts, "%s", name);
		FILE* fpi = fopen(ppmname, "r");
		if (fpi) {
			char line[200];
			fgets(line, 200, fpi);
			fgets(line, 200, fpi);
			//skip comments and blank lines
			while (line[0] == '#' || strlen(line) < 2)
				fgets(line, 200, fpi);
			sscanf(line, "%i %i", &width, &height);
			fgets(line, 200, fpi);
			//get pixel data
			int n = width * height * 3;
			data = new unsigned char[n];
			for (int i = 0; i < n; i++)
				data[i] = fgetc(fpi);
			fclose(fpi);
		}
		else {
			printf("ERROR opening image: %s\n", ppmname);
			exit(0);
		}
		if (!ppmFlag)
			unlink(ppmname);
	}
};

Image img = "Goo.PNG";

//Function prototypes
void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void movement();
void render();



//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	srand((unsigned)time(NULL));
	init_opengl();
	//Main animation loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			check_mouse(&e);
			done = check_keys(&e);
		}
		movement();
		render();
		x11.swapBuffers();
	}
	cleanup_fonts();
	return 0;
}

//-----------------------------------------------------------------------------
//Global class functions
//-----------------------------------------------------------------------------
Global::Global()
{
	xres = 800;
	yres = 600;
	n = 0;
}

//-----------------------------------------------------------------------------
//X11_wrapper class functions
//-----------------------------------------------------------------------------
X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 Lab1");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}
//-----------------------------------------------------------------------------

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

void makeParticle(int x, int y)
{
	//Add a particle to the particle system.
	//
	if (g.n >= MAX_PARTICLES)
		return;
	cout << "makeParticle() " << x << " " << y << endl;
	//set position of particle
	Particle *p = &g.particle[g.n];
	p->s.center.x = x;
	p->s.center.y = y;
	p->velocity.y = 0.0;
	//p->velocity.y =  ((double)rand()/(double)RAND_MAX)-0.5 - .25;
	p->velocity.y = 0;
	p->velocity.x =  ((double)rand()/(double)RAND_MAX)-0.5 +0.85;
	++g.n;
}

void check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed.
			int y = g.yres - e->xbutton.y;
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			makeParticle(e->xbutton.x, y);
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.
			int y = g.yres - e->xbutton.y;
			for (int i =0; i < 10; i++)
			makeParticle(e->xbutton.x, y);


		}
	}
}

int check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_a:
				//Key A was pressed
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void movement()
{
	if (g.n <= 0)
		return;
	for (int i=0; i <g.n; i++) {
		Particle *p = &g.particle[i];

		p->s.center.x += p->velocity.x;
		p->s.center.y += p->velocity.y;
		p->velocity.y -= GRAVITY;

		//check for collision with shapes...
		int x_scale = 100;
		int y_scale = 75;
		g.box[0].center.x = 150;
		g.box[0].center.y = 475;
		for (int j = 0; j < 6; j++) {
			g.box[j].center.x = 150 + j * x_scale;
			g.box[j].center.y = 475 - j * y_scale;
			if (p->s.center.y < g.box[j].center.y + g.box[j].height &&
				p->s.center.x > g.box[j].center.x - g.box[j].width &&
				p->s.center.x < g.box[j].center.x + g.box[j].width &&
				p->s.center.y > g.box[j].center.y - g.box[j].height) {
				if (p->velocity.y < 0) {
					p->s.center.y = g.box[j].center.y + g.box[j].height;
				}
				else {
					p->s.center.y = g.box[j].center.y - g.box[j].height;
				}
				p->velocity.y = -(p->velocity.y * 0.8);
			}
		}

		//g.box[1].center.x = 225;
		//g.box[1].center.y = 400;

/*		if (p->s.center.y < g.box[1].center.y + g.box[1].height &&
			p->s.center.x > g.box[1].center.x - g.box[1].width &&
			p->s.center.x < g.box[1].center.x + g.box[1].width &&
			p->s.center.y > g.box[1].center.y - g.box[1].height) {
			if (p->velocity.y < 0) {
				p->s.center.y = g.box[1].center.y + g.box[1].height;
			}
			else {
				p->s.center.y = g.box[1].center.y - g.box[1].height;
			}
			p->velocity.y = -(p->velocity.y * 0.8);
		}
*/
		//check for off-screen
		if (p->s.center.y < 0.0) {
			//cout << "off screen" << endl;
			g.particle[i] = g.particle[g.n - i];
			--g.n;
		}
	}
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	//Drawing the first shape
	//draw the box
	int x_scale = 100;
	int y_scale = 75;

	float h, w;

	for (int i = 0; i < 5; i++) {
		g.box[i].center.x = 150 + i * x_scale;
		g.box[i].center.y = 475 - i * y_scale;
		glColor3ub(255, 143, 143);
		glPushMatrix();
		glTranslatef(g.box[i].center.x, g.box[i].center.y, 0);
		w = g.box[i].width;
		h = g.box[i].height;
		glBegin(GL_QUADS);
		glVertex2i(-w, -h);
		glVertex2i(-w, h);
		glVertex2i(w, h);
		glVertex2i(w, -h);
		glEnd();
		glPopMatrix();
	}
/*	//Draw Box 2
	glColor3ub(255, 143, 143);
	glPushMatrix();
	glTranslatef(225, 400, 0);
	w = g.box[1].width;
	h = g.box[1].height;
	glBegin(GL_QUADS);
	glVertex2i(-w, -h);
	glVertex2i(-w, h);
	glVertex2i(w, h);
	glVertex2i(w, -h);
	glEnd();
	glPopMatrix();
*/
	//
	//Draw particles here
	//if (g.n > 0) 
	for (int i=0; i <g.n; i++){
		//There is at least one particle to draw.
		glPushMatrix();
		glColor3ub(10,255,63);
		Vec *c = &g.particle[i].s.center;
		w = h = 2;
		glBegin(GL_QUADS);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+h);
			glVertex2i(c->x+w, c->y+h);
			glVertex2i(c->x+w, c->y-h);
		glEnd();
		glPopMatrix();
	}
	//
	//Draw your 2D text here
	Rect r[1];
	r[0].bot = (g.yres/2)-105;
	r[0].left = g.xres/2;
	r[0].center = 0;
	ggprint8b(&r[0], 16, 0xff0008f, "Requirements");
}






