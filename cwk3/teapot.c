#include <GL/glut.h>
#include <math.h>
#include "bitmap.c"
/*
This is for COMP27112 Coursework Assignment 3.
Author: Toby Howard.
 */

GLfloat xRotation, yRotation= 0.0;
GLint mouseX, mouseY;

GLfloat white_light[] =     { 1, 1, 0.7, 1.0 };
GLfloat light_position0[] = { 5.0, 5.0, 5.0, 0.0 };
GLfloat light_position1[] = { -5.0, -5.0, -5.0, 0.0 };
GLfloat matSpecular[] =     { 1.0, 1.0, 3.0, 1.0 };
GLfloat matShininess[] =    { 100.0 };
GLfloat matSurface[] =      { 2.0, 0.0, 0.5, 0.0 };

BITMAPINFO *TexInfo; // Texture bitmap information
GLubyte    *TexBits; // Texture bitmap pixel bits

void drawGround() {
    int i,j;
    int dim = 10;
       glMaterialfv(GL_FRONT, GL_SPECULAR,  matSpecular);
       glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);
       glMaterialfv(GL_FRONT, GL_AMBIENT,   matSurface);


      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, 3, TexInfo->bmiHeader.biWidth,
                   TexInfo->bmiHeader.biHeight, 0, GL_BGR_EXT,
                   GL_UNSIGNED_BYTE, TexBits);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glEnable(GL_TEXTURE_2D);
    for (i = -dim; i < dim; i++)
        for (j = -dim; j < dim; j++) {
            glPushMatrix();
                glTranslatef (0, -1, i);
                glTranslatef (j, -1, 0.0);
                glColor3f(0.1, 0.0, 0.2);
                glutSolidTorus(0.2, 0.2, 10, 10);
            glPopMatrix();
            glPushMatrix();
                glTranslatef (0, -1, i - 0.5);
                glTranslatef (j, -1, 0.0);
                glColor3f(0.1, 0.0, 0.2);
                glutSolidTeapot(0.2);
            glPopMatrix();
        }
}

void initialise(void) {
   TexBits = LoadDIBitmap("coyote.bmp", &TexInfo);

   glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
   glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);


   glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
   glLightfv(GL_LIGHT1, GL_DIFFUSE, white_light);
   glLightfv(GL_LIGHT1, GL_SPECULAR, white_light);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_LIGHT1);
   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_SMOOTH);


}

void display(void) {
   glClearColor(1.0, 1.0, 1.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
/*
// define material properties
   glMaterialfv(GL_FRONT, GL_SPECULAR,  matSpecular);
   glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);
   glMaterialfv(GL_FRONT, GL_AMBIENT,   matSurface);


  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, TexInfo->bmiHeader.biWidth,
               TexInfo->bmiHeader.biHeight, 0, GL_BGR_EXT,
               GL_UNSIGNED_BYTE, TexBits);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_TEXTURE_2D);


 // draw the teapot
   glPushMatrix();
      glRotatef(xRotation, 1.0, 0.0, 0.0);
      glRotatef(yRotation, 0.0, 1.0, 0.0);
      glutSolidTeapot(0.5);
   glPopMatrix();
*/
   drawGround();
   glutSwapBuffers();
}

void reshape(int w, int h) {
   glViewport(0, 0,(GLsizei) w,(GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(17.0, (GLfloat)w/(GLfloat)h, 1.0, 20.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   gluLookAt(5.0,5.0,5.0, 0.0,0.0,0.0, 0.0,1.0,0.0);
}

int isSmooth = 1;
void keyboard(unsigned char key, int x, int y) {
   switch(key) {
	  case 27:  exit(0);
                break;
     case 's':
     case 'S':
        if (isSmooth)
            glShadeModel(GL_FLAT);
        else
            glShadeModel(GL_SMOOTH);

        isSmooth = abs(isSmooth - 1);
     break;
   }

   glutPostRedisplay();
}

void mouseMotion(int x, int y) {
// Called when mouse moves
	xRotation+=(y-mouseY);	mouseY= y;
	yRotation+=(x-mouseX);	mouseX= x;
	// keep all rotations between 0 and 360.
	if ( xRotation > 360.0) xRotation-= 360.0;
	if ( xRotation < 0.0)   xRotation+= 360.0;
	if ( yRotation > 360.0) yRotation-= 360.0;
	if ( yRotation < 0.0)   yRotation+= 360.0;
	// ask for redisplay
	glutPostRedisplay();
}


void mousePress(int button, int state, int x, int y) {
// When left mouse button is pressed, save the mouse(x,y)
	if((button == GLUT_LEFT_BUTTON) &&(state == GLUT_DOWN)) {
		mouseX= x;
		mouseY= y;
	}
}

int main(int argc, char** argv) {
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize(800, 800);
   glutCreateWindow("COMP20072 Coursework Assignment 5: The Teapot");
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMouseFunc(mousePress);
   glutMotionFunc(mouseMotion);
   initialise();
   glutMainLoop();
   return 0;
}
