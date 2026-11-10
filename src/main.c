#include <SDL3/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <glad/glad.h>

#define WIDTH 800
#define HEIGHT 600

struct progstate {
	SDL_Window *window;
	SDL_GLContext gl_context;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
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



	return SDL_APP_CONTINUE;
}
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	struct progstate *st = appstate;
	if(st == NULL) {
		return;
	}

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

	return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate(void *appstate) {
	struct progstate *st = appstate;

	SDL_GL_SwapWindow(st->window);
	return SDL_APP_CONTINUE;
}
