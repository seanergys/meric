
#ifndef MERIC_H_
#define MERIC_H_

#ifdef __cplusplus
extern "C" {
#endif

void   MERIC_Init();
void   MERIC_Close();

void   MERIC_MeasureStart(const char* region);
double MERIC_MeasureStop();
double MERIC_MeasureStopStart(const char* region);

void   MERIC_IgnoreStart();
void   MERIC_IgnoreStop();

//void MERIC_captureScope(const char * region);
//RAII scope measurement - it does not return time
void MERIC_scopeEnd(const char ** region)
{
    MERIC_MeasureStop();
}
#define MERIC_captureScope( region ) \
	MERIC_MeasureStart( region ); \
	const char * tmp __attribute__((cleanup(MERIC_scopeEnd))) = region;

#ifdef __cplusplus
}
#endif



#endif /* MERIC_H_ */
