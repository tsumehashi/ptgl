#ifndef PTGL_CORE_GLPATH_H_
#define PTGL_CORE_GLPATH_H_

#ifdef EMSCRIPTEN
#define GL_GLEXT_PROTOTYPES
#define    EGL_EGLEXT_PROTOTYPES
#include <GL/glew.h>
#else    // EMSCRIPTEN
#include <GL/glew.h>
#endif    // EMSCRIPTEN

#endif /* PTGL_CORE_GLPATH_H_ */

