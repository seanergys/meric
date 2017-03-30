
#ifndef READEX_H_
#define READEX_H_

#include "scorep/SCOREP_User.h"
#include "scorep/SCOREP_User_Types.h"

#ifdef USE_MERIC
#include <meric.h>
#endif


//READEX_INIT()
//READEX_CLOSE()
#ifdef USE_MERIC
	#define READEX_INIT() \
		MERIC_Init();

	#define READEX_CLOSE() \
		MERIC_Close();
#else
	#define READEX_INIT()
	#define READEX_CLOSE()
#endif

//READEX_REGION_DEFINE(handle)
#ifdef USE_SCOREP
	//Score-P 3.0
	//SCOREP_USER_REGION_DEFINE(handle)
	#define READEX_REGION_DEFINE( handle ) \
		static SCOREP_User_RegionHandle handle = SCOREP_USER_INVALID_REGION
#else
	#define READEX_REGION_DEFINE( handle )
#endif

//READEX_REGION_START(handle, name, type) 
#ifdef USE_SCOREP
	#define READEX_REGION_START( handle, name, type ) \
		SCOREP_USER_REGION_BEGIN( handle, name, type )
#elif defined USE_MERIC
	#define READEX_REGION_START( handle, name, type ) \
		MERIC_MeasureStart( name );
#else
	#define READEX_REGION_START( handle, name, type )
#endif

//READEX_REGION_STOP(handle)
#ifdef USE_SCOREP
	#define READEX_REGION_STOP( handle ) \
		SCOREP_USER_REGION_END( handle )
#elif defined USE_MERIC
	#define READEX_REGION_STOP( handle ) \
		MERIC_MeasureStop();
#else
	#define READEX_REGION_STOP( handle )
#endif

//READEX_PHASE_START(handle, name, type)
#ifdef USE_SCOREP
	#define READEX_PHASE_START( handle, name, type ) \
		SCOREP_USER_OA_PHASE_BEGIN( handle, name, type )
#elif defined USE_MERIC
	#define READEX_PHASE_START( handle, name, type ) \
		MERIC_MeasureStart( name );
	#else
		#define READEX_PHASE_START( handle, name, type )
#endif

//READEX_PHASE_STOP(handle)
#ifdef USE_SCOREP
	#define READEX_PHASE_STOP( handle ) \
		SCOREP_USER_OA_PHASE_END( handle )
#elif defined USE_MERIC
	#define READEX_PHASE_STOP( handle ) \
		MERIC_MeasureStop();
#else
	#define READEX_PHASE_STOP( handle )
#endif


#endif /* READEX_H_ */
