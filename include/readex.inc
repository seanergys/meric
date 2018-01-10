 !
 ! Shared fortran intarface for MERIC and Score-P libraries
 !
 ! version 1.0
 ! 09.01.2018
 ! Martin Beseda and Ondrej Vysocky, martin.beseda@vsb.cz, ondrej.vysocky@vsb.cz
 ! IT4Innovations national supercomputing center, Czech Republic
 !
 
#ifndef READEX_H_
#define READEX_H_

#ifdef USE_SCOREP_MANUAL
#define USE_SCOREP
#endif

! ------------------------------------------------------------------------------
! USE_SCOREP or USE_SCOREP_MANUAL
#ifdef USE_SCOREP
#include "scorep/SCOREP_User.inc"

#define READEX_INIT()
#define READEX_CLOSE()

#define READEX_IGNORE_START() SCOREP_RECORDING_OFF()
#define READEX_IGNORE_STOP() SCOREP_RECORDING_ON()

#define READEX_PHASE_START( reg_handle, reg_name, reg_type ) SCOREP_USER_OA_PHASE_BEGIN( reg_handle, reg_name, reg_type )
#define READEX_PHASE_STOP( reg_handle ) SCOREP_USER_OA_PHASE_END( reg_handle )

! ------------------------------------------------------------------------------
! USE_SCOREP_MANUAL
#ifdef USE_SCOREP_MANUAL

#define READEX_REGION_START( reg_handle, reg_name, reg_type ) SCOREP_USER_REGION_BEGIN( reg_handle, reg_name, reg_type )
#define READEX_REGION_STOP( reg_handle ) SCOREP_USER_REGION_END( reg_handle )

#define READEX_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_PHASE_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_SIGNIFICANT_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)

! ------------------------------------------------------------------------------
! USE_SCOREP
#else

#define READEX_REGION_START( reg_handle, reg_name, reg_type )
#define READEX_REGION_STOP( reg_handle )

#define READEX_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_PHASE_REGION_DEFINE( reg_handle ) SCOREP_USER_REGION_DEFINE(reg_handle)
#define READEX_SIGNIFICANT_REGION_DEFINE( reg_handle )

#endif
! ------------------------------------------------------------------------------
! USE_MERIC
#elif defined USE_MERIC

#define READEX_INIT() call MERIC_Init()
#define READEX_CLOSE() call MERIC_Close()
	
#define READEX_IGNORE_START() call MERIC_IgnoreStart()
#define READEX_IGNORE_STOP() call MERIC_IgnoreStop()

#define READEX_PHASE_START( reg_handle, reg_name, reg_type ) call MERIC_MeasureStart(reg_name)
#define READEX_PHASE_STOP( reg_handle ) call MERIC_MeasureStop()

#define READEX_REGION_START( reg_handle, reg_name, reg_type ) call MERIC_MeasureStart(reg_name)
#define READEX_REGION_STOP( reg_handle ) call MERIC_MeasureStop()

#define READEX_REGION_DEFINE( reg_handle )
#define READEX_PHASE_REGION_DEFINE( reg_handle )
#define READEX_SIGNIFICANT_REGION_DEFINE( reg_handle )


#include "meric.inc"

! ------------------------------------------------------------------------------
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
#define READEX_PHASE_START( reg_handle, reg_name, reg_type )
#define READEX_PHASE_STOP( reg_handle )

#endif


! READEX_H_
#endif



