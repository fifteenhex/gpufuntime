#ifndef MODEL_H_
#define MODEL_H_

#include <cgltf.h>
#include <sys/types.h>
#include "SDL3/SDL_gpu.h"

#include "cntx.h"
#include "buffers.h"

struct model {
	cgltf_data* cube;

	off_t vertices, indices;
	size_t vertices_sz, indices_sz;
	unsigned indices_num;
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUBuffer* index_buffer;
};

static inline void model_get_vertices(cgltf_data *model, off_t *off, size_t *sz)
{
	// todo: mm
	*sz = model->buffer_views[0].size;
	*off = model->buffer_views[0].offset;
}

static inline void model_get_indices(cgltf_data *model,
									 off_t *off,
									 size_t *sz,
									 unsigned int *num)
{
	// todo: mm
	*sz = model->buffer_views[1].size;
	*off = model->buffer_views[1].offset;
	*num = model->buffer_views[1].size / 4;
}

static inline bool upload_model(struct cntx *cntx, struct model *model)
{
	SDL_GPUTransferBuffer* transfer_buffer;
	SDL_GPUDevice *device = cntx->device;
	bool ret = false;

	if (!alloc_transfer_buffer(cntx, &transfer_buffer, model->cube->bin_size))
		return false;

	update_transferbuffer(cntx, transfer_buffer, model->cube->bin, model->cube->bin_size);

	SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (!command_buffer)
		goto out;

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
	if (!copy_pass)
		goto out;

	upload_transferbuffer_partial(copy_pass, model->vertex_buffer, transfer_buffer, model->vertices, model->vertices_sz);
	upload_transferbuffer_partial(copy_pass, model->index_buffer, transfer_buffer, model->indices, model->indices_sz);

	SDL_EndGPUCopyPass(copy_pass);

	if(!SDL_SubmitGPUCommandBuffer(command_buffer))
		goto out;

	ret = true;

out:
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
	return ret;
}


static inline void model_free(struct cntx *cntx, struct model *model)
{
	SDL_GPUDevice *device = cntx->device;

	if (model->vertex_buffer)
		SDL_ReleaseGPUBuffer(device, model->vertex_buffer);
	if (model->index_buffer)
		SDL_ReleaseGPUBuffer(device, model->index_buffer);

	cgltf_free(model->cube);
}
#endif
