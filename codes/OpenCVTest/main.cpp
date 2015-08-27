// OpenCVTest.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include <stdio.h>
#include "stdafx.h"
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <string>
#include <vector>

using namespace std;



bool goalFrameChk(int ver_max_findex, int ver_max_sindex, double ver_max_first, double ver_max_second, int width, int height)
{
	double distance;
	double thres_d1, thres_d2, thres_h;

	// 골대 양 축간 거리
	distance = abs(ver_max_findex - ver_max_sindex);
	
	// threshold값
	thres_d1 = width * 0.1;
	thres_d2 = width * 0.3;
	thres_h = height * 0.05;

	// 축간 거리가 프레임 width의 10% ~ 20% 일때
	// 골대 세로축이 프레임 height의 1/5~ 1/7 일때
	if( (distance >= thres_d1 && distance <= thres_d2) 
		&& ((ver_max_first >= thres_h) && (ver_max_second >= thres_h) || (ver_max_first >= thres_h) && (ver_max_second >= thres_h)) )
			return true;
	else
		return false;
}

void init(int width, int height, int* hor_proj, int* ver_proj) 
{
	for(int i=0; i<height; ++i)
		hor_proj[i] = 0;
	for(int i=0; i<width; ++i)
		ver_proj[i] = 0;
}

//픽셀값 저장
void getPixelValue(int width, int height, unsigned char* valueOfPixel, IplImage* src_gray)
{
	
	for(int n=0;n<height;n++) {
		for(int c=0;c<width;c++) {
			if(n > height/5)
				valueOfPixel[n*width+c] = src_gray->imageData[n*width+c];
			else
				valueOfPixel[n*width+c] = '0';
		}
	}
}

void thresholding(int width, int height, unsigned char* valueOfPixel, IplImage* thrImg)
{
	int pixel;

	//이진화 수행
	for(int n=0;n<height;n++) {
		for(int c=0;c<width;c++) {
			pixel = valueOfPixel[n*width+c];
			if(pixel > 190)
				pixel = 255;
			else
				pixel = 0;
			thrImg->imageData[n*width+c] = pixel;
		}
	}
}

bool check(IplImage* src) 
{
	//이미지의 폭과 높이 정보를 가져와서 저장
	int width = src->width;
	int height = src->height;

	//grayscale로 변환을 위한 선언
	IplImage *src_gray;
	src_gray = cvCreateImage(cvSize(width,height), 8, 1);

	//gray로 변환
	cvCvtColor(src, src_gray, CV_BGR2GRAY);    

	//threshold된 이진화영상 저장할 곳
	IplImage *thrImg;
    thrImg = cvCreateImage(cvSize(width,height), 8, 1);

	//픽셀값 저장할 곳
	unsigned char* valueOfPixel = new unsigned char [width*height];

	//픽셀값 저장패
	getPixelValue(width, height, valueOfPixel, src_gray);

	//이진화
	thresholding(width, height, valueOfPixel, thrImg);

	//projection 
	int *hor_proj, *ver_proj;
	hor_proj = new int[height];
	ver_proj = new int[width];
	int hor_max_first=0, ver_max_first=0, hor_max_second=0, ver_max_second=0;  //각 프로젝션의 최대값 저장
	int hor_max_findex=0, ver_max_findex=0, hor_max_sindex=0, ver_max_sindex=0;  //각 프로젝션의 최대값 저장
	int hist_view_size = 100;  //히스토그램 막대의 최대 크기 지정

	//골대 존재 여부 체커
	bool goalFrameChecker;
	
	while(1)
	{
		init(width, height, hor_proj, ver_proj);
		
		//프로젝션 계산
		for(int i=0; i<height; ++i)
		{
			for(int j=0; j<width; ++j)
			{
				if(cvGetReal2D(thrImg, i, j) != 0) //edge 픽셀이면
				{
					hor_proj[i]++;
					ver_proj[j]++;
					if(hor_proj[i] > hor_max_first) {  //최대값 갱신
						hor_max_first = hor_proj[i];
						hor_max_findex = i;
					}
					if(ver_proj[j] > ver_max_first) {
						ver_max_first = ver_proj[j];
						ver_max_findex = j;
					}
				}  
			}
		}

		//두번째 최대값 계산i
		for(int i=0; i<height; ++i)
		{
			for(int j=0; j<width; ++j)
			{
				if(cvGetReal2D(thrImg, i, j) != 0) //edge 픽셀이면
				{
					if(i != hor_max_findex) {
						if(hor_proj[i] > hor_max_second) {  //최대값 갱신
							hor_max_second = hor_proj[i];
							hor_max_sindex = i;
						}
					}

					if( (j != ver_max_findex)  && ((j <= ver_max_findex - 5) || (j >= ver_max_findex + 5)) ) {
						if(ver_proj[j] > ver_max_second) {
							ver_max_second = ver_proj[j];
							ver_max_sindex = j;
						}
					}
				}
			}
		}  
		goalFrameChecker = goalFrameChk(ver_max_findex, ver_max_sindex, ver_max_first, ver_max_second, width, height);
		
		int key_input=cvWaitKey(20);
		//key input
		if(true)  //quit
			break;
	}

	return goalFrameChecker;
}


int main(int argc, char **argv)
{
	IplImage *img;
	int timeslice[255][2];
	int index = 0;

	CvCapture *capture;
	capture = cvCaptureFromFile("D:\\soccer.mp4");

	int fps;
	int count = 0;
	int numberOfIn = 0;

	
	if(capture)
	{
		while(1)
		{
			img = cvQueryFrame(capture);
			if(!img) break;

			cvNamedWindow("image");
			cvShowImage("image",img);
			
			
			if(count%29==0){
				
				//string filename;
				//filename = cv::format("capture%03d.bmp", count);
				//cvSaveImage(filename.c_str(),img);
				
				bool test = false;
				test = check(img);
				
				if(test){
					//printf("In");
					numberOfIn++;
					if(numberOfIn>=2 && ((count/29)-timeslice[index-1][1])<=1){
						timeslice[index-1][1] = (int)(count/29);
						printf("%d    %d\n",timeslice[index-1][0],timeslice[index-1][1]);
					}

					else if(numberOfIn>=2){
						timeslice[index][0] = (int)(count/29)-1;
						timeslice[index][1] = (int)(count/29)+1;
						printf("%d   %d\n",timeslice[index][0],timeslice[index][1]);
						index++;
						
					}
				}
				else{
					numberOfIn = 0;
					//printf("Out");
				}
			}	//29로 나눔

			//cvWaitKey(1);
			count++;

		}
	}

	cvReleaseCapture(&capture);
	
	char m[10], s[10];
	char m1[10], s1[10];
	char f[256];
	char k[256];
	char add[2000];
	char add2[2000];

	

	for(int i=0 ; i<index ; i++){
		
		itoa(timeslice[i][0]/60,m,10);
		itoa(timeslice[i][0]%60,s,10);

		itoa((timeslice[i][1]-timeslice[i][0])/60,m1,10);
		itoa((timeslice[i][1]-timeslice[i][0])%60,s1,10);
		sprintf(f,"ffmpeg -y -i D:\\soccer.mp4 -ss 00:%s:%s -t 00:%s:%s D:\\s1\\%d.mp4",m,s,m1,s1,i);

		printf("%s\n\n",f);
		system(f);
		sprintf(k,"ffmpeg -i D:\\s1\\%d.mp4 -c copy -bsf:v h264_mp4toannexb -f mpegts D:\\s1\\inter%d.ts",i,i);
		system(k);

		//(문자열 초기화)
		f[0] = '\0';
		k[0] = '\0';

		m[0] = '\0';
		s[0] = '\0';
		m1[0] = '\0';
		s1[0] = '\0';
	}


	char a[30];
	add[0] = '\0';
	for(int j=1 ; j<index ; j++){
		
		sprintf(a,"|D:\\s1\\inter%d.ts",j);
		strcat(add,a);
		printf("%s\n", add);
		a[0] = '\0';
	}


	sprintf(add2,"ffmpeg -f mpegts -i \"concat:D:\\s1\\inter0.ts%s\" -c copy -bsf:a aac_adtstoasc D:\\output.mp4",add);
	system(add2);
	
	////////////
	
	//printf("%s\n",add);
	//printf("%s\n",add2);
	system("ffmpeg -i D:\\output.mp4 -af \"volume=10dB\" D:\\outputS.mp4");


	return 0;
}