
#ifndef MERIC_H_
#define MERIC_H_

#ifdef __cplusplus
extern "C" {
#endif

void MERIC_Init();
void MERIC_Close();

void MERIC_MeasureStart(const char* region);
void MERIC_MeasureStop();

#ifdef __cplusplus
}
#endif



#endif /* MERIC_H_ */
