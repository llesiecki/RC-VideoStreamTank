#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <raspicam/raspicam.h>
#include <unistd.h>
#include <cstring>
#include <array>
#include <algorithm>

constexpr unsigned int HRESOLUTION = 1280;
constexpr unsigned int VRESOLUTION = 960;
constexpr unsigned int ERODE = 10;

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
		fwrite(data + width*i*3, 3, width, f);
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
	float h = 0;
	unsigned char s, v;
	
	const std::pair<unsigned char, unsigned char> minmax =
		std::minmax({rgb[0], rgb[1], rgb[2]});
	
	const unsigned char min = minmax.first;
	const unsigned char max = minmax.second;
	const unsigned char delta = max - min;
	
	//std::cout << (int)min << ' ' << (int)max << ' ' << (int)delta << '\n'; 
	
	if(delta == 0 || max == 0)
		return std::array<unsigned char, 3>();
	
	s = (int)delta * 255 / max;
	v = max;
	
	if( rgb[0] == max )
        h = ( (float)rgb[1] - rgb[2] ) / delta; // between yellow & magenta
    else if( rgb[1] == max )
        h = 2.0 + ( (float)rgb[2] - rgb[0] ) / delta;  // between cyan & yellow
    else
        h = 4.0 + ( (float)rgb[0] - rgb[1] ) / delta;  // between magenta & cyan
	
    h *= 60.0; // degrees

    if( h < 0.0 )
        h += 360.0;
	
	h *= 255.0f/360;
	
	return {h, s, v};
}
 
int main(int argc, char **argv)
{
  std::array<unsigned char, 3> hsv = rgb2hsv({0, 17, 26});

  std::cout << (int)hsv[0] << ' ' << (int)hsv[1] << ' ' << (int)hsv[2] << '\n';

  raspicam::RaspiCam camera;

  configure_camera(camera);

  size_t img_size = camera.getImageTypeSize(raspicam::RASPICAM_FORMAT_RGB);

  unsigned char *data = new unsigned char[img_size];
  unsigned char *dataRnE = new unsigned char[img_size];
  
  camera.grab();
  camera.retrieve (data, raspicam::RASPICAM_FORMAT_IGNORE);
  saveBMP(data, HRESOLUTION, VRESOLUTION, "raw.bmp");
  
  for(int y = 0; y < VRESOLUTION; y++)
    for(int x = 0; x < HRESOLUTION; x++)
    {
      index = y*HRESOLUTION*3 + x*3;
      std::array<unsigned char, 3> hsv = rgb2hsv({data[index + 2], data[index + 1], data[index]});

      if((hsv[0] < 7 || hsv[0] > 249) && hsv[1] > 0 && hsv[2] > 0)
      {
        dataRnE[index] = data[index];
        dataRnE[index + 1] = data[index + 1];
        dataRnE[index + 2] = data[index + 2];
      }
    }
  
  saveBMP(dataRnE, camera.getWidth(), camera.getHeight(), "red.bmp");
  
  delete [] data;
  delete [] dataRnE;
  
  return 0;
}
