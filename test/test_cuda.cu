#include <cuda_runtime.h>
#include <stdio.h>
#include <meric.h>
#include <unistd.h>
#include <iostream>

#define TIME 3000000000

#define CUDA_CHECK(call)                                                    \
{                                                                           \
	cudaError_t err__ = (call);                                             \
	if (err__ != cudaSuccess)                                               \
	{                                                                       \
		std::cerr << "CUDA error: " << cudaGetErrorString(err__)            \
			<< " at " << __FILE__ << ":" << __LINE__ << std::endl;          \
		std::exit(EXIT_FAILURE);                                            \
	}                                                                       \
}


__global__ void compute(unsigned long long time = TIME)
{
	int x_tid = blockIdx.x * blockDim.x + threadIdx.x;
	int y_tid = blockIdx.y * blockDim.y + threadIdx.y;
	
	float x = 0.0f + (float)(x_tid) / (float)(blockDim.x * gridDim.x);
	float r = 2.4f + 1.5f * (float)(y_tid) / (float)(blockDim.y * gridDim.y);

	unsigned long long end;
	asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(end) : : "memory");
	end = end + time;
	unsigned long long timer_now = 0;

	while(timer_now < end)
	{
		for (unsigned long long i = 0; i < 12500; i++)
		{
			x = r * x * (1.0f - x);
			x = r * x * (1.0f - x);
			x = r * x * (1.0f - x);
			x = r * x * (1.0f - x);
			x = r * x * (1.0f - x);
			x = r * x * (1.0f - x);
			x = r * x * (1.0f - x);
			x = r * x * (1.0f - x);
		}
		asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(timer_now) : : "memory");
	}

	//this branch is to avoid compiler optimization
	if(x == 1000.0)
	{
		printf("%.3f\n", x);
	}
}


__global__ void set(float *xr)
{
	int x_tid = blockIdx.x * blockDim.x + threadIdx.x;
	int y_tid = blockIdx.y * blockDim.y + threadIdx.y;
	
	xr[x_tid + y_tid * gridDim.x * blockDim.x] = (float)x_tid * (float)y_tid;
}


__global__ void read(float *xr, unsigned long long time = TIME)
{
	int x_tid = blockIdx.x * blockDim.x + threadIdx.x;
	int y_tid = blockIdx.y * blockDim.y + threadIdx.y;
		
	int total_size = blockDim.x * gridDim.x * blockDim.y * gridDim.y;
	
	float x;
	
	for(int i = 0; i < 100000; i++)
	{
		x = xr[(x_tid + y_tid * blockDim.x * gridDim.x + i) % total_size];
		if(x ==  3.14159)
		{
			printf("pi = %.3f\n", x);
		}
	}
}

int main(int argc, char** argv)
{
	MERIC_Init();

	MERIC_MeasureStart("SLEEP");
	usleep(1000000);
	MERIC_MeasureStop("SLEEP");

	dim3 block(32,16);
	dim3 grid(16,32);

	MERIC_MeasureStart("COMPUTE");


	int deviceCount;
	CUDA_CHECK(cudaGetDeviceCount(&deviceCount));
	if(deviceCount < 1)
	{
		printf("No CUDA devices found, app will exit now!\n");
		return -1;
	}

	for (int device=0; device<deviceCount; device++)
	{ 
		CUDA_CHECK(cudaSetDevice(device));
		compute<<<grid, block>>>();
	}
	CUDA_CHECK(cudaPeekAtLastError());
	for (int device=0; device<deviceCount; device++)
	{
		CUDA_CHECK(cudaSetDevice(device));
		CUDA_CHECK(cudaDeviceSynchronize());
	}
	printf("Compute phase end\n");
	
	MERIC_MeasureStopStart("COMPUTE", "MOVE");
	
	float *xr;
	size_t elements = (size_t)block.x * (size_t)block.y * (size_t)grid.x * (size_t)grid.y;
	size_t bytes = elements * sizeof(float);
	CUDA_CHECK(cudaMallocHost((void**)&xr, (size_t)(deviceCount)*bytes));

	for (int device=0; device<deviceCount; device++)
	{
		CUDA_CHECK(cudaSetDevice(device));
		set<<<grid, block>>>(xr+device*elements);
		read<<<grid, block>>>(xr+device*elements);
		CUDA_CHECK(cudaPeekAtLastError());
		CUDA_CHECK(cudaDeviceSynchronize());
	}
	printf("Memory phase end\n");
	
	MERIC_MeasureStop("MOVE");
	
	CUDA_CHECK(cudaFreeHost(xr));

	MERIC_Close();
	return 0;
}
