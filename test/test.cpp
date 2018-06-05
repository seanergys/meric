
//#include <meric.h>
#include <readex.h>

#include <cstdlib>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv)
{
//	MERIC_Init();
	READEX_INIT();

//	MERIC_MeasureStart("A");
	READEX_REGION_DEFINE(A);
	READEX_REGION_START(A, "A", SCOREP_USER_REGION_TYPE_COMMON);

//	MERIC_MeasureStart("B");
	READEX_REGION_DEFINE(B);
	READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON);
	sleep(1);
//	MERIC_MeasureStop();
	READEX_REGION_STOP(B);

//	MERIC_MeasureStart("C");
	READEX_REGION_DEFINE(C);
	READEX_REGION_START(C, "C", SCOREP_USER_REGION_TYPE_COMMON);
	sleep(1);
//	MERIC_MeasureStop();
	READEX_REGION_STOP(C);

//	MERIC_MeasureStart("B");
	READEX_REGION_START(B, "B", SCOREP_USER_REGION_TYPE_COMMON);
	sleep(2);
//	MERIC_MeasureStop();
	READEX_REGION_STOP(B);
	
//	MERIC_MeasureStop();
	READEX_REGION_STOP(A);

//	MERIC_Close();
	READEX_CLOSE();
	return 0;
}




