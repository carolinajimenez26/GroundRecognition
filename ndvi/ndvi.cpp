#include<iostream>
#include<stdio.h>
#include<malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include<opencv2/opencv.hpp>
#include <math.h>
#include <time.h>
#include <fstream>
#include <sstream>

using namespace std;
using namespace cv;

#define dbg(x) cout << #x << ": " << x << endl

#define RADIANCE_MULT_BAND_3 0.62165
#define RADIANCE_ADD_BAND_3 -5.62165

#define RADIANCE_MULT_BAND_4 0.96929
#define RADIANCE_ADD_BAND_4 -6.06929

#define REFLECTANCE_MULT_BAND_3 0.0012936
#define REFLECTANCE_ADD_BAND_3 -0.011698

#define REFLECTANCE_MULT_BAND_4 0.0028720
#define REFLECTANCE_ADD_BAND_4 -0.017983

#define theta_SE 65.46088663

#define BLUE 0
#define GREEN 1
#define RED 2

void saveTime(double elapsedTime, string fileName){
	FILE *stream;
	stream = fopen("ndvi_time.txt", "a");
	fprintf(stream, "%f\n", elapsedTime);
	fclose(stream);
}

double getRadiance(int grayPixel, int band){
	if (grayPixel < 5)
		return 0;

	if (band == 3)
		return grayPixel * RADIANCE_MULT_BAND_3 + RADIANCE_ADD_BAND_3;
	return grayPixel * RADIANCE_MULT_BAND_4 + RADIANCE_ADD_BAND_4;
}

double getReflectance(int pixel, int band){
	if (pixel < 5)
		return 0;

	double theta = (theta_SE * M_PI) / 180;
	if (band == 3)
		return (REFLECTANCE_MULT_BAND_3 * pixel + REFLECTANCE_ADD_BAND_3) / sin(theta);
	return (REFLECTANCE_MULT_BAND_4 * pixel + REFLECTANCE_ADD_BAND_4) / sin(theta);
}

void setRadiance(unsigned char *data, int const &height, int const &width, int const &band){

	double value = 0.0;
	int pos;
	for (int row = 0; row < height; row++){
		for (int col = 0; col < width; col++){
			pos = row*width+col;
			value = getRadiance(data[pos], band);
			data[pos] = value;

		}
	}
}

void setReflectance(unsigned char *data, int const &height, int const &width, int const &band){

	double value = 0.0;
	int pos;
	for (int row = 0; row < height; row++){
		for (int col = 0; col < width; col++){
			pos = row*width+col;
			value = getReflectance((int)data[pos], band);
			data[pos] = (double)value;

		}
	}
}

void setHighGreen(unsigned char *image, int pos){
	image[pos + BLUE] = 0;
	image[pos + GREEN] = 207;
	image[pos + RED] = 35;
}

void setMediumGreen(unsigned char *image, int pos){
	image[pos + BLUE] = 0;
	image[pos + GREEN] = 236;
	image[pos + RED] = 39;
}

void setLowGreen(unsigned char *image, int pos){
	image[pos + BLUE] = 74;
	image[pos + GREEN] = 231;
	image[pos + RED] = 100;
}

void setYellow(unsigned char *image, int pos){
	image[pos + BLUE] = 106;
	image[pos + GREEN] = 251;
	image[pos + RED] = 236;
}

void setBlue(unsigned char *image, int pos){
	image[pos + BLUE] = 252;
	image[pos + GREEN] = 69;
	image[pos + RED] = 83;
}

void setHighBlue(unsigned char *image, int pos){
	image[pos + BLUE] = 191;
	image[pos + GREEN] = 0;
	image[pos + RED] = 14;
}

void setBlack(unsigned char *image, int pos){
	image[pos + BLUE] = 0;
	image[pos + GREEN] = 0;
	image[pos + RED] = 0;
}

void normalize(double const &min, double const &max, double *image, int const &height, int const &width, unsigned char *out){

	int posGray, posResult;
	double value = 0.0;
	for (int row = 0; row < height; row++){
		for (int col = 0; col < width; col++){
			posGray = (row*width)+col;
			posResult = ((row*width)+col)*3;
			/*
			double I = image[posGray];
			double aux = I - min;
			double value =  aux*(255.0/(max-min));
			*/
			value = image[posGray];
			if (value > 0.8)
				setHighGreen(out, posResult);
			else if (value >= 0.6 && value <= 0.8)
				setMediumGreen(out, posResult);
			else if (value >= 0.2 && value < 0.6)
				setLowGreen(out, posResult);
			else if (value > 0 && value < 0.2)
				setYellow(out, posResult);
			else if (value > -0.5 && value < 0)
				setBlue(out, posResult);
			else if (value <= -0.5)
				setHighBlue(out, posResult);
			else if (value == 0)
				setBlack(out, posResult);

		}
	}
}

void setNDVI(unsigned char *shortWave, unsigned char *redVisible, unsigned char *result, int const &height, int const &width){

	int posGray;
	double minValue = -1.0, maxValue = 1.0;
	double value = 0.0;
	int size = sizeof(double)*width*height;
	double *temp;
	temp = (double*)malloc(size);

	int i, posResult;
	for (int row = 0; row < height; row++){
		for (int col = 0; col < width; col++){
			posGray = (row*width)+col;
			double aux1 = getReflectance(shortWave[posGray], 4);
			double aux2 = getReflectance(redVisible[posGray], 3);

			if (aux1 + aux2 != 0){
				value = (aux1 - aux2) / (aux1 + aux2);
				temp[posGray] = (double)value;
				//cout << "NDVI: " <<(double)value << endl;
			}else {
				temp[posGray] = (double)redVisible[posGray];
				//cout << "RED: " <<(double)redVisible[posGray] << endl;
			}
			/*
			if (temp[posGray] > maxValue)
				maxValue = temp[posGray];
			if (temp[posGray] < minValue)
				minValue = temp[posGray];
			*/
		}
	}

	normalize(-1, 1, temp, height, width, result);
	free(temp);

}

int main(int argc, char **argv)
{
	if (argc != 3){
		cout << "Missing parameters" << endl;
		exit(1);
	}

	clock_t start, end;
	double time_used;
	start = clock();

	unsigned char *shortWave;
	unsigned char *redVisible;
	char* redVisiblePath = argv[1];
	char* shortWavePath = argv[2];

	Mat shortWaveMat, redVisibleMat;
	redVisibleMat = imread(redVisiblePath, 0);
	shortWaveMat = imread(shortWavePath, 0);
    Size s = redVisibleMat.size();
    int width = s.width;
    int height = s.height;

    if (!shortWaveMat.data || !redVisibleMat.data){
    	cout << "error loading image" << endl;
    	exit(1);
    }


    shortWave = shortWaveMat.data;
    redVisible = redVisibleMat.data;

    if (!shortWaveMat.data || !redVisibleMat.data){
    	cout << "error loading image" << endl;
    	exit(1);
    }

    setRadiance(redVisible, height, width, 3);
    setRadiance(shortWave, height, width, 5);

    //--------------------- for debug -------------
    Mat redVisibleRadiance;
    redVisibleRadiance.create(height,width,CV_8UC1);
    redVisibleRadiance.data = redVisible;
    imwrite("./redVisibleRadiance.png", redVisibleRadiance);

    Mat shortWaveRadiance;
    shortWaveRadiance.create(height,width,CV_8UC1);
    shortWaveRadiance.data = shortWave;
    imwrite("./NIRradiance.png", shortWaveRadiance);

    //------------------------------------------------

    int size = sizeof(unsigned char)*width*height*3;
    unsigned char* result;
    result = (unsigned char*)malloc(size);

    setNDVI(shortWave, redVisible, result, height, width);
    Mat imageResult;
    imageResult.create(height,width,CV_8UC3);
    imageResult.data = result;

    imwrite("./NDVIimage.jpg", imageResult);

		end = clock();

		time_used = ((double) (end - start))/CLOCKS_PER_SEC;
		saveTime(time_used, "ndvi_time.txt");

    //Se libera memoria
    free(result);

	return 0;
}
