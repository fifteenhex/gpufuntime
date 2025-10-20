#ifndef CNTX_H_
#define CNTX_H_

#include "SDL3/SDL_gpu.h"

struct model;

static struct cntx {
	SDL_Window* window;
	SDL_GPUDevice *device;

	/* Shaders */
	SDL_GPUShader* vertexShader;
	SDL_GPUShader* fragmentShader;

	SDL_GPUBuffer* vertexBuffer;

	struct model *cube, *donut;
} _cntx;
#endif
