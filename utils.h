#define EMBEDDED_BIN(_name)			\
extern char _binary_##_name##_start;		\
extern char _binary_##_name##_end;

#define VERTEX_ATTR_FLOAT(_loc, _off)			\
{							\
	.buffer_slot = 0,				\
	.location = _loc,				\
	.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,	\
	.offset = _off,					\
}

#define VERTEX_ATTR_VEC3(_loc, _off)			\
{							\
	.buffer_slot = 0,				\
	.location = _loc,				\
	.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,	\
	.offset = _off,					\
}

#define VERTEX_ATTR_VEC4(_loc, _off)			\
{							\
	.buffer_slot = 0,				\
	.location = _loc,				\
	.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,	\
	.offset = _off,					\
}
