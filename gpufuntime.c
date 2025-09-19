#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_stdinc.h"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

#include <cglm/cglm.h>

struct Vertex
{
	vec3 pos;
	vec4 rgba;
} __attribute((packed));

struct UniformBufferObject
{
	mat4 model;
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
static SDL_GPUDevice *device;

static SDL_GPUBuffer* vertexBuffer;
static SDL_GPUTransferBuffer* transferBuffer;

static SDL_GPUShader* vertexShader;
static SDL_GPUShader* fragmentShader;

static SDL_GPUGraphicsPipeline* graphicsPipeline;

static bool create_vertex_buffer(void)
{
	// create the vertex buffer
	SDL_GPUBufferCreateInfo bufferInfo = {};
	bufferInfo.size = sizeof(vertices);
	bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	vertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

	return true;
}

static bool create_transfer_buffer(void)
{
	// create a transfer buffer to upload to the vertex buffer
	SDL_GPUTransferBufferCreateInfo transferInfo = {};
	transferInfo.size = sizeof(vertices);
	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

	return true;
}

static void update_transfer_buffer(void)
{
	struct Vertex* data = (struct Vertex*)SDL_MapGPUTransferBuffer(device, transferBuffer, false);

	SDL_memcpy(data, vertices, sizeof(vertices));

	SDL_UnmapGPUTransferBuffer(device, transferBuffer);
}

static void upload_vertex_buffer(void)
{
	// start a copy pass
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

	// where is the data
	SDL_GPUTransferBufferLocation location = {};
	location.transfer_buffer = transferBuffer;
	location.offset = 0; // start from the beginning

	// where to upload the data
	SDL_GPUBufferRegion region = {
		.buffer = vertexBuffer,
		.size = sizeof(vertices),
		.offset = 0,
	};

	// upload the data
	SDL_UploadToGPUBuffer(copyPass, &location, &region, true);

	// end the copy pass
	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(commandBuffer);
}

static bool load_vertex_shader(void)
{
	size_t vertexCodeSize;

	void* vertexCode = SDL_LoadFile("vertex.spv", &vertexCodeSize);
	if (!vertexCode)
		return false;

	// create the vertex shader
	SDL_GPUShaderCreateInfo vertexInfo = {
		.code = (Uint8*)vertexCode,
		.code_size = vertexCodeSize,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_samplers = 0,
		.num_storage_buffers = 0,
		.num_storage_textures = 0,
		.num_uniform_buffers = 1,
	};
	vertexShader = SDL_CreateGPUShader(device, &vertexInfo);

	SDL_free(vertexCode);

	return true;
}

static bool load_fragment_shader(void)
{
	// create the fragment shader
	size_t fragmentCodeSize;

	void* fragmentCode = SDL_LoadFile("fragment.spv", &fragmentCodeSize);
	if (!fragmentCode)
		return false;

	// create the fragment shader
	SDL_GPUShaderCreateInfo fragmentInfo = {
		.code = (Uint8*)fragmentCode,
		.code_size = fragmentCodeSize,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
		.num_samplers = 0,
		.num_storage_buffers = 0,
		.num_storage_textures = 0,
		.num_uniform_buffers = 0,
	};

	fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);

	SDL_free(fragmentCode);
	return true;
}

static bool create_pipeline(void)
{
	// create the graphics pipeline
	SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.vertex_shader = vertexShader;
	pipelineInfo.fragment_shader = fragmentShader;
	pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

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

	pipelineInfo.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertexBufferDesctiptions);
	pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDesctiptions;

	// describe the vertex attribute
	SDL_GPUVertexAttribute vertexAttributes[] = {
		// a_position
		{
			.buffer_slot = 0,
			.location = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = 0,
		},
		// a_color
		{
			.buffer_slot = 0,
			.location = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = sizeof(float) * 3,
		},
	};
	pipelineInfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertexAttributes);
	pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

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


	pipelineInfo.target_info.num_color_targets = SDL_arraysize(colorTargetDescriptions);
	pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

	// create the pipeline
	graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

	return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
	window = SDL_CreateWindow("Hello, Triangle!", 480, 480, SDL_WINDOW_RESIZABLE);
	if (!window)
		return SDL_APP_FAILURE;

	device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	if (!device)
		return SDL_APP_FAILURE;

	if (!SDL_ClaimWindowForGPUDevice(device, window))
		return SDL_APP_FAILURE;

	if (!create_vertex_buffer())
		return SDL_APP_FAILURE;

	if (!create_transfer_buffer())
		return SDL_APP_FAILURE;

	update_transfer_buffer();
	upload_vertex_buffer();

	if (!load_vertex_shader())
		return SDL_APP_FAILURE;

	if (!load_fragment_shader())
		return SDL_APP_FAILURE;

	if (!create_pipeline())
		return SDL_APP_FAILURE;

	return SDL_APP_CONTINUE;
}


int rot = 0;
SDL_AppResult SDL_AppIterate(void *appstate)
{
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
			.buffer = vertexBuffer,
			.offset = 0,
		},
	};
	SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, SDL_arraysize(bufferBindings));

	struct UniformBufferObject ubo = {};
	//glm_scale_make(ubo.model, (vec3){1.00f, 1.00f, 1.00f});
	//glm_rotate_x(tmp, 45, ubo.model);
	rot = (rot + 1) %360;
	glm_rotate_atm(ubo.model, (vec3) {0.0f,0.0f,0.0f}, glm_rad(rot), (vec3) {1.0f,0.0f,0.0f});

	//glm_mat4_mul(scale, rot, ubo.model);

	glm_ortho_default(480.0f/480.0f, ubo.projection);
	//glm_perspective_default(, ubo.perspective);
	//glm_look(CamPos, (vec3) { 0, 0, 0 },  (vec3){ 0, 1, 0 }, ubo.view);

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
	SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
	SDL_ReleaseGPUBuffer(device, vertexBuffer);
	SDL_ReleaseGPUShader(device, vertexShader);
	SDL_ReleaseGPUShader(device, fragmentShader);
	SDL_DestroyGPUDevice(device);
	SDL_DestroyWindow(window);
}
