
#ifndef MERIC_H_
#define MERIC_H_

#ifdef __cplusplus
extern "C" {
#endif

void   MERIC_Init();
void   MERIC_Close();

void   MERIC_MeasureStart(const char* region);
double MERIC_MeasureStop(const char* region);
double MERIC_MeasureStopStart(const char* stopRegion, const char* startRegion);

void   MERIC_IgnoreStart();
void   MERIC_IgnoreStop();

//void MERIC_CaptureScope(const char * region);
//RAII scope measurement - it does not return time
//only compilers supporting cleanup attribute (such as GCC or Clang)
#ifdef __has_attribute
	#if __has_attribute (cleanup)
		static void MERIC_ScopeEnd(const char ** region)
		{
			MERIC_MeasureStop(*region);
		}
		#define MERIC_CaptureScope( region ) \
			MERIC_MeasureStart( region ); \
			const char * tmp __attribute__((cleanup(MERIC_ScopeEnd))) = region;
	#else
		#define MERIC_CaptureScope( region ) 
	#endif
#else
	#define MERIC_CaptureScope( region ) 
#endif


#ifdef __cplusplus
}
#endif

#endif /* MERIC_H_ */
