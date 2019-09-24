//
//modified by: Anna Poon
//date: 09/08/2019
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
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

const int MAX_PARTICLES = 2000000;
const float GRAVITY     = 0.1;
const int SIZE = 5;
int flag = 0;
//some structures

struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
};

struct Particle {
	Shape s;
	Vec velocity;
};

class Global {
public:
	int xres, yres;
	Shape box[SIZE];
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
	srand(time(NULL));
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
	return 0;
}

//-----------------------------------------------------------------------------
//Global class functions
//-----------------------------------------------------------------------------
Global::Global()
{
	xres = 500;
	yres = 360;
	//define a box shape
	for (int i = 0; i < SIZE; i++) {
		box[i].width = 80;
		box[i].height = 10;
		box[i].center.x = -140+(i*60);
		box[i].center.y = 130-(i*60);

	}
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
	//glOrtho(left, right, bottom, top)
	glOrtho(-g.xres/2, g.xres/2, -g.yres/2, g.yres/2, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
	//This is to allow fonts on the program
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

void makeParticle(int x, int y)
{
	//Add a particle to the particle system.
	//set n as the number of the current particle.
	
	if (g.n >= MAX_PARTICLES)
		return;
	cout << "makeParticle() " << x << " " << y << endl;
	//set position of particle
	Particle *p = &g.particle[g.n];
	p->s.center.x = x;
	p->s.center.y = y;
	p->velocity.y = -0.2;
	p->velocity.x =  ((double)rand() /(double)RAND_MAX) - 0.5;
	p->velocity.y =  ((double)rand() /(double)RAND_MAX) - 0.5 + 0.25;
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
			int y = ((2*((float)e->xbutton.y/g.yres)-1)*-g.yres/2);
			int x = ((2*((float)e->xbutton.x/g.xres)-1)*g.xres/2);
			makeParticle(x, y);
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
			int y = ((2*((float)e->xbutton.y/g.yres)-1)*-g.yres/2);
			int x = ((2*((float)e->xbutton.x/g.xres)-1)*g.xres/2);
			for(int i = 0; i < 10; i++){
				makeParticle(x, y);
			}

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

	//We need all the particles to work with the movement 
	//so we need a for loop
	//we need the iterator to count for the number of particles
	//
	for(int i = 0; i < g.n; i++){
	    Particle *p = &g.particle[i];
	    p->s.center.x += p->velocity.x;
	    p->s.center.y += p->velocity.y;	
	    p->velocity.y -= GRAVITY;
	    Shape * s;
	    for(int j = 0; j < SIZE; j++){
			s = &g.box[j]; 
			if(p->s.center.y < s->center.y + s->height && 
				p->s.center.y > s->center.y - s->height && 
				p->s.center.x > s->center.x - s->width && 
				p->s.center.x < s->center.x + s->width){
				p->velocity.y = -(p->velocity.y * 0.8);
				p->velocity.x = (p->velocity.x*0.7 + 0.2);
			}
		}
		//Check for off-screen
		if (p->s.center.y < (-g.yres)/2) {
			g.particle[i] = g.particle[g.n-1];
			--g.n;
			flag = flag^1;
		}
		
	}

}

int randomHexColor()
{
	int hex = 0;
	hex = rand() % 900 + 100;
	return hex;
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	
	Shape * s;
	float w, h;
	
	for (int i = 0; i < SIZE; i++) {
		if (flag) {
			/* Line below: changes the lighter shades of each individual color
			glColor3ub(125+(i*85), 250+(i*85), 50+(i*85));*/
			glColor3ub(255+(i*85), 120+(i*85), 135+(i*85));
		} else {
			glColor3ub(100+(i*85), 225+(i*85), 25+(i*85));
		}
		s = &g.box[i];
		
		glPushMatrix();
		glTranslatef(s->center.x, s->center.y, s->center.z);
		
		w = s->width;
		h = s->height;
		glBegin(GL_QUADS);
		glVertex2i(-w , -h);
		glVertex2i(-w , h);
		glVertex2i( w , h);
		glVertex2i( w , -h);

		glEnd();
		glPopMatrix();
	}
	//
	//Draw particles here
	for (int i = 0; i < g.n; i++) {
		glPushMatrix();
		w = h = rand()%3;
		if (i%3 == 0) {
			//All Diamonds have a Random Shade of Blue
			glColor3ub(0+(i%100),90+(i%100),255);
			Vec *c = &g.particle[i].s.center;
			glBegin(GL_POLYGON);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+2.5*h);
			glVertex2i(c->x+w, c->y+h);
			glVertex2i(c->x+w, c->y-2.5*h);
		} else if (i%3== 1) {
			//All Squares have a Random Shade of Green
			glColor3ub(11+(i%150),255, 50+(i%150));
			Vec *c = &g.particle[i].s.center;
			glBegin(GL_QUADS);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+h);
			glVertex2i(c->x+w, c->y+h);
			glVertex2i(c->x+w, c->y-h);
		} else {
			//All Triangles have a Random Shade of Red
			glColor3ub(255,30+(i%150),22+(i%150));
			Vec *c = &g.particle[i].s.center;
			glBegin(GL_TRIANGLE_STRIP);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+h);
			glVertex2i(c->x+w, c->y+h);
		}
		glEnd();
		glPopMatrix();
	}
	//
	//Draw your 2D text here
	Rect * r = new Rect[SIZE];
	string model[SIZE] = {"Requirements", "     Design", "Implementation", "    Testing", " Maintenance"};
	
	for (int i = 0; i < SIZE; i++) {
		r[i].left = -175+(i*60);
		r[i].bot = 125-(i*60);
		r[i].center = 0;
		ggprint8b(&r[i], 16, 0x00ffffff, model[i].c_str());
	}	
}






