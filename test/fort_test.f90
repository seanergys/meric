 ! Fortran version of test.cpp

program fort_test

! #include "meric.inc"
#include "readex.inc"

!	call MERIC_Init()
	READEX_INIT()

	READEX_REGION_DEFINE(A)
	READEX_REGION_DEFINE(B)
	READEX_REGION_DEFINE(C)

!	call MERIC_MeasureStart("A")
	READEX_REGION_START(A, "A", SCOREP_USER_REGION_TYPE_COMMON)

!	call MERIC_MeasureStart("B")
	READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON)
	call sleep(1)
!	call MERIC_MeasureStop()
	READEX_REGION_STOP(B)

!	call MERIC_MeasureStart("C")
	READEX_REGION_START(C, "C", SCOREP_USER_REGION_TYPE_COMMON)
	call sleep(1)
!	call MERIC_MeasureStop()
	READEX_REGION_STOP(C)

!	call MERIC_MeasureStart("B")
	READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON)
	call sleep(2)
!	call MERIC_MeasureStop()
	READEX_REGION_STOP(B)

!	call MERIC_MeasureStop()
	READEX_REGION_STOP(A)

!	call MERIC_Close()
	READEX_CLOSE()

end program fort_test

