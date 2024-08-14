#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
#include <math.h>
#include "tgaimage.h"
#include <algorithm>

// TGAImage::TGAImage() : data(NULL), width(0), height(0), bytespp(0) {
//}
typedef cv::Point3_<uint8_t> Pixel; 

TGAImage::TGAImage(int w, int h, int bpp) : width(w), height(h), bytespp(bpp) {
	img = cv::Mat::zeros(height, width, CV_8UC(bpp)); 
}

TGAImage::TGAImage(const TGAImage &img) {
	width = img.width;
	height = img.height;
	bytespp = img.bytespp;
	this->img = Mat(width, height,  CV_8UC(bytespp));
	this->img = img.img.clone();
}


TGAImage & TGAImage::operator =(const TGAImage &img) {
	if (this != &img) {
		width  = img.width;
		height = img.height;
		bytespp = img.bytespp;
		this->img = Mat(width, height,  CV_8UC(bytespp));
		this->img = img.img.clone();
	}
	return *this;
}

bool TGAImage::read_tga_file(const char *filename) {
	std::ifstream in;
	in.open (filename, std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		in.close();
		return false;
	}
	TGA_Header header;
	in.read((char *)&header, sizeof(header));
	if (!in.good()) {
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	width   = header.width;
	height  = header.height;
	bytespp = header.bitsperpixel>>3;
	if (width<=0 || height<=0 || (bytespp!=GRAYSCALE && bytespp!=RGB && bytespp!=RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}
	unsigned long nbytes = bytespp*width*height;
	img = Mat(height, width, CV_8UC(bytespp));
	if (3==header.datatypecode || 2==header.datatypecode) {
		in.read(reinterpret_cast<char*>(img.data), nbytes);
		if (!in.good()) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	} else if (10==header.datatypecode||11==header.datatypecode) {
		if (!load_rle_data(in)) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	} else {
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}
	if (!(header.imagedescriptor & 0x20)) {
		flip_vertically();
	}
	if (header.imagedescriptor & 0x10) {
		flip_horizontally();
	}
	std::cerr << width << "x" << height << "/" << bytespp*8 << "\n";
	in.close();
	return true;
}

bool TGAImage::load_rle_data(std::ifstream &in) {
  unsigned long pixelcount = width * height;
  unsigned long currentpixel = 0;
  TGAColor colorbuffer;

  for (int y = 0; y < height; ++y) {
    unsigned char* rowPtr = img.ptr<unsigned char>(y); 
    int currentByte = 0; // Индекс в строке

    while (currentpixel < pixelcount && currentByte < width * bytespp) {
      unsigned char chunkheader = 0;
      chunkheader = in.get();
      if (!in.good()) {
        std::cerr << "an error occured while reading the data\n";
        return false;
      }

      if (chunkheader < 128) { 
        // Неповторяющиеся данные
        chunkheader++;
        for (int i = 0; i < chunkheader; ++i) {
          in.read(reinterpret_cast<char*>(colorbuffer.raw), bytespp);
          if (!in.good()) {
            std::cerr << "an error occured while reading the header\n";
            return false;
          }
          for (int t = 0; t < bytespp; ++t) {
            rowPtr[currentByte++] = colorbuffer.raw[t]; 
          }

          currentpixel++;
          if (currentpixel > pixelcount) {
            std::cerr << "Too many pixels read\n";
            return false;
          }
        }
      } else { 
        // Повторяющиеся данные
        chunkheader -= 127;
        in.read(reinterpret_cast<char*>(colorbuffer.raw), bytespp);
        if (!in.good()) {
          std::cerr << "an error occured while reading the header\n";
          return false;
        }

        for (int i = 0; i < chunkheader; ++i) {
          for (int t = 0; t < bytespp; ++t) {
            rowPtr[currentByte++] = colorbuffer.raw[t];
          }
          currentpixel++;
          if (currentpixel > pixelcount) {
            std::cerr << "Too many pixels read\n";
            return false;
          }
        }
      }
    }
  }

  return true;
}


bool TGAImage::write_tga_file(const char *filename, bool rle) {
	unsigned char developer_area_ref[4] = {0, 0, 0, 0};
	unsigned char extension_area_ref[4] = {0, 0, 0, 0};
	unsigned char footer[18] = {'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'};
	std::ofstream out;
	out.open (filename, std::ios::binary);
	if (!out.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGA_Header header;
	memset((void *)&header, 0, sizeof(header));
	header.bitsperpixel = bytespp<<3;
	header.width  = width;
	header.height = height;
	header.datatypecode = (bytespp==GRAYSCALE?(rle?11:3):(rle?10:2));
	header.imagedescriptor = 0x20; // top-left origin
	out.write((char *)&header, sizeof(header));
	if (!out.good()) {
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle) {
		size_t size = img.total() * img.elemSize();
  	unsigned char* mas = new unsigned char[size];
	  memcpy(mas, img.data, size);
		out.write((char *)mas, size);
		if (!out.good()) {
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	} else {
		if (!unload_rle_data(out)) {
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	out.write((char *)developer_area_ref, sizeof(developer_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char *)extension_area_ref, sizeof(extension_area_ref));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char *)footer, sizeof(footer));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();
	return true;
}

// TODO: it is not necessary to break a raw chunk for two equal pixels (for the matter of the resulting size)
bool TGAImage::unload_rle_data(std::ofstream &out) {
  const unsigned char max_chunk_length = 128;  unsigned long npixels = width*height;
  unsigned long curpix = 0;  while (curpix<npixels) {
    unsigned long chunkstart = curpix*bytespp;    unsigned long curbyte = curpix*bytespp;
    unsigned char run_length = 1;    bool raw = true;
    while (curpix+run_length<npixels && run_length<max_chunk_length) {  
			bool succ_eq = true;
     for (int t = 0; succ_eq && t < bytespp; t++) {
		int row1 = curbyte / bytespp / width;
		int col1 = (curbyte / bytespp) % width * bytespp + t;
		int row2 = (curbyte + bytespp) / bytespp / width;
		int col2 = ((curbyte + bytespp) / bytespp) % width * bytespp + t;

  	succ_eq = (img.at<uchar>(row1, col1) == img.at<uchar>(row2, col2));
		}
 
			curbyte += bytespp;
      if (1==run_length) {        raw = !succ_eq;
      }      if (raw && succ_eq) {
        run_length--;        break;
      }      if (!raw && !succ_eq) {
        break;
      }      run_length++;
    }    curpix += run_length;
    out.put(raw?run_length-1:run_length+127);    if (!out.good()) {
      std::cerr << "can't dump the tga file\n";      return false;
    }    unsigned char* data = img.data + chunkstart;
    out.write(reinterpret_cast<char*>(data), (raw?run_length*bytespp:bytespp));    if (!out.good()) {
      std::cerr << "can't dump the tga file\n";     
			 return false;
    }  }
  return true;
}

TGAColor TGAImage::get(int x, int y) {
	if (img.empty() || x<0 || y<0 || x>=width || y>=height) {
		return TGAColor();
	}
	unsigned char* rowPtr = img.ptr<unsigned char>(y);

  int offset = x * img.channels(); 
	return TGAColor(rowPtr + offset, bytespp);
}

bool TGAImage::set(int x, int y, TGAColor c) {
	if (img.empty() || x<0 || y<0 || x>=width || y>=height) {
		return false;
	}
	unsigned char* rowPtr = img.ptr<unsigned char>(y);

    // Вычисляем смещение для нужного пикселя
  int offset = x * img.channels(); 

    // Копируем данные цвета в Mat
  for (int i = 0; i < bytespp; ++i)
	{
    rowPtr[offset + i] = c.raw[i];
  }
	return true;
}

int TGAImage::get_bytespp() {
	return bytespp;
}

int TGAImage::get_width() {
	return width;
}

int TGAImage::get_height() {
	return height;
}

bool TGAImage::flip_horizontally() {
	if (img.empty()) return false;
	int half = width>>1;
	for (int i=0; i<half; i++) {
		for (int j=0; j<height; j++) {
			TGAColor c1 = get(i, j);
			TGAColor c2 = get(width-1-i, j);
			set(i, j, c2);
			set(width-1-i, j, c1);
		}
	}
	return true;
}

bool TGAImage::flip_vertically() {
  if (img.empty()) {
    return false;
  }

  cv::Mat flippedImg; 
  cv::flip(img, flippedImg, 0); // 0 - переворачивание по вертикали
  img = flippedImg.clone();    // Копируем данные в img

  return true;
}

unsigned char* TGAImage::buffer() {
  return img.data; 
}

void TGAImage::clear() {
  img.setTo(cv::Scalar::all(0)); 
}

bool TGAImage::scale(int w, int h) {
  if (w <= 0 || h <= 0 || img.empty()) {
    return false;
  }

  cv::Mat resizedImg;
  cv::resize(img, resizedImg, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);

  img = resizedImg;
  width = img.cols; // Обновляем ширину и высоту
  height = img.rows;

  return true;
}

void TGAImage::line(int x0, int y0, int x1, int y1, TGAColor color)
{
	bool steep = false;
    if (std::abs(x0-x1)<std::abs(y0-y1)) { // if the line is steep, we transpose the image
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0>x1) { // make it left-to-right
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    for (int x=x0; x<=x1; x++) {
        float t = (x-x0)/(float)(x1-x0);
        int y = y0*(1-t) + y1*t;
				if(y1 == y0)
					y = y1;
        if (steep) {
            set(y, x, color); // if transposed, de-transpose
        } else {
            set(x, y, color);
        }
    }
}
void TGAImage::rasterizePoligon(std::vector<Point2D> &vec, TGAColor color)
{
	std::cout<<"rasterize"<<std::endl;
	for (size_t pointIndex = 0; pointIndex < (vec.size()-1); pointIndex++)
		{
			line(vec[pointIndex].x, vec[pointIndex].y, vec[pointIndex+1].x, vec[pointIndex+1].y, color);
		}
	if(vec.size() <= 1)
		return;
	double startY = height;
	double endY = 0.0;
	for (size_t i = 0; i<vec.size(); i++)
	{
		if(vec[i].y < startY)
			startY = vec[i].y;
		if(vec[i].y > endY)
			endY = vec[i].y;
	}
	std::cout<<"start y: "<<startY<<std::endl;
	std::cout<<"end y: "<<endY<<std::endl;
	for (int y = startY; y <= endY; y++)
	{
		std::vector<Point2D> crossingPoints = {};
		for (size_t pointIndex = 0; pointIndex < vec.size(); pointIndex++)
		{
			Point2D p0 = Point2D(0.0, double(y));
			Point2D p1 = Point2D(double(width), double(y));
			Point2D p2 = vec[pointIndex];
			Point2D p3 = vec[(pointIndex + 1)%vec.size()];
			Point2D points[4] = {p0, p1, p2, p3};
			auto point = crossingPoint(points);
			if(point.x >= std::min(p3.x, p2.x) && point.x <= std::max(p3.x, p2.x) && point.x != -1 && point.y != -1)
			{
				crossingPoints.push_back(point);
			}
		}
		std::sort(crossingPoints.begin(), crossingPoints.end(), [](Point2D p0, Point2D p1) 
		{
			return p0.x < p1.x;
		});
		std::cout<<"point size: "<<crossingPoints.size()<<std::endl;

		for(size_t i = 0; i < crossingPoints.size() - 1; i++)
		{
				double x0 = crossingPoints[i].x;
				double x1 = crossingPoints[i+1].x;
				Point2D checkPoint =  Point2D(x0 + (x1 - x0)/2, y);
				bool isInside = isInsidePoint(checkPoint, vec);
				if(isInside)
					line(x0, y, x1, y, color);
		}
	}
	
}
Point2D TGAImage::crossingPoint(Point2D line[4])
{
	Point2D p00 = line[0];
	Point2D p01 = line[1];
	Point2D p10 = line[2];
	Point2D p11 = line[3];
	double itersectionX = -1.0;
	double itersectionY = -1.0;
	if (p11.x == p10.x && p01.x == p00.x)
	{
		if(p01.x == p11.x)
			return Point2D(p11.x, std::max(p10.y, p11.y));
		return Point2D(-1.0, -1.0);
	}
	if(p01.x == p00.x)
	{
		itersectionX = p01.x;
		double k11 = (p11.y - p10.y)/double(p11.x - p10.x);
		double b11 = (p10.y*p11.x - p10.x*p11.y)/double(p11.x - p10.x);
		itersectionY = k11*itersectionX + b11;
	}
	else if(p11.x == p10.x)
	{
		itersectionX = p11.x;
		double k00 = (p01.y - p00.y)/double(p01.x - p00.x);
		double b00 = (p00.y*p01.x - p00.x*p01.y)/double(p01.x - p00.x);
		itersectionY = k00*itersectionX + b00;
	}
	else
	{
		double b0 = (p00.y*p01.x - p00.x*p01.y)/double(p01.x - p00.x);
		double b1 = (p10.y*p11.x - p10.x*p11.y)/double(p11.x - p10.x);
		double k0 = (p01.y - p00.y)/double(p01.x - p00.x);
		double k1 = (p11.y - p10.y)/double(p11.x - p10.x);
		itersectionX = (b1 - b0)/(k0 - k1);
		itersectionY = k1*itersectionX + b1;
	}
	if(itersectionX < 0 || itersectionX > width || itersectionY < 0 || itersectionY > height)
			return Point2D(-1.0, -1.0);
	return Point2D(itersectionX, itersectionY);
	
}
bool TGAImage::isInsidePoint(Point2D point, std::vector<Point2D> &points)
{
	double inX = point.x;
	double inY = point.y;
	Point2D p0 = Point2D(inX, 0.0);
	Point2D p1 = Point2D(inX, double(height));
	size_t crossingCount = 0;
	for(size_t i = 0; i < points.size(); i++)
	{
		Point2D pointVector[4] = {p0, p1, points[i], points[(i+1)%points.size()]};
		Point2D crossPoint = crossingPoint(pointVector);
		if(crossPoint.x != -1 && crossPoint.y != -1)
			if(crossPoint.x >= std::min(points[i].x, points[(i+1)%points.size()].x) && point.x <= std::max(points[i].x, points[(i+1)%points.size()].x) && crossPoint.y <= inY)
				crossingCount++;
	}
	std::cout<<"x: "<<inX<<" y: "<<inY<<std::endl;
	std::cout<<crossingCount<<std::endl;
	if(crossingCount%2 == 0)
		return false;
	return true;
}
