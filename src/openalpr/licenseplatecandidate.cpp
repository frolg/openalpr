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

#include <opencv2/core/core.hpp>

#include "licenseplatecandidate.h"
#include "edges/edgefinder.h"
#include "transformation.h"

using namespace std;
using namespace cv;

namespace alpr
{

  LicensePlateCandidate::LicensePlateCandidate(PipelineData* pipeline_data)
  {
    this->pipeline_data = pipeline_data;
    this->config = pipeline_data->config;

  }

  LicensePlateCandidate::~LicensePlateCandidate()
  {
  }

  // Must delete this pointer in parent class
  void LicensePlateCandidate::recognize()
  {
	  if (this->config->debugPlateCorners)
		  cout << "LicensePlateCandidate: start" << endl;

    pipeline_data->isMultiline = config->multiline;


    Rect expandedRegion = this->pipeline_data->regionOfInterest;

    if (this->config->debugPlateCorners) {
        	cout << "LicensePlateCandidate 111: pipeline_data->crop_gray.cols=" << pipeline_data->crop_gray.cols << ", pipeline_data->crop_gray.rows=" << pipeline_data->crop_gray.rows << endl;
        	cout << "LicensePlateCandidate 222: pipeline_data->grayImg.cols=" << pipeline_data->grayImg.cols << ", pipeline_data->grayImg.rows=" << pipeline_data->grayImg.rows << endl;
        	cout << "LicensePlateCandidate 333: pipeline_data->regionOfInterest.x=" << pipeline_data->regionOfInterest.x << ", pipeline_data->regionOfInterest.y=" << pipeline_data->regionOfInterest.y << endl;
        	cout << "LicensePlateCandidate 444:, pipeline_data->regionOfInterest.width=" << pipeline_data->regionOfInterest.width << ", pipeline_data->regionOfInterest.height=" << pipeline_data->regionOfInterest.height << endl;
    }

    pipeline_data->crop_gray = Mat(this->pipeline_data->grayImg, expandedRegion);
    if (this->config->debugPlateCorners)
         cout << "LicensePlateCandidate 555: pipeline_data->crop_gray reset" << endl;


    resize(pipeline_data->crop_gray, pipeline_data->crop_gray, Size(config->templateWidthPx, config->templateHeightPx));

    if (this->config->debugPlateCorners)
                        {
                          Mat debugImg(pipeline_data->crop_gray.size(), pipeline_data->crop_gray.type());
                          pipeline_data->crop_gray.copyTo(debugImg);
                          //is not set yet
                          for (unsigned int i = 0; i < pipeline_data->textLines.size(); i++) {
                        	  line(debugImg, pipeline_data->textLines[i].topLine.p1, pipeline_data->textLines[i].topLine.p2, Scalar(255, 0, 0), 1, CV_AA);
                			line(debugImg, pipeline_data->textLines[i].bottomLine.p1, pipeline_data->textLines[i].bottomLine.p2, Scalar(0, 0, 255), 1, CV_AA);
                			cout << "LicensePlateCandidate 551: pipeline_data->textLines[i].topLine.p1(" << pipeline_data->textLines[i].topLine.p1.x << ", " << pipeline_data->textLines[i].topLine.p1.y << "), p2(" << pipeline_data->textLines[i].topLine.p2.x << ", " << pipeline_data->textLines[i].topLine.p2.y << ")" << endl;
                			cout << "LicensePlateCandidate 552: pipeline_data->textLines[i].bottomLine.p1(" << pipeline_data->textLines[i].bottomLine.p1.x << ", " << pipeline_data->textLines[i].bottomLine.p1.y << "), p2(" << pipeline_data->textLines[i].bottomLine.p2.x << ", " << pipeline_data->textLines[i].bottomLine.p2.y << ")" << endl;
                          }
                          displayImage(this->config, "Plate corners (LicensePlateCandidate) 11", debugImg);


                        }


    CharacterAnalysis textAnalysis(pipeline_data);

    if (pipeline_data->disqualified) {
    if (this->config->debugPlateCorners)
                    cout << "LicensePlateCandidate END 1: pipeline_data->disqulified" << endl;

    return;
    }

    if (this->config->debugPlateCorners)
                        cout << "LicensePlateCandidate before EdgeFinder" << endl;
    EdgeFinder edgeFinder(pipeline_data);

    pipeline_data->plate_corners = edgeFinder.findEdgeCorners();
    if (this->config->debugPlateCorners)
            cout << "LicensePlateCandidate cropSize(1): width=" << (pipeline_data->plate_corners[1].x - pipeline_data->plate_corners[0].x)
				<< ", height=" << (pipeline_data->plate_corners[3].y - pipeline_data->plate_corners[0].y) << endl;
//    Size cropSize111 = Size(pipeline_data->plate_corners[1].x - pipeline_data->plate_corners[0].x, pipeline_data->plate_corners[3].y - pipeline_data->plate_corners[0].y);
//    if (this->config->debugPlateCorners)
//           cout << "LicensePlateCandidate cropSize(2): width=" << cropSize111.width << ", height=" << cropSize111.height << endl;

    if (pipeline_data->disqualified) {
    if (this->config->debugPlateCorners)
                    cout << "LicensePlateCandidate END 2: pipeline_data->disqulified" << endl;

    return;
    }


    timespec startTime;
    getTimeMonotonic(&startTime);


    // Compute the transformation matrix to go from the current image to the new plate corners
    Transformation imgTransform(this->pipeline_data->grayImg, pipeline_data->crop_gray, expandedRegion);
    Size cropSize = imgTransform.getCropSize(pipeline_data->plate_corners,
            Size(pipeline_data->config->ocrImageWidthPx, pipeline_data->config->ocrImageHeightPx));
    Mat transmtx = imgTransform.getTransformationMatrix(pipeline_data->plate_corners, cropSize);
    if (this->config->debugPlateCorners)
              cout << "LicensePlateCandidate cropSize (ocrImage): width=" << cropSize.width << ", height=" << cropSize.height << endl;

/*

//    vector<Point2f> remappedCorners = imgTransform.transformSmallPointsToBigImage(corners);
//    Size cropSize = imgTransform.getCropSize(pipeline_data->plate_corners,
//               Size(pipeline_data->config->ocrImageWidthPx, pipeline_data->config->ocrImageHeightPx));

    Size cropSize = imgTransform.getCropSize(pipeline_data->plate_corners,
            Size(pipeline_data->config->ocrImageWidthPx, pipeline_data->config->ocrImageHeightPx));
    if (this->config->debugPlateCorners)
          cout << "LicensePlateCandidate cropSize (ocrImage): width=" << cropSize.width << ", height=" << cropSize.height << endl;

//    if (cropSize.width > pipeline_data->plate_corners[1].x - pipeline_data->plate_corners[0].x) {
//    	cropSize = imgTransform.getCropSize(pipeline_data->plate_corners,
//    		Size(pipeline_data->config->templateWidthPx, pipeline_data->config->templateHeightPx));
//    	if (this->config->debugPlateCorners)
//    	   cout << "LicensePlateCandidate cropSize (template): width=" << cropSize.width << ", height=" << cropSize.height << endl;
//    }
    //Size cropSize = Size(pipeline_data->plate_corners[1].x - pipeline_data->plate_corners[0].x, pipeline_data->plate_corners[3].y - pipeline_data->plate_corners[0].y);

    //Size cropSize = imgTransform.getCropSize(pipeline_data->plate_corners);
//    if (this->config->debugPlateCorners)
//                        cout << "LicensePlateCandidate cropSize: width=" << cropSize.width << ", height=" << cropSize.height << endl;

    Mat transmtx = imgTransform.getTransformationMatrix(pipeline_data->plate_corners, cropSize);*/
    Mat newCropDebug = imgTransform.crop(cropSize, transmtx);
    if (this->config->debugPlateCorners)
	  displayImage(this->config, "LPC: Image after Transformation", newCropDebug);


    // Crop the plate corners from the original color image (after un-applying prewarp)
    vector<Point2f> projectedPoints = pipeline_data->prewarp->projectPoints(pipeline_data->plate_corners, true);
    pipeline_data->color_deskewed = Mat::zeros(cropSize, pipeline_data->colorImg.type());

    if (this->config->debugPlateCorners)
                    {
                      Mat debugImg(pipeline_data->color_deskewed.size(), pipeline_data->color_deskewed.type());
                      pipeline_data->color_deskewed.copyTo(debugImg);
                      for (unsigned int i = 0; i < pipeline_data->textLines.size(); i++) {
                    	  line(debugImg, pipeline_data->textLines[i].topLine.p1, pipeline_data->textLines[i].topLine.p2, Scalar(255, 0, 0), 1, CV_AA);
            			line(debugImg, pipeline_data->textLines[i].bottomLine.p1, pipeline_data->textLines[i].bottomLine.p2, Scalar(0, 0, 255), 1, CV_AA);
                      }
                      displayImage(this->config, "Plate corners 22", debugImg);
                    }

    std::vector<cv::Point2f> deskewed_points;
    deskewed_points.push_back(cv::Point2f(0,0));
    deskewed_points.push_back(cv::Point2f(pipeline_data->color_deskewed.cols,0));
    deskewed_points.push_back(cv::Point2f(pipeline_data->color_deskewed.cols,pipeline_data->color_deskewed.rows));
    deskewed_points.push_back(cv::Point2f(0,pipeline_data->color_deskewed.rows));
    cv::Mat color_transmtx = cv::getPerspectiveTransform(projectedPoints, deskewed_points);
    cv::warpPerspective(pipeline_data->colorImg, pipeline_data->color_deskewed, color_transmtx, pipeline_data->color_deskewed.size());

    if (pipeline_data->color_deskewed.channels() > 2)
    {
      // Make a grayscale copy as well for faster processing downstream
      cv::cvtColor(pipeline_data->color_deskewed, pipeline_data->crop_gray, CV_BGR2GRAY);
    }
    else
    {
      // Copy the already grayscale image to the crop_gray img
      pipeline_data->color_deskewed.copyTo(pipeline_data->crop_gray);
    }

    if (this->config->debugGeneral) {
    	cout << "LicensePlateCandidate final1: pipeline_data->crop_gray.cols=" << pipeline_data->crop_gray.cols << ", pipeline_data->crop_gray.rows=" << pipeline_data->crop_gray.rows << endl;
    	cout << "LicensePlateCandidate final2: pipeline_data->color_deskewed.cols=" << pipeline_data->color_deskewed.cols << ", pipeline_data->color_deskewed.rows=" << pipeline_data->color_deskewed.rows << endl;
    	displayImage(config, "quadrilateral", pipeline_data->color_deskewed);
    }


    if (this->config->debugPlateCorners)
            {
              Mat debugImg(pipeline_data->color_deskewed.size(), pipeline_data->color_deskewed.type());
              pipeline_data->color_deskewed.copyTo(debugImg);
              for (unsigned int i = 0; i < pipeline_data->textLines.size(); i++) {
            	  line(debugImg, pipeline_data->textLines[i].topLine.p1, pipeline_data->textLines[i].topLine.p2, Scalar(255, 0, 0), 1, CV_AA);
    			line(debugImg, pipeline_data->textLines[i].bottomLine.p1, pipeline_data->textLines[i].bottomLine.p2, Scalar(0, 0, 255), 1, CV_AA);
    			cout << "Plate corners 2: pipeline_data->textLines[i].topLine.p1(" << pipeline_data->textLines[i].topLine.p1.x << ", " << pipeline_data->textLines[i].topLine.p1.y << "), p2(" << pipeline_data->textLines[i].topLine.p2.x << ", " << pipeline_data->textLines[i].topLine.p2.y << ")" << endl;
    			              cout << "Plate corners 2: pipeline_data->textLines[i].bottomLine.p1(" << pipeline_data->textLines[i].bottomLine.p1.x << ", " << pipeline_data->textLines[i].bottomLine.p1.y << "), p2(" << pipeline_data->textLines[i].bottomLine.p2.x << ", " << pipeline_data->textLines[i].bottomLine.p2.y << ")" << endl;
              }
              displayImage(this->config, "Plate corners 2", debugImg);


            }

    // Apply a perspective transformation to the TextLine objects
    // to match the newly deskewed license plate crop
    vector<TextLine> newLines;
    for (unsigned int i = 0; i < pipeline_data->textLines.size(); i++)
    {
//      vector<Point2f> textArea = imgTransform.transformSmallPointsToBigImage(pipeline_data->textLines[i].textArea);
//      vector<Point2f> linePolygon = imgTransform.transformSmallPointsToBigImage(pipeline_data->textLines[i].linePolygon);

      //vector<Point> textArea = pipeline_data->textLines[i].textArea;//imgTransform.transformSmallPointsToBigImage(pipeline_data->textLines[i].textArea);
      //vector<Point> linePolygon = pipeline_data->textLines[i].linePolygon;//imgTransform.transformSmallPointsToBigImage(pipeline_data->textLines[i].linePolygon);

      //vector<Point2f> textAreaRemapped = imgTransform.remapSmallPointstoCrop(textArea, transmtx);
      //vector<Point2f> linePolygonRemapped = imgTransform.remapSmallPointstoCrop(linePolygon, transmtx);

      vector<Point2f> textAreaRemapped = imgTransform.remapSmallPointstoCrop(pipeline_data->textLines[i].textArea, transmtx);
      vector<Point2f> linePolygonRemapped = imgTransform.remapSmallPointstoCrop(pipeline_data->textLines[i].linePolygon, transmtx);


      if (this->config->debugPlateCorners)
	  {
		cout << "Plate corners 311: pipeline_data->textLines[i].linePolygon: ";
		for (unsigned int n = 0; n < pipeline_data->textLines[i].linePolygon.size(); n++)
			cout << " (" << pipeline_data->textLines[i].linePolygon[n].x << ", " << pipeline_data->textLines[i].linePolygon[n].y << ") ";
		cout << endl;
        cout << "Plate corners 311: linePolygonRemapped: ";
		for (unsigned int n = 0; n < linePolygonRemapped.size(); n++)
			cout << " (" << linePolygonRemapped[n].x << ", " << linePolygonRemapped[n].y << ") ";
		cout << endl;
	  }

      std::vector<std::vector<cv::Point> > newContours;
		for (unsigned int c = 0; c < pipeline_data->textLines[i].textContours.contours.size(); c++) {
		  vector<Point> contour = pipeline_data->textLines[i].textContours.contours[c];
		  //vector<Point2f> contourArea = imgTransform.transformSmallPointsToBigImage(contour);
		  //vector<Point2f> contourRemapped = imgTransform.remapSmallPointstoCrop(contourArea, transmtx);
		  vector<Point2f> contourRemapped = imgTransform.remapSmallPointstoCrop(pipeline_data->textLines[i].textContours.contours[c], transmtx);
		  vector<Point> contourRemappedInt;
		  for (unsigned int n = 0; n < contourRemapped.size(); n++)
			  contourRemappedInt.push_back(Point(round(contourRemapped[n].x), round(contourRemapped[n].y)));
		  newContours.push_back(contourRemappedInt);
		}
	  //pipeline_data->textLines[i].textContours.contours = newContours;
		pipeline_data->textLines[i].textContours.contours.clear();
		    for (unsigned int n = 0; n < newContours.size(); n++)
		    	pipeline_data->textLines[i].textContours.contours.push_back(newContours[n]);
      newLines.push_back(TextLine(textAreaRemapped, linePolygonRemapped, pipeline_data->crop_gray.size(), pipeline_data->textLines[i].textContours));
    }

    pipeline_data->textLines.clear();
    for (unsigned int i = 0; i < newLines.size(); i++)
      pipeline_data->textLines.push_back(newLines[i]);


    /*if (this->config->debugPlateCorners)
        {
          Mat debugImg(pipeline_data->color_deskewed.size(), pipeline_data->color_deskewed.type());
          pipeline_data->color_deskewed.copyTo(debugImg);
          for (unsigned int i = 0; i < newLines.size(); i++) {
        	  line(debugImg, newLines[i].topLine.p1, newLines[i].topLine.p2, Scalar(255, 0, 0), 1, CV_AA);
			line(debugImg, newLines[i].bottomLine.p1, newLines[i].bottomLine.p2, Scalar(0, 0, 255), 1, CV_AA);

			for (unsigned int c = 0; c < pipeline_data->textLines[0].textContours.contours.size(); c++) {
				  if (pipeline_data->textLines[0].textContours.goodIndices[c] == true) {
					  Rect r = boundingRect(pipeline_data->textLines[0].textContours.contours[c]);
					  rectangle(debugImg, r, Scalar(255, 255, 0), 1);
				  }
			  }

			cout << "Plate corners 3: newLines[i].topLine.p1(" << newLines[i].topLine.p1.x << ", " << newLines[i].topLine.p1.y << "), p2(" << newLines[i].topLine.p2.x << ", " << newLines[i].topLine.p2.y << ")" << endl;
			cout << "Plate corners 3: newLines[i].bottomLine.p1(" << newLines[i].bottomLine.p1.x << ", " << newLines[i].bottomLine.p1.y << "), p2(" << newLines[i].bottomLine.p2.x << ", " << newLines[i].bottomLine.p2.y << ")" << endl;
          }
          displayImage(this->config, "Plate corners 3", debugImg);

        }
*/

    if (this->config->debugPlateCorners)
            {
              Mat debugImg(newCropDebug.size(), newCropDebug.type());
              newCropDebug.copyTo(debugImg);
              cout << "Plate corners 3: pipeline_data->textLines.size()=" << pipeline_data->textLines.size() << endl;
              for (unsigned int i = 0; i < pipeline_data->textLines.size(); i++) {
            	  line(debugImg, pipeline_data->textLines[i].topLine.p1, pipeline_data->textLines[i].topLine.p2, Scalar(255, 255, 0), 1, CV_AA);
    			line(debugImg, pipeline_data->textLines[i].bottomLine.p1, pipeline_data->textLines[i].bottomLine.p2, Scalar(255, 255, 0), 1, CV_AA);

    			for (unsigned int c = 0; c < pipeline_data->textLines[i].textContours.contours.size(); c++) {
    				  if (pipeline_data->textLines[i].textContours.goodIndices[c] == true) {
    					  Rect r = boundingRect(pipeline_data->textLines[0].textContours.contours[c]);
    					  rectangle(debugImg, r, Scalar(255, 255, 0), 1);
    				  }
    			  }

    			cout << "Plate corners 3: pipeline_data->textLines[i].topLine.p1(" << pipeline_data->textLines[i].topLine.p1.x << ", " << pipeline_data->textLines[i].topLine.p1.y << "), p2(" << pipeline_data->textLines[i].topLine.p2.x << ", " << pipeline_data->textLines[i].topLine.p2.y << ")" << endl;
    			cout << "Plate corners 3: pipeline_data->textLines[i].bottomLine.p1(" << pipeline_data->textLines[i].bottomLine.p1.x << ", " << pipeline_data->textLines[i].bottomLine.p1.y << "), p2(" << pipeline_data->textLines[i].bottomLine.p2.x << ", " << pipeline_data->textLines[i].bottomLine.p2.y << ")" << endl;
              }
              displayImage(this->config, "Plate corners 3", debugImg);
              //drawAndWait(debugImg);
            }


    if (config->debugTiming)
    {
      timespec endTime;
      getTimeMonotonic(&endTime);
      cout << "deskew Time: " << diffclock(startTime, endTime) << "ms." << endl;
    }



  }


}
