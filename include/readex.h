
#ifndef READEX_H_
#define READEX_H_

#include "scorep/SCOREP_User.h"
#include "scorep/SCOREP_User_Types.h"

#ifdef USE_MERIC
#include <meric.h>
#endif

void READEX_INIT()
{
	#ifdef USE_MERIC
		MERIC_Init();
	#endif
}

void READEX_CLOSE()
{
	#ifdef USE_MERIC
		MERIC_Close();
	#endif
}

//Score-P 3.0
//SCOREP_USER_REGION_DEFINE(handle)
#define READEX_REGION_DEFINE( handle ) \
	static SCOREP_User_RegionHandle handle = SCOREP_USER_INVALID_REGION

/*

#ifdef USE_SCOREP
	#define READEX_REGION_START( handle, name, type ) \
		SCOREP_USER_REGION_BEGIN( handle, name, type )
#elif defined USE_MERIC
	#define READEX_REGION_START( handle, name, type ) \
		MERIC_MeasureStart( name );
#endif

#ifdef USE_SCOREP
	#define READEX_REGION_STOP( handle ) \
		SCOREP_USER_REGION_END( handle )
#elif defined USE_MERIC
	#define READEX_REGION_STOP( handle ) \
		MERIC_MeasureStop();
#endif

*/

void READEX_REGION_START(struct SCOREP_User_Region* handle, const char* name, uint32_t type)
{
	#ifdef USE_SCOREP
		SCOREP_USER_REGION_BEGIN(handle, name, type)
	#elif defined USE_MERIC
		MERIC_MeasureStart(name);
	#endif
}


void READEX_REGION_STOP(struct SCOREP_User_Region* handle)
{
	#ifdef USE_SCOREP
		SCOREP_USER_REGION_END(handle)
	#elif defined USE_MERIC
		MERIC_MeasureStop();
	#endif
}

void READEX_PHASE_START(struct SCOREP_User_Region* handle, const char* name, uint32_t type)
{
	#ifdef USE_SCOREP
		SCOREP_USER_OA_PHASE_BEGIN(handle, name, type)
	#elif defined USE_MERIC
		MERIC_MeasureStart(name);
	#endif
}

void READEX_PHASE_STOP(struct SCOREP_User_Region* handle)
{
	#ifdef USE_SCOREP
		SCOREP_USER_OA_PHASE_END(handle)
	#elif defined USE_MERIC
		MERIC_MeasureStop();
	#endif
}

#endif /* READEX_H_ */
