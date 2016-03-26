/** 
 * @file main.c
 * @author Fabjan Sukalia <fsukalia@gmail.com>
 * @date 2016-03-26
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static char *prog_name = "shader_compiler";

static Window xwin;
static Display *disp = NULL;
static EGLDisplay egl_disp;
static EGLSurface egl_surf;
static EGLContext ctxt;

static EGLint default_config[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,

	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_NONE
};

static EGLint ctxt_attr[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

static bool create_context(void)
{
	disp = XOpenDisplay(NULL);
	if (disp == NULL) {
		fprintf(stderr, "Failed to open X display\n");
		return false;
	}

	XSetWindowAttributes attr;
	attr.event_mask = ExposureMask;
	
	Window root = DefaultRootWindow(disp);
	xwin = XCreateWindow(disp, root, 0, 0, 1, 1, 0, CopyFromParent, 
		InputOutput, CopyFromParent, CWEventMask, &attr);
	
	XStoreName(disp, xwin, prog_name);
	//XMapWindow(disp, xwin);

	egl_disp = eglGetDisplay(disp);
	if (egl_disp == EGL_NO_DISPLAY) {
		fprintf(stderr, "Failed to create EGL display\n");
		return false;
	}

	if (!eglInitialize(egl_disp, NULL, NULL)) {
		fprintf(stderr, "Failed to initialize EGL\n");
		return false;
	}

	EGLConfig egl_config;
	EGLint egl_num_config = 0;
	if (!eglChooseConfig(egl_disp, default_config, &egl_config, 1, &egl_num_config)) {
		fprintf(stderr, "Failed to select EGL configuration\n");
		return false;
	}

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		fprintf(stderr, "Failed to bind OpenGL ES\n");
		return false;
	}

	ctxt = eglCreateContext(egl_disp, egl_config, EGL_NO_CONTEXT, ctxt_attr);
	if (ctxt == EGL_NO_CONTEXT) {
		fprintf(stderr, "Failed to create EGL context\n");
		return false;
	}

	egl_surf = eglCreateWindowSurface(egl_disp, egl_config, xwin, NULL);
	if (egl_surf == EGL_NO_SURFACE) {
		fprintf(stderr, "Failed to create EGL Window\n");
		return false;
	}

	if (!eglMakeCurrent(egl_disp, egl_surf, egl_surf, ctxt)) {
		fprintf(stderr, "Failed to make context active\n");
		return false;
	}
	return true;
}

static bool dump_program_binary(const char *filename, GLuint prog)
{
	FILE *file = fopen(filename, "w");
	if (file == NULL) {
		fprintf(stderr, "Failed to create file \"%s\"\n", filename);
		return false;
	}

	int length = 0;
	glGetProgramiv(prog, GL_PROGRAM_BINARY_LENGTH_OES, &length);
	uint8_t *buffer = malloc(length);
	if (buffer == NULL) {
		fprintf(stderr, "Failed to allocate memory\n");
		return false;
	}

	GLenum bin_fmt = GL_MALI_PROGRAM_BINARY_ARM;
	glGetProgramBinaryOES(prog, length, NULL, &bin_fmt, buffer);
	fwrite(buffer, length, 1, file);
	fclose(file);

	return true;
}

static GLuint create_shader(GLenum type, const char *path)
{
	int fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Couldn't open file \"%s\"\n", path);
		exit(EXIT_FAILURE);
	}

	struct stat st;
	if (fstat(fd, &st) != 0) {
		fprintf(stderr, "Couldn't retrieve file size\n");
		exit(EXIT_FAILURE);
	}

	off_t size = st.st_size;

	char *src = calloc(1, size + 1);
	if (src == NULL) {
		fprintf(stderr, "Failed to allocate memory\n");
		exit(EXIT_FAILURE);
	}

	if (read(fd, src, size) != size) {
		fprintf(stderr, "Failed to read file\n");
		exit(EXIT_FAILURE);
	}

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const char *const *)&src, NULL);
	glCompileShader(shader);

	close(fd);
	free(src);

	return shader;
}

static void log_shader(GLuint shader, FILE *out)
{
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE)
		fprintf(out, "Compile successfull.\nLog:\n");
	else
		fprintf(out, "Compile failed\nLog:\n");
	
	GLint len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	if (len == 0)
		return;

	char *log = malloc(len);
	if (log == NULL)
		exit(EXIT_FAILURE);
	
	glGetShaderInfoLog(shader, len, NULL, log);
	fprintf(out, "%s\n", log);
}

static char *type_to_str(GLenum type)
{
	switch(type) {
	case GL_FLOAT:
		return "float";
	
	case GL_FLOAT_VEC2:
		return "vec2f";
	
	case GL_FLOAT_VEC3:
		return "vec3f";
	
	case GL_FLOAT_VEC4:
		return "vec4f";

	case GL_FLOAT_MAT2:
		return "mat2f";
	
	case GL_FLOAT_MAT3:
		return "mat3f";
	
	case GL_FLOAT_MAT4:
		return "mat4f";
	
	case GL_INT:
		return "int";
	
	case GL_INT_VEC2:
		return "vec2i";
	
	case GL_INT_VEC3:
		return "vec3i";
	
	case GL_INT_VEC4:
		return "vec4i";

	case GL_BOOL:
		return "bool";
	
	case GL_BOOL_VEC2:
		return "vec2b";
	
	case GL_BOOL_VEC3:
		return "vec3b";
	
	case GL_BOOL_VEC4:
		return "vec4b";

	case GL_SAMPLER_2D:
		return "sampler2d";
	
	case GL_SAMPLER_CUBE:
		return "samplerCube";

	default:
		return "unknown type";
	}
}

static void log_program(GLuint program, FILE *out)
{
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_TRUE)
		fprintf(out, "Link successfull\nLog:\n");
	else
		fprintf(out, "Link failed\nLog:\n");
	
	GLint len;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	if (len != 0) {
		char *log = malloc(len);
		if (log == NULL)
			exit(EXIT_FAILURE);
		
		glGetProgramInfoLog(program, len, NULL, log);
		fprintf(out, "%s\n", log);
	}

	GLint num_attr = 0;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &num_attr);
	if (num_attr > 0) {
		fprintf(out, "Active attributes:\n");
		GLint max_len = 0;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_len);

		char *attr_name = malloc(max_len);

		for (int i = 0; i < num_attr; i++) {
			GLint size;
			GLenum type;
			glGetActiveAttrib(program, i, max_len, NULL, &size, &type, attr_name);
			fprintf(out, " * %s: %d x %s\n", attr_name, size, type_to_str(type));
		}
		free(attr_name);
	} else {
		fprintf(out, "No active attributes\n");
	}

	GLint num_uni = 0;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num_uni);
	if (num_uni > 0) {
		fprintf(out, "Active uniforms:\n");
		GLint max_len = 0;
		glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_len);

		char *uni_name = malloc(max_len);

		for (int i = 0; i < num_uni; i++) {
			GLint size;
			GLenum type;
			glGetActiveUniform(program, i, max_len, NULL, &size, &type, uni_name);
			fprintf(out, " * %s: %d x %s\n", uni_name, size, type_to_str(type));
		}
		free(uni_name);
	} else {
		fprintf(out, "No active uniforms\n");
	}
}

static void usage(void) {
	fprintf(stderr, "Usage: %s VERTEX_FILE FRAGMENT_FILE OUTPUT_FILE\n",
		prog_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	if (argc > 0)
		prog_name = argv[0];

	if (argc != 4)
		usage();

	if (!create_context())
		exit(EXIT_FAILURE);
	
	char *vs_file = argv[1];
	char *fs_file = argv[2];
	char *out_file = argv[3];

	GLuint vs = create_shader(GL_VERTEX_SHADER, vs_file);
	log_shader(vs, stdout);
	GLuint fs = create_shader(GL_FRAGMENT_SHADER, fs_file);
	log_shader(fs, stdout);

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);

	glLinkProgram(prog);
	log_program(prog, stdout);

	dump_program_binary(out_file, prog);
	
	return 0;
}
