#ifndef READEX_H_
#define READEX_H_
#ifdef USE_SCOREP_MANUAL
#define USE_SCOREP
#endif
#if defined USE_SCOREP_MANUAL
#include <scorep/SCOREP_User.inc>
#define READEX_INIT()
#define READEX_CLOSE()
#define READEX_IGNORE_START() SCOREP_RECORDING_OFF()
#define READEX_IGNORE_STOP() SCOREP_RECORDING_ON()
#define READEX_PHASE_START( reg_handle, reg_name, reg_type ) SCOREP_USER_OA_PHASE_BEGIN( reg_handle, reg_name, reg_type )
#define READEX_PHASE_STOP( reg_handle ) SCOREP_USER_OA_PHASE_END( reg_handle )
#define READEX_REGION_START( reg_handle, reg_name, reg_type ) SCOREP_USER_REGION_BEGIN( reg_handle, reg_name, reg_type )
#define READEX_REGION_STOP( reg_handle ) SCOREP_USER_REGION_END( reg_handle )
#define READEX_REGION_STOP_START( stop_handle, start_handle, start_name, start_type ) SCOREP_USER_REGION_END( stop_handle ); SCOREP_USER_REGION_BEGIN( start_handle, start_name, start_type )
#define READEX_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_PHASE_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_SIGNIFICANT_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#elif defined USE_SCOREP
#include <scorep/SCOREP_User.inc>
#define READEX_INIT() ;
#define READEX_CLOSE() ;
#define READEX_IGNORE_START() SCOREP_RECORDING_OFF()
#define READEX_IGNORE_STOP() SCOREP_RECORDING_ON()
#define READEX_PHASE_START( reg_handle, reg_name, reg_type ) SCOREP_USER_OA_PHASE_BEGIN( reg_handle, reg_name, reg_type )
#define READEX_PHASE_STOP( reg_handle ) SCOREP_USER_OA_PHASE_END( reg_handle )
#define READEX_REGION_START( reg_handle, reg_name, reg_type ) ;
#define READEX_REGION_STOP( reg_handle ) ;
#define READEX_REGION_STOP_START( stop_handle, start_handle, start_name, start_type ) ;
#define READEX_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_PHASE_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_SIGNIFICANT_REGION_DEFINE( reg_handle ) ;
#elif defined USE_MERIC
#include "meric.inc"
#define READEX_INIT() call MERIC_Init() ;
#define READEX_CLOSE() call MERIC_Close() ;
#define READEX_IGNORE_START() call MERIC_IgnoreStart() ;
#define READEX_IGNORE_STOP() call MERIC_IgnoreStop() ;
#define READEX_PHASE_START( reg_handle, reg_name, reg_type ) call MERIC_MeasureStart( reg_name//char(0) ) ;
#define READEX_PHASE_STOP( reg_handle ) call MERIC_MeasureStop() ;
#define READEX_REGION_START( reg_handle, reg_name, reg_type ) call MERIC_MeasureStart( reg_name//char(0) ) ;
#define READEX_REGION_STOP( reg_handle ) call MERIC_MeasureStop() ;
#define READEX_REGION_STOP_START( stop_handle, start_handle, start_name, start_type ) call MERIC_MeasureStopStart( start_name//char(0) ) ;
#define READEX_REGION_DEFINE( reg_handle ) ;
#define READEX_PHASE_REGION_DEFINE( reg_handle ) ;
#define READEX_SIGNIFICANT_REGION_DEFINE( reg_handle ) ;
#else
#define READEX_INIT()
#define READEX_CLOSE()
#define READEX_REGION_DEFINE( reg_handle )
#define READEX_PHASE_REGION_DEFINE( reg_handle )
#define READEX_SIGNIFICANT_REGION_DEFINE( reg_handle )
#define READEX_IGNORE_START()
#define READEX_IGNORE_STOP()
#define READEX_REGION_START( reg_handle, reg_name, reg_type )
#define READEX_REGION_STOP( reg_handle )
#define READEX_REGION_STOP_START( stop_handle, start_handle, start_name, start_type )
#define READEX_PHASE_START( reg_handle, reg_name, reg_type )
#define READEX_PHASE_STOP( reg_handle )
#endif
#ifndef USE_SCOREP
#define SCOREP_USER_REWIND_DEFINE( handle )
#define SCOREP_USER_REWIND_POINT( handle, name )
#define SCOREP_USER_REGION_INIT( handle, name, type )
#define SCOREP_USER_REGION_BY_NAME_BEGIN( name, type )
#define SCOREP_USER_REGION_BY_NAME_END( name )
#define SCOREP_USER_REWIND_CHECK( handle, value )
#define SCOREP_USER_REGION_ENTER( handle )
#define SCOREP_USER_FUNC_DEFINE()
#define SCOREP_USER_FUNC_BEGIN( name )
#define SCOREP_USER_FUNC_END()
#define SCOREP_USER_GLOBAL_REGION_DEFINE( handle )
#define SCOREP_USER_GLOBAL_REGION_EXTERNAL( handle )
#define SCOREP_USER_PARAMETER_DEFINE( handle )
#define SCOREP_USER_PARAMETER_INT64( handle, name, value )
#define SCOREP_USER_PARAMETER_UINT64( handle, name, value )
#define SCOREP_USER_PARAMETER_STRING( handle, name, value )
#define SCOREP_USER_METRIC_GLOBAL( metricHandle )
#define SCOREP_USER_METRIC_EXTERNAL( metricHandle )
#define SCOREP_USER_METRIC_LOCAL( metricHandle )
#define SCOREP_USER_METRIC_INIT( metricHandle, name, unit, type, context )
#define SCOREP_USER_METRIC_INT64( metricHandle, value )
#define SCOREP_USER_METRIC_UINT64( metricHandle, value )
#define SCOREP_USER_METRIC_DOUBLE( metricHandle, value )
#define SCOREP_RECORDING_IS_ON( isOn ) isOn = .false.
#endif
#endif
