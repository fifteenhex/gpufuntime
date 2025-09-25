#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_stdinc.h"
#include <stddef.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

#include <cglm/cglm.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "utils.h"

struct Vertex
{
	vec3 pos;
	vec4 rgba;

	float rot;
};

static SDL_GPUVertexAttribute vertexAttributes[] = {
	VERTEX_ATTR_VEC3(0, offsetof(struct Vertex, pos)),
	VERTEX_ATTR_VEC4(1, offsetof(struct Vertex, rgba)),
	VERTEX_ATTR_FLOAT(2, offsetof(struct Vertex, rot)),
};

struct UniformBufferObject
{
	mat4 projection;
	mat4 view;
};

// a list of vertices
static struct Vertex vertices[] =
{
	// top vertex
	{
		{0.0f, 0.5f, 0.0f},
		{1.0f, 0.0f, 0.0f, 1.0f},
	},
	// bottom left vertex
	{
		{-0.75f, -0.5f, 0.0f},
		{1.0f, 1.0f, 0.0f, 1.0f}
	},
	// bottom right vertex
	{
		{0.75f, -0.5f, 0.0f},
		{1.0f, 0.0f, 1.0f, 1.0f},
	}
};

static SDL_Window* window;

static SDL_GPUShader* vertexShader;
static SDL_GPUShader* fragmentShader;

static SDL_GPUGraphicsPipeline* graphicsPipeline;

static struct cntx {
	SDL_GPUDevice *device;

	SDL_GPUBuffer* vertexBuffer;
	SDL_GPUBuffer* indexBuffer;
	SDL_GPUTransferBuffer* transferBuffer;

	cgltf_data* cube;
} _cntx;

static bool create_vertex_buffer(struct cntx *cntx)
{
	SDL_GPUBufferCreateInfo bufferInfo = {
		.size = sizeof(vertices),
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
	};

	cntx->vertexBuffer = SDL_CreateGPUBuffer(cntx->device, &bufferInfo);
	if (!cntx->vertexBuffer)
		return false;

	return true;
}

static bool create_index_buffer(struct cntx *cntx)
{
	SDL_GPUBufferCreateInfo bufferInfo = {
		.size = sizeof(vertices),
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
	};

	cntx->indexBuffer = SDL_CreateGPUBuffer(cntx->device, &bufferInfo);
	if (!cntx->indexBuffer)
		return false;

	return true;
}

static bool create_transfer_buffer(struct cntx *cntx)
{
	SDL_GPUTransferBufferCreateInfo transferInfo = {
		.size = sizeof(vertices),
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	};

	cntx->transferBuffer = SDL_CreateGPUTransferBuffer(cntx->device, &transferInfo);
	if (!cntx->transferBuffer)
		return false;

	return true;
}


int rot = 0;

static void update_and_upload_vertex_buffer(struct cntx *cntx)
{
	SDL_GPUDevice *device = cntx->device;

	rot = (rot + 1) %360;
	vertices[0].rot = glm_rad(rot);
	vertices[1].rot = glm_rad(rot);
	vertices[2].rot = glm_rad(rot);

	struct Vertex* data = (struct Vertex*)SDL_MapGPUTransferBuffer(device, cntx->transferBuffer, false);

	SDL_memcpy(data, vertices, sizeof(vertices));

	SDL_UnmapGPUTransferBuffer(device, cntx->transferBuffer);

	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

	SDL_GPUTransferBufferLocation location = {
		.transfer_buffer = cntx->transferBuffer,
	};

	SDL_GPUBufferRegion region = {
		.buffer = cntx->vertexBuffer,
		.size = sizeof(vertices),
	};

	SDL_UploadToGPUBuffer(copyPass, &location, &region, true);

	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(commandBuffer);
}



EMBEDDED_BIN(models_cube_glb);
static const void *cube_glb = &_binary_models_cube_glb_start;
static const void *cube_glb_end = &_binary_models_cube_glb_end;


static bool load_model_cube(struct cntx *cntx)
{
	size_t sz = cube_glb_end - cube_glb;
	cgltf_options options = {0};


	cgltf_result result = cgltf_parse(&options, cube_glb, sz, &cntx->cube);
	if (result != cgltf_result_success)
		return false;

	return true;
}

EMBEDDED_BIN(vertex_spv);
static const void *vertex_spv = &_binary_vertex_spv_start;
static const void *vertex_spv_end = &_binary_vertex_spv_end;

static bool load_vertex_shader(struct cntx *cntx)
{
	SDL_GPUDevice *device = cntx->device;
	size_t sz = vertex_spv_end - vertex_spv;

	SDL_GPUShaderCreateInfo vertexInfo = {
		.code = vertex_spv,
		.code_size = sz,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_samplers = 0,
		.num_storage_buffers = 0,
		.num_storage_textures = 0,
		.num_uniform_buffers = 1,
	};

	vertexShader = SDL_CreateGPUShader(device, &vertexInfo);
	if (!vertexShader)
		return false;

	return true;
}

EMBEDDED_BIN(fragment_spv);
static const void *fragment_spv = &_binary_fragment_spv_start;
static const void *fragment_spv_end = &_binary_fragment_spv_end;

static bool load_fragment_shader(struct cntx *cntx)
{
	SDL_GPUDevice *device = cntx->device;
	size_t sz = fragment_spv_end - fragment_spv;

	SDL_GPUShaderCreateInfo fragmentInfo = {
		.code = fragment_spv,
		.code_size = sz,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
		.num_samplers = 0,
		.num_storage_buffers = 0,
		.num_storage_textures = 0,
		.num_uniform_buffers = 0,
	};

	fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);
	if (!fragmentShader)
		return false;

	return true;
}

static bool create_pipeline(struct cntx *cntx)
{
	SDL_GPUDevice *device = cntx->device;

	// describe the vertex buffers
	SDL_GPUVertexBufferDescription vertexBufferDesctiptions[] =
	{
		{
			.slot = 0,
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			.instance_step_rate = 0,
			.pitch = sizeof(struct Vertex),
		}
	};

	// describe the color target
	SDL_GPUColorTargetDescription colorTargetDescriptions[] = {
		{
			.blend_state.enable_blend = true,
			.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.format = SDL_GetGPUSwapchainTextureFormat(device, window),
		},
	};

	// create the graphics pipeline
	SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,

		.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertexBufferDesctiptions),
		.vertex_input_state.vertex_buffer_descriptions = vertexBufferDesctiptions,

		.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertexAttributes),
		.vertex_input_state.vertex_attributes = vertexAttributes,

		.target_info.num_color_targets = SDL_arraysize(colorTargetDescriptions),
		.target_info.color_target_descriptions = colorTargetDescriptions,
	};

	// create the pipeline
	graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

	return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
	struct cntx *cntx = &_cntx;

	memset(cntx, 0, sizeof(*cntx));

	window = SDL_CreateWindow("Can you see? I'm triangle", 480, 480, SDL_WINDOW_RESIZABLE);
	if (!window)
		return SDL_APP_FAILURE;

	cntx->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	if (!cntx->device)
		return SDL_APP_FAILURE;

	if (!SDL_ClaimWindowForGPUDevice(cntx->device, window))
		return SDL_APP_FAILURE;

	if (!create_vertex_buffer(cntx))
		return SDL_APP_FAILURE;

	if (!create_index_buffer(cntx))
		return SDL_APP_FAILURE;

	if (!create_transfer_buffer(cntx))
		return SDL_APP_FAILURE;

	if (!load_vertex_shader(cntx))
		return SDL_APP_FAILURE;

	if (!load_fragment_shader(cntx))
		return SDL_APP_FAILURE;

	if (!create_pipeline(cntx))
		return SDL_APP_FAILURE;

	if (!load_model_cube(cntx))
		return SDL_APP_FAILURE;

	return SDL_APP_CONTINUE;
}

static void do_uniform_data(struct UniformBufferObject *ubo)
{
	//glm_mat4_mul(scale, rot, ubo.model);

	glm_ortho_default(480.0f/480.0f, ubo->projection);
	//glm_perspective_default(, ubo.perspective);
	//glm_look(CamPos, (vec3) { 0, 0, 0 },  (vec3){ 0, 1, 0 }, ubo.view);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	struct cntx *cntx = &_cntx;
	SDL_GPUDevice *device = cntx->device;

	update_and_upload_vertex_buffer(cntx);

	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUTexture* swapchainTexture;
	Uint32 width, height;

	SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height);

	if (swapchainTexture == NULL)
	{
		SDL_SubmitGPUCommandBuffer(commandBuffer);
		return SDL_APP_CONTINUE;
	}

	SDL_GPUColorTargetInfo colorTargetInfo = {
		.clear_color =
		{
			.r = 240/255.0f,
			.g = 240/255.0f,
			.b = 240/255.0f,
			.a = 255/255.0f
		},
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.texture = swapchainTexture,
	};

	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
	SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);

	SDL_GPUBufferBinding bufferBindings[] = {
		{
			.buffer = cntx->vertexBuffer,
			.offset = 0,
		},
	};
	SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, SDL_arraysize(bufferBindings));

	struct UniformBufferObject ubo = {};
	do_uniform_data(&ubo);
	SDL_PushGPUVertexUniformData(commandBuffer, 0, &ubo, sizeof(ubo));

	// issue a draw call
	SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

	SDL_EndGPURenderPass(renderPass);

	SDL_SubmitGPUCommandBuffer(commandBuffer);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	switch(event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	struct cntx *cntx = &_cntx;
	SDL_GPUDevice *device = cntx->device;

	SDL_ReleaseGPUTransferBuffer(device, cntx->transferBuffer);

	cgltf_free(cntx->cube);

	SDL_ReleaseGPUBuffer(device, cntx->vertexBuffer);
	SDL_ReleaseGPUBuffer(device, cntx->indexBuffer);
	SDL_ReleaseGPUShader(device, vertexShader);
	SDL_ReleaseGPUShader(device, fragmentShader);
	SDL_DestroyGPUDevice(device);
	SDL_DestroyWindow(window);
}
