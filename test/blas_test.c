#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mkl.h>

#include <meric.h>

#define min(x,y) (((x) < (y)) ? (x) : (y))


double timespec2double(timespec time) 
{
	return time.tv_sec + (double)time.tv_nsec/1e9;
}

int main()
{
    double *A, *B, *C, *Vmat, *v, *vOut;
    int m, n, k, i, j, vMatN;
    double alpha, beta, timeAvg, gflop;
    timespec timeStart, timeEnd;
    int loopDGEMM, loopDGEMV;
    struct timeval stime, etime, diff;
//-------------------------------------------------------------------
MERIC_Init();
//-------------------------------------------------------------------
/*
    printf("LARGE MATRICES\n\n");
    m = 5000, k = 5000, n = 5000, vMatN=20000; 
    loopDGEMM=5;  //20
    loopDGEMV=125; //550;
*/
//-------------------------------------------------------------------
/*
    printf("SMALL MATRICES\n\n");
    m = 500, k = 500, n = 500, vMatN=500;
    loopDGEMM=4500; //5000;
    loopDGEMV=450000;//500000;
*/
//-------------------------------------------------------------------
    printf("FLOPs mearurement TEST\n\n");
    m = 3000, k = 3000, n = 3000, vMatN=500;
    loopDGEMM=100; //5000;
    loopDGEMV=0;//500000;
//-------------------------------------------------------------------
    alpha = 1.0; beta = 0.0;

    printf("\n Allocating memory \n");
MERIC_MeasureStart("malloc");
    A = (double *)mkl_malloc( m*k*sizeof( double ), 64 );
    B = (double *)mkl_malloc( k*n*sizeof( double ), 64 );
    C = (double *)mkl_malloc( m*n*sizeof( double ), 64 );
    Vmat = (double *)mkl_malloc( vMatN*vMatN*sizeof( double ), 64 );

    v = (double *)mkl_malloc( vMatN*sizeof( double ), 64 );
    vOut = (double *)mkl_malloc( vMatN*sizeof( double ), 64 );
MERIC_MeasureStop("malloc");
    if (A == NULL || B == NULL || C == NULL || v == NULL || vOut == NULL) {
      printf( "\n ERROR: Can't allocate memory. Aborting... \n\n");
      mkl_free(A);
      mkl_free(B);
      mkl_free(C);
      mkl_free(Vmat);
      mkl_free(v);
      mkl_free(vOut);
      return 1;
    }

    printf (" Intializing data \n\n");
MERIC_MeasureStart("init");
	#pragma omp parallel for
    for (i = 0; i < (m*k); i++) {
        A[i] = (double)i;  //(double)(i+1);
    }
	#pragma omp parallel for
    for (i = 0; i < (k*n); i++) {
        B[i] = (double)i; //(double)(-i-1);
    }
	#pragma omp parallel for
    for (i = 0; i < (m*n); i++) {
        C[i] = 0.0;
    }
	#pragma omp parallel for
    for(i = 0; i < (vMatN*vMatN); i++) {
        Vmat[i] = 1.0;
    }
	#pragma omp parallel for
    for (i = 0; i < vMatN; i++) {
        v[i] = (double)i;
        vOut[i] = 0.0;
    }
MERIC_MeasureStop("init");
/*
    printf("\n dgemm input: \n");
    printf("\n Matrix A:\n");
    for(i = 0; i < m; i++) {
        for(j = 0; j < k; j++) {
            printf("%f ", A[i*k+j]);
        }
        printf("\n");
    }
    printf("\n Matrix B:\n");
    for(i = 0; i < k; i++) {
        for(j = 0; j < n; j++) {
            printf("%f ", B[i*n+j]);
        }
        printf("\n");
    }
*/

    printf("\n 'Heating' cache with dgemm... \n");
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C, n);
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                m, n, k, alpha, A, k, B, n, beta, C, n);


    printf ("\n Computing matrix product using Intel(R) MKL dgemm function via CBLAS interface \n\n");
    clock_gettime(CLOCK_REALTIME, &timeStart);

MERIC_MeasureStart("DGEMM");
    for(int i=0;i<loopDGEMM;i++)
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                    m, n, k, alpha, A, k, B, n, beta, C, n);
MERIC_MeasureStop("DGEMM");

    clock_gettime(CLOCK_REALTIME, &timeEnd);
    printf(" >>> DGEMM TIME: %lf\n", timespec2double(timeEnd)-timespec2double(timeStart));
  
    timeAvg = (timespec2double(timeEnd) - timespec2double(timeStart))/loopDGEMM;
    gflop = (2.0*m*n*k + 1.0*m*n)*1E-9;
    printf("Average time:%e secs \n", timeAvg);
    printf("GFlop       :%.5f \n", gflop);
    printf("GFlop/sec   :%.5f \n", gflop/timeAvg);

/*
    printf("\n Matrix C:\n");
    for(i = 0; i < m; i++) {
        for(j = 0; j < k; j++) {
            printf("%f ", C[i*k+j]);
        }
        printf("\n");
    }

    printf("\n dgemv input:  \n");
    
    printf("\n vMat: \n");
    for(i = 0; i < vMatN; i++) {
        for(int j = 0; j < vMatN; j++) {
            printf("%f ", Vmat[i*vMatN + j]);
        }
        printf("\n");
    }

    printf("\n v: \n");
    for(i = 0; i < vMatN; i++) {
        printf("%f ", v[i]);
    }
    printf("\n");
*/

    printf("\n 'Heating' cache with ggemv... \n");
    cblas_dgemv(CblasRowMajor, CblasNoTrans,
                vMatN, vMatN, alpha, Vmat, vMatN, v, 1, beta, vOut, 1);
    cblas_dgemv(CblasRowMajor, CblasNoTrans,
                vMatN, vMatN, alpha, Vmat, vMatN, v, 1, beta, vOut, 1);


    printf("\n Computing matrix-vector product using dgemv function. \n\n");
    clock_gettime(CLOCK_REALTIME, &timeStart);
MERIC_MeasureStart("DGEMV");
    for(int i=0;i<loopDGEMV;i++)
        cblas_dgemv(CblasRowMajor, CblasNoTrans, 
                    vMatN, vMatN, alpha, Vmat, vMatN, v, 1, beta, vOut, 1);
MERIC_MeasureStop("DGEMV");
    clock_gettime(CLOCK_REALTIME, &timeEnd);
    printf(" >>> DGEMV TIME: %lf\n", timespec2double(timeEnd)-timespec2double(timeStart));

/*
    printf("\n Vector vOut:\n");
    for(i = 0; i < vMatN; i++) {
        printf("%f ", vOut[i]);
    }
*/
    printf ("\n Computations completed.\n\n");

    printf ("\n Deallocating memory \n\n");
    mkl_free(A);
    mkl_free(B);
    mkl_free(C);
MERIC_Close();
    return 0;
}
