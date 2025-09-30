#ifndef BUFFERS_H_
#define BUFFERS_H_

#include "SDL3/SDL_gpu.h"

#include "cntx.h"

static inline bool alloc_vertex_buffer(struct cntx *cntx, SDL_GPUBuffer **buf, size_t sz)
{
	SDL_GPUDevice *device = cntx->device;
	const SDL_GPUBufferCreateInfo ci = {
		.size = sz,
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
	};

	SDL_GPUBuffer *b = SDL_CreateGPUBuffer(device, &ci);
	if (!b)
		return false;

	*buf = b;

	return true;
}
#endif
