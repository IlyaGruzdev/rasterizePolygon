#include "tgaimage.h"
#include<iostream>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor gray = TGAColor();
 int main(int argc, char** argv) {
	TGAImage image(1000, 1000, TGAImage::GRAYSCALE);
	
	Point2D p1 = Point2D(100.0, 400.0);
	Point2D p2 = Point2D(100.0, 700.0);
	Point2D p3 = Point2D(250.0, 800.0);
	Point2D p4 = Point2D(300.0, 750.0);
	Point2D p5 = Point2D(450.0, 850.0);
	Point2D p6 = Point2D(550.0, 700.0);
	Point2D p7 = Point2D(700.0, 700.0);
	Point2D p8 = Point2D(800.0, 500.0);
	std::vector<Point2D> v;
	v.push_back(p1);
	v.push_back(p2);
	v.push_back(p3);
	v.push_back(p4);
	v.push_back(p5);
	v.push_back(p6);
	v.push_back(p7);
	v.push_back(p8);
	
	image.rasterizePoligon(v, red);
	image.flip_vertically();
 	image.write_tga_file("output.tga");
	return 0;
}


