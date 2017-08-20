#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/video.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <time.h>
#include "BlobAnalysis.h";


using namespace cv;
using namespace std;

struct t {
	blob blob;
	int hitcount;
	int misscount;
	bool active;
};

void processVideo();
Mat processFrame(Mat frame, unsigned char** backgroundFrame);
void noiseFilter(unsigned char **image, unsigned char **result, int rows, int cols);
void getBackground(unsigned char **background, Mat backgroundFrame);
void approximateMedian(unsigned char** frame, unsigned char** background, unsigned char** resultBG, int rows, int cols);
void trackingAlghorithm(int rows, int cols, unsigned char **b, unsigned char** background, unsigned char** image);
void updateBackground(int rows, int cols, unsigned char** background);


vector<t> track;		//œledzone obiekty
vector<t> dontUpdate;	//obiekty statyczne
vector<t> abandoned;	//obiekty, które wywo³a³y alarm
unsigned char **bb;		//t³o aktualizuj¹ce siê co d³u¿yszy okres czasu
int cnt = 1;			//licznik klatek


int main(int argc, char* argv[])
{
	processVideo();
	return 0;
}

void processVideo() {
	//VideoCapture cap(0);
	VideoCapture cap("test.mp4");
	Mat frame;
	Mat backgroundFrame;
	
	if (!cap.isOpened())
	{
		cout << "Cannot open the video" << endl;
	}
	double dWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double dHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	unsigned char **bg = new unsigned char*[dHeight];
	for (int i = 0; i <= dHeight; i++)
		bg[i] = new unsigned char[dWidth];

	cout << "Frame size : " << dWidth << " x " << dHeight << endl;

	cap.read(backgroundFrame);
	getBackground(bg, backgroundFrame);
	int i = 0;
	time_t start, end;
	time(&start);
	while (1)
	{
		bool bSuccess = cap.read(frame);

		if (!bSuccess)
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		processFrame(frame, bg);
		if (waitKey(30) == 27)
		{
			time(&end);
			double seconds = difftime(end, start);
			double fps = i / seconds;
			cout << "Estimated frames per second : " << fps << endl;
			cout << "esc key is pressed by user" << endl;
			getchar();
			break;
		}
		i++;
	}
}


//odejmowanie t³a
void subtr(int rows, int cols, unsigned char** background, unsigned char** image, unsigned char** b) {
	int i, j;
	int k = 3;
	double sumUp;
	for (i = 0; i< rows; i += k) {
		for (j = 0; j< cols; j += k) {
			sumUp = 0.0;
			for (int y = i; y < i + k; y++) {
				for (int x = j; x < j + k; x++) {
					b[y][x] = (abs(image[y][x] - background[y][x])) > 20 ? 255 : 0;
					sumUp += b[y][x];
				}
			}

			if (sumUp > 1800) {
				for (int y = i; y < i + k; y++) {
					for (int x = j; x < j + k; x++) {
						b[y][x] = 255;
					}
				}
			}
			else {
				for (int y = i; y < i + k; y++) {
					for (int x = j; x < j + k; x++) {
						b[y][x] = 0;
					}
				}
			}
		}
	}
}


Mat processFrame(Mat frame, unsigned char** background)
{
	int frameCntTracking = 10;		// licznik wskazuj¹cy co ile klatek algorytm sprawdza ustawienie obiektów
	int frameCntBGupdate = 5;		// licznik wskazuj¹cy co ile klatek t³o jest aktualizowane algorytmem przybli¿onej mediany
	int rows, cols;
	rows = frame.rows;
	cols = frame.cols;
	unsigned char *frameFG = (unsigned char*)(frame.data);

	Mat greyFrame;
	cvtColor(frame, greyFrame, cv::COLOR_BGR2GRAY);

	unsigned char *data = new unsigned char[rows*cols];
	unsigned char *data1 = new unsigned char[rows*cols];
	
	uchar **img = new uchar*[rows];
	for (int i = 0; i<rows; ++i)
		img[i] = new uchar[cols];
	for (int i = 0; i<rows; ++i)
		img[i] = greyFrame.ptr<uchar>(i);


	unsigned char **image = new unsigned char*[rows];
	for (int i = 0; i <= rows; i++)
		image[i] = new unsigned char[cols];

	// filtr szumów
	noiseFilter(img, image, rows, cols);


	unsigned char **b = new unsigned char*[rows];
	b[0] = new unsigned char[rows*cols];
	for (int i = 1; i < rows; i++)
		b[i] = b[i - 1] + cols;

	unsigned char **b2 = new unsigned char*[rows];
	b2[0] = new unsigned char[rows*cols];
	for (int i = 1; i < rows; i++)
		b2[i] = b2[i - 1] + cols;


	if (cnt  == 1) {
		subtr(rows, cols, background, image, b);
	}
	else {
		subtr(rows, cols, bb, image, b);
	}
	
	//œledzenie
	if (cnt == 1 || cnt % frameCntTracking == 0) {
		trackingAlghorithm(rows, cols, b, background, image);
	}

	for (int i = 0; i < abandoned.size(); i++) {
		drawRectangle(abandoned[i].blob, frameFG, frame.step, 2);
	}

	for (int q = 0; q < rows; q++)
	{
		for (int t = 0; t < cols; t++)
		{
			data[q * cols + t] = background[q][t];
		}
	}

	for (int q = 0; q < rows; q++)
	{
		for (int t = 0; t < cols; t++)
		{
			data1[q * cols + t] = b[q][t];
		}
	}

	Mat afterSub(rows, cols, CV_8UC1, data1);

	Mat matbg(rows, cols, CV_8UC1, data);

	imshow("Current background", matbg);
	imshow("Current frame", frame);
	imshow("Foreground", afterSub);

	if(cnt%frameCntBGupdate == 0)
		approximateMedian(image, background, background, rows, cols);

	for (int i = 0; i < rows; ++i) {
		delete[] image[i];
	}
	delete image;
	delete b;
	delete img;
	delete data;
	delete data1;

	cnt++;
	return afterSub;
}


//œledzenie obiektów pierwszego planu
void trackingAlghorithm(int rows, int cols, unsigned char **b, unsigned char** background, unsigned char** image) {
	vector<blob> blobs;
	blobs = blobAnalysis(rows, cols, b);

	int frameCntBBupdate = 150;		// licznik wskazuj¹cy co ile klatek t³o bb jest aktualizowane
	int dontUpdateT = 5;			// próg okreœlaj¹cy czy aktualizowaæ obszary, w których znajduj¹ siê obiekty
	int deleteT = 5;				// próg okreœlaj¹cy po ilu nietrafionych usun¹æ obiekty
	double t = 0.05;				// próg podobieñstwa dwóch blobów
	

	// usuniêcie podobnych blobów
	for (int i = 0; i < track.size(); i++) {
		for (int j = 0; j < track.size(); j++) {
			if (i != j) {
				if (((abs(track[i].blob.centroid.x - track[j].blob.centroid.x) +
					abs(track[i].blob.centroid.y - track[j].blob.centroid.y)) / (double)(track[i].blob.centroid.x + track[i].blob.centroid.y) < t) &&
					(abs(track[i].blob.reg.points.size() - track[j].blob.reg.points.size()) / (double)(track[j].blob.reg.points.size()) < t)
					) {
					if (track[i].hitcount > track[j].hitcount) {
						track.erase(track.begin() + j);
					}
					else {
						track.erase(track.begin() + i);
					}
				}
				if (track[i].blob.bb.topLeft.x <= track[j].blob.centroid.x && track[i].blob.bb.topLeft.y <= track[j].blob.centroid.y
					&& track[i].blob.bb.topLeft.x + track[i].blob.bb.width >= track[j].blob.centroid.x && track[i].blob.bb.topLeft.y + track[i].blob.bb.height >= track[j].blob.centroid.y
					) {
					track.erase(track.begin() + j);
				}
			}
		}
	}

	// sprawdzanie czy dany obiekt nadal jest obiektem statycznym
	if (!dontUpdate.empty()) {
		unsigned char **subtract = new unsigned char*[rows];
		subtract[0] = new unsigned char[rows*cols];
		for (int i = 1; i < rows; i++)
			subtract[i] = subtract[i - 1] + cols;

		subtr(rows, cols, bb, background, subtract);
		unsigned char **subtract2 = new unsigned char*[rows];
		subtract2[0] = new unsigned char[rows*cols];
		for (int i = 1; i < rows; i++)
			subtract2[i] = subtract2[i - 1] + cols;

		subtr(rows, cols, background, image, subtract2);

		for (int i = 0; i < dontUpdate.size(); i++) {
			int count = 0;
			int count2 = 0;
			for (int y = dontUpdate[i].blob.bb.topLeft.y; y < dontUpdate[i].blob.bb.topLeft.y + dontUpdate[i].blob.bb.height; y++) {
				for (int x = dontUpdate[i].blob.bb.topLeft.x; x < dontUpdate[i].blob.bb.topLeft.x + dontUpdate[i].blob.bb.width; x++) {
					if (0 == subtract[y][x]) {
						count++;
					}
					if (0 == subtract2[y][x]) {
						count2++;
					}
				}
			}
			if ((count * 100) / (dontUpdate[i].blob.bb.height*dontUpdate[i].blob.bb.width) > 50 && (count2 * 100) / (dontUpdate[i].blob.bb.height*dontUpdate[i].blob.bb.width) == 100) {
				dontUpdate.erase(dontUpdate.begin() + i);
			}
		}
		for (int i = 0; i < abandoned.size(); i++) {
			int count = 0;
			for (int y = abandoned[i].blob.bb.topLeft.y; y < abandoned[i].blob.bb.topLeft.y + abandoned[i].blob.bb.height; y++) {
				for (int x = abandoned[i].blob.bb.topLeft.x; x < abandoned[i].blob.bb.topLeft.x + abandoned[i].blob.bb.width; x++) {
					if (0 == subtract[y][x]) {
						count++;
					}
				}
			}
			if ((count * 100) / (abandoned[i].blob.bb.height*abandoned[i].blob.bb.width) > 50) {
				abandoned.erase(abandoned.begin() + i);
			}
		}
	}
	
	// algorytm œledzenia
	for (int j = 0; j < blobs.size(); j++) {
		int x = 0;
		for (int i = 0; i < track.size(); i++) {
			if (((abs(track[i].blob.centroid.x - blobs[j].centroid.x) +
				abs(track[i].blob.centroid.y - blobs[j].centroid.y)) / (double)(track[i].blob.centroid.x + track[i].blob.centroid.y)<t) &&
				(abs(track[i].blob.reg.points.size() - blobs[j].reg.points.size()) / (double)(blobs[j].reg.points.size())<t)
				) {
				x = 1;
				track[i].active = true;
				break;
			}
		}
		if (x == 0) {
			track.push_back({ blobs[j], 1 , 0, true});
		}
	}
	for (int i = 0; i < track.size(); i++) {
		for (int j = 0; j < blobs.size(); j++) {
			if (((abs(track[i].blob.centroid.x - blobs[j].centroid.x) +
				abs(track[i].blob.centroid.y - blobs[j].centroid.y)) / (double)(track[i].blob.centroid.x + track[i].blob.centroid.y)<t) &&
					(abs(track[i].blob.reg.points.size() - blobs[j].reg.points.size()) / (double)(blobs[j].reg.points.size())<t)
				) {
				track[i].active = true;
				break;
			}
			else {
				track[i].active = false;
			}
		}
	}
	for (int i = 0; i < track.size(); i++) {
		int x = 0;
		if (track[i].active == true && blobs.size() != 0) {
			track[i].hitcount++;
			track[i].misscount = 0;
			if (track[i].hitcount > dontUpdateT) {
				for (int j = 0; j < dontUpdate.size(); j++) {
					if (((abs(dontUpdate[j].blob.centroid.x - track[i].blob.centroid.x) +
						abs(dontUpdate[j].blob.centroid.y - track[i].blob.centroid.y)) / (double)(dontUpdate[j].blob.centroid.x + dontUpdate[j].blob.centroid.y)<0.08) &&
						(abs(dontUpdate[j].blob.reg.points.size() - track[i].blob.reg.points.size()) / (double)(dontUpdate[j].blob.reg.points.size())<0.08)
						) {
						x = 1;
					}
				}
				if (x == 0) {
					dontUpdate.push_back(track[i]);
				}
			}
		}
		else if (track[i].active == false){
			track[i].misscount++;
		}
		if (track[i].active == false && track[i].misscount > deleteT) {
			track.erase(track.begin() + i);
		}
	}


	if (cnt == 1 || cnt % frameCntBBupdate == 0) {
		updateBackground(rows, cols, background);
	}

	blobs.clear();
}


//aktualizacja t³a bb
void updateBackground(int rows, int cols, unsigned char** background) {
	unsigned char *data = new unsigned char[rows*cols];
	int alarmT = 30;		// próg uruchamiaj¹cy alarm dla wykrytych obiektów

	if (cnt == 1) {
		bb = new unsigned char*[rows];
		for (int i = 0; i <= rows; i++)
			bb[i] = new unsigned char[cols];

		for (int y = 0; y < rows; y++) {
			for (int x = 0; x < cols; x++) {
				bb[y][x] = background[y][x];
			}
		}
	}
	else {
		for (int y = 0; y < rows; y++) {
			for (int x = 0; x < cols; x++) {
				int f = 0;
				for (int i = 0; i < dontUpdate.size(); i++) {
					if (dontUpdate[i].blob.bb.topLeft.x <= x && dontUpdate[i].blob.bb.topLeft.y <= y
						&& dontUpdate[i].blob.bb.topLeft.x + dontUpdate[i].blob.bb.width >= x && dontUpdate[i].blob.bb.topLeft.y + dontUpdate[i].blob.bb.height >= y) {
						f = 1;
						break;
					}
				}

				if (f == 0) {
					bb[y][x] = background[y][x];
				}
			}
		}
	}

	for (int i = 0; i < track.size(); i++) {
		int f = 0;
		for (int j = 0; j < abandoned.size(); j++) {
			if (((abs(track[i].blob.centroid.x - abandoned[j].blob.centroid.x) +
				abs(track[i].blob.centroid.y - abandoned[j].blob.centroid.y)) / (double)(track[i].blob.centroid.x + track[i].blob.centroid.y) < 0.05) &&
				(abs(track[i].blob.reg.points.size() - abandoned[j].blob.reg.points.size()) / (double)(abandoned[j].blob.reg.points.size()) < 0.05)
				) {
				f = 1;
			}
		}
		if (f == 0) {
			if (track[i].active == true && track[i].hitcount > alarmT) {
				abandoned.push_back(track[i]);
			}
		}
	}
	unsigned char **b2 = new unsigned char*[rows];
	b2[0] = new unsigned char[rows*cols];
	for (int i = 1; i < rows; i++)
		b2[i] = b2[i - 1] + cols;
	subtr(rows, cols, bb, background, b2);
	

	unsigned char *data2 = new unsigned char[rows*cols];
	for (int q = 0; q < rows; q++)
	{
		for (int t = 0; t < cols; t++)
		{
			data2[q * cols + t] = bb[q][t];
		}
	}
	Mat b2mat(rows, cols, CV_8UC1, data2);
	imshow("Buffered Background", b2mat);

	for (int q = 0; q < rows; q++)
	{
		for (int t = 0; t < cols; t++)
		{
			data[q * cols + t] = b2[q][t];
		}
	}
	Mat bbMat(rows, cols, CV_8UC1, data);
	imshow("Foreground from backgrounds", bbMat);

	delete data;
	delete data2;
}

void getBackground(unsigned char **background, Mat backgroundFrame) {
	Mat greyBackground;
	int rows = backgroundFrame.rows;
	int cols = backgroundFrame.cols;
	cvtColor(backgroundFrame, greyBackground, cv::COLOR_BGR2GRAY);

	uchar **bg = new uchar*[rows];
	for (int i = 0; i<rows; ++i)
		bg[i] = new uchar[cols];
	for (int i = 0; i<rows; ++i)
		bg[i] = greyBackground.ptr<uchar>(i);

	noiseFilter(bg, background, rows, cols);

	delete bg;
}

void noiseFilter(unsigned char **image, unsigned char **result, int rows, int cols) {
	int k = 3;
	for (int i = 1; i < rows-1; i ++) {
		for (int j = 1; j < cols-1; j ++) {
			int z = 0;
			unsigned char *matrix = new unsigned char[k*k];
			for (int y = i - 1; y < i + k - 1; y++) {
				for (int x = j - 1; x < j + k - 1; x++) {
					matrix[z++] = image[y][x];
				}
			} 
			for (int j = 0; j < 5; ++j)
			{
				int min = j;
				for (int l = j + 1; l < 9; ++l)
					if (matrix[l] < matrix[min])
						min = l;
				unsigned char temp = matrix[j];
				matrix[j] = matrix[min];
				matrix[min] = temp;
			}
			result[i][j] = matrix[4];
			delete matrix;
		}
	} 
}

//algorytm przybli¿onej mediany
void approximateMedian(unsigned char** frame, unsigned char** background, unsigned char** resultBG, int rows, int cols ) {
	for (int i = 0; i < rows; i ++) {
		for (int j = 0; j < cols; j ++) {
			if (frame[i][j]>background[i][j]) {
				resultBG[i][j] = background[i][j] + 1;
			}
			if (frame[i][j]<background[i][j]) {
				resultBG[i][j] = background[i][j] - 1;
			}
		}
	}

}