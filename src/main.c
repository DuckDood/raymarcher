#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <glad/glad.h>



#include <vectors.h>

#define WIDTH 800
#define HEIGHT 600

struct progstate {
	SDL_Window *window;
	SDL_GLContext gl_context;

	GLuint screen_tri_vao;
	GLuint screen_tri_vbo;

	GLuint raymarch_prog;

	vec3 cam_pos;
	vec3 cam_rotation; // 0,1,2 for pitch,yaw,roll
};

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]]int argc, [[maybe_unused]]char **argv) {
	*appstate = calloc(1, sizeof(struct progstate));
	struct progstate *st = *appstate;
	if(st == NULL) {
		// cooked atp
		fprintf(stderr, "Failed to allocate memory for program state, exiting\n");
		fflush(stderr);
		return SDL_APP_FAILURE;
	}

	if(!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Failed to initialize SDL3: %s\n", SDL_GetError());
		fflush(stderr);
		return SDL_APP_FAILURE;
	}

	st->window = SDL_CreateWindow("raymarcher", WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	if(st->window == NULL) {
		fprintf(stderr, "Failed to create SDL3 window: %s\n", SDL_GetError());
		fflush(stderr);
		return SDL_APP_FAILURE;
	}

	st->gl_context = SDL_GL_CreateContext(st->window);
	if(st->gl_context == NULL) {
		fprintf(stderr, "Failed to create SDL3 OpenGL context: %s\n", SDL_GetError());
		fflush(stderr);
		return SDL_APP_FAILURE;
	}

	if(!SDL_GL_MakeCurrent(st->window, st->gl_context)) {
		fprintf(stderr, "Failed to create set GL context to window: %s\n", SDL_GetError());
		fflush(stderr);
		return SDL_APP_FAILURE;
	}

	if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		fprintf(stderr, "Failed to init GLAD.");
		return SDL_APP_FAILURE;
	}

	GLfloat tri_vertices[6] = {
		-5, -5,
		5, -5,
		0, 5
	};

	glGenVertexArrays(1, &st->screen_tri_vao);
	glGenBuffers(1, &st->screen_tri_vbo);

	glBindVertexArray(st->screen_tri_vao);
	glBindBuffer(GL_ARRAY_BUFFER, st->screen_tri_vbo);
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri_vertices), tri_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);



	const char *vertex_shader_source = (const char[]){
		#embed "main.vert"
		,'\0'
	};

	const char *fragment_shader_source = (const char[]){
		#embed "main.frag"
		,'\0'
	};

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	int compile_status;
	GLchar info_log[512];

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);
	if(!compile_status) {
		glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
		fprintf(stderr, "Failed to compile vertex shader:\n%s\n", info_log);
	}

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
	if(!compile_status) {
		glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
		fprintf(stderr, "Failed to compile fragment shader:\n%s\n", info_log);
	}

	st->raymarch_prog = glCreateProgram();
	glAttachShader(st->raymarch_prog, vertex_shader);
	glAttachShader(st->raymarch_prog, fragment_shader);

	glLinkProgram(st->raymarch_prog);

	glGetProgramiv(st->raymarch_prog, GL_LINK_STATUS, &compile_status);

	if(!compile_status) {
		glGetProgramInfoLog(st->raymarch_prog, 512, NULL, info_log);
		fprintf(stderr, "Failed to link shader program:\n%s\n", info_log);
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	st->cam_pos = (vec3){0};
	st->cam_rotation = (vec3){0};

	SDL_GL_SetSwapInterval(1);
	SDL_SetWindowRelativeMouseMode(st->window, 1);



	return SDL_APP_CONTINUE;
}
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	(void)result;
	struct progstate *st = appstate;
	if(st == NULL) {
		return;
	}

	glDeleteProgram(st->raymarch_prog);
	glDeleteBuffers(1, &st->screen_tri_vbo);
	glDeleteVertexArrays(1, &st->screen_tri_vao);

	if(st->gl_context != NULL) {
		SDL_GL_DestroyContext(st->gl_context);
	}

	if(st->window != NULL) {
		SDL_DestroyWindow(st->window);
	}



	SDL_Quit();

}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	struct progstate *st = appstate;

	switch(event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			st->cam_rotation[0] += event->motion.xrel / HEIGHT;
			st->cam_rotation[1] += event->motion.yrel / HEIGHT;

			if(st->cam_rotation[1] > 3.141/2) st->cam_rotation[1] = 3.141/2;
			if(st->cam_rotation[1] < -3.141/2) st->cam_rotation[1] = -3.141/2;
			break;
	}

	return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate(void *appstate) {
	struct progstate *st = appstate;
	
	const bool * const key_states = SDL_GetKeyboardState(NULL);

	if(key_states[SDL_SCANCODE_W]) {
		st->cam_pos[0] += 0.1 * sin(st->cam_rotation[0]) * cos(st->cam_rotation[1]);
		st->cam_pos[2] += 0.1 * cos(st->cam_rotation[0]) * cos(st->cam_rotation[1]);

		st->cam_pos[1] -= 0.1 * sin(st->cam_rotation[1]);
	}

	if(key_states[SDL_SCANCODE_S]) {
		st->cam_pos[0] -= 0.1 * sin(st->cam_rotation[0]) * cos(st->cam_rotation[1]);
		st->cam_pos[2] -= 0.1 * cos(st->cam_rotation[0]) * cos(st->cam_rotation[1]);

		st->cam_pos[1] += 0.1 * sin(st->cam_rotation[1]);
	}

	if(key_states[SDL_SCANCODE_A]) {
		st->cam_pos[0] -= 0.1 * cos(st->cam_rotation[0]) * cos(st->cam_rotation[1]);
		st->cam_pos[2] += 0.1 * sin(st->cam_rotation[0]) * cos(st->cam_rotation[1]);
	}

	if(key_states[SDL_SCANCODE_D]) {
		st->cam_pos[0] += 0.1 * cos(st->cam_rotation[0]) * cos(st->cam_rotation[1]);
		st->cam_pos[2] -= 0.1 * sin(st->cam_rotation[0]) * cos(st->cam_rotation[1]);
	}


	mat4 rotation_matrix = gen_rotation_matrix(st->cam_rotation[0], st->cam_rotation[1], st->cam_rotation[2]);

	glClearColor(1, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(st->raymarch_prog);
	glUniform2f(glGetUniformLocation(st->raymarch_prog, "uResolution"), WIDTH, HEIGHT);
	glUniform3fv(glGetUniformLocation(st->raymarch_prog, "uCamPosition"), 1, (GLfloat*)&st->cam_pos);
	glUniformMatrix4fv(glGetUniformLocation(st->raymarch_prog, "uCamRotationMatrix"), 1, false, (GLfloat*)&rotation_matrix);

	glBindVertexArray(st->screen_tri_vao);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	SDL_GL_SwapWindow(st->window);
	return SDL_APP_CONTINUE;
}
