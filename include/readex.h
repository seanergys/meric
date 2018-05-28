/**
 * Shared intarface for MERIC and Score-P libraries
 *
 * version 3.4
 * 19.02.2018
 * Ondrej Vysocky, ondrej.vysocky@vsb.cz
 * IT4Innovations national supercomputing center, Czech Republic
 **/

#ifndef READEX_H_
#define READEX_H_

#ifdef USE_SCOREP_MANUAL
	#define USE_SCOREP
#endif

#ifdef USE_SCOREP
#include <scorep/SCOREP_User.h>
#include <scorep/SCOREP_User_Types.h>
#endif

#ifdef USE_MERIC
	#include <meric.h>
#elif defined USE_SCOREP && defined __cplusplus
	#include <cstddef>
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

//READEX_IGNORE_START()
//READEX_IGNORE_STOP()
#ifdef USE_MERIC
	#define READEX_IGNORE_START() \
		MERIC_IgnoreStart();
	#define READEX_IGNORE_STOP() \
		MERIC_IgnoreStop();
#elif defined USE_SCOREP
	#define READEX_IGNORE_START() \
		SCOREP_RECORDING_OFF();
	#define READEX_IGNORE_STOP() \
		SCOREP_RECORDING_ON();
#else
	#define READEX_IGNORE_START()
	#define READEX_IGNORE_STOP()
#endif

//READEX_REGION_DEFINE(handle) - can be used for PHASE and SIGNIFICANT regions too
//READEX_PHASE_REGION_DEFINE(handle) - should be used for PHASE only
//READEX_SIGNIFICANT_REGION_DEFINE(handle) - cannot be used for PHASE region
#ifdef USE_SCOREP_MANUAL
	//Score-P 3.0
	//SCOREP_USER_REGION_DEFINE(handle)
	#define READEX_REGION_DEFINE( handle ) \
		static SCOREP_User_RegionHandle handle = SCOREP_USER_INVALID_REGION;
	#define READEX_PHASE_REGION_DEFINE( handle ) \
		static SCOREP_User_RegionHandle handle = SCOREP_USER_INVALID_REGION;
	#define READEX_SIGNIFICANT_REGION_DEFINE( handle ) \
		static SCOREP_User_RegionHandle handle = SCOREP_USER_INVALID_REGION;
#elif defined USE_SCOREP
	//Score-P 3.0
	//SCOREP_USER_REGION_DEFINE(handle)
	#define READEX_REGION_DEFINE( handle ) \
		static SCOREP_User_RegionHandle handle = SCOREP_USER_INVALID_REGION;
	#define READEX_PHASE_REGION_DEFINE( handle ) \
		static SCOREP_User_RegionHandle handle = SCOREP_USER_INVALID_REGION;
	#define READEX_SIGNIFICANT_REGION_DEFINE( handle )
#else
	#define READEX_REGION_DEFINE( handle )
	#define READEX_PHASE_REGION_DEFINE( handle )
	#define READEX_SIGNIFICANT_REGION_DEFINE( handle )
#endif

//READEX_REGION_START(handle, name, type) 
#ifdef USE_SCOREP_MANUAL
	#define READEX_REGION_START( handle, name, type ) \
		SCOREP_USER_REGION_BEGIN( handle, name, type );
#elif defined USE_MERIC
	#define READEX_REGION_START( handle, name, type ) \
		MERIC_MeasureStart( name );
#else
	#define READEX_REGION_START( handle, name, type )
#endif

//READEX_REGION_STOP(handle)
#ifdef USE_SCOREP_MANUAL
	#define READEX_REGION_STOP( handle ) \
		SCOREP_USER_REGION_END( handle );
#elif defined USE_MERIC
	#define READEX_REGION_STOP( handle ) \
		MERIC_MeasureStop();
#else
	#define READEX_REGION_STOP( handle )
#endif

//READEX_REGION_STOP_START(stop_handle, start_handle, start_name, start_type)
#ifdef USE_SCOREP_MANUAL
	#define READEX_REGION_STOP_START( stop_handle, start_handle, start_name, start_type ) \
		SCOREP_USER_REGION_END( stop_handle ); \
		SCOREP_USER_REGION_BEGIN( start_handle, start_name, start_type );
#elif defined USE_MERIC
	#define READEX_REGION_STOP_START( stop_handle, start_handle, start_name, start_type ) \
		MERIC_MeasureStopStart( start_name );
#else
	#define READEX_REGION_STOP_START( stop_handle, start_handle, start_name, start_type )
#endif



//READEX_PHASE_START(handle, name, type)
#ifdef USE_SCOREP
	#define READEX_PHASE_START( handle, name, type ) \
		SCOREP_USER_OA_PHASE_BEGIN( handle, name, type );
#elif defined USE_MERIC
	#define READEX_PHASE_START( handle, name, type ) \
		MERIC_MeasureStart( name );
#else
	#define READEX_PHASE_START( handle, name, type )
#endif

//READEX_PHASE_STOP(handle)
#ifdef USE_SCOREP
	#define READEX_PHASE_STOP( handle ) \
		SCOREP_USER_OA_PHASE_END( handle );
#elif defined USE_MERIC
	#define READEX_PHASE_STOP( handle ) \
		MERIC_MeasureStop();
#else
	#define READEX_PHASE_STOP( handle )
#endif

//List of Score-P API functions (SCOREP_User.h) that will be ignored if the code is not compiled with Score-P
#ifndef USE_SCOREP
	#define SCOREP_USER_REGION( name, type )
	//#define SCOREP_USER_REGION_DEFINE( handle )
	#define SCOREP_USER_REWIND_DEFINE( handle )
	//#define SCOREP_USER_OA_PHASE_BEGIN( handle, name, type )
	//#define SCOREP_USER_OA_PHASE_END( handle )
	#define SCOREP_USER_REWIND_POINT( handle, name )
	//#define SCOREP_USER_REGION_BEGIN( handle, name, type )
	#define SCOREP_USER_REGION_BY_NAME_BEGIN( name, type )
	#define SCOREP_USER_REGION_INIT( handle, name, type )
	//#define SCOREP_USER_REGION_END( handle )
	#define SCOREP_USER_REGION_BY_NAME_END( name )
	#define SCOREP_USER_REWIND_CHECK( handle, value )
	#define SCOREP_USER_REGION_ENTER( handle )
	#define SCOREP_USER_FUNC_BEGIN()
	#define SCOREP_USER_FUNC_END()
	#define SCOREP_USER_GLOBAL_REGION_DEFINE( handle )
	#define SCOREP_USER_GLOBAL_REGION_EXTERNAL( handle )
	#define SCOREP_USER_PARAMETER_INT64( name, value )
	#define SCOREP_USER_PARAMETER_UINT64( name, value )
	#define SCOREP_USER_PARAMETER_STRING( name, value )
	#define SCOREP_USER_METRIC_GLOBAL( metricHandle )
	#define SCOREP_USER_METRIC_EXTERNAL( metricHandle )
	#define SCOREP_USER_METRIC_LOCAL( metricHandle )
	#define SCOREP_USER_METRIC_INIT( metricHandle, name, unit, type, context )
	#define SCOREP_USER_METRIC_INT64( metricHandle, value )
	#define SCOREP_USER_METRIC_UINT64( metricHandle, value )
	#define SCOREP_USER_METRIC_DOUBLE( metricHandle, value )
	//#define SCOREP_RECORDING_ON()
	//#define SCOREP_RECORDING_OFF()
	#define SCOREP_RECORDING_IS_ON() 0
#endif

#endif /* READEX_H_ */
