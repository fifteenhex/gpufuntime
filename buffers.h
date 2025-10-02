#ifndef BUFFERS_H_
#define BUFFERS_H_

#include "SDL3/SDL_gpu.h"

#include "cntx.h"
#include <sys/types.h>

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

static inline bool alloc_index_buffer(struct cntx *cntx, SDL_GPUBuffer **buf, size_t sz)
{
	SDL_GPUDevice *device = cntx->device;
	const SDL_GPUBufferCreateInfo ci = {
		.size = sz,
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
	};

	SDL_GPUBuffer *b = SDL_CreateGPUBuffer(device, &ci);
	if (!b)
		return false;

	*buf = b;

	return true;
}

static inline bool alloc_transfer_buffer(struct cntx *cntx,
                                  SDL_GPUTransferBuffer **transfer_buffer,
                                  size_t sz)
{
	SDL_GPUDevice *device = cntx->device;

	const SDL_GPUTransferBufferCreateInfo transferInfo = {
		.size = sz,
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	};

	*transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
	if (!*transfer_buffer)
		return false;

	return true;
}

static inline void update_transferbuffer(struct cntx *cntx,
				  SDL_GPUTransferBuffer *txbuf,
				  const void *data, size_t sz)
{
	SDL_GPUDevice *device = cntx->device;
	void *mapped;

	mapped = SDL_MapGPUTransferBuffer(device, txbuf, false);
	if (!mapped)
		return;

	SDL_memcpy(mapped, data, sz);
	SDL_UnmapGPUTransferBuffer(device, txbuf);
}

/* Upload part of a transfer buffer into a gpu buffer */
static inline void upload_transferbuffer_partial(SDL_GPUCopyPass* copypass,
				  SDL_GPUBuffer *dstbuf,
				  SDL_GPUTransferBuffer *txbuf,
				  off_t whence,
				  size_t sz)
{
	const SDL_GPUTransferBufferLocation location = {
		.transfer_buffer = txbuf,
		.offset = whence,
	};

	const SDL_GPUBufferRegion region = {
		.buffer = dstbuf,
		.size = sz,
	};

	SDL_UploadToGPUBuffer(copypass, &location, &region, true);
}

/* Upload all of a transfer buffer into a gpu buffer */
static inline void upload_transferbuffer(SDL_GPUCopyPass* copypass,
				  SDL_GPUBuffer *dstbuf,
				  SDL_GPUTransferBuffer *txbuf,
				  size_t sz)
{
	upload_transferbuffer_partial(copypass, dstbuf, txbuf, 0, sz);
}
#endif
