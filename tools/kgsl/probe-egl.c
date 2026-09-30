/* Isolated offscreen probe. Does not open a display or change scanout. */
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <string.h>
static GLuint shader(GLenum type,const char *source) {
 GLuint s=glCreateShader(type);glShaderSource(s,1,&source,NULL);glCompileShader(s);
 GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[1024];glGetShaderInfoLog(s,sizeof(log),NULL,log);fprintf(stderr,"shader: %s\n",log);glDeleteShader(s);return 0;}return s;
}
int main(void) {
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY); EGLint major,minor,n;
 EGLContext c=EGL_NO_CONTEXT; EGLSurface s=EGL_NO_SURFACE; int result=1;
 if(!eglInitialize(d,&major,&minor)){fprintf(stderr,"eglInitialize: %#x\n",eglGetError());return 1;}
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
 EGLConfig cfg;
 if(!eglBindAPI(EGL_OPENGL_ES_API)||!eglChooseConfig(d,attributes,&cfg,1,&n)||n!=1)goto cleanup;
 const EGLint size[]={EGL_WIDTH,16,EGL_HEIGHT,16,EGL_NONE};
 const EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
 s=eglCreatePbufferSurface(d,cfg,size);c=eglCreateContext(d,cfg,EGL_NO_CONTEXT,version);
 if(s==EGL_NO_SURFACE||c==EGL_NO_CONTEXT||!eglMakeCurrent(d,s,s,c))goto cleanup;
 const char *renderer=(const char*)glGetString(GL_RENDERER);
 printf("EGL %d.%d vendor=%s\nGL vendor=%s renderer=%s version=%s\n",major,minor,eglQueryString(d,EGL_VENDOR),glGetString(GL_VENDOR),renderer,glGetString(GL_VERSION));
 glViewport(0,0,16,16);glClearColor(0.25f,0.5f,0.75f,1.0f);glClear(GL_COLOR_BUFFER_BIT);
 GLuint vs=shader(GL_VERTEX_SHADER,"attribute vec2 position; void main(){gl_Position=vec4(position,0.0,1.0);}");
 GLuint fs=shader(GL_FRAGMENT_SHADER,"precision mediump float; void main(){gl_FragColor=vec4(0.75,0.25,0.5,1.0);}");
 if(!vs||!fs)goto cleanup;
 GLuint program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glBindAttribLocation(program,0,"position");glLinkProgram(program);
 GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);
 if(!linked){char log[1024];glGetProgramInfoLog(program,sizeof(log),NULL,log);fprintf(stderr,"link: %s\n",log);goto cleanup;}
 const GLfloat triangle[]={-1,-1,1,-1,0,1};glUseProgram(program);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,triangle);glDrawArrays(GL_TRIANGLES,0,3);glFinish();
 unsigned char pixel[4]={0};glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
 GLenum error=glGetError();printf("pixel=%u,%u,%u,%u error=%#x\n",pixel[0],pixel[1],pixel[2],pixel[3],error);
 result=error!=GL_NO_ERROR||pixel[0]<190||pixel[0]>192||pixel[1]<63||pixel[1]>65||pixel[2]<127||pixel[2]>129;
 if(!renderer||strstr(renderer,"llvmpipe")||strstr(renderer,"softpipe")){fprintf(stderr,"Software fallback: hardware acceleration NOT validated\n");result=1;}
cleanup:
 eglMakeCurrent(d,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
 if(c!=EGL_NO_CONTEXT)eglDestroyContext(d,c);if(s!=EGL_NO_SURFACE)eglDestroySurface(d,s);eglTerminate(d);return result;
}
