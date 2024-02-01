#pragma once

#include <functional>

namespace SVF
{
	enum class Feature
	{
		NO_FEATURE,

#if _GTA_III || _GTA_VC
		TAXI_LIGHT, // Corrected light placement for Taxi
#endif

#if _GTA_VC
		FBI_RANCHER_SIREN,
		FBI_WASHINGTON_SIREN,
		VICE_CHEETAH_SIREN,
#endif

#if _GTA_SA
		// Those are fully controlled by SilentPatch
		PHOENIX_FLUTTER,
		SWEEPER_BRUSHES,
		NEWSVAN_DISH,
		EXTRA_AILERONS1, // Like on Beagle
		EXTRA_AILERONS2, // Like on Stuntplane
		DOUBLE_TRAILER, // Like on artict3

		// Those are partially controlled by SilentPatch (only affected by minor fixes)
		VORTEX_EXHAUST,
		TOWTRUCK_HOOK,
		TRACTOR_HOOK,
		RHINO_WHEELS,
		FIRELA_LADDER,

		// Internal SP use only, formerly "rotor exceptions"
		// Unreachable from RegisterSpecialVehicleFeature
		_INTERNAL_NO_ROTOR_FADE,
		_INTERNAL_NO_LIGHTBEAM_BFC_FIX,
		_INTERNAL_FORCE_DOUBLE_RWHEELS_OFF,
		_INTERNAL_FORCE_DOUBLE_RWHEELS_ON,
#endif
	};

	int32_t RegisterFeature( int32_t modelID, Feature feature );
	void DeleteFeature( int32_t cookie );
	void DisableStockVehiclesForFeature( Feature feature );
	bool ModelHasFeature( int32_t modelID, Feature feature );
	std::function<bool(Feature)> ForAllModelFeatures( int32_t modelID, std::function<bool(Feature)> pred );
};