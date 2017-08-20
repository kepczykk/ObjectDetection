#pragma once
#ifndef BLOB
#define BLOB

#include <vector>
using namespace std;
struct point {
	int x;
	int y;
	int regionCnt;
	int equivalentReg;
};

struct region {
	int regionCnt;
	vector<point> points;
};

struct bbox {
	int width;
	int height;
	point topLeft;
};

struct blob {
	region reg;
	bbox bb;
	point centroid;
};


void drawRectangle(blob blob, unsigned char *b, int step, int color);
bbox getBbox(region reg);
vector<blob> getBlobs(vector<region> regionsAA);
vector<blob> blobAnalysis(int rows, int cols, unsigned char **image);

#endif