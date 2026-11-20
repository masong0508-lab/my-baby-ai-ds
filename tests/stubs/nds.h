#ifndef MBAI_TEST_STUB_NDS_H
#define MBAI_TEST_STUB_NDS_H
/* No-op 3D API for native geometry-count tests; NOT a DS emulator. */
#include <stdint.h>
#define RGB15(r,g,b) ((unsigned)((r)|((g)<<5)|((b)<<10)))
#define GL_TRIANGLES 0
#define GL_QUADS 1
#define GL_QUAD_STRIP 3
#define GL_PROJECTION 0
#define GL_MODELVIEW 1
#define GL_ANTIALIAS 2
#define POLY_ALPHA(n) ((n)<<16)
#define POLY_CULL_NONE 0
#define glInit() ((void)0)
#define glEnable(...) ((void)0)
#define glClearColor(...) ((void)0)
#define glClearPolyID(...) ((void)0)
#define glClearDepth(...) ((void)0)
#define glViewport(...) ((void)0)
#define glMatrixMode(...) ((void)0)
#define glLoadIdentity() ((void)0)
#define gluPerspective(...) ((void)0)
#define gluLookAt(...) ((void)0)
#define glPolyFmt(...) ((void)0)
#define glPushMatrix() ((void)0)
#define glPopMatrix(n) ((void)(n))
#define glTranslatef(x,y,z) ((void)(x),(void)(y),(void)(z))
#define degreesToAngle(n) (n)
#define glRotateZ(n) ((void)(n))
#define glBegin(mode) ((void)(mode))
#define glEnd() ((void)0)
#define glColor(value) ((void)(value))
#define glVertex3f(x,y,z) ((void)(x),(void)(y),(void)(z))
#define glFlush(...) ((void)0)
#endif
