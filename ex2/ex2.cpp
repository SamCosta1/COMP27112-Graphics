/*
==========================================================================
File:        ex2.c (skeleton)
Authors:     Toby Howard
==========================================================================
*/

/* The following ratios are not to scale: */
/* Moon orbit : planet orbit */
/* Orbit radius : body radius */
/* Sun radius : planet radius */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BODIES 20
#define TOP_VIEW 1
#define ECLIPTIC_VIEW 2
#define SHIP_VIEW 3
#define EARTH_VIEW 4
#define PI 3.14159
#define DEG_TO_RAD 0.017453293
#define ORBIT_POLY_SIDES 40
#define TIME_STEP 0.5   /* days per frame */

typedef struct {
  char    name[20];       /* name */
  GLfloat r, g, b;        /* colour */
  GLfloat orbital_radius; /* distance to parent body (km) */
  GLfloat orbital_tilt;   /* angle of orbit wrt ecliptic (deg) */
  GLfloat orbital_period; /* time taken to orbit (days) */
  GLfloat radius;         /* radius of body (km) */
  GLfloat axis_tilt;      /* tilt of axis wrt body's orbital plane (deg) */
  GLfloat rot_period;     /* body's period of rotation (days) */
  GLint   orbits_body;    /* identifier of parent body */
  GLfloat spin;           /* current spin value (deg) */
  GLfloat orbit;          /* current orbit value (deg) */
 } body;

body  bodies[MAX_BODIES];
int   numBodies, current_view;
bool draw_labels, draw_orbits, draw_starfield, earth_view;
float date;

/*****************************/

float myRand (void) {
  /* return a random float in the range [0,1] */
  return (float) rand() / RAND_MAX;
}

/*****************************/

float randomInRange(float rand) {
    float cubeWidth = 800000000;

    // Shifting result to get -ive and +ive values
    return (rand * cubeWidth) - cubeWidth / 2;
}

int starField[3001];

void initStarField() {
    for (int i = 0; i < 3000; i++)
        starField[i] = (int)randomInRange(myRand());
}

void drawStarfield (void) {

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_POINTS);
        for (int i = 0; i < 1000; i+=3)
            glVertex3f(starField[i], starField[i+1], starField[i+2]);
    glEnd();
}

/*****************************/

void readSystem(void) {
  /* reads in the description of the solar system */

  FILE *f;
  int i;

  f= fopen("sys", "r");
  if (f == NULL) {
     printf("ex2.c: Couldn't open the datafile 'sys'\n");
     printf("To get this file, use the following command:\n");
     printf("  cp /opt/info/courses/COMP27112/ex2/sys .\n");
     exit(0);
  }
  fscanf(f, "%d", &numBodies);
  for (i= 0; i < numBodies; i++)  {
    fscanf(f, "%s %f %f %f %f %f %f %f %f %f %d",
      bodies[i].name,
      &bodies[i].r, &bodies[i].g, &bodies[i].b,
      &bodies[i].orbital_radius,
      &bodies[i].orbital_tilt,
      &bodies[i].orbital_period,
      &bodies[i].radius,
      &bodies[i].axis_tilt,
      &bodies[i].rot_period,
      &bodies[i].orbits_body);

    /* Initialise the body's state */
    bodies[i].spin= 0.0;
    bodies[i].orbit= myRand() * 360.0; /* Start each body's orbit at a
                                          random angle */
    bodies[i].radius*= 1000.0; /* Magnify the radii to make them visible */
  }
  fclose(f);
}

/*****************************/

void drawString (void *font, float x, float y, char *str)
{ /* Displays the string "str" at (x,y,0), using font "font" */

  /* This is for you to complete. */

}

/*****************************/

void drawAxis() {
    GLfloat endPoint = 3000000000;


    glLineWidth(1.0);

    glBegin(GL_LINES);

    // X axis
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(endPoint, 0.0, 0.0);
    glVertex3f(-endPoint, 0.0,  0.0);

    //Y axis
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, endPoint, 0.0);
    glVertex3f(0.0, -endPoint, 0.0);

    //Z axis
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0.0, 0.0, endPoint);
    glVertex3f(0.0, 0.0, -endPoint);

    glEnd();
}

void setView (void) {
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  earth_view = false;
  switch (current_view) {
  case TOP_VIEW:
    gluLookAt (0.0, 600000000.0, 0.0,
               0.0, 0.0, 0.0,
               0.0, 0.0, -1.0);
    break;
  case ECLIPTIC_VIEW:
    gluLookAt (0.0, 0.0, 400000000.0,
               0.0, 0.0, 0.0,
               0.0, 1.0, 0.0);

    break;
  case SHIP_VIEW:
    gluLookAt (315193193.291, 105193193.291, 315193193.291,
               0.0, 0.0, 0.0,
               0.0, 1.0, 0.0);

    break;
  case EARTH_VIEW:
    float x = cos(bodies[3].orbit * DEG_TO_RAD) * bodies[3].orbital_radius;
    float y = x * tan(bodies[3].orbital_tilt * DEG_TO_RAD) + 1.1*bodies[3].radius;
    float z = sin(bodies[3].orbit * DEG_TO_RAD) * bodies[3].orbital_radius;
    gluLookAt(x  , y  , z ,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);
    break;
  }
}

/*****************************/

void menu (int menuentry) {
  switch (menuentry) {
  case 1: current_view= TOP_VIEW;
          break;
  case 2: current_view= ECLIPTIC_VIEW;
          break;
  case 3: current_view= SHIP_VIEW;
          break;
  case 4: current_view= EARTH_VIEW;
          break;
  case 5: draw_labels= !draw_labels;
          break;
  case 6: draw_orbits= !draw_orbits;
          break;
  case 7: draw_starfield= !draw_starfield;
          break;
  case 8: exit(0);
  }
}

/*****************************/

void init(void)
{
  initStarField();
  /* Define background colour */
  glClearColor(0.0, 0.0, 0.0, 0.0);

  glutCreateMenu (menu);
  glutAddMenuEntry ("Top view", 1);
  glutAddMenuEntry ("Ecliptic view", 2);
  glutAddMenuEntry ("Spaceship view", 3);
  glutAddMenuEntry ("Earth view", 4);
  glutAddMenuEntry ("", 999);
  glutAddMenuEntry ("Toggle labels", 5);
  glutAddMenuEntry ("Toggle orbits", 6);
  glutAddMenuEntry ("Toggle starfield", 7);
  glutAddMenuEntry ("", 999);
  glutAddMenuEntry ("Quit", 8);
  glutAttachMenu (GLUT_RIGHT_BUTTON);

  current_view = TOP_VIEW;
  draw_labels = true;
  draw_orbits = true;
  draw_starfield = true;
  earth_view = false;
}

/*****************************/

void animate(void)
{
  int i;

  date+= TIME_STEP;

    for (i= 0; i < numBodies; i++)  {
      bodies[i].spin += 360.0 * TIME_STEP / bodies[i].rot_period;
      bodies[i].orbit += 360.0 * TIME_STEP / bodies[i].orbital_period;
      glutPostRedisplay();
    }
}

/*****************************/

float getBodyX(int n, float angle) {
    return cos(angle * DEG_TO_RAD) * bodies[n].orbital_radius;
}

float getBodyY(int n, float angle) {
    return getBodyX(n, angle) * tan(bodies[n].orbital_tilt * DEG_TO_RAD);
}

float getBodyZ(int n, float angle) {
    return sin(angle * DEG_TO_RAD) * bodies[n].orbital_radius;
}
void drawOrbit (int n) {
    if (!draw_orbits)
        return;

    int noVerticies = ORBIT_POLY_SIDES;
    float theta = 360 / noVerticies;

    glColor3f(bodies[n].r, bodies[n].g, bodies[n].b);
    glBegin(GL_LINE_LOOP);

    for (int i = 0; i < noVerticies; i++) {
        float x = getBodyX(n, i * theta);
        float y = getBodyY(n, i * theta);
        float z = getBodyZ(n, i * theta);

        if (bodies[n].orbits_body != 0) {
            x += getBodyX(bodies[n].orbits_body, bodies[n].orbit);
            y += getBodyY(bodies[n].orbits_body, bodies[n].orbit);
            z += getBodyZ(bodies[n].orbits_body, bodies[n].orbit);
        }
        glVertex3f(x, y , z);
    }
    glEnd();
}


/*****************************/

void drawLabel(int n) {
    if (!draw_labels)
        return;
     /* Draws the name of body "n" */

    /* This is for you to complete. */
}

/*****************************/

void tiltAndDrawAxis(int n) {
    float angle = bodies[n].axis_tilt;
    GLfloat x = sin(angle * DEG_TO_RAD);
    GLfloat y = cos(angle * DEG_TO_RAD);


    glRotatef(bodies[n].spin, x, y, 0);

    glLineWidth(1.5);
    glBegin(GL_LINES);
        glColor3f(0.5, 0.5, 0.0);
        glVertex3f(bodies[n].radius *  1.1 * x, bodies[n].radius *  1.1 * y, 0.0);
        glVertex3f(bodies[n].radius *  -1.1 * x, bodies[n].radius *  -1.1 * y,  0.0);
    glEnd();
}


void drawBody(int n) {

    glPushMatrix();
        float x = getBodyX(n, bodies[n].orbit);
        float y = getBodyY(n, bodies[n].orbit);
        float z = getBodyZ(n, bodies[n].orbit);

        int parent = bodies[n].orbits_body;

        float parentX = getBodyX(parent, bodies[parent].orbit);
        float parentY = getBodyY(parent, bodies[parent].orbit);
        float parentZ = getBodyZ(parent, bodies[parent].orbit);

        if (bodies[n].orbits_body != 0) {
            glRotatef(bodies[n].orbital_tilt, parentX, parentY, 1);
            glTranslatef(x, 1, z);
        }

        glRotatef(bodies[n].orbital_tilt, 0, 0, 1);

        if (bodies[n].orbits_body == 0)
            glTranslatef(x, 1, z);
        else
            glTranslatef(parentX, 1, parentZ);

        tiltAndDrawAxis(n);
        glRotatef(90.0, 1.0, 0, 0);
        glScalef(bodies[n].radius, bodies[n].radius, bodies[n].radius);

        glColor3f(bodies[n].r, bodies[n].g, bodies[n].b);
        glutSolidSphere(1.0, 100, 100);
    glPopMatrix();
    drawOrbit(n);
}

/*****************************/

void display(void)
{
  int i;

  glClear(GL_COLOR_BUFFER_BIT);

  /* set the camera */
  setView();
  drawAxis();

  if (draw_starfield)
    drawStarfield();

  for (i= 0; i < numBodies; i++)
  {
    glPushMatrix();
      drawBody (i);
    glPopMatrix();
  }

  glutSwapBuffers();
}

/*****************************/

void reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective (48.0, (GLfloat) w/(GLfloat) h, 10000.0, 800000000.0);
}

/*****************************/

void keyboard(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27:  /* Escape key */
      exit(0);
  }
}

/*****************************/

int main(int argc, char** argv)
{
  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow ("COMP27112 Exercise 2");
  glutFullScreen();
  init ();
  glutDisplayFunc (display);
  glutReshapeFunc (reshape);
  glutKeyboardFunc (keyboard);
  glutIdleFunc (animate);
  readSystem();
  glutMainLoop ();
  return 0;
}
/* end of ex2.c */
