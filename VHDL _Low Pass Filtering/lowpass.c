/* tested on Fedora 24, 64 bit machine using gcc 6.31 */


#include <string.h>
#include <stdio.h>
#include<stdlib.h>

#define MAX_HEIGHT 256
#define MAX_WIDTH 256

int temp;

typedef struct BMP{ 

	unsigned short bType;           /* Magic number for file */
	unsigned int   bSize;           /* Size of file */
	unsigned short bReserved1;      /* Reserved */
	unsigned short bReserved2;      /* ... */
	unsigned int   bOffBits;        /* Offset to bitmap data */

	unsigned int  bISize;           /* Size of info header */
	unsigned int  bWidth;          /* Width of image */
	unsigned int   bHeight;         /* Height of image */
	unsigned short bPlanes;         /* Number of color planes */
	unsigned short bBitCount;       /* Number of bits per pixel */
	unsigned int  bCompression;    /* Type of compression to use */
	unsigned int  bSizeImage;      /* Size of image data */
	int           bXPelsPerMeter;  /* X pixels per meter */
	int      	    bYPelsPerMeter;  /* Y pixels per meter */
	unsigned int   bClrUsed;        /* Number of colors used */
	unsigned int   bClrImportant;   /* Number of important colors */
}BMP;

int R[MAX_HEIGHT][MAX_WIDTH], G[MAX_HEIGHT][MAX_WIDTH], B[MAX_HEIGHT][MAX_WIDTH];
int R1[MAX_HEIGHT][MAX_WIDTH], G1[MAX_HEIGHT][MAX_WIDTH], B1[MAX_HEIGHT][MAX_WIDTH];


//void RGB2YUV();
int Read_BMP_Header(char *filename, int *h, int *w,BMP *bmp) 
{

	FILE *f;
	int *p;
	f=fopen("test.bmp","r");
	printf("\nReading BMP Header ");
	fread(&bmp->bType,sizeof(unsigned short),1,f);
	p=(int *)bmp;
	fread(p+1,sizeof(BMP)-4,1,f);
	if (bmp->bType != 19778) {
		printf("Error, not a BMP file!\n");
		return 0;
	} 

	*w = bmp->bWidth;
	*h = bmp->bHeight;
	return 1;
}

void Read_BMP_Data(char *filename,int *h,int *w,BMP *bmp)
{

	int i,j,i1,H,W,Wp,PAD;
	unsigned char *RGB;
	FILE *f;
	printf("\nReading BMP Data ");
	f=fopen(filename,"r");
	fseek(f, 0, SEEK_SET);
	fseek(f, bmp->bOffBits, SEEK_SET);
	W = bmp->bWidth;
	H = bmp->bHeight;
	printf("\nheight = %d width= %d \n",H,W);
	PAD = (3 * W) % 4 ? 4 - (3 * W) % 4 : 0;
	Wp = 3 * W + PAD;
	RGB = (unsigned char *)malloc(Wp*H *sizeof(unsigned char));
	for(i=0;i<Wp*H;i++) RGB[i]=0;
	
	fread(RGB, sizeof(unsigned char), Wp * H, f);

	i1=0;
	for (i = 0; i < H; i++) {
		for (j = 0; j < W; j++){
			i1=i*(Wp)+j*3;
			B[i][j]=RGB[i1];
			G[i][j]=RGB[i1+1];
			R[i][j]=RGB[i1+2];
		}
	}
	fclose(f);
	free(RGB);
}

///void YUV2RGB();
int write_BMP_Header(char *filename,int *h,int *w,BMP *bmp) 
{


	FILE *f;
	int *p;
	f=fopen(filename,"w");
	printf("\n Writing BMP Header ");
	fwrite(&bmp->bType,sizeof(unsigned short),1,f);
	p=(int *)bmp;
	fwrite(p+1,sizeof(BMP)-4,1,f);
	return 1;
}

void write_BMP_Data(char *filename,int *h,int *w,BMP *bmp){

	int i,j,i1,H,W,Wp,PAD;
	unsigned char *RGB;
	FILE *f;
	printf("\nWriting BMP Data\n");
	f=fopen(filename,"w");
	fseek(f, 0, SEEK_SET);
	fseek(f, bmp->bOffBits, SEEK_SET);
	W = bmp->bWidth;
	H = bmp->bHeight;
	printf("\nheight = %d width= %d ",H,W);
	PAD = (3 * W) % 4 ? 4 - (3 * W) % 4 : 0;
	Wp = 3 * W + PAD;
	RGB = (unsigned char *)malloc(Wp* H * sizeof(unsigned char));
	fread(RGB, sizeof(unsigned char), Wp * H, f);

	i1=0;
	for (i = 0; i < H; i++) {
		for (j = 0; j < W; j++){
			i1=i*(Wp)+j*3;
			RGB[i1]=B1[i][j];
			RGB[i1+1]=G1[i][j];
			RGB[i1+2]=R1[i][j];
		}
	}
	fwrite(RGB, sizeof(unsigned char), Wp * H, f);
	fclose(f);
	free(RGB);
}


int main(){

	int PERFORM;
	int h,w;
	BMP b;
	int i,j;
	BMP *bmp=&b;


	Read_BMP_Header("test.bmp",&h,&w,bmp);



	Read_BMP_Data("test.bmp",&h,&w,bmp);

	/* Low pass filtering computation
	 * */
	for(i=1;i<h-1;i++)        {
		for(j=1;j<w-1;j++) {

			R1[i][j] = (R[i-1][j-1]+R[i-1][j]+R[i-1][j+1]+
					R[i][j-1]+R[i][j]+R[i][j+1]+   
				    R[i+1][j-1]+R[i+1][j]+R[i+1][j+1])/9;

			G1[i][j] = (G[i-1][j-1]+G[i-1][j]+G[i-1][j+1]+
					G[i][j-1]+G[i][j]+G[i][j+1]+
					G[i+1][j-1]+G[i+1][j]+G[i+1][j+1])/9;

			B1[i][j] = (B[i-1][j-1]+B[i-1][j]+B[i-1][j+1]+
					B[i][j-1]+B[i][j]+B[i][j+1]+
					B[i+1][j-1]+B[i+1][j]+B[i+1][j+1])/9;
		}
	}


	write_BMP_Header("lowpass.bmp",&h,&w,bmp);
	write_BMP_Data("lowpass.bmp",&h,&w,bmp);
	printf("\n");
	return 0;
}
