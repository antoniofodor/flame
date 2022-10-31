#pragma once

#include "../foundation/foundation.h"
#include "../graphics/graphics.h"
#ifdef USE_AUDIO_MODULE
#include "../audio/audio.h"
#endif
#ifdef USE_PHYSICS_MODULE
#include "../physics/physics.h"
#endif

#ifdef FLAME_UNIVERSE_MODULE

#define FLAME_UNIVERSE_API __declspec(dllexport)

#define FLAME_UNIVERSE_TYPE(name) FLAME_TYPE_PRIVATE(name)

#else

#define FLAME_UNIVERSE_API __declspec(dllimport)

#define FLAME_UNIVERSE_TYPE(name) FLAME_TYPE(name)

#endif

namespace flame
{
	FLAME_UNIVERSE_TYPE(Entity);
	FLAME_UNIVERSE_TYPE(Component);
	FLAME_UNIVERSE_TYPE(System);
	FLAME_UNIVERSE_TYPE(World);

	FLAME_UNIVERSE_TYPE(cElement);
	FLAME_UNIVERSE_TYPE(cText);
	FLAME_UNIVERSE_TYPE(cImage);
	FLAME_UNIVERSE_TYPE(cReceiver);
	FLAME_UNIVERSE_TYPE(cNode);
	FLAME_UNIVERSE_TYPE(cMesh);
	FLAME_UNIVERSE_TYPE(cArmature);
	FLAME_UNIVERSE_TYPE(cTerrain);
	FLAME_UNIVERSE_TYPE(cSdf);
	FLAME_UNIVERSE_TYPE(cVolume);
	FLAME_UNIVERSE_TYPE(cParticleSystem);
	FLAME_UNIVERSE_TYPE(cRigid);
	FLAME_UNIVERSE_TYPE(cShape);
	FLAME_UNIVERSE_TYPE(cCController);
	FLAME_UNIVERSE_TYPE(cNavMesh);
	FLAME_UNIVERSE_TYPE(cNavAgent);
	FLAME_UNIVERSE_TYPE(cNavObstacle);
	FLAME_UNIVERSE_TYPE(cNavScene);
	FLAME_UNIVERSE_TYPE(cAudioSource);
	FLAME_UNIVERSE_TYPE(cAudioListener);
	FLAME_UNIVERSE_TYPE(cDirLight);
	FLAME_UNIVERSE_TYPE(cPtLight);
	FLAME_UNIVERSE_TYPE(cCamera);
	FLAME_UNIVERSE_TYPE(cRendererSettings);

	FLAME_UNIVERSE_TYPE(sInput);
	FLAME_UNIVERSE_TYPE(sScene);
	FLAME_UNIVERSE_TYPE(sRenderer);
	FLAME_UNIVERSE_TYPE(sAudio);

	enum StateFlags
	{
		StateNone = 0,
		StateHovering = 1 << 0,
		StateFocusing = 1 << 1,
		StateActive = 1 << 2,
		StateSelected = 1 << 3
	};

	inline StateFlags operator| (StateFlags a, StateFlags b) { return (StateFlags)((int)a | (int)b); }

	enum LightType
	{
		LightDirectional,
		LightPoint,
		LightSpot
	};

	enum RenderType
	{
		RenderWireframe,
		RenderShaded,
		RenderNormalData
	};

	struct OctNode;

	FLAME_UNIVERSE_API void* universe_info();
}
