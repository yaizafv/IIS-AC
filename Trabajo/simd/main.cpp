/*
 * Main.cpp
 *
 *  Created on: Fall 2019
 */

#include <stdio.h>
#include <immintrin.h> // Required to use intrinsic functions
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <CImg.h>
#include <fstream>
#include <time.h>

using namespace cimg_library;

// Data type for image components
typedef float data_t;

#define VECTOR_SIZE 18 // Array size. Note: It is not a multiple of 8
#define ITEMS_PER_PACKET (sizeof(__m256) / sizeof(data_t))

const char *SOURCE_IMG = "bailarina.bmp";
const char *SOURCE_IMG2 = "flores_3.bmp";
const char *DESTINATION_IMG = "resultado_simd.bmp";

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

void process(__m256 *rPacketA, __m256 *gPacketA, __m256 *bPacketA, __m256 *rPacketB, __m256 *gPacketB, __m256 *bPacketB)
{
	__m256 raiz2 = _mm256_set1_ps(2);
	raiz2 = _mm256_sqrt_ps(raiz2);
	__m256 opMulR, opMulG, opMulB;

	//Rojo
	*rPacketA = _mm256_mul_ps(*rPacketA, *rPacketA);
	opMulR = _mm256_mul_ps(*rPacketB, *rPacketB);
	*rPacketA = _mm256_add_ps(*rPacketA, opMulR);
	*rPacketA = _mm256_sqrt_ps(*rPacketA);
	*rPacketA = _mm256_div_ps(*rPacketA, raiz2);

	//Verde
	*gPacketA = _mm256_mul_ps(*gPacketA, *gPacketA);
	opMulG = _mm256_mul_ps(*gPacketB, *gPacketB);
	*gPacketA = _mm256_add_ps(*gPacketA, opMulG);
	*gPacketA = _mm256_sqrt_ps(*gPacketA);
	*gPacketA = _mm256_div_ps(*gPacketA, raiz2);

	//Azul
	*bPacketA = _mm256_mul_ps(*bPacketA, *bPacketA);
	opMulB = _mm256_mul_ps(*bPacketB, *bPacketB);
	*bPacketA = _mm256_add_ps(*bPacketA, opMulB);
	*bPacketA = _mm256_sqrt_ps(*bPacketA);
	*bPacketA = _mm256_div_ps(*bPacketA, raiz2);
}

/***********************************************
 *
 * Algorithm. Image filter.
 *
 * *********************************************/
void filter(filter_args_t args)
{

	uint j;

	for (j = 0; j < args.pixelCount; j += ITEMS_PER_PACKET)
	{
		//Cargar pixeles imagen 1
		__m256 rPacketA = _mm256_loadu_ps(args.pRsrc + j);
		__m256 gPacketA = _mm256_loadu_ps(args.pGsrc + j);
		__m256 bPacketA = _mm256_loadu_ps(args.pBsrc + j);

		//Cargar pixeles imagen 2
		__m256 rPacketB = _mm256_loadu_ps(args.pRsrc2 + j);
		__m256 gPacketB = _mm256_loadu_ps(args.pGsrc2 + j);
		__m256 bPacketB = _mm256_loadu_ps(args.pBsrc2 + j);

		process(&rPacketA, &gPacketA, &bPacketA, &rPacketB, &gPacketB, &bPacketB);

		//Guardar en imagen destino
		_mm256_storeu_ps(args.pRdst + j, rPacketA);
		_mm256_storeu_ps(args.pGdst + j, gPacketA);
		_mm256_storeu_ps(args.pBdst + j, bPacketA);
	}

	// Procesa los píxeles restantes
	for (; j < args.pixelCount; j++)
	{
		args.pRdst[j] = sqrt(args.pRsrc[j] * args.pRsrc[j] + args.pRsrc2[j] * args.pRsrc2[j]) / sqrt(2);
		args.pGdst[j] = sqrt(args.pGsrc[j] * args.pGsrc[j] + args.pGsrc2[j] * args.pGsrc2[j]) / sqrt(2);
		args.pBdst[j] = sqrt(args.pBsrc[j] * args.pBsrc[j] + args.pBsrc2[j] * args.pBsrc2[j]) / sqrt(2);
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