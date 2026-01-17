/*
 * Main.cpp
 *
 *  Created on: Fall 2019
 */

#include <stdio.h>
#include <math.h>
#include <CImg.h>
#include <fstream>

using namespace cimg_library;

// Data type for image components
typedef float data_t;

const char *SOURCE_IMG = "bailarina.bmp";
const char *SOURCE_IMG2 = "flores_3.bmp";
const char *DESTINATION_IMG = "resultado_monohilo.bmp";

#define N_REPEATS 20

// Filter argument data type
typedef struct
{
	data_t *pRsrc; // Pointers to the R, G and B components
	data_t *pGsrc;
	data_t *pBsrc;
	data_t *pRsrc2;
	data_t *pGsrc2;
	data_t *pBsrc2;
	data_t *pRdst;
	data_t *pGdst;
	data_t *pBdst;
	uint pixelCount; // Size of the image in pixels
} filter_args_t;

/***********************************************
 *
 * Algorithm. Image filter.
 *
 * *********************************************/
void filter(filter_args_t args)
{
	/************************************************
	 * Algorithm.
	 */

	for (uint i = 0; i < args.pixelCount; i++)
	{
		*(args.pRdst + i) = sqrtf((*(args.pRsrc + i) * *(args.pRsrc + i)) + (*(args.pRsrc2 + i) * *(args.pRsrc2 + i))) / sqrt(2);
		*(args.pGdst + i) = sqrtf((*(args.pGsrc + i) * *(args.pGsrc + i)) + (*(args.pGsrc2 + i) * *(args.pGsrc2 + i))) / sqrt(2);
		*(args.pBdst + i) = sqrtf((*(args.pBsrc + i) * *(args.pBsrc + i)) + (*(args.pBsrc2 + i) * *(args.pBsrc2 + i))) / sqrt(2);
	}
}

int main()
{
	// Open file and object initialization
	CImg<data_t> srcImage;
	CImg<data_t> srcImage2;

	try
	{
		srcImage.load(SOURCE_IMG);
	}
	catch (CImgIOException e)
	{
		printf("Image source1 not found\n");
		exit(-1);
	}

	try
	{
		srcImage2.load(SOURCE_IMG2);
	}
	catch (CImgIOException e)
	{
		printf("Image source2 not found\n");
		exit(-1);
	}
	filter_args_t filter_args;
	data_t *pDstImage; // Pointer to the new image pixels

	/***************************************************
	 * Variables initialization.
	 * - Prepare variables for the algorithm
	 * - This is not included in the benchmark time
	 */

	// srcImage.display();			   // Displays the source image
	uint width = srcImage.width(); // Getting information from the source image
	uint height = srcImage.height();
	uint nComp = srcImage.spectrum(); // source image number of components
	uint nComp2 = srcImage2.spectrum();
	uint width2 = srcImage2.width();
	uint height2 = srcImage2.height();

	// Common values for spectrum (number of image components):
	//  B&W images = 1
	//	Normal color images = 3 (RGB)
	//  Special color images = 4 (RGB and alpha/transparency channel)

	// Calculating image size in pixels
	filter_args.pixelCount = width * height;

	// Allocate memory space for destination image components
	pDstImage = (data_t *)malloc(filter_args.pixelCount * nComp * sizeof(data_t));
	if (pDstImage == NULL)
	{
		perror("Allocating destination image");
		exit(-2);
	}

	if (width != width2 || height != height2 || nComp != nComp2)
	{
		perror("El tamaño es distinto");
		exit(EXIT_FAILURE);
	}

	// Pointers to the componet arrays of the source image
	filter_args.pRsrc = srcImage.data();							// pRcomp points to the R component array
	filter_args.pGsrc = filter_args.pRsrc + filter_args.pixelCount; // pGcomp points to the G component array
	filter_args.pBsrc = filter_args.pGsrc + filter_args.pixelCount; // pBcomp points to B component array

	filter_args.pRsrc2 = srcImage2.data();							  // pRcomp points to the R component array
	filter_args.pGsrc2 = filter_args.pRsrc2 + filter_args.pixelCount; // pGcomp points to the G component array
	filter_args.pBsrc2 = filter_args.pGsrc2 + filter_args.pixelCount; // pBcomp points to B component array
	// Pointers to the RGB arrays of the destination image
	filter_args.pRdst = pDstImage;
	filter_args.pGdst = filter_args.pRdst + filter_args.pixelCount;
	filter_args.pBdst = filter_args.pGdst + filter_args.pixelCount;

	/***********************************************
	 * Algorithm start.
	 * - Measure initial time
	 */
	struct timespec tStart, tEnd;
	double dElapsedTimeS;

	if (clock_gettime(CLOCK_REALTIME, &tStart) != 0)
	{
		perror("Error al obtener tiempo inicial");
		exit(EXIT_FAILURE);
	}

	/************************************************
	 * Algorithm.
	 */

	for (int i = 0; i < N_REPEATS; i++)
	{
		filter(filter_args);
	}

	/***********************************************
	 * End of the algorithm.
	 * - Measure the end time
	 * - Calculate the elapsed time
	 */

	if (clock_gettime(CLOCK_REALTIME, &tEnd) != 0)
	{
		perror("Error al obtener tiempo final");
		exit(EXIT_FAILURE);
	}
	dElapsedTimeS = (tEnd.tv_sec - tStart.tv_sec);
	dElapsedTimeS += (tEnd.tv_nsec - tStart.tv_nsec) / 1e+9;
	printf("Tiempo total: %f", dElapsedTimeS);

	// Create a new image object with the calculated pixels
	// In case of normal color images use nComp=3,
	// In case of B/W images use nComp=1.
	CImg<data_t> dstImage(pDstImage, width, height, 1, nComp);

	// Store destination image in disk
	dstImage.save(DESTINATION_IMG);

	// Display destination image
	dstImage.display();

	// Free memory
	free(pDstImage);

	return 0;
}