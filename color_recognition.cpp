#include <fstream>
#include <iostream>
#include <raspicam/raspicam.h>
#include <unistd.h>
#include <cstring>
#include <array>
#include <algorithm>
#include <cmath>

constexpr unsigned int HRESOLUTION = 1280;
constexpr unsigned int VRESOLUTION = 960;

void saveBMP(unsigned char *data, unsigned int width, unsigned int height, std::string filename)
{
	unsigned int filesize = 54 + 3 * width * height;
	// disable padding
	#pragma pack(push, 1)
	// expected size is 54 bytes
	struct BMP_Header
	{
		// file header, size = 14
		uint8_t b;
		uint8_t m;
		uint32_t filesize;
		uint32_t dummy_0;
		uint8_t magic_number;
		uint8_t dummy_1[3];
		// info header, size = 40
		uint8_t s;
		uint8_t dummy_2[3];
		uint32_t width;
		uint32_t height;
		uint8_t o;
		uint8_t dummy_3;
		uint8_t l;
		uint8_t dummy_4[25];
	};
	#pragma pack(pop)
	
	BMP_Header header;
	// set header bytes to 0
	memset(&header, 0, sizeof(BMP_Header));
	// fill header
	header.b = 'B';
	header.m = 'M';
	header.filesize = filesize;
	header.magic_number = 54;
	// fill info
	header.s = 40;
	header.width = width;
	header.height = height;
	header.o = 1;
	header.l = 24;
	// remove the old image
	system(("rm -f " + filename).c_str());
	// write the data
	FILE* f = fopen(filename.c_str(), "wb");
	fwrite(&header, 1, sizeof(BMP_Header), f);
	for(int i = height; i >= 0; i--)
		fwrite(data + width * i * 3, 3, width, f);
	fclose(f);
}

void configure_camera(raspicam::RaspiCam& cam)
{
	cam.setRotation(180);
	cam.setCaptureSize(HRESOLUTION, VRESOLUTION);
	
	// Set image brightness [0,100]
	cam.setBrightness(50);
	
	//Set image sharpness (-100 to 100)
	cam.setSharpness(0);
	
	//Set image contrast (-100 to 100)
	cam.setContrast(0);
	
	//Set capture ISO (100 to 800)
	cam.setISO(800);
	
	//Set image saturation (-100 to 100)
	cam.setSaturation(0);
	
	/* Set cmaera white balance, available options:
		raspicam::RASPICAM_AWB_OFF
		raspicam::RASPICAM_AWB_AUTO
		raspicam::RASPICAM_AWB_SUNLIGHT
		raspicam::RASPICAM_AWB_CLOUDY
		raspicam::RASPICAM_AWB_SHADE
		raspicam::RASPICAM_AWB_TUNGSTEN
		raspicam::RASPICAM_AWB_FLUORESCENT
		raspicam::RASPICAM_AWB_INCANDESCENT
		raspicam::RASPICAM_AWB_FLASH
		raspicam::RASPICAM_AWB_HORIZON
	*/
	cam.setAWB(raspicam::RASPICAM_AWB_FLUORESCENT);
	
	if(!cam.open())
	{
		throw std::runtime_error("Error opening camera.\n");
	}
	
	// Give some time for the camera to stabilize
	sleep(1);
}

std::array<unsigned char, 3> rgb2hsv(const std::array<unsigned char, 3> rgb)
{
	float h, s, v;
	
	const std::pair<unsigned char, unsigned char> minmax =
		std::minmax({rgb[0], rgb[1], rgb[2]});
	
	const float min = minmax.first;
	const float max = minmax.second;// float <---> unsigned char
	const float delta = max - min;
	
	if(delta == 0.0f || max == 0.0f)
		return std::array<unsigned char, 3>();
	
	s = delta * 255.0f / max;
	v = max;
	
	if (rgb[0] == max)
		h = (rgb[1] - rgb[2]) / delta; // between yellow & magenta
	else if (rgb[1] == max)
		h = 2.0f + (rgb[2] - rgb[0]) / delta;  // between cyan & yellow
	else
		h = 4.0f + (rgb[0] - rgb[1]) / delta;  // between magenta & cyan
	
	h *= 60.0f; // degrees

	if( h < 0.0f )
		h += 360.0f;
	
	h *= 255.0f / 360.0f;
	
	return {h, s, v};
}

void treshold_image(unsigned char *data, unsigned char *output, float hue, float tolerance)
{	
	unsigned char* in = data;
	unsigned char* out = output;
	
	for(int y = 0; y < VRESOLUTION; ++y)
	{
		for(int x = 0; x < HRESOLUTION; ++x)
		{
			std::array<unsigned char, 3> hsv = rgb2hsv({*(in + 2), *(in + 1), *in});
			
			const float diff = std::abs(hsv[0] - hue);
			
			if((diff < tolerance || diff > 255.0f - tolerance) && hsv[1] > 150 && hsv[2] > 30)
			{
				*out = 255;
			}
			else
			{
				*out = 0;
			}
			
			in += 3;
			++out;
		}
	}
}

void fast_erode(unsigned char *data, unsigned char *output, size_t step)
{
	for(int y = step; y < VRESOLUTION - step; y += step)
	{
		for(int x = step; x < HRESOLUTION - step; x += step)
		{
			size_t index = y * HRESOLUTION + x;
			
			size_t index1 = (y - step)*HRESOLUTION + (x - step);
			size_t index2 = (y - step)*HRESOLUTION + (x + step);
			size_t index3 = (y + step)*HRESOLUTION + (x - step);
			size_t index4 = (y + step)*HRESOLUTION + (x + step);
			
			output[index] = data[index] && data[index1] && data[index2] && data[index3] && data[index4];
			output[index] *= 255;
		}
	}
}

void convert_8_to_24_bit(unsigned char *in, unsigned char *out, unsigned char *color_source = nullptr)
{
	for(int y = 0; y < VRESOLUTION; ++y)
	{
		for(int x = 0; x < HRESOLUTION; ++x)
		{
			size_t index = y * HRESOLUTION * 3 + x * 3;
			if(color_source != nullptr)
			{
				if(in[y * HRESOLUTION + x] != 0)
				{
					out[index] = color_source[index];
					out[index + 1] = color_source[index + 1];
					out[index + 2] = color_source[index + 2];
				}
				else
				{
					out[index] = 0;
					out[index + 1] = 0;
					out[index + 2] = 0;
				}
			}
			else
			{
				out[index] = in[y * HRESOLUTION + x];
				out[index + 1] = 0;
				out[index + 2] = 0;
			}
		}
	}
}
 
int main(int argc, char **argv)
{
	raspicam::RaspiCam camera;
	
	configure_camera(camera);
	
	size_t img_size = camera.getImageTypeSize(raspicam::RASPICAM_FORMAT_RGB);
	
	unsigned char *data = new unsigned char[img_size];
	unsigned char *bmp_buffer = new unsigned char[img_size];
	unsigned char *buffer1 = new unsigned char[VRESOLUTION * HRESOLUTION];
	unsigned char *buffer2 = new unsigned char[VRESOLUTION * HRESOLUTION];
	
	camera.grab();
	camera.retrieve(data, raspicam::RASPICAM_FORMAT_IGNORE);
	
	saveBMP(data, HRESOLUTION, VRESOLUTION, "raw.bmp");
	
	// pick red color (hue = 3, tolerance = 2)
	// from 'data' and save it to the buffer1
	treshold_image(data, buffer1, 3, 2);
	
	convert_8_to_24_bit(buffer1, bmp_buffer, data);
	saveBMP(bmp_buffer, camera.getWidth(), camera.getHeight(), "red.bmp");
	
	// erode the buffer1 with setep = 3px
	// and save the result into buffer2
	fast_erode(buffer1, buffer2, 3);
	
	delete[] data;
	delete[] buffer1;
	delete[] buffer2;
	
	return 0;
}
