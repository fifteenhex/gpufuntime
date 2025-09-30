#ifndef MODEL_H_
#define MODEL_H_

#include <cgltf.h>
#include "SDL3/SDL_gpu.h"

struct model {
	cgltf_data* cube;

	const void *vertices, *indices;
	size_t vertices_sz, indices_sz;
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* index_buffer;
};

#endif
