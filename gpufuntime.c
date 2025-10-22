#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_timer.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>

#include <cglm/cglm.h>

#include "buffers.h"
#include "cntx.h"
#include "model.h"
#include "utils.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

struct Vertex
{
	vec3 pos;
};

static SDL_GPUVertexAttribute vertexAttributes[] = {
	VERTEX_ATTR_VEC3(0, 0),
};

struct UniformBufferObject
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec3 colour;
};

static SDL_GPUGraphicsPipeline* graphicsPipeline;

static bool load_model(struct cntx *cntx,
					   struct model **_model,
					   const void *start,
					   const void *end)
{
	struct model *model = malloc(sizeof(*model));
	size_t sz = end - start;
	cgltf_options options = {0};

	if (!model)
		return false;

	cgltf_result result = cgltf_parse(&options, start, sz, &model->cube);
	if (result != cgltf_result_success)
		return false;

	model_get_vertices(model->cube, &model->vertices, &model->vertices_sz);
	if (!alloc_vertex_buffer(cntx, &model->vertex_buffer, model->vertices_sz))
		return false;

	model_get_indices(model->cube, &model->indices, &model->indices_sz, &model->indices_num);
	if (!alloc_index_buffer(cntx, &model->index_buffer, model->indices_sz))
		return false;

	*_model = model;
	return true;
}

EMBEDDED_BIN(models_cube_glb);
static const void *cube_glb = &_binary_models_cube_glb_start;
static const void *cube_glb_end = &_binary_models_cube_glb_end;

static bool load_model_cube(struct cntx *cntx)
{
	return load_model(cntx, &cntx->cube, cube_glb, cube_glb_end);
}

EMBEDDED_BIN(models_donut_glb);
static const void *donut_glb = &_binary_models_donut_glb_start;
static const void *donut_glb_end = &_binary_models_donut_glb_end;

static bool load_model_donut(struct cntx *cntx)
{
	return load_model(cntx, &cntx->donut, donut_glb, donut_glb_end);
}

int rot = 0;

EMBEDDED_BIN(vertex_spv);
static const void *vertex_spv = &_binary_vertex_spv_start;
static const void *vertex_spv_end = &_binary_vertex_spv_end;

static bool load_vertex_shader(struct cntx *cntx)
{
	SDL_GPUDevice *device = cntx->device;
	size_t sz = vertex_spv_end - vertex_spv;

	const SDL_GPUShaderCreateInfo vertexInfo = {
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

	cntx->vertexShader = SDL_CreateGPUShader(device, &vertexInfo);
	if (!cntx->vertexShader)
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

	const SDL_GPUShaderCreateInfo fragmentInfo = {
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

	cntx->fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);
	if (!cntx->fragmentShader)
		return false;

	return true;
}

void setup_depth_buffer(struct cntx *cntx)
{
	const SDL_GPUTextureCreateInfo textureinfo = {
		.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width = 480,
		.height = 480,
		.layer_count_or_depth = 1,
		.num_levels = 1
	};

	cntx->depth_texture = SDL_CreateGPUTexture(cntx->device, &textureinfo);
}

static bool create_pipeline(struct cntx *cntx)
{
	SDL_GPUDevice *device = cntx->device;
	SDL_Window *window = cntx->window;

	// describe the vertex buffers
	const SDL_GPUVertexBufferDescription vertexBufferDesctiptions[] = {
		{
			.slot = 0,
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
			.instance_step_rate = 0,
			.pitch = sizeof(struct Vertex),
		}
	};

	// describe the color target
	const SDL_GPUColorTargetDescription colorTargetDescriptions[] = {
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
	const SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
		.vertex_shader = cntx->vertexShader,
		.fragment_shader = cntx->fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,

		.vertex_input_state = {
			.num_vertex_buffers = SDL_arraysize(vertexBufferDesctiptions),
			.vertex_buffer_descriptions = vertexBufferDesctiptions,

			.num_vertex_attributes = SDL_arraysize(vertexAttributes),
			.vertex_attributes = vertexAttributes,
		},

		.target_info.num_color_targets = SDL_arraysize(colorTargetDescriptions),
		.target_info.color_target_descriptions = colorTargetDescriptions,

		.target_info.has_depth_stencil_target = true,
		.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM,

		//.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE,

		.depth_stencil_state = {
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS,
		},
	};

	// create the pipeline
	graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
	if (!graphicsPipeline)
		return false;

	return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
	struct cntx *cntx = &_cntx;

	memset(cntx, 0, sizeof(*cntx));

	cntx->window = SDL_CreateWindow("Can you see? I'm cube", 480, 480, SDL_WINDOW_RESIZABLE);
	if (!cntx->window)
		return SDL_APP_FAILURE;

	cntx->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
	if (!cntx->device)
		return SDL_APP_FAILURE;

	if (!SDL_ClaimWindowForGPUDevice(cntx->device, cntx->window))
		return SDL_APP_FAILURE;

	if (!load_vertex_shader(cntx))
		return SDL_APP_FAILURE;

	if (!load_fragment_shader(cntx))
		return SDL_APP_FAILURE;

	if (!create_pipeline(cntx))
		return SDL_APP_FAILURE;

	if (!load_model_cube(cntx))
		return SDL_APP_FAILURE;

	if (!load_model_donut(cntx))
		return SDL_APP_FAILURE;

	if (!upload_model(cntx, cntx->cube))
		return SDL_APP_FAILURE;

	if (!upload_model(cntx, cntx->donut))
		return SDL_APP_FAILURE;

	setup_depth_buffer(cntx);

	return SDL_APP_CONTINUE;
}

float eye_y = 0.0f;
float eye_x = 0.0f;
float eye_z = -5.0f;

static void do_uniform_data(struct UniformBufferObject *ubo,
							float x, float y, float z,
							float r, float g, float b)
{
	//glm_mat4_mul(scale, rot, ubo.model);

	//glm_ortho_default(480.0f/480.0f, ubo->projection);
	glm_perspective_default(480.0f/480.0f, ubo->projection);
	glm_look_anyup((vec3){0, 0, eye_z}, (vec3){0.0f,0.0f,0.1f}, ubo->view);
	{
		vec3 axis = {1.0f, 0.0f, 0.0f};
		glm_rotate(ubo->view, eye_y, axis);
	}
	{
		vec3 axis = {0.0f, 1.0f, 0.0f};
		glm_rotate(ubo->view, eye_x, axis);
	}
	rot = (rot + 1); //% (360 * 100);

	unsigned long ticks = SDL_GetTicks();

	glm_mat4_identity(ubo->model);
	glm_translate_x(ubo->model, x * sin(glm_rad(ticks / 10)));
	//glm_translate_y(ubo->model, y * cos(glm_rad(ticks/ 10)));
	glm_translate_z(ubo->model, z * cos(glm_rad(ticks/ 10)));
	{
		vec3 axis = {0.0f, 1.0f, 1.0f};
		glm_rotate(ubo->model, glm_rad(ticks / 20.0f), axis);
	}
	//vec3 scale = {.8f, .8f, .8f};
	//glm_scale(ubo->model, scale);

	vec3 colour = {r, g, b};
	memcpy(&ubo->colour, colour, sizeof(ubo->colour));
}

void draw_cube(struct cntx *cntx, SDL_GPURenderPass *renderpass)
{
	const SDL_GPUBufferBinding buffer_bindings0[] = {
		{
			.buffer = cntx->cube->vertex_buffer,
			.offset = 0,
		},
	};
		const SDL_GPUBufferBinding buffer_bindings1[] = {
		{
			.buffer = cntx->cube->index_buffer,
			.offset = 0,
		},
	};
	SDL_BindGPUVertexBuffers(renderpass, 0, buffer_bindings0, SDL_arraysize(buffer_bindings0));
	SDL_BindGPUIndexBuffer(renderpass, buffer_bindings1, SDL_GPU_INDEXELEMENTSIZE_32BIT);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	struct cntx *cntx = &_cntx;
	SDL_GPUDevice *device = cntx->device;
	SDL_Window *window = cntx->window;

	SDL_GPUTexture* swapchainTexture;
	Uint32 width, height;

	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
	SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &width, &height);

	if (swapchainTexture == NULL)
	{
		SDL_SubmitGPUCommandBuffer(commandBuffer);
		return SDL_APP_CONTINUE;
	}

	const SDL_GPUColorTargetInfo colorTargetInfo = {
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

	const SDL_GPUDepthStencilTargetInfo depth_target = {
		.texture = cntx->depth_texture,
		.clear_depth = 1.0f,
		.load_op = SDL_GPU_LOADOP_CLEAR,
	};

	SDL_GPURenderPass* renderpass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depth_target);
	SDL_BindGPUGraphicsPipeline(renderpass, graphicsPipeline);

	struct UniformBufferObject ubo = {};

	{
		draw_cube(cntx, renderpass);
		do_uniform_data(&ubo, -.75, -.75f, -3.0f, 1.0f, 0, 0);
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &ubo, sizeof(ubo));
		SDL_DrawGPUIndexedPrimitives(renderpass, cntx->cube->indices_num, 1, 0, 0, 0);
	}

	{
		const SDL_GPUBufferBinding buffer_bindings0[] = {
			{
				.buffer = cntx->donut->vertex_buffer,
				.offset = 0,
			},
		};
		const SDL_GPUBufferBinding buffer_bindings1[] = {
			{
				.buffer = cntx->donut->index_buffer,
				.offset = 0,
			},
		};
		SDL_BindGPUVertexBuffers(renderpass, 0, buffer_bindings0, SDL_arraysize(buffer_bindings0));
		SDL_BindGPUIndexBuffer(renderpass, buffer_bindings1, SDL_GPU_INDEXELEMENTSIZE_32BIT);
		do_uniform_data(&ubo, 0.0, 0.0f, 0.0f, 0, 1.0f, 0);
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &ubo, sizeof(ubo));
		SDL_DrawGPUIndexedPrimitives(renderpass, cntx->donut->indices_num, 1, 0, 0, 0);
	}

	{
		draw_cube(cntx, renderpass);
		do_uniform_data(&ubo, .75f, .75f, 3.0f, 0, 0, 1.0f);
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &ubo, sizeof(ubo));
		SDL_DrawGPUIndexedPrimitives(renderpass, cntx->cube->indices_num, 1, 0, 0, 0);
	}

	SDL_EndGPURenderPass(renderpass);

	SDL_SubmitGPUCommandBuffer(commandBuffer);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	switch(event->type) {
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;
		case SDL_EVENT_KEY_DOWN:
			switch(event->key.scancode) {
				case SDL_SCANCODE_DOWN:
					eye_y += 0.05f;
					break;
				case SDL_SCANCODE_UP:
					eye_y += -0.05f;
					break;
				case SDL_SCANCODE_LEFT:
					eye_x += 0.05f;
					break;
				case SDL_SCANCODE_RIGHT:
					eye_x += -0.05f;
					break;
				case SDL_SCANCODE_Z:
					eye_z += -0.2f;
					break;
				case SDL_SCANCODE_X:
					eye_z += 0.2f;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	struct cntx *cntx = &_cntx;
	SDL_GPUDevice *device = cntx->device;
	SDL_Window *window = cntx->window;

	model_free(cntx, cntx->cube);
	model_free(cntx, cntx->donut);

	SDL_ReleaseGPUBuffer(device, cntx->vertexBuffer);
	SDL_ReleaseGPUShader(device, cntx->vertexShader);
	SDL_ReleaseGPUShader(device, cntx->fragmentShader);
	SDL_DestroyGPUDevice(device);
	SDL_DestroyWindow(window);
}
