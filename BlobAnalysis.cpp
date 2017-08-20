#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include<iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <ctime>
#include "BlobAnalysis.h";

using namespace std;


vector<blob> blobAnalysis(int rows, int cols, unsigned char **image)
{
	int i, j;

	int regionCnt = 0;

	vector<point> points;
	vector<point> whitePoints;

	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			point var = { j,i,-1,-1 };
			points.push_back(var);
		}
	}
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			if (image[i][j] == 255) {

				if (j - 1 >= 0 && i - 1 >= 0) {
					if (image[i - 1][j] == 255 && image[i][j - 1] == 255) {
						if (points[(i - 1)*cols + j].regionCnt == points[i*cols + (j - 1)].regionCnt) {
							points[i*cols + j].regionCnt = points[(i - 1)*cols + j].regionCnt;
						}
						else {
							points[i*cols + j].regionCnt = points[i*cols + (j - 1)].regionCnt;
							points[i*cols + j].equivalentReg = points[(i - 1)*cols + j].regionCnt;
						}
					}
					else if (image[i - 1][j] == 255) {
						points[i*cols + j].regionCnt = points[(i - 1)*cols + j].regionCnt;
					}
					else if (image[i][j - 1] == 255) {
						points[i*cols + j].regionCnt = points[i*cols + (j - 1)].regionCnt;
					}
					else {
						points[i*cols + j].regionCnt = regionCnt;
						regionCnt++;
					}
					whitePoints.push_back(points[i*cols + j]);
				}
			}
		}
	}

	vector<region> regions;

	for (i = 0; i < regionCnt; i++) {
		region reg;
		int flag = 0;
		for (j = 0; j < whitePoints.size(); j++) {
			if (whitePoints[j].regionCnt == i) {
				reg.points.push_back(whitePoints[j]);
				if (whitePoints[j].equivalentReg != -1) {
					reg.regionCnt = whitePoints[j].equivalentReg;
					flag = 1;
				}
				else if (flag == 0) {
					reg.regionCnt = whitePoints[j].regionCnt;
				}
			}
		}
		regions.push_back(reg);
	}

	vector<region> regionsAfterAnalysis;
	int cnt = 0;
	for (i = regions.size() - 1; i >= 0; i--) {
		if (regions[i].regionCnt != i) {
			for (j = i; j >= 0; j--) {
				if (i == regions[j].regionCnt) {
					regions[j].regionCnt = regions[i].regionCnt;
				}
			}
			regions[regions[i].regionCnt].points.insert(regions[regions[i].regionCnt].points.end(), regions[i].points.begin(), regions[i].points.end());
		}
	}

	int minSizeOfBlob = 100;

	for (i = regions.size() - 1; i >= 0; i--) {
		if (regions[i].regionCnt == i && regions[i].points.size() > minSizeOfBlob) {
			regionsAfterAnalysis.push_back(regions[i]);
			cnt++;
		}
	}

	vector<blob> blobs = getBlobs(regionsAfterAnalysis);

	return blobs;
}


void drawRectangle(blob blob, unsigned char *b, int step, int color) {
	for (int i = blob.bb.topLeft.y; i < blob.bb.topLeft.y + blob.bb.height; i++) {
		b[step * i + blob.bb.topLeft.x * 3 + color] = 255;
		b[step * i + (blob.bb.topLeft.x + blob.bb.width) * 3 + color] = 255;
	}
	for (int j = blob.bb.topLeft.x; j < blob.bb.topLeft.x + blob.bb.width; j++) {
		b[step * blob.bb.topLeft.y + j * 3 + color] = 255;
		b[step * (blob.bb.topLeft.y + blob.bb.height) + j * 3 + color] = 255;
	}
}


bbox getBbox(region reg) {
	point top = { 0,480,-1,-1 };
	point right = { 0,0,-1,-1 };
	point down = { 0,0,-1,-1 };
	point left = { 640,0,-1,-1 };
	for (int i = 0; i < reg.points.size(); i++) {
		if (top.y > reg.points[i].y) {
			top.x = reg.points[i].x;
			top.y = reg.points[i].y;
		}
		if (right.x < reg.points[i].x) {
			right.x = reg.points[i].x;
			right.y = reg.points[i].y;
		}
		if (down.y < reg.points[i].y) {
			down.x = reg.points[i].x;
			down.y = reg.points[i].y;
		}
		if (left.x > reg.points[i].x) {
			left.x = reg.points[i].x;
			left.y = reg.points[i].y;
		}
	}

	int width = right.x - left.x;
	int height = down.y - top.y;

	point topleft = { left.x, top.y, -1, -1 };

	return{ width, height, topleft };

}

point getCentroid(bbox bb) {
	return{ bb.topLeft.x + (bb.width / 2), bb.topLeft.y + (bb.height / 2), -1, -1 };
}


vector<blob> getBlobs(vector<region> regionsAA) {
	vector<blob> blobs;
	for (int i = 0; i < regionsAA.size(); i++) {
		bbox bbox = getBbox(regionsAA[i]);
		blobs.push_back({ regionsAA[i], bbox, getCentroid(bbox) });
	}
	return blobs;
}