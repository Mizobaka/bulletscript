#ifndef __BS_CONFIG_H__
#define __BS_CONFIG_H__

#define BS_NMSP							BS

#define BS_USE_BOOST

// value type
namespace BS_NMSP
{
	typedef float bstype;
	const bstype bsepsilon = 0.001f;
}

#define TYPE_TO_UINT32(x)				(*((BS_NMSP::uint32*) ((void*) (&x))))
#define UINT32_TO_TYPE(x)				(*((bstype*) ((void*) (&x))))

// runtime behaviour
#define BS_MAX_AFFECTOR_ARGS			8

#define BS_MAX_AFFECTORS_PER_GUN		4

#define BS_SCRIPT_STACK_SIZE			32

#define BS_SCRIPT_LOOP_DEPTH			4

#define BS_MAX_PROPERTIES				8

#endif
