// 1. Program header

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <omp.h>

#include "CL/cl.h"
#include "CL/cl_platform.h"

#ifndef GLOBAL_SIZE
#define GLOBAL_SIZE			64*1024*1024	
#endif

#ifndef LOCAL_SIZE
#define	LOCAL_SIZE			64	 
#endif

#define	NUM_WORK_GROUPS		GLOBAL_SIZE/LOCAL_SIZE

// These are the three seperate files which contain the code for each operation
const char *			CL_FILE_NAME_MULT 		= { "project_6_mult.cl" };
const char * 			CL_FILE_NAME_MULTADD 	= { "project_6_multAdd.cl" };
const char *			CL_FILE_NAME_MULTRED	= { "project_6_multRed.cl" };

// The following enum will signify which operation has been selected by the user
// via the command line. UV stands for unknown value, and is the default value
enum application {mult, multAdd, multRed, UV};

void	Wait( cl_command_queue );
int		LookAtTheBits( float );


int
main( int argc, char *argv[ ] )
{
	// see if we can even open the opencl kernel program
	// (no point going on if we can't):

	// The below chunk of code handles the command line selection of the operation, and saves it
	application app = UV;

	if(argc == 2)
	{
		//fprintf(stderr, "argv[1]: %s\n", argv[1]);
		if(!strcmp(argv[1], "multAdd"))
			app = multAdd;
		else if(!strcmp(argv[1], "mult"))
			app = mult;
		else if(!strcmp(argv[1], "multRed"))
			app = multRed;
		else
		{
			fprintf(stderr, "Command line arg invalid, mult for multiply, multAdd for multiply then add\n");
			return 0;
		}
	}
	else
	{
		fprintf(stderr, "Command line arg invalid, mult for multiply, multAdd for multiply then add\n");
			return 0;
	}

	FILE *fp;
#ifdef WIN32
	errno_t err = fopen_s( &fp, CL_FILE_NAME, "r" );
	if( err != 0 )
#else
	// This bit of code opens the corresponding file to the
	// selected operation as each is different.
	if(app == mult)
		fp = fopen( CL_FILE_NAME_MULT, "r" );
	else if(app == multAdd)
		fp = fopen( CL_FILE_NAME_MULTADD, "r");
	else if(app == multRed)
		fp = fopen(CL_FILE_NAME_MULTRED, "r");
	if( fp == NULL )
#endif
	{
		// This bit of code is just error output if the file is missing or corrupted or smth
		if(app == mult)
			fprintf( stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME_MULT );
		else if(app == multAdd)
			fprintf( stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME_MULTADD );
		else if(app == multRed)
			fprintf( stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME_MULTRED );
		return 1;
	}

	cl_int status;		// returned status from opencl calls
				// test against CL_SUCCESS

	// get the platform id:

	cl_platform_id platform;
	status = clGetPlatformIDs( 1, &platform, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clGetPlatformIDs failed (2)\n" );
	
	// get the device id:

	cl_device_id device;
	status = clGetDeviceIDs( platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clGetDeviceIDs failed (2)\n" );

	// 2. allocate the host memory buffers:

	// Here hC may not be used depending on operation, could be optimized but im tired
	float *hA = new float[ GLOBAL_SIZE ];
	float *hB = new float[ GLOBAL_SIZE ];
	float *hC = new float[ GLOBAL_SIZE ];
	float *hD = new float[ GLOBAL_SIZE ];
    //TODO// add creation of hD

	// fill the host memory buffers:

    //TODO// add hC to this
	for( int i = 0; i < GLOBAL_SIZE; i++ )
	{
		hA[i] = hB[i] = hC[i] = (float) sqrt(  (double)i  );
	}

	size_t dataSize = GLOBAL_SIZE * sizeof(float);

	// 3. create an opencl context:

	cl_context context = clCreateContext( NULL, 1, &device, NULL, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateContext failed\n" );

	// 4. create an opencl command queue:

	cl_command_queue cmdQueue = clCreateCommandQueue( context, device, 0, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateCommandQueue failed\n" );

	// 5. allocate the device memory buffers:

	cl_mem dA = clCreateBuffer( context, CL_MEM_READ_ONLY,  dataSize, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateBuffer failed (1)\n" );

	cl_mem dB = clCreateBuffer( context, CL_MEM_READ_ONLY,  dataSize, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateBuffer failed (2)\n" );

	// Only create the device C memory if needed (multAdd operation)
	cl_mem dC;
	if(app == multAdd)
	{
		dC = clCreateBuffer( context, CL_MEM_WRITE_ONLY, dataSize, NULL, &status );
		if( status != CL_SUCCESS )
			fprintf( stderr, "clCreateBuffer failed (3)\n" );
	}
	
	cl_mem dD = clCreateBuffer( context, CL_MEM_WRITE_ONLY, dataSize, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateBuffer failed (4)\n" );

    //TODO// Create a new buffer for dD

	// 6. enqueue the 2 commands to write the data from the host buffers to the device buffers:

	status = clEnqueueWriteBuffer( cmdQueue, dA, CL_FALSE, 0, dataSize, hA, 0, NULL, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clEnqueueWriteBuffer failed (1)\n" );

	status = clEnqueueWriteBuffer( cmdQueue, dB, CL_FALSE, 0, dataSize, hB, 0, NULL, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clEnqueueWriteBuffer failed (2)\n" );

	// Only write to the buffer if needed (multAdd operation)
	if(app == multAdd)
	{
		status = clEnqueueWriteBuffer( cmdQueue, dC, CL_FALSE, 0, dataSize, hC, 0, NULL, NULL );
		if( status != CL_SUCCESS )
			fprintf( stderr, "clEnqueueWriteBuffer failed (3)\n" );
	}

	//TODO// Add dC to this thing :) pretty sure it populates the device data
	Wait( cmdQueue );

	// 7. read the kernel code from a file:

	fseek( fp, 0, SEEK_END );
	size_t fileSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );
	char *clProgramText = new char[ fileSize+1 ];		// leave room for '\0'
	size_t n = fread( clProgramText, 1, fileSize, fp );
	clProgramText[fileSize] = '\0';
	fclose( fp );
	if( n != fileSize )
	{
		// Seperate error outputs depending on operation
		if(app == multAdd)
			fprintf( stderr, "Expected to read %d bytes read from '%s' -- actually read %d.\n", fileSize, CL_FILE_NAME_MULTADD, n );
		else if(app == mult)
			fprintf( stderr, "Expected to read %d bytes read from '%s' -- actually read %d.\n", fileSize, CL_FILE_NAME_MULT, n );
		else if(app == multRed)
			fprintf( stderr, "Expected to read %d bytes read from '%s' -- actually read %d.\n", fileSize, CL_FILE_NAME_MULTRED, n );
	}
	// create the text for the kernel program:

	char *strings[1];
	strings[0] = clProgramText;
	cl_program program = clCreateProgramWithSource( context, 1, (const char **)strings, NULL, &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateProgramWithSource failed\n" );
	delete [ ] clProgramText;

	// 8. compile and link the kernel code:

	char *options = { "" };
	status = clBuildProgram( program, 1, &device, options, NULL, NULL );
	if( status != CL_SUCCESS )
	{
		size_t size;
		clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size );
		cl_char *log = new cl_char[ size ];
		clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, size, log, NULL );
		fprintf( stderr, "clBuildProgram failed:\n%s\n", log );
		delete [ ] log;
	}

	// 9. create the kernel object:

	// This bit of code creates a different kernel depending on the selected operation
	cl_kernel kernel;
	if(app == multAdd)
		kernel = clCreateKernel( program, "ArrayMultAdd", &status );
	else if(app == mult)
		kernel = clCreateKernel( program, "ArrayMult", &status);
	else if(app == multRed)
		kernel = clCreateKernel( program, "ArrayMultRed", &status );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clCreateKernel failed\n" );

	// 10. setup the arguments to the kernel object:

	status = clSetKernelArg( kernel, 0, sizeof(cl_mem), &dA );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clSetKernelArg failed (1)\n" );

	status = clSetKernelArg( kernel, 1, sizeof(cl_mem), &dB );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clSetKernelArg failed (2)\n" );

	// The following if else tree tests to set args appropriate depending on the
	// user selected operation. dD is always used for the result
	if(app == multAdd)
	{
		status = clSetKernelArg( kernel, 2, sizeof(cl_mem), &dC );
		if( status != CL_SUCCESS )
			fprintf( stderr, "clSetKernelArg failed (3)\n" );

		status = clSetKernelArg( kernel, 3, sizeof(cl_mem), &dD );
		if( status != CL_SUCCESS )
			fprintf( stderr, "clSetKernelArg failed (4)\n" );
	}
	else if(app == mult)
	{
		status = clSetKernelArg( kernel, 2, sizeof(cl_mem), &dD );
		if( status != CL_SUCCESS )
			fprintf( stderr, "clSetKernelArg failed (3)\n" );
	}
	else if(app == multRed)
	{
		// This special arg set is for a local variable for intermediate operations
		// novel to the reduction operation.
		status = clSetKernelArg( kernel, 2, LOCAL_SIZE * sizeof(float), NULL);
		if( status != CL_SUCCESS )
			fprintf( stderr, "clSetKernelArg failed(3)\n" );
		
		status = clSetKernelArg( kernel, 3, sizeof(cl_mem), &dD );
		if( status != CL_SUCCESS )
			fprintf( stderr, "clSetKernelArg failed(4)\n" );
	}

    //TODO// here add another clSetKernelArg for the fourth element

	// 11. enqueue the kernel object for execution:

	size_t globalWorkSize[3] = { GLOBAL_SIZE, 1, 1 };
	size_t localWorkSize[3]  = { LOCAL_SIZE,   1, 1 };

	Wait( cmdQueue );
	double time0 = omp_get_wtime( );

	time0 = omp_get_wtime( );

	status = clEnqueueNDRangeKernel( cmdQueue, kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL );
	if( status != CL_SUCCESS )
		fprintf( stderr, "clEnqueueNDRangeKernel failed: %d\n", status );

	Wait( cmdQueue );
	double time1 = omp_get_wtime( );

	// 12. read the results buffer back from the device to the host:

    // Refactor below call to read in hD not hC to account for the new stuff
	status = clEnqueueReadBuffer( cmdQueue, dD, CL_TRUE, 0, dataSize, hD, 0, NULL, NULL );
	if( status != CL_SUCCESS )
			fprintf( stderr, "clEnqueueReadBuffer failed\n" );

	// The final summation for the reduction operation, only executed if the reduction
	// operation was selected.
	if(app == multRed)
	{
		float sum = 0.0;
		for(int i = 0; i < NUM_WORK_GROUPS; i++)
			sum += hD[i];
	}
	// fprintf( stderr, "%8d\t%4d\t%10d\t%10.3lf GigaMultsPerSecond\n",
	// 	NMB, LOCAL_SIZE, NUM_WORK_GROUPS, (double)GLOBAL_SIZE/(time1-time0)/1000000000. );
	fprintf( stderr, "%d, %d, %10.3lf\n", GLOBAL_SIZE, LOCAL_SIZE, (double)GLOBAL_SIZE/(time1-time0)/1000000000. );

#ifdef WIN32
	Sleep( 2000 );
#endif


	// 13. clean everything up:

	clReleaseKernel(        kernel   );
	clReleaseProgram(       program  );
	clReleaseCommandQueue(  cmdQueue );
	clReleaseMemObject(     dA  );
	clReleaseMemObject(     dB  );
	// Only necessary to clean dC if dC was even used, i.e. multAdd operation
	if(app == multAdd)
		clReleaseMemObject(     dC  );
	clReleaseMemObject(		dD	);

	//? idk if this is needed but maybe there is some kind of special cleanup necessary for
	//? the local variable in the reduction operation? i have no idea, i hope not

	//TODO// Add dD to the above and hD to the below for cleanup stuff

	delete [ ] hA;
	delete [ ] hB;
	delete [ ] hC;
	delete [ ] hD;

	return 0;
}


int
LookAtTheBits( float fp )
{
	int *ip = (int *)&fp;
	return *ip;
}


// wait until all queued tasks have taken place:

void
Wait( cl_command_queue queue )
{
      cl_event wait;
      cl_int      status;

      status = clEnqueueMarker( queue, &wait );
      if( status != CL_SUCCESS )
	      fprintf( stderr, "Wait: clEnqueueMarker failed\n" );

      status = clWaitForEvents( 1, &wait );
      if( status != CL_SUCCESS )
	      fprintf( stderr, "Wait: clWaitForEvents failed\n" );
}