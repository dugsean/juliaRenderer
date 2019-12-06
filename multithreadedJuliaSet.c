#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#define ONE_MEMORY_BLOCK 1
#define PIXEL_SIZE	4 
#define BYTE_SIZE 8
#define ONE_BYTE 1
#define LOG_2		log(2.)
#define NB_THREADS  8
#define PARCEL_WIDTH 100
#define PARCEL_HEIGHT 100
#define HEADER_SIZE 54
#define ARRAY_START 0

#define TRUE 1
#define FALSE 0

#define COUNT_RESET 0
#define COLOR_RESET 0.
#define COLOR_MAX 255.

#define UNIT 1.
#define HALF 0.5
#define SQUARE_POWER 2

#define BLUE_OFFSET 0
#define GREEN_OFFSET 1
#define RED_OFFSET 2

#define MAGIC_BMP_HEADER_NUMBER 40
#define EMPTY 0
#define ONE 1

#define PIXEL_SIZE_IN_BITS PIXEL_SIZE*BYTE_SIZE


#define TOTAL_FILE_SIZE_INDEX 2
#define HEADER_SIZE_INDEX 10
#define MAGIC_BMP_HEADER_NUMBER_INDEX 14
#define WIDTH_START_INDEX 18
#define HEIGTH_START_INDEX 22
#define ONE_INDEX 26
#define PIXEL_SIZE_IN_BITS_INDEX 28
#define BITMAP_SIZE_INDEX 34


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
char end = FALSE;

typedef struct
{
	double complex 		c;
	double 				minX;
	double 				maxX;
	double 				minY;
	double 				maxY;
	
	double				density;
	
	double				maxRadius;
	int					maxIter;
	int 				oversample;
	
} juliaInputs;

typedef struct
{
	int x0;
	int y0;
	
	int width;
	int height;
	
} parcelInput;

typedef struct
{
	unsigned char *pixels;
	int width;
	int height;
	int size;
} juliaOutputs;

typedef struct
{
	juliaInputs *julia;
	unsigned char *pixels;
	int *nextTaskIndPtr;
	int nbParcels;
	parcelInput **allParcels;
	
}threadInputs;
	
void fillParcel(parcelInput *myParcel, juliaInputs *mainInputs, unsigned char *pixels)
{
	int density = mainInputs->density;
	int totalWidth = (int)((double)density*(mainInputs->maxX - mainInputs->minX));
	//int totalHeight = (int)((double)density*(mainInputs->maxY - mainInputs->minY));
	
	double w = UNIT/((double)(density));
	//printf("hi\n");
	
	int oversample = mainInputs->oversample;
	int nbOversample = oversample*oversample;
	double increment = w /((double)oversample);

	double offset = increment*HALF;
	
	int i, ix,iy;
	
	double red, green, blue;
	
	double x,y,x0,y0,modulus;
	
	double complex z0, z1;
	
	double mu;
	double angle;
	
	for(int row = myParcel->height - 1; row >= ARRAY_START; row--) { // myParcel->height - 1 is the "index" of the last row
		y0 = mainInputs->minY+(row+myParcel->y0)*w;
        for(int column = ARRAY_START; column < myParcel->width; column++) {
			x0 = mainInputs->minX+(column+myParcel->x0)*w;
			//int p = ((column+myParcel->x0) * totalHeight + row + myParcel->y0) * PIXEL_SIZE; //ERROR
			int p = ((row+myParcel->y0) * totalWidth + column + myParcel->x0) * PIXEL_SIZE; //ERROR
			red = COLOR_RESET;
			green = COLOR_RESET;
			blue = COLOR_RESET;
			for (ix=ARRAY_START;ix<oversample;ix++)
			{
				x = x0 + offset+ix*increment;
				for (iy=ARRAY_START;iy<oversample;iy++)
				{
					y = y0 + offset+iy*increment;
					z0 = x + y*I;
					i = COUNT_RESET;
					do
					{
						z1 = z0*z0+mainInputs->c;
						z0 = z1;
						modulus = sqrt(pow(creal(z1), SQUARE_POWER) + pow(cimag(z1),SQUARE_POWER));
						i++;
					}while(i < mainInputs->maxIter && modulus<mainInputs->maxRadius);
					mu = (double)i - log(log(modulus)) / LOG_2;
					
					if (mu!=mu) //true if mu is NaN
					{
						blue += COLOR_MAX;
						green += COLOR_MAX;
						red += COLOR_MAX;
					}
					else
					{
						//<magic sauce>
						angle=(atan(mu)+M_PI/2)*2;
						blue += ((1.0/(150.0*pow(angle-6.28,2)+1))*COLOR_MAX);
						green += ((1.0/(100.0*pow(angle-5.6,2)+1))*COLOR_MAX);
						red += ((1.0/(200.0*pow(angle-5.6,2)*pow(angle-5.8,2)+1))*COLOR_MAX);
						//<\magic sauce>
					}
				}
			}
			//printf("p: %d\n",p);
			*(pixels+p+BLUE_OFFSET) = (unsigned char)(blue/nbOversample); //blue
			*(pixels+p+GREEN_OFFSET) = (unsigned char)(green/nbOversample);//green
			*(pixels+p+RED_OFFSET) = (unsigned char)(red/nbOversample);//red
        }
	}
}
	
juliaOutputs *juliaRender(juliaInputs *args)
{
	juliaOutputs *output=malloc(sizeof(juliaOutputs));
	
	
	double complexWidth = args->maxX-args->minX;
	double complexHeight = args->maxY-args->minY;
	
	int width = (int)(complexWidth*args->density);
	int height = (int)(complexHeight*args->density);
	
	
	int nbOversample = args->oversample*args->oversample;
	
	int i = 0, ix,iy;
	
	double red, green, blue;
	
	double x,y,x0,y0,modulus;
	
	double complex z0, z1;
	
	double mu;
	double angle;
	
	double w = 1.0/args->density;
	double increment = w/args->oversample;
	double offset = increment/2.0;
	
	int size = width*height*PIXEL_SIZE;
	output->width = width;
	output->height = height;
	output->size = size;
	
	output->pixels = malloc(size);
	
	for(int row = height - 1; row >= 0; row--) {
		y0 = (double)row/(double)height*complexHeight+args->minY;
        for(int column = 0; column < width; column++) {
			int p = (row * width + column) * 4;
            
			
			x0 = (double)column/(double)width*complexWidth+args->minX;
			
			
			red = 0;
			green = 0;
			blue = 0;
			for (ix=0;ix<args->oversample;ix++)
			{
				x = x0 + offset+ix*increment;
				for (iy=0;iy<args->oversample;iy++)
				{
					y = y0 + offset+iy*increment;
					z0 = x + y*I;
					i=0;
					do
					{
						z1 = z0*z0+args->c;
						z0 = z1;
						modulus = sqrt(pow(creal(z1),2.0) + pow(cimag(z1),2.0));
						i++;
					}while(i < args->maxIter && modulus<args->maxRadius);
					mu = (double)i - log(log(modulus)) / LOG_2;
					
					if (mu != mu) //true if mu is NaN
					{
						blue += 255.0; //blue
						green += 255.0;//green
						red += 255.0;//red
					}
					else
					{
						angle=(atan(mu)+M_PI/2)*2;
						blue += ((1.0/(150.0*pow(angle-6.28,2)+1))*255); //blue
						green += ((1.0/(100.0*pow(angle-5.6,2)+1))*255);//green
						red += ((1.0/(200.0*pow(angle-5.6,2)*pow(angle-5.8,2)+1))*255);//red
					}
				}
			}
			output->pixels[p + 0] = (char)(blue/nbOversample); //blue
			output->pixels[p + 1] = (char)(green/nbOversample);//green
			output->pixels[p + 2] = (char)(red/nbOversample);//red
        }
    }
	
	return output;
}

char *bmpHeader(int width, int height)
{
	int widthInd = WIDTH_START_INDEX;
	int heightInd = HEIGTH_START_INDEX;
	int size = width*height*PIXEL_SIZE;
	char * header = calloc(HEADER_SIZE, sizeof(unsigned char));
	strcpy(header, "BM");
    memset(&header[TOTAL_FILE_SIZE_INDEX],  (HEADER_SIZE + size), ONE_BYTE);//total file size
    memset(&header[HEADER_SIZE_INDEX], HEADER_SIZE, ONE_BYTE);//always 54
    memset(&header[MAGIC_BMP_HEADER_NUMBER_INDEX], 
					MAGIC_BMP_HEADER_NUMBER, ONE_BYTE);//always 40
	
	do //writes width in memory, byte by byte
	{
		memset(&header[widthInd], width, ONE_BYTE);
		width = width>>BYTE_SIZE;
		widthInd++;
	}while(width!=EMPTY && widthInd<=HEIGTH_START_INDEX);
	
   do //writes width in memory, byte by byte
	{
		memset(&header[heightInd], height, ONE_BYTE);
		height = height>>BYTE_SIZE;
		heightInd++;
	}while(height!=EMPTY && heightInd<=ONE_INDEX);
   

	memset(&header[ONE_INDEX], ONE, ONE_BYTE);//always one
    memset(&header[PIXEL_SIZE_IN_BITS_INDEX], PIXEL_SIZE_IN_BITS, ONE_BYTE);//32bit
    memset(&header[BITMAP_SIZE_INDEX], (int)size, ONE_BYTE);//bitmap size
	return header;
}

void *threadRender(void * args)
{
	threadInputs * inputPtr = (threadInputs *)args;
	int * nextTaskIndPtr = (inputPtr->nextTaskIndPtr);
	juliaInputs * mainInputs = inputPtr->julia;
	unsigned char * pixels = inputPtr->pixels;
	parcelInput **allParcels = inputPtr->allParcels;
	int nbParcels = inputPtr->nbParcels;
	int currentID;
	while(!end)
	{
		pthread_mutex_lock(&lock);
		currentID = (*nextTaskIndPtr);
		(*nextTaskIndPtr)++;
		pthread_mutex_unlock(&lock);
		if(currentID<nbParcels)
		{
			parcelInput * myParcel = (allParcels[currentID]);
			
			fillParcel(myParcel, mainInputs, pixels);
			
		}
		else
		{
			end = TRUE;
		}
	}
	return inputPtr;
}

int main()
{		
	juliaInputs * myInput = malloc(sizeof(juliaInputs));
	
	myInput->c = -0.70176 - 0.3842*I;
	myInput->minX = -1.;
	myInput->maxX = 1.;
	myInput->minY = -1.1;
	myInput->maxY = 1.09;
	myInput->density = 500;
	myInput->maxRadius = 20.;
	myInput->maxIter = 300;
	myInput->oversample = 1;
	
	
	int * nextTaskInd = calloc(ONE_MEMORY_BLOCK,sizeof(int));
	
	
	
	pthread_t thread_ids[NB_THREADS];
	
	
	int sizeX = (int)((myInput->maxX-myInput->minX)*myInput->density);
	int sizeY = (int)((myInput->maxY-myInput->minY)*myInput->density);
	
	int nbWholeParcelsX = sizeX/PARCEL_WIDTH;
	int nbWholeParcelsY = sizeY/PARCEL_HEIGHT;
	
	
	int nbWholeParcels = nbWholeParcelsX*nbWholeParcelsY;
	
	int leftoverX = sizeX-nbWholeParcelsX*PARCEL_WIDTH;
	int leftoverY = sizeY-nbWholeParcelsY*PARCEL_HEIGHT;
	
	int hasXleftover = (leftoverX!=EMPTY);
	int hasYleftover = (leftoverY!=EMPTY);
	int hasXYleftover = hasXleftover*hasYleftover;

	
	int nbPartialParcels =  hasXleftover*nbWholeParcelsY
							+ hasYleftover*nbWholeParcelsX
							+ hasXYleftover;
	
	int nbParcels = nbWholeParcels + nbPartialParcels;
	
	parcelInput * myParcels[nbParcels];
	
	int ind = 0;
	
	for (int x = ARRAY_START; x<nbWholeParcelsX;x++)
	{
		for (int y = ARRAY_START; y<nbWholeParcelsY;y++)
		{
			ind = y+x*nbWholeParcelsY;
			parcelInput * parcel = malloc(sizeof(parcelInput));
			parcel->x0 = x*PARCEL_WIDTH;
			parcel->y0 = y*PARCEL_HEIGHT;
			parcel->width = PARCEL_WIDTH;
			parcel->height = PARCEL_HEIGHT;
			myParcels[ind] = parcel;
			
		}
	}
	
	if (hasXleftover) 
	{
		for(int y = ARRAY_START; y < nbWholeParcelsY; y++)
		{
			ind++;
			parcelInput * parcel = malloc(sizeof(parcelInput));
			
			parcel->x0 = nbWholeParcelsX*PARCEL_WIDTH;
			parcel->y0 = y*PARCEL_HEIGHT;
			parcel->width = leftoverX;
			parcel->height = PARCEL_HEIGHT;
			myParcels[ind] = parcel;		
		}
	}
	
	if(hasYleftover)
	{
		
		for(int x = ARRAY_START; x < nbWholeParcelsX;x++)
		{
			ind++;
			parcelInput * parcel = malloc(sizeof(parcelInput));
			parcel->x0 = x*PARCEL_WIDTH;
			parcel->y0 = nbWholeParcelsY*PARCEL_HEIGHT;
			parcel->width = PARCEL_WIDTH;
			parcel->height = leftoverY;
			myParcels[ind] = parcel;
			

			
		}
		
	}
	
	if(hasXYleftover == TRUE)	
	{
		ind++;
		parcelInput * parcel = malloc(sizeof(parcelInput));
		parcel->x0 = nbWholeParcelsX*PARCEL_WIDTH;
		parcel->y0 = nbWholeParcelsY*PARCEL_HEIGHT;
		parcel->width = leftoverY;
		parcel->height = leftoverY;
		myParcels[ind] = parcel;
		
	}
	printf("sizeX : %d\n", sizeX);
	printf("sizeY : %d\n", sizeY);
	
	int size = sizeX*sizeY;
	printf("size: %d\n", size);
	
	
	
	unsigned char * pixels = calloc(size, PIXEL_SIZE);
	
	//printf("size : %d\n", size);
	
	
	
	for (int i = ARRAY_START; i<NB_THREADS;i++)
	{
		threadInputs * myThreadInputPtr = malloc(sizeof(threadInputs));
		myThreadInputPtr->julia = myInput;
		myThreadInputPtr->pixels = pixels;
		myThreadInputPtr->nextTaskIndPtr = nextTaskInd;
		myThreadInputPtr->allParcels = myParcels;
		myThreadInputPtr->nbParcels = nbParcels;
		
		pthread_create(&thread_ids[i], NULL, threadRender, myThreadInputPtr);
		
		
	}
	
	//pthread_mutex_unlock(&lock);
	
	for (int j = ARRAY_START; j<NB_THREADS;j++)
	{
		void * myThreadInputsPtr;
		pthread_join(thread_ids[j], &myThreadInputsPtr);
		free(myThreadInputsPtr);
	}
	
	char *header ;

	header = bmpHeader(sizeX,sizeY);
	
	FILE *fout = fopen("img.bmp", "wb");
	
	fwrite(header, sizeof(char), HEADER_SIZE, fout);
    
	fwrite(pixels, sizeof(char), size*PIXEL_SIZE, fout);
	
	free(header);
    free(pixels);
    fclose(fout);
	
	return FALSE;
}
