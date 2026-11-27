#ifndef MBAI_TEST_STUB_NDS_H
#define MBAI_TEST_STUB_NDS_H
/* Host geometry inspection hooks; transforms remain no-ops.
 * This checks emitted topology, NOT DS rasterization or GPU behavior. */
#include <stdint.h>
void test_gl_begin(int mode);
void test_gl_vertex(float x,float y,float z);
void test_gl_end(void);
void test_gl_color(unsigned value);
void test_gl_flush(void);
void test_gl_matrix_barrier(void);
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
#define glDisable(...) ((void)0)
#define glClearColor(...) ((void)0)
#define glClearPolyID(...) ((void)0)
#define glClearDepth(...) ((void)0)
#define glViewport(...) ((void)0)
#define glMatrixMode(...) test_gl_matrix_barrier()
#define glLoadIdentity() test_gl_matrix_barrier()
#define gluPerspective(...) ((void)0)
#define gluLookAt(...) test_gl_matrix_barrier()
#define glPolyFmt(...) ((void)0)
#define glPushMatrix() test_gl_matrix_barrier()
#define glPopMatrix(n) ((void)(n),test_gl_matrix_barrier())
#define glTranslatef(x,y,z) ((void)(x),(void)(y),(void)(z),test_gl_matrix_barrier())
#define degreesToAngle(n) (n)
#define glRotateZ(n) ((void)(n))
#define glRotateZi(n) ((void)(n),test_gl_matrix_barrier())
#define glRotateX(n) ((void)(n))
#define glRotateXi(n) ((void)(n),test_gl_matrix_barrier())
#define glRotateY(n) ((void)(n))
#define glRotateYi(n) ((void)(n),test_gl_matrix_barrier())
#define glBegin(mode) test_gl_begin(mode)
#define glEnd() test_gl_end()
#define glColor(value) test_gl_color(value)
#define glVertex3f(x,y,z) test_gl_vertex((x),(y),(z))
#define glFlush(...) test_gl_flush()
#endif
