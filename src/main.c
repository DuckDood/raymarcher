#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif



#include <vectors.h>

#define WIDTH 800
#define HEIGHT 600


struct progstate {
	SDL_Window *window;
	SDL_GLContext gl_context;

	GLuint screen_tri_vao;
	GLuint screen_tri_vbo;

	GLuint raymarch_prog;

	int screen_width;
	int screen_height;

	vec3 cam_pos;
	vec3 cam_rotation; // 0,1,2 for pitch,yaw,roll

	GLuint uniform_buffers[2];

	GLuint grid_texture;
	GLuint grid_program;

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

	st->screen_width = WIDTH;
	st->screen_height = HEIGHT;

	if(!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Failed to initialize SDL3: %s\n", SDL_GetError());
		fflush(stderr);
		return SDL_APP_FAILURE;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	st->window = SDL_CreateWindow("raymarcher", WIDTH, HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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

#ifndef __EMSCRIPTEN__
	if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		fprintf(stderr, "Failed to init GLAD.");
		return SDL_APP_FAILURE;
	}
#endif

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
		return SDL_APP_FAILURE;
	}

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
	if(!compile_status) {
		glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
		fprintf(stderr, "Failed to compile fragment shader:\n%s\n", info_log);
		return SDL_APP_FAILURE;
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

	const char *compute_shader_source = (const char[]){
		#embed "main.comp"
		,'\0'
	};
	GLuint grid_compute = glCreateShader(GL_COMPUTE_SHADER);

	glShaderSource(grid_compute, 1, &compute_shader_source, NULL);
	glCompileShader(grid_compute);

	glGetShaderiv(grid_compute, GL_COMPILE_STATUS, &compile_status);
	if(!compile_status) {
		glGetShaderInfoLog(grid_compute, 512, NULL, info_log);
		fprintf(stderr, "Failed to compile compute shader:\n%s\n", info_log);
		return SDL_APP_FAILURE;
	}

	st->grid_program = glCreateProgram();
	glAttachShader(st->grid_program, grid_compute);

	glLinkProgram(st->grid_program);

	glGetProgramiv(st->grid_program, GL_LINK_STATUS, &compile_status);

	if(!compile_status) {
		glGetProgramInfoLog(st->grid_program, 512, NULL, info_log);
		fprintf(stderr, "Failed to link compute program:\n%s\n", info_log);
	}
	glDeleteShader(grid_compute);


	glGenBuffers(2, st->uniform_buffers);
	glBindBuffer(GL_UNIFORM_BUFFER, st->uniform_buffers[0]);
	glBufferData(GL_UNIFORM_BUFFER, 1024, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glUniformBlockBinding(st->raymarch_prog, glGetUniformBlockIndex(st->raymarch_prog, "uShapes"), 0);

	glUniformBlockBinding(st->grid_program, glGetUniformBlockIndex(st->raymarch_prog, "uShapes"), 0);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, st->uniform_buffers[0], 0, 1024);


	glBindBuffer(GL_UNIFORM_BUFFER, st->uniform_buffers[1]);
	glBufferData(GL_UNIFORM_BUFFER, 1024, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glUniformBlockBinding(st->raymarch_prog, glGetUniformBlockIndex(st->raymarch_prog, "uOperations"), 1);
	glUniformBlockBinding(st->grid_program, glGetUniformBlockIndex(st->raymarch_prog, "uOperations"), 1);

	glBindBufferRange(GL_UNIFORM_BUFFER, 1, st->uniform_buffers[1], 0, 1024);

	glGenTextures(1, &st->grid_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, st->grid_texture);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#define TEX_SIZE 80

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, TEX_SIZE,TEX_SIZE,TEX_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

	glBindImageTexture(0, st->grid_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);



	glUseProgram(st->grid_program);
	glDispatchCompute(TEX_SIZE, TEX_SIZE, TEX_SIZE);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//st->cam_pos = (vec3){1, 0, -3};
	//st->cam_rotation = (vec3){-3.141/2 + 1, 0, 0};
	//
	st->cam_pos = (vec3){0, 0, -7};
	st->cam_rotation = (vec3){0};

	//SDL_GL_SetSwapInterval(1);
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

		case SDL_EVENT_WINDOW_RESIZED:
			st->screen_width = event->window.data1;
			st->screen_height = event->window.data2;

			glViewport(0, 0, st->screen_width, st->screen_height);

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

	float time = SDL_GetTicks()/1000.;


	struct shape{
		mat4 inv_transformation;
		vec4 params;
		vec4 color;
		uint32_t type;
	};
	struct shape sp[5];
	//sp[0].transformation = gen_translation_matrix((vec3){0, -200, 0});
	sp[0].inv_transformation = gen_inverse_transformation_matrix((vec3){0, -200, 0}, (vec3){0}, (vec3){1,1,1});

	sp[0].params[0] = 198;
	sp[0].type = 0;

	sp[0].color = (vec3){1, 0, 0};

	sp[1].inv_transformation = gen_inverse_transformation_matrix((vec3){-3, 0, 0}, (vec3){1.2, 1.5, 1.5}, (vec3){1,1,1});

	sp[1].params[0] = 1;
	sp[1].params[1] = 1;
	sp[1].params[2] = 1;
	sp[1].type = 1;

	sp[1].color = (vec3){0, 1, 0};


	//sp[2].transformation = m4_m4_mul(gen_translation_matrix((vec3){-3 + sin(time) * 3, 0, 0}),gen_rotation_matrix(0, time*3, time));
	sp[2].inv_transformation = gen_inverse_transformation_matrix((vec3){-3 + sin(time) * 3, 0, 0}, (vec3){time*3, 0, time}, (vec3){1,1,1});

	sp[2].params[0] = 0.5;
	sp[2].params[1] = 0.2;
	sp[2].type = 2;

	sp[2].color = (vec3){0, 1, 1};

	sp[3].inv_transformation = gen_inverse_transformation_matrix((vec3){3, 0, 0}, (vec3){3.14159/2, 0, 0}, (vec3){1,1,1});

	sp[3].params[0] = 1;
	sp[3].params[1] = 1;
	sp[3].params[2] = 1;
	sp[3].type = 3;

	sp[3].color = (vec3){1, 1, 0};


	//sp[4].transformation = m4_m4_mul(gen_translation_matrix((vec3){3, 0, sin(time)*4}),gen_rotation_matrix(0, time*3, time));
	sp[4].inv_transformation = gen_inverse_transformation_matrix((vec3){3, 0, sin(time)*4}, (vec3){0}, (vec3){1,1,1});

	sp[4].params[0] = 0.5;
	sp[4].type = 0;

	sp[4].color = (vec3){0, 0, 1};

	int shape_count = 5;

	uint64_t start_time = SDL_GetTicks();
	glBindBuffer(GL_UNIFORM_BUFFER, st->uniform_buffers[0]);
	glBufferSubData(GL_UNIFORM_BUFFER, (long)16, sizeof(struct shape) * shape_count, &sp);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &shape_count);


struct renderOperation {
	// 0 0 0 0 0 0 0 0
	// ^ ^-^ ^-^ ^ ^ ^
	// t o-o b-b 1?2?
	int operation; 
	/*
	   ** BYTE 1 **

	   * first (highest) bit: type of operation (set distance or mix)
	   * second and third highest bits: out index
	   * fourth and fifth bits: do a boolean operation on either current value with first operand (if setting) or on each operand together (if mixing) boolean operations are clear (only for setting), union, difference, intersection
	   * sixth bit: whether first operand is a shape or a distance.
	   * seventh bit: whether second operand is a shape or a distance.	
	   * eighth bit: whether this is a sized operation (meaning it will operate on more than 1 shape at once)

	   ** BYTE 2 **

	   * first and second bit: color mixing type (0 for mix, 1 for operand1 color, 2 for operand2 color)


	   ** BYTE 3 AND 4 **
	   * byte 3 is operand1 length
	   * byte 4 is operand2 length
	 */
	// no ai used in this process i just wanted to make a useful comment for once

	int operand1; // operand indices
	int operand2; 

	float smoothing;
};

#define highestBit8 (1 << 7)
#define highestBits8 ((1 << 7) | (1<<6))
#define opToByte(x) (x >> 24)

#define paramsToOp(type, out, boolean, op1Type, op2Type, sized, mixType) (type << 31) | (out << 29) | (boolean << 27) | (op1Type << 26) | (op2Type << 25) | (sized << 24) | (mixType << 22)


#define opType(x) ((opToByte(x) & highestBit8) >> 7)
#define opOut(x) ((opToByte(x) & (highestBits8>>1)) >> 5)
#define opBool(x) ((opToByte(x) & (highestBits8>>3)) >> 3)

#define opOp1Type(x) ((opToByte(x) & (highestBit8>>5)) >> 2)
#define opOp2Type(x) ((opToByte(x) & (highestBit8>>6)) >> 1)

#define opSized(x) ((opToByte(x) & (highestBit8>>7)) >> 0)

#define opMixType(x) ((opToByte((x<<8))&highestBits8)>>6)


	struct renderOperation ops[5] = { 
		{
			paramsToOp(1, 2, 0, 0, 0, 1, 0),
			1,
			2,
			0.5
		},

		{
			paramsToOp(1, 3, 1, 0, 0, 1, 1),
			3,
			4,
			0.2
		},

		{
			paramsToOp(1, 0, 0, 1, 0, 1, 0),
			3,
			4,
			0.0
		},
		{
			paramsToOp(1, 0, 0, 1, 1, 1, 0),
			0,
			2,
			0.0
		},
		{
			paramsToOp(1, 0, 0, 1, 0, 1, 0),
			0,
			0,
			0.0
		},
	};
	int op_count = 5;

	glBindBuffer(GL_UNIFORM_BUFFER, st->uniform_buffers[1]);
	glBufferSubData(GL_UNIFORM_BUFFER, (long)16, sizeof(struct renderOperation) * op_count, &ops);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &op_count);

	glUseProgram(st->grid_program);
	glDispatchCompute(TEX_SIZE, TEX_SIZE, TEX_SIZE);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(st->raymarch_prog);
	glUniform1f(glGetUniformLocation(st->raymarch_prog, "uTime"), (float)time);
	glUniform2f(glGetUniformLocation(st->raymarch_prog, "uResolution"), st->screen_width, st->screen_height);
	glUniform3fv(glGetUniformLocation(st->raymarch_prog, "uCamPosition"), 1, (GLfloat*)&st->cam_pos);
	glUniformMatrix4fv(glGetUniformLocation(st->raymarch_prog, "uCamRotationMatrix"), 1, false, (GLfloat*)&rotation_matrix);

	glBindVertexArray(st->screen_tri_vao);


	glBindTexture(GL_TEXTURE_3D, st->grid_texture);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	SDL_GL_SwapWindow(st->window);

	printf("render time: %ims\n", (int)(SDL_GetTicks()-start_time));
	return SDL_APP_CONTINUE;
}
