/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 * Open source Automated License Plate Recognition [http://www.openalpr.com]
 *
 * This file is part of OpenALPR.
 *
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <sys/time.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "tclap/CmdLine.h"
#include "support/filesystem.h"
#include "support/timing.h"
#include "support/platform.h"
#include "video/videobuffer.h"
#include "motiondetector.h"
#include "alpr.h"

using namespace alpr;

const std::string MAIN_WINDOW_NAME = "ALPR main window";

const bool SAVE_LAST_VIDEO_STILL = false;
const std::string LAST_VIDEO_STILL_LOCATION = "/tmp/laststill.jpg";
const std::string WEBCAM_PREFIX = "/dev/video";
//const std::string DEFDIR = "/var/lib/openalpr/plateimages/";
const std::string DEFDIR = "/home/srv/work/plateimages/";
const std::string LOGDIR = "/home/srv/work/logs/";
MotionDetector motiondetector;
bool do_motiondetection = true;

/** Function Headers */
bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson);
bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson, std::string fileName);
bool is_supported_image(std::string image_file);
std::string fname();
std::string fnameToLat(std::string fname);
void process_dir(std::string dir, Alpr alpr, bool outputJson);

bool measureProcessingTime = false;
std::string templatePattern;

// This boolean is set to false when the user hits terminates (e.g., CTRL+C )
// so we can end infinite loops for things like video processing.
bool program_active = true;

static int totalCount = 0;
static int detectedCount = 0;
static int additionalDetectedCount = 0;

std::ofstream logDetected;
std::ofstream logNotDetected;

int main( int argc, const char** argv )
{
	timespec startTime;
	getTimeMonotonic(&startTime);

  std::vector<std::string> filenames;
  std::string configFile = "";
  bool outputJson = false;
  int seektoms = 0;
  bool detectRegion = false;
  std::string country;
  int topn;
  bool debug_mode = false;
  bool save = false;

  struct tm *tm;
  char buf[200];
//  /* convert time_t to broken-down time representation */
//  tm = localtime(&t);
//  /* format time days.month.year hour:minute:seconds */
//  strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", ts.tv_nsec);
//
//  printf("%lld.%.9ld", (long long)ts.tv_sec, ts.tv_nsec)
//  printf("Date: %s %ldns\n", p, ts.tv_nsec);

  TCLAP::CmdLine cmd("OpenAlpr Command Line Utility", ' ', Alpr::getVersion());

  TCLAP::UnlabeledMultiArg<std::string>  fileArg( "image_file", "Image containing license plates", true, "", "image_file_path"  );

  
  TCLAP::ValueArg<std::string> countryCodeArg("c","country","Country code to identify (either us for USA or eu for Europe).  Default=us",false, "us" ,"country_code");
  TCLAP::ValueArg<int> seekToMsArg("","seek","Seek to the specified millisecond in a video file. Default=0",false, 0 ,"integer_ms");
  TCLAP::ValueArg<std::string> configFileArg("","config","Path to the openalpr.conf file",false, "" ,"config_file");
  TCLAP::ValueArg<std::string> templatePatternArg("p","pattern","Attempt to match the plate number against a plate pattern (e.g., md for Maryland, ca for California)",false, "" ,"pattern code");
  TCLAP::ValueArg<int> topNArg("n","topn","Max number of possible plate numbers to return.  Default=10",false, 10 ,"topN");

  TCLAP::SwitchArg jsonSwitch("j","json","Output recognition results in JSON format.  Default=off", cmd, false);
  TCLAP::SwitchArg debugSwitch("","debug","Enable debug output.  Default=off", cmd, false);
  TCLAP::SwitchArg detectRegionSwitch("d","detect_region","Attempt to detect the region of the plate image.  [Experimental]  Default=off", cmd, false);
  TCLAP::SwitchArg clockSwitch("","clock","Measure/print the total time to process image and all plates.  Default=off", cmd, false);
  TCLAP::SwitchArg motiondetect("", "motion", "Use motion detection on video file or stream.  Default=off", cmd, false);
  TCLAP::SwitchArg saveFrameSwitch("s","saveframe","Save frame with recognized plate to /var/lib/openalpr/plateimages.  [Experimental]  Default=off", cmd, false);

  try
  {
    cmd.add( templatePatternArg );
    cmd.add( seekToMsArg );
    cmd.add( topNArg );
    cmd.add( configFileArg );
    cmd.add( fileArg );
    cmd.add( countryCodeArg );

    
    if (cmd.parse( argc, argv ) == false)
    {
      // Error occurred while parsing.  Exit now.
      return 1;
    }

    filenames = fileArg.getValue();

    country = countryCodeArg.getValue();
    seektoms = seekToMsArg.getValue();
    outputJson = jsonSwitch.getValue();
    debug_mode = debugSwitch.getValue();
    configFile = configFileArg.getValue();
    detectRegion = detectRegionSwitch.getValue();
    templatePattern = templatePatternArg.getValue();
    topn = topNArg.getValue();
    measureProcessingTime = clockSwitch.getValue();
	do_motiondetection = motiondetect.getValue();
	save = saveFrameSwitch.getValue()&&debug_mode; //save frames in debug mode only
	//std::cout << "MAIN save=" << save << std::endl;
  }
  catch (TCLAP::ArgException &e)    // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return 1;
  }

    //time_t nowtime = startTime.tv_sec;
    //tm = localtime(&nowtime);
    time_t nowtime;
    time(&nowtime);
	tm = localtime(&nowtime);
	strftime(buf, sizeof(buf), "%Y.%m.%d_%H:%M:%S", tm);

	logDetected.open(LOGDIR + buf + "_detected.log");
	logNotDetected.open(LOGDIR + buf + "_not_detected.log");
  
  cv::Mat frame;

  Alpr alpr(country, configFile);
  alpr.setTopN(topn);
  
  if (debug_mode)
  {
    alpr.getConfig()->setDebug(true);
  }

  if (detectRegion)
    alpr.setDetectRegion(detectRegion);

  if (templatePattern.empty() == false)
    alpr.setDefaultRegion(templatePattern);

  if (alpr.isLoaded() == false)
  {
    std::cerr << "Error loading OpenALPR" << std::endl;
    return 1;
  }

  for (unsigned int i = 0; i < filenames.size(); i++)
  {
    std::string filename = filenames[i];

    if (filename == "-")
    {
      std::vector<uchar> data;
      int c;

      while ((c = fgetc(stdin)) != EOF)
      {
        data.push_back((uchar) c);
      }

      frame = cv::imdecode(cv::Mat(data), 1);
      if (!frame.empty())
      {
        //detectandshow(&alpr, frame, "", outputJson);
    	  if (detectandshow(&alpr, frame, "", outputJson)&&save)
    		  cv::imwrite(fname(), frame);
      }
      else
      {
        std::cerr << "Image invalid: " << filename << std::endl;
      }
    }
    else if (filename == "stdin")
    {
      std::string filename;
      while (std::getline(std::cin, filename))
      {
        if (fileExists(filename.c_str()))
        {
          frame = cv::imread(filename);
          if (detectandshow(&alpr, frame, "", outputJson)&&save)
				cv::imwrite(fname(), frame);
        }
        else
        {
          std::cerr << "Image file not found: " << filename << std::endl;
        }

      }
    }
    else if (filename == "webcam" || startsWith(filename, WEBCAM_PREFIX))
    {
      int webcamnumber = 0;
      
      // If they supplied "/dev/video[number]" parse the "number" here
      if(startsWith(filename, WEBCAM_PREFIX) && filename.length() > WEBCAM_PREFIX.length())
      {
        webcamnumber = atoi(filename.substr(WEBCAM_PREFIX.length()).c_str());
      }
      
      int framenum = 0;
      cv::VideoCapture cap(webcamnumber);
      if (!cap.isOpened())
      {
        std::cerr << "Error opening webcam" << std::endl;
        return 1;
      }

      while (cap.read(frame))
      {
        if (framenum == 0)
          motiondetector.ResetMotionDetection(&frame);
        if (detectandshow(&alpr, frame, "", outputJson)&&save)
        	cv::imwrite(fname(), frame);
        sleep_ms(10);
        framenum++;
      }
    }
    else if (startsWith(filename, "http://") || startsWith(filename, "https://"))
    {
      int framenum = 0;

      VideoBuffer videoBuffer;

      videoBuffer.connect(filename, 5);

      cv::Mat latestFrame;

      while (program_active)
      {
        std::vector<cv::Rect> regionsOfInterest;
        int response = videoBuffer.getLatestFrame(&latestFrame, regionsOfInterest);

        if (response != -1)
        {
          if (framenum == 0)
            motiondetector.ResetMotionDetection(&latestFrame);
          if (detectandshow(&alpr, latestFrame, "", outputJson)&&save)
        	  cv::imwrite(fname(), latestFrame);
        }

        // Sleep 10ms
        sleep_ms(10);
        framenum++;
      }

      videoBuffer.disconnect();

      std::cout << "Video processing ended" << std::endl;
    }
    else if (hasEndingInsensitive(filename, ".avi") || hasEndingInsensitive(filename, ".mp4") ||
                                                       hasEndingInsensitive(filename, ".webm") ||
                                                       hasEndingInsensitive(filename, ".flv") || hasEndingInsensitive(filename, ".mjpg") ||
                                                       hasEndingInsensitive(filename, ".mjpeg") ||
             hasEndingInsensitive(filename, ".mkv")
        )
    {
      if (fileExists(filename.c_str()))
      {
        int framenum = 0;

        cv::VideoCapture cap = cv::VideoCapture();
        cap.open(filename);
        cap.set(CV_CAP_PROP_POS_MSEC, seektoms);

        while (cap.read(frame))
        {
          if (SAVE_LAST_VIDEO_STILL)
          {
            cv::imwrite(LAST_VIDEO_STILL_LOCATION, frame);
          }
          if (!outputJson)
            std::cout << "Frame: " << framenum << std::endl;
          
          if (framenum == 0)
            motiondetector.ResetMotionDetection(&frame);
          if (detectandshow(&alpr, frame, "", outputJson)&&save)
        	  cv::imwrite(fname(), frame);
          //create a 1ms delay
          sleep_ms(1);
          framenum++;
        }
      }
      else
      {
        std::cerr << "Video file not found: " << filename << std::endl;
      }
    }
    else if (is_supported_image(filename))
    {
      if (fileExists(filename.c_str()))
      {
        frame = cv::imread(filename);

        bool plate_found = detectandshow(&alpr, frame, "", outputJson, filename);
        if (plate_found&&save) {
        	cv::imwrite(fname(), frame);
        }

        if (!plate_found && !outputJson)
          std::cout << "No license plates found." << std::endl;
      }
      else
      {
        std::cerr << "Image file not found: " << filename << std::endl;
      }
    }
    else if (DirectoryExists(filename.c_str()))
    {
      /*std::vector<std::string> files = getFilesInDir(filename.c_str());

      std::sort(files.begin(), files.end(), stringCompare);

      for (int i = 0; i < files.size(); i++)
      {
        if (is_supported_image(files[i]))
        {
          std::string fullpath = filename + "/" + files[i];
          std::cout << fullpath << std::endl;
          frame = cv::imread(fullpath.c_str());
          if (detectandshow(&alpr, frame, "", outputJson))
          {
            //while ((char) cv::waitKey(50) != 'c') { }
        	  cv::imwrite(fname(), frame);
          }
          else
          {
            //cv::waitKey(50);
          }
        }
      }*/
    	 process_dir(filename, alpr, outputJson);
    }
    else
    {
      std::cerr << "Unknown file type" << std::endl;
      return 1;
    }
  }

  std::cout << "Total count: " << totalCount << ", detectedCount=" << detectedCount << ", additionalDetectedCount=" << additionalDetectedCount << std::endl;
  timespec endTime;
  getTimeMonotonic(&endTime);
  std::cout << "OpenALPR Total Time: " << diffclock(startTime, endTime) << "ms." << std::endl;

  logDetected.close();
  logNotDetected.close();

  return 0;
}

void process_dir(std::string dir, Alpr alpr, bool outputJson)
{
  if (DirectoryExists(dir.c_str()))
  {
      std::vector<std::string> files = getFilesInDir(dir.c_str());

      std::sort(files.begin(), files.end(), stringCompare);

      for (int i = 0; i < files.size(); i++)
      {
        std::string fullpath = dir + "/" + files[i];
        if (is_supported_image(files[i]))
        {
          std::cout << fullpath << std::endl;
          cv::Mat frame = cv::imread(fullpath.c_str());
          if (detectandshow(&alpr, frame, "", outputJson, files[i]))
          {
            //while ((char) cv::waitKey(50) != 'c') { }
                  cv::imwrite(fname(), frame);
          }
          else
          {
            //cv::waitKey(50);
          }
        }
        else if (files[i].compare("PaxHeader") != 0 && DirectoryExists(fullpath.c_str()))
        {
          process_dir(fullpath, alpr, outputJson);
        }
      }
  }
}


bool is_supported_image(std::string image_file)
{
  return (hasEndingInsensitive(image_file, ".png") || hasEndingInsensitive(image_file, ".jpg") || 
	  hasEndingInsensitive(image_file, ".tif") || hasEndingInsensitive(image_file, ".bmp") ||  
	  hasEndingInsensitive(image_file, ".jpeg") || hasEndingInsensitive(image_file, ".gif"));
}


bool detectandshow( Alpr* alpr, cv::Mat frame, std::string region, bool writeJson) {
	return detectandshow(alpr, frame, region, writeJson, "");
}

bool detectandshow(Alpr* alpr, cv::Mat frame, std::string region, bool writeJson, std::string fileName)
{

  timespec startTime;
  getTimeMonotonic(&startTime);

  std::vector<AlprRegionOfInterest> regionsOfInterest;
  if (do_motiondetection)
  {
	  cv::Rect rectan = motiondetector.MotionDetect(&frame);
	  if (rectan.width>0) regionsOfInterest.push_back(AlprRegionOfInterest(rectan.x, rectan.y, rectan.width, rectan.height));
  }
  else regionsOfInterest.push_back(AlprRegionOfInterest(0, 0, frame.cols, frame.rows));
  AlprResults results;
  if (regionsOfInterest.size()>0) results = alpr->recognize(frame.data, frame.elemSize(), frame.cols, frame.rows, regionsOfInterest);

  timespec endTime;
  getTimeMonotonic(&endTime);
  double totalProcessingTime = diffclock(startTime, endTime);
  if (measureProcessingTime)
    std::cout << "Total Time to process image: " << totalProcessingTime << "ms." << std::endl;
  
  
  if (writeJson)
  {
    std::cout << alpr->toJson( results ) << std::endl;
  }
  else
  {
	  bool detected = false;
	  totalCount++;

    for (int i = 0; i < results.plates.size(); i++)
    {
    	std::cout << "plate" << i << " thresholdOcrLines.size()=" << results.plates[i].thresholdOcrLines.size() << std::endl;
      std::cout << "plate" << i << ": " << results.plates[i].topNPlates.size() << " results";
      if (measureProcessingTime)
        std::cout << " -- Processing Time = " << results.plates[i].processing_time_ms << "ms.";
      std::cout << std::endl;

      if (results.plates[i].regionConfidence > 0)
        std::cout << "State ID: " << results.plates[i].region << " (" << results.plates[i].regionConfidence << "% confidence)" << std::endl;
      

      if (fileName.size() != 0) {
    	  std::string newFilename = fnameToLat(fileName);
//    	  if (debug_mode)
//    	    	std::cout << "LAT filename: " << newFilename << std::endl;
		  for (int k = 0; k < results.plates[i].topNPlates.size(); k++)
			{
				std::string plateText = results.plates[i].topNPlates[k].characters;
				if(plateText.compare(newFilename) == 0) {
					detected = true;
				}
		   }

      }

      for (int k = 0; k < results.plates[i].topNPlates.size(); k++)
		{
			// Replace the multiline newline character with a dash
			std::string no_newline = results.plates[i].topNPlates[k].characters;
			std::replace(no_newline.begin(), no_newline.end(), '\n','-');

			std::cout << "    - " << no_newline << "\t confidence: " << results.plates[i].topNPlates[k].overall_confidence;
			if (templatePattern.size() > 0 || results.plates[i].regionConfidence > 0)
			  std::cout << "\t pattern_match: " << results.plates[i].topNPlates[k].matches_template;

			std::cout << std::endl;
		}

    }
    if (detected) {
    	detectedCount++;
    	logDetected << fileName << std::endl;
    } else {
    	logNotDetected << fileName << std::endl;
    	std::cout << "NOT DETECTED" << std::endl;
    	std::cout << "fileName.size()=" << fileName.size() << std::endl;
    	if (fileName.size() != 0) {
    	    std::string newFilename = fnameToLat(fileName);
    	    std::cout << "newFilename=" << newFilename << std::endl;
			for (int i = 0; i < results.plates.size(); i++) {
				std::cout << "results.plates[" << i << "].thresholdOcrLines.size()=" << results.plates[i].thresholdOcrLines.size() << std::endl;
				for (int k = 0; k < results.plates[i].thresholdOcrLines.size(); k++) {
					std::string s = results.plates[i].thresholdOcrLines[k];
					std::cout << "OCR Line: " << s << std::endl;
					s.erase(std::remove_if(
					    begin(s), end(s),
					    [l = std::locale{}](auto ch) { return std::isspace(ch, l); }
					), end(s));
					//if (results.plates[i].thresholdOcrLines[k].find(newFilename) != std::string::npos){
					if (s.find(newFilename) != std::string::npos) {
						additionalDetectedCount++;
					}
				}
			}
    	}
    }
  }

  return results.plates.size() > 0;
}

std::string fname(){
//generate filename for snapshot based on timestamp
  struct timeval tp;
  gettimeofday(&tp, NULL);
  long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  return DEFDIR + std::to_string(ms) + ".jpeg";
}

std::string fnameToLat(std::string fname){
	//[A,B,E,K,M,H,0,P,C,T,Y,X]
	//setlocale( LC_CTYPE, "rus" );
	std::map <char,char> cirToLat = 	{{'а', 'A'},
	                                 {'в', 'B'},
	                                 {'е', 'E'},
	                                 {'к', 'K'},
									 {'м', 'M'},
									 {'н', 'H'},
									 {'о', '0'},
									 {'р', 'P'},
									 {'с', 'C'},
									 {'т', 'T'},
									 {'у', 'Y'},
									 {'х', 'X'}
	};

	std::size_t foundSlash = fname.find_last_of("/\\");
	if (foundSlash != std::string::npos) {
		fname = fname.substr(foundSlash + 1);
	}

	std::size_t foundDot = fname.find_last_of(".");
	if (foundDot != std::string::npos) {
		fname = fname.substr(0, foundDot);
	}

	std::string newName;
	for (unsigned int i = 0; i < fname.size(); ++i) {
		std::map<char,char>::iterator it = cirToLat.find(fname[i]);

		if (it != cirToLat.end()) {
		   //element found;
			char latCh = it->second;
		   //fname.replace(i, 1, latCh);
			newName.push_back(latCh);
		} else if ((fname[i] >= '0' && fname[i] <= '9') || (fname[i] >= 'A' && fname[i] <= 'Z') || (fname[i] >= 'a' && fname[i] <= 'z') || fname[i] == '.') {
			newName.push_back(fname[i]);
		}

	}
	return newName;
}

