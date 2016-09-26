#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <GLUT/glut.h>
#include "debug.h"

#define WIDTH  (512)
#define HEIGHT (512)
#define VIEWING_DISTANCE_MIN  3.0

static int Width = WIDTH;
static int Height = HEIGHT;
static uint TextureId = 0;
static uint TextureTarget = GL_TEXTURE_2D;
static uint TextureInternal = GL_RGBA;
static uint TextureFormat = GL_RGBA;
static uint TextureType = GL_UNSIGNED_BYTE;
static uint TextureWidth = WIDTH;
static uint TextureHeight = HEIGHT;
static size_t TextureTypeSize = sizeof(char);
static uint ActiveTextureUnit = GL_TEXTURE1_ARB;
static void* HostImageBuffer = 0;
static float TexCoords[4][2];
static float VertexPos[4][2] = { { -1.0f, -1.0f }, { +1.0f, -1.0f }, { +1.0f, +1.0f }, { -1.0f,
        +1.0f } };
typedef int BOOL;
#define TRUE 1
#define FALSE 0
static int g_bButton1Down = 0;
static GLfloat g_fViewDistance = 3 * VIEWING_DISTANCE_MIN;
static int g_yClick = 0;

static int m_TextureBufferUpdated = 0;

static int nAggregate = 0;
static int reshaping = 0;

int updateTextureBuffer(void *pixels, int w, int h) {
    if (reshaping) return nAggregate;
    //trace("updateTextureBuffer");
    if (!HostImageBuffer)
        return 0;

    static int x = 0, y = 0;

    int stride = w;
    if (h > Height)
        h = Height;
    if (w > Width)
        w = Width;

    char *in = (char *) pixels + (y * Width + x) * 4;
    char *out = (char *) HostImageBuffer;
    for (int i = 0; i < h; i++, in += stride * 4, out += w * 4) {
        memcpy(out, in, w * 4);
    }
    m_TextureBufferUpdated = 1;
    return nAggregate;
}

static void RenderTexture(void *pvData) {
    glDisable (GL_LIGHTING);
    glViewport(0, 0, Width, Height);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode (GL_TEXTURE);
    glLoadIdentity();
    glEnable(TextureTarget);
    glBindTexture(TextureTarget, TextureId);

    static int count = 0;
    static int countG = 0;
    count++;
    countG += 2;
    countG %= 254;
    count %= 255;
    unsigned char *p = (unsigned char*) pvData;
    if (pvData && m_TextureBufferUpdated) {
        glTexSubImage2D(TextureTarget, 0, 0, 0, TextureWidth, TextureHeight, TextureFormat,
                TextureType, pvData);
        m_TextureBufferUpdated = 0;
    }

    glTexParameteri(TextureTarget, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
    glBegin (GL_QUADS);
    //glColor3f(1.0f, 1.0f, 1.0f);

    // left top
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, +1.0f, 0.0f);
    // left bottom
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    // right bottom
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    // top right
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);

    glEnd();
    glBindTexture(TextureTarget, 0);
    glDisable(TextureTarget);
}

static void createTexture(uint width, uint height) {
    if (TextureId) {
        glDeleteTextures(1, &TextureId);
        TextureId = 0;
    }

    trace("Creating Texture %d x %d...\n", width, height);

    TextureWidth = width;
    TextureHeight = height;

    glActiveTextureARB(ActiveTextureUnit);
    glGenTextures(1, &TextureId);
    glBindTexture(TextureTarget, TextureId);
    glTexParameteri(TextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(TextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(TextureTarget, 0, TextureInternal, TextureWidth, TextureHeight, 0, TextureFormat,
            TextureType, 0);
    glBindTexture(TextureTarget, 0);
}

static void setupGraphics(void) {
    createTexture(Width, Height);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable (GL_DEPTH_TEST);
    glActiveTexture (GL_TEXTURE0);
    glViewport(0, 0, Width, Height);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();

//    TexCoords[3][0] = 0.0f;
//    TexCoords[3][1] = 0.0f;
//    TexCoords[2][0] = Width;
//    TexCoords[2][1] = 0.0f;
//    TexCoords[1][0] = Width;
//    TexCoords[1][1] = Height;
//    TexCoords[0][0] = 0.0f;
//    TexCoords[0][1] = Height;

    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, VertexPos);
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, TexCoords);
    if (HostImageBuffer) {
        free(HostImageBuffer);
    }
    HostImageBuffer = malloc(Width * Height * 4);
}

void Idle(void) {
    if (m_TextureBufferUpdated)
        glutPostRedisplay();
}

static void Display(void) {
//    glLoadIdentity();
//    gluLookAt(0, 0, -g_fViewDistance, 0, 0, -1, 0, 1, 0);
//    GLfloat zoom = g_fViewDistance;
//    glOrtho(-1.5 + zoom, 1.0 - zoom, -2.0 + zoom, 0.5 - zoom, -1.0, 3.5); // Changed some of the signs here
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear (GL_COLOR_BUFFER_BIT);
    RenderTexture(HostImageBuffer);
//    glFinish(); // for timing
    glutSwapBuffers();
}

static void Reshape(int w, int h) {
    //reshaping = 1;
    glViewport(0, 0, w, h);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glClear (GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();

//    if (w > 2 * Width || h > 2 * Height) {
//    Width = w;
//    Height = h;
//    setupGraphics();
//    }

    Width = w;
    Height = h;
    setupGraphics();
    //reshaping = 0;
}

void MouseButton(int button, int state, int x, int y) {
    // Respond to mouse button presses.
    // If button1 pressed, mark this state so we know in motion function.
    if (button == GLUT_LEFT_BUTTON) {
        trace("button == GLUT_LEFT_BUTTON");
        g_bButton1Down = (state == GLUT_DOWN) ? TRUE : FALSE;
        g_yClick = y - 3 * g_fViewDistance;
    }
}

void MouseMotion(int x, int y) {
    // If button1 pressed, zoom in/out if mouse is moved up/down.
    if (g_bButton1Down) {
        trace("MouseMotion x= %d, y = %d, vd= %f", x, y, g_fViewDistance);
        g_fViewDistance = (y - g_yClick) / 3.0;
        if (g_fViewDistance < VIEWING_DISTANCE_MIN)
            g_fViewDistance = VIEWING_DISTANCE_MIN;
        glutPostRedisplay();
    }
}

void Keyboard(unsigned char key, int x, int y) {
    const float fStepSize = 0.05f;

    switch (key) {
        case 27:
            exit(0);
            break;

//        case ' ':
//            Animated = !Animated;
//            sprintf(InfoString, "Animated = %s\n", Animated ? "true" : "false");
//            ShowInfo = 1;
//            break;
//
//        case 'i':
//            ShowInfo = ShowInfo > 0 ? 0 : 1;
//            break;
//
//        case 's':
//            ShowStats = ShowStats > 0 ? 0 : 1;
//            break;

        case '+':
        case '=':
            nAggregate++;
            break;

        case '-':
            if (nAggregate > 0) nAggregate--;
            break;
//
//        case 'w':
//            MuC[0] += fStepSize;
//            break;
//
//        case 'x':
//            MuC[0] -= fStepSize;
//            break;
//
//        case 'q':
//            MuC[1] += fStepSize;
//            break;
//
//        case 'z':
//            MuC[1] -= fStepSize;
//            break;
//
//        case 'a':
//            MuC[2] += fStepSize;
//            break;
//
//        case 'd':
//            MuC[2] -= fStepSize;
//            break;
//
//        case 'e':
//            MuC[3] += fStepSize;
//            break;
//
//        case 'c':
//            MuC[3] -= fStepSize;
//            break;

        case 'f':
            glutFullScreen();
            break;

    }
}

int glut(int argc, char** argv) {
    trace("glutInit");
    glutInit(&argc, argv);
    trace("glutInitDisplayMode");
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(Width, Height);
    glutCreateWindow("GLUT Program");
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutIdleFunc(Idle);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);
    setupGraphics();

//    atexit (stopGL);
    trace("glutMainLoop");
    glutMainLoop();
    return EXIT_SUCCESS;
}
