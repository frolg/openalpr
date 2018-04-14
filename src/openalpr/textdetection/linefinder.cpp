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
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc.hpp>

#include "linefinder.h"
#include "utility.h"
#include "pipeline_data.h"

using namespace std;
using namespace cv;

namespace alpr
{

  LineFinder::LineFinder(PipelineData* pipeline_data) {
    this->pipeline_data = pipeline_data;
  }

  LineFinder::~LineFinder() {
  }

//  vector<vector<Point> > LineFinder::findLinesBak(Mat image, const TextContours contours)
//  {
//    const float MIN_AREA_TO_IGNORE = 0.65;
//
//    vector<vector<Point> > linesFound;
//
//    cvtColor(image, image, CV_GRAY2BGR);
//
//    vector<CharPointInfo> charPoints;
//    unsigned int minX = 0;
//    unsigned int topY = 0;
//    unsigned int maxX = 0;
//    unsigned int bottomY = 0;
//
//    unsigned int leftBottomY = 0;
//    unsigned int rightBottomY = 0;
//    unsigned int leftTopY = 0;
//    unsigned int rightTopY = 0;
//    unsigned int maxHeight = 0;
//
//    /*double sumxTop = 0;
//	  double sumyTop = 0;
//	  double sumx2Top = 0;
//	  double sumxyTop = 0;
//
//
//	  double sumxBottom = 0;
//	  double sumyBottom = 0;
//	  double sumx2Bottom = 0;
//	  double sumxyBottom = 0;*/
//
//    bool isFirst = true;
//
//    for (unsigned int i = 0; i < contours.contours.size(); i++)
//        {
//        	if (pipeline_data->config->debugCharAnalysis) {
//        		Rect ra(boundingRect(contours.contours[i]));
//        	    cout << "LINEFINDER contours[" << i << "]: goodIndices=" << contours.goodIndices[i] << ", x=" << ra.x << ", y=" << ra.y << ", width=" << ra.width << ", height=" << ra.height << endl;
//        	}
//          if (contours.goodIndices[i] == false)
//            continue;
//
//          CharPointInfo cpi = CharPointInfo(contours.contours[i], i);
//
//          if (cpi.boundingBox.height > maxHeight) {
//        	  maxHeight = cpi.boundingBox.height;
//          }
//
//
//          if (isFirst) {
//        	  minX = cpi.boundingBox.x;
//        	  bottomY = cpi.boundingBox.y + cpi.boundingBox.height;
//        	  topY = cpi.boundingBox.y;
//        	  leftBottomY = cpi.boundingBox.y + cpi.boundingBox.height;
//        	  leftTopY = cpi.boundingBox.y;
//        	  isFirst = false;
//          } else {
//        	  maxX = cpi.boundingBox.x + cpi.boundingBox.width;
//        	  rightBottomY = cpi.boundingBox.y + cpi.boundingBox.height;
//        	  rightTopY = cpi.boundingBox.y;
//        	  if (cpi.boundingBox.y + cpi.boundingBox.height > bottomY) {
//        		  bottomY = cpi.boundingBox.y + cpi.boundingBox.height;
//    		}
//    		if (cpi.boundingBox.y < topY) {
//    			topY = cpi.boundingBox.y;
//    		}
//          }
//
//        }
//
//
//    for (unsigned int i = 0; i < contours.contours.size(); i++)
//    {
//    	if (pipeline_data->config->debugCharAnalysis) {
//    		Rect ra(boundingRect(contours.contours[i]));
//    	    cout << "LINEFINDER contours[" << i << "]: goodIndices=" << contours.goodIndices[i] << ", x=" << ra.x << ", y=" << ra.y << ", width=" << ra.width << ", height=" << ra.height << endl;
//    	}
//      if (contours.goodIndices[i] == false)
//        continue;
//
//
//      CharPointInfo cpi = CharPointInfo(contours.contours[i], i);
//      charPoints.push_back( cpi );
//
//      /*if (isFirst) {
//          	  minX = cpi.bottom.x;
//          	  minY = cpi.bottom.y;
//          	maxX = cpi.top.x;
//          	maxY = cpi.top.y;
//            } else {
//            	if (cpi.bottom.x < minX) {
//            		minX = cpi.bottom.x;
//            	}
//            	if (cpi.top.x > maxX) {
//            		maxX = cpi.top.x;
//            	}
//            	if (cpi.bottom.y > minY) {
//					minY = cpi.bottom.y;
//				}
//				if (cpi.top.y < maxY) {
//					maxY = cpi.top.y;
//				}
//            }*/
//
//          /*sumxTop += cpi.top.x;
//          sumyTop += cpi.top.y;
//          sumx2Top += cpi.top.x * cpi.top.x;
//          sumxyTop += cpi.top.x * cpi.top.y;
//
//          sumxBottom += cpi.bottom.x;
//			sumyBottom += cpi.bottom.y;
//			sumx2Bottom += cpi.bottom.x * cpi.bottom.x;
//			sumxyBottom += cpi.bottom.x * cpi.bottom.y;*/
//if (bottomY - (cpi.boundingBox.y + cpi.boundingBox.height) < maxHeight*0.9) {
//	addData(cpi.boundingBox.x, cpi.boundingBox.y + cpi.boundingBox.height, i, false);
//	      addData(cpi.boundingBox.x + cpi.boundingBox.width, cpi.boundingBox.y + cpi.boundingBox.height, i, false);
//}
//if (cpi.boundingBox.y - topY < maxHeight*0.9) {
//      addData(cpi.boundingBox.x, cpi.boundingBox.y, i, true);
//      addData(cpi.boundingBox.x + cpi.boundingBox.width, cpi.boundingBox.y, i, true);
//}
//
//
//
//    }
//
//    /*int n = contours.contours.size();
//    double aTop = (n*sumxyTop - (sumxTop*sumyTop)) / (n*sumx2Top - sumxTop*sumxTop);
//    double bTop = (sumyTop - aTop*sumxTop) / n;
//
//    double aBottom = (n*sumxyBottom - (sumxBottom*sumyBottom)) / (n*sumx2Bottom - sumxBottom*sumxBottom);
//    double bBottom = (sumyBottom - aBottom*sumxBottom) / n;*/
//
//
//
//    /*if (pipeline_data->config->debugCharAnalysis) {
//		Mat debugImg(image.size(), image.type());
//		image.copyTo(debugImg);
//		for (unsigned int i = 0; i < charPoints.size(); i++) {
//			rectangle(debugImg, charPoints[i].boundingBox, Scalar(0, 255, 0));
//			//line(debugImg, topLeft, topRight, Scalar(255,0,255), 2);
//			//line(debugImg, bottomLeft, bottomRight, Scalar(255,0,255), 2);
//			  /////////charheights.push_back(charPoints[i].boundingBox.height);
//		}
//		displayImage(pipeline_data->config, "LINEFINDER 11", debugImg);
//	}*/
//
//    vector<Point> extended;
//
//    Point topLeft 		= Point(minX, predict(minX, sumXXTop, sumXYTop, sumXTop, sumYTop, contours.contours.size()));
//    Point topRight 		= Point(maxX, predict(minX, sumXXTop, sumXYTop, sumXTop, sumYTop, contours.contours.size()));
//    Point bottomRight 	= Point(maxX, predict(maxX, sumXXBottom, sumXYBottom, sumXBottom, sumYBottom, contours.contours.size()));
//    Point bottomLeft 	= Point(minX, predict(minX, sumXXBottom, sumXYBottom, sumXBottom, sumYBottom, contours.contours.size()));
//
//
//    /*LineSegment top(Point(minX, leftTopY), Point(maxX, rightTopY));
//    LineSegment bottom(Point(minX, leftBottomY), Point(maxX, rightBottomY));
//    int diffBottomY = 0;
//    int maxY = max(leftBottomY, bottomY, rightBottomY);
//    if (maxY == bottomY) {
//		if (leftBottomY < rightBottomY) {
//			diffBottomY = bottomY - leftBottomY;
//		} else {
//			diffBottomY = bottomY - rightBottomY;
//		}
//    }
//    int diffTopY = 0;
//	int minY = min(leftTopY, topY, rightTopY);
//	if (minY == topY) {
//		if (leftTopY < rightTopY) {
//			diffTopY = topY - leftTopY;
//		} else {
//			diffTopY = topY - rightTopY;
//		}
//	}
//	LineSegment parallelBot = top.getParallelLine(-1*diffBottomY);
//	LineSegment parallelTop = bottom.getParallelLine(medianCharHeight);*/
//
//    if (pipeline_data->config->debugCharAnalysis) {
//        cout << "LINEFINDER: minX=" << minX << ", maxX=" << maxX << ", topY=" << topY << ", bottomY=" << bottomY << endl;
//        cout << "LINEFINDER topLeft[" << topLeft.x << ", " << topLeft.y << "]" << endl;
//        cout << "LINEFINDER topRight[" << topRight.x << ", " << topRight.y << "]" << endl;
//        cout << "LINEFINDER bottomLeft[" << bottomLeft.x << ", " << bottomLeft.y << "]" << endl;
//        cout << "LINEFINDER bottomRight[" << bottomRight.x << ", " << bottomRight.y << "]" << endl;
//    }
//	/*Point topLeft 		= Point(minX, minX*aTop + bTop);
//	Point topRight 		= Point(maxX, maxX*aTop + bTop);
//	Point bottomRight 	= Point(maxX, maxX*aBottom + bBottom);
//	Point bottomLeft 	= Point(minX, minX*aBottom + bBottom);*/
//
//	extended.push_back(topLeft);
//	extended.push_back(topRight);
//	extended.push_back(bottomRight);
//	extended.push_back(bottomLeft);
//
//	linesFound.push_back(extended);
//
//
//	if (pipeline_data->config->debugCharAnalysis) {
//			Mat debugImg(image.size(), image.type());
//			image.copyTo(debugImg);
//			for (unsigned int i = 0; i < charPoints.size(); i++) {
//				rectangle(debugImg, charPoints[i].boundingBox, Scalar(0, 255, 0));
//				line(debugImg, topLeft, topRight, Scalar(255,0,255), 1);
//				line(debugImg, bottomLeft, bottomRight, Scalar(255,0,255), 1);
//				  //charheights.push_back(charPoints[i].boundingBox.height);
//			}
//			displayImage(pipeline_data->config, "LINEFINDER 11", debugImg);
//		}
//
//	return linesFound;
//
//
//    /*vector<Point> extended;
//
//	Point topLeft 		= Point(minX, maxY);
//	Point topRight 		= Point(maxX, maxY);
//	Point bottomRight 	= Point(maxX, minY);
//	Point bottomLeft 	= Point(minX, minY);
//
//	extended.push_back(topLeft);
//	extended.push_back(topRight);
//	extended.push_back(bottomRight);
//	extended.push_back(bottomLeft);
//
//	linesFound.push_back(extended);
//
//
//    if (pipeline_data->config->debugCharAnalysis) {
//        	Mat debugImg(image.size(), image.type());
//        	image.copyTo(debugImg);
//        	for (unsigned int i = 0; i < charPoints.size(); i++) {
//        		rectangle(debugImg, charPoints[i].boundingBox, Scalar(0, 255, 0));
//        		line(debugImg, topLeft, topRight, Scalar(255,0,255), 2);
//        		line(debugImg, bottomLeft, bottomRight, Scalar(255,0,255), 2);
//        	      //charheights.push_back(charPoints[i].boundingBox.height);
//        	}
//        	displayImage(pipeline_data->config, "LINEFINDER 11", debugImg);
//        }
//
//    return linesFound;*/
//
//    /*
//    //orig
//    vector<Point> bestCharArea = getBestLine(contours, charPoints);
//    vector<Point> bestLine = extendToEdges(Size(contours.width, contours.height), bestCharArea);
//
//    if (bestLine.size() > 0)
//      linesFound.push_back(bestLine);
//
//    if (pipeline_data->isMultiline && bestCharArea.size() > 0)
//    {
//
//      vector<Point> next_best_line = findNextBestLine(Size(contours.width, contours.height), bestCharArea);
//
//      if (next_best_line.size() > 0)
//      {
//        vector<Point> next_best_line_extended = extendToEdges(Size(contours.width, contours.height), next_best_line);
//        linesFound.push_back(next_best_line_extended);
//      }
//
//
//    }
//
//
//    return linesFound;*/
//  }



  vector<vector<Point> > LineFinder::findLines(Mat image, const TextContours contours)
    {
      const float MIN_AREA_TO_IGNORE = 0.65;

      vector<vector<Point> > linesFound;

      cvtColor(image, image, CV_GRAY2BGR);

      vector<CharPointInfo> charPoints;
      Point top;
      Point bottom;
      bool isFirst = true;

      for (unsigned int i = 0; i < contours.contours.size(); i++)
	  {
		if (pipeline_data->config->debugCharAnalysis) {
			Rect ra(boundingRect(contours.contours[i]));
			cout << "LINEFINDER contours[" << i << "]: goodIndices=" << contours.goodIndices[i] << ", x=" << ra.x << ", y=" << ra.y << ", width=" << ra.width << ", height=" << ra.height << endl;
		}
		if (contours.goodIndices[i] == false)
		  continue;

		CharPointInfo cpi = CharPointInfo(contours.contours[i], i);

		if (isFirst) {
			top = Point(cpi.top.x, cpi.top.y);
			bottom = Point(cpi.bottom.x, cpi.bottom.y);
			isFirst = false;
		} else {
		  if (cpi.bottom.y > bottom.y) {
			  bottom = Point(cpi.bottom.x, cpi.bottom.y);
		  }
		  if (cpi.top.y < top.y) {
			top = Point(cpi.top.x, cpi.top.y);
		  }

		}

	  }


      for (unsigned int i = 0; i < contours.contours.size(); i++)
      {
//      	if (pipeline_data->config->debugCharAnalysis) {
//      		Rect ra(boundingRect(contours.contours[i]));
//      	    cout << "LINEFINDER contours[" << i << "]: goodIndices=" << contours.goodIndices[i] << ", x=" << ra.x << ", y=" << ra.y << ", width=" << ra.width << ", height=" << ra.height << endl;
//      	}
        if (contours.goodIndices[i] == false)
          continue;


        CharPointInfo cpi = CharPointInfo(contours.contours[i], i);
        charPoints.push_back( cpi );
      }

      //orig
      vector<Point> bestCharArea = getBestLine(contours, charPoints);

      for (unsigned int i = 0; i < contours.contours.size(); i++)
            {
//            	if (pipeline_data->config->debugCharAnalysis) {
//            		Rect ra(boundingRect(contours.contours[i]));
//            	    cout << "LINEFINDER contours[" << i << "]: goodIndices=" << contours.goodIndices[i] << ", x=" << ra.x << ", y=" << ra.y << ", width=" << ra.width << ", height=" << ra.height << endl;
//            	}
              if (contours.goodIndices[i] == false)
                continue;


              CharPointInfo cpi = CharPointInfo(contours.contours[i], i);
              charPoints.push_back( cpi );
            }

      //vector<Point> bestLine = extendToEdgesAndPoints(Size(contours.width, contours.height), bestCharArea, top, bottom);
      vector<Point> bestLine = extendToEdgesAndPoints(Size(contours.width, contours.height), bestCharArea, contours);

      if (bestLine.size() > 0)
        linesFound.push_back(bestLine);

      if (pipeline_data->isMultiline && bestCharArea.size() > 0)
      {

        vector<Point> next_best_line = findNextBestLine(Size(contours.width, contours.height), bestCharArea);

        if (next_best_line.size() > 0)
        {
          vector<Point> next_best_line_extended = extendToEdges(Size(contours.width, contours.height), next_best_line);
          linesFound.push_back(next_best_line_extended);
        }


      }
      if (pipeline_data->config->debugCharAnalysis) {
      			Mat debugImg(image.size(), image.type());
      			image.copyTo(debugImg);
      			for (unsigned int i = 0; i < charPoints.size(); i++) {
      				rectangle(debugImg, charPoints[i].boundingBox, Scalar(0, 255, 0));
      				line(debugImg, bestLine[0], bestLine[1], Scalar(255,0,255), 1);
      				line(debugImg, bestLine[3], bestLine[2], Scalar(255,0,255), 1);
      				  //charheights.push_back(charPoints[i].boundingBox.height);
      			}
      			displayImage(pipeline_data->config, "LINEFINDER 11", debugImg);
      		}

      return linesFound;
    }

  void LineFinder::addData(double x, double y, int n, bool isTop) {
	  if (isTop) {
	  if (n == 0) {
			  xbarTop = x;
			  ybarTop = y;
		  } else {
				  double fact1 = 1.0 + n;
				  double fact2 = n / (1.0 + n);
				  double dx = x - xbarTop;
				  double dy = y - ybarTop;
				  sumXXTop += dx * dx * fact2;
				  //sumYY += dy * dy * fact2;
				  sumXYTop += dx * dy * fact2;
				  xbarTop += dx / fact1;
				  ybarTop += dy / fact1;
		   }
		  sumXTop += x;
		  sumYTop += y;
	  } else {

		if (n == 0) {
		  xbarBottom = x;
		  ybarBottom = y;
		  } else {
				  double fact1 = 1.0 + n;
				  double fact2 = n / (1.0 + n);
				  double dx = x - xbarBottom;
				  double dy = y - ybarBottom;
				  sumXXBottom += dx * dx * fact2;
				  //sumYY += dy * dy * fact2;
				  sumXYBottom += dx * dy * fact2;
				  xbarBottom += dx / fact1;
				  ybarBottom += dy / fact1;
		   }
		  sumXBottom += x;
		  sumYBottom += y;
	  }
    }

    double LineFinder::predict(double x, double sumXX, double sumXY, double sumX, double sumY, int n) {
		double b1 = sumXY / sumXX;
		return (sumY - b1 * sumX) / n + b1 * x;
	}

//  void getApprox(double **x, double *a, double *b, int n) {
//    double sumx = 0;
//    double sumy = 0;
//    double sumx2 = 0;
//    double sumxy = 0;
//    for (int i = 0; i<n; i++) {
//      sumx += x[0][i];
//      sumy += x[1][i];
//      sumx2 += x[0][i] * x[0][i];
//      sumxy += x[0][i] * x[1][i];
//    }
//    *a = (n*sumxy - (sumx*sumy)) / (n*sumx2 - sumx*sumx);
//    *b = (sumy - *a*sumx) / n;
//    return;
//  }

    /*void LineFinder::addData(double x, double y, int n, bool isTop) {
    	if (pipeline_data->config->debugCharAnalysis) {
    		if (isTop)
    	        cout << "LINEFINDER: addData (top): [" << x << ", " << y << "]" << endl;
    		else
    			cout << "LINEFINDER: addData (bottom): [" << x << ", " << y << "]" << endl;
    	    }
    	if (isTop) {
    		sumXTop += x;
    		sumYTop += y;
    		sumXXTop += x*x;
		    sumXYTop += x*y;
		  } else {
			sumXBottom += x;
			sumYBottom += y;
			sumXXBottom += x*x;
			sumXYBottom += x*y;
		  }
    }

    double LineFinder::predict(double x, double sumXX, double sumXY, double sumX, double sumY, int n) {
		double a = (n*sumXY - (sumX*sumY)) / (n*sumXX - sumX*sumX);
		return (sumY - a * sumX) / n + a * x;
	}*/

  std::vector<cv::Point> LineFinder::calculateCroppedRegionForHistogram(cv::Size imageSize, std::vector<cv::Point> charArea) {
      
      LineSegment topLine(charArea[0], charArea[1]);
                  
      
      LineSegment new_top;
      LineSegment new_bottom;
      if (topLine.angle < 0)
      {
        float distance_from_top = topLine.p1.y;
        new_top = topLine.getParallelLine(distance_from_top);
        
        float distance_from_bottom = imageSize.height - topLine.p2.y;
        new_bottom = topLine.getParallelLine(-1 * distance_from_bottom);
      }
      else
      {
        float distance_from_top = topLine.p2.y;
        new_top = topLine.getParallelLine(distance_from_top);
        
        float distance_from_bottom = imageSize.height - topLine.p1.y;
        new_bottom = topLine.getParallelLine(-1 * distance_from_bottom);
      }

      
      vector<Point> points;
      points.push_back(new_top.p1);
      points.push_back(new_top.p2);
      points.push_back(new_bottom.p2);
      points.push_back(new_bottom.p1);
      
      return points;
  }
  
  
  std::vector<cv::Point> LineFinder::findNextBestLine(cv::Size imageSize, std::vector<cv::Point> bestLine) {

      // Pull out a crop of the plate around the line we know about,
      // then do a horizontal histogram on all the thresholds.  Find the other line based on that histogram
       
      vector<Point> histogramArea = calculateCroppedRegionForHistogram(imageSize, bestLine);
      
      Size cropped_quad_size(distanceBetweenPoints(histogramArea[0], histogramArea[1]), distanceBetweenPoints(histogramArea[0], histogramArea[3]));
      
      Mat mask = Mat::zeros(cropped_quad_size, CV_8U);
      bitwise_not(mask, mask);

      vector<Point2f> inputQuad;
      for (int i = 0; i < histogramArea.size(); i++)
        inputQuad.push_back(histogramArea[i]);
      vector<Point2f> outputQuad;
      outputQuad.push_back(Point2f(0,0));
      outputQuad.push_back(Point2f(cropped_quad_size.width,0));
      outputQuad.push_back(Point2f(cropped_quad_size.width,cropped_quad_size.height));
      outputQuad.push_back(Point2f(0,cropped_quad_size.height));
      
      int pxLeniency = 2;
      
      Mat trans_matrix = getPerspectiveTransform(inputQuad, outputQuad);
      vector<Point2f> orig_best_line;
      for (int i = 0; i < bestLine.size(); i++)
        orig_best_line.push_back(bestLine[i]);
      vector<Point2f> transformed_best_line;
      perspectiveTransform(orig_best_line, transformed_best_line, trans_matrix);
      
      int transformed_best_line_start = round(transformed_best_line[0].y);
      int transformed_best_line_end = round(transformed_best_line[3].y);
      int transformed_best_line_width = transformed_best_line_end - transformed_best_line_start;
      int transformed_best_line_variance = (int) ((float) transformed_best_line_width) * 0.1;//0.25;
      
      

      float lowest_width_diff = 99999999999;
      int best_secondline_index = -1;
      int best_secondline_threshold = -1;
      int best_secondline_top_pixel_offset_from_bestline_top = 0;
      int best_secondline_bottom_pixel_offset_from_bestline_top = 0;
      
      for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
      {
        Mat warpedImage = Mat::zeros(cropped_quad_size, CV_8U);
        warpPerspective(pipeline_data->thresholds[i], warpedImage, 
                        trans_matrix, 
                        cropped_quad_size);
        


        HistogramHorizontal histogram(warpedImage, mask);
        
        vector<pair<int, int> > histogram_hits = histogram.get1DHits(pxLeniency);
        
        // First find the histogram blob for the "best line" that we already found
        // Do this by comparing the "transformed_best_line" points to the histogram_hits
        
        
        int best_line_index = -1;
        for (unsigned int hitidx = 0; hitidx < histogram_hits.size(); hitidx++)
        {
          pair<int,int> hit = histogram_hits[hitidx];
          
          if ((hit.first >= transformed_best_line_start - transformed_best_line_variance) &&
              (hit.first <= transformed_best_line_start + transformed_best_line_variance) &&
              (hit.second >= transformed_best_line_end - transformed_best_line_variance) &&
              (hit.second <= transformed_best_line_end + transformed_best_line_variance))
          {           
            best_line_index = hitidx;
            break;
          }
                    
        }
        
        if (best_line_index < 0)  // Best line not found on this threshold...
        {
          if (pipeline_data->config->debugCharAnalysis)
            cout << "Could not find best line for multiline plate" << endl;
          continue;
        }

        if (pipeline_data->config->debugCharAnalysis)
          cout << "Found a multiline best line " << histogram_hits[best_line_index].first << " -> " << histogram_hits[best_line_index].second << endl;
        
        // Now look at all other hits and find one that is above or below our best line and has the correct text height ratio
        // I'm either looking for a bigger line above or a smaller line below (or vice versa, or two same sized lines depending on the plate config)
        
        // Assume maximum of two lines per plate for now
        
        // TODO: Use char_whitespace_between_lines_mm to score lines better
        
        //float best_line_width = histogram_hits[best_line_index].second - histogram_hits[best_line_index].first;
        if (pipeline_data->config->debugCharAnalysis)
          cout << "Ideal calculation: " << pipeline_data->config->charHeightMM[0] << " : " << pipeline_data->config->charHeightMM[1] << " - " << transformed_best_line_width << endl;
        
        float ideal_above_size = (pipeline_data->config->charHeightMM[0] / pipeline_data->config->charHeightMM[1]) * transformed_best_line_width;
        float ideal_below_size = (pipeline_data->config->charHeightMM[1] / pipeline_data->config->charHeightMM[0]) * transformed_best_line_width;
        
        float max_deviation_percent = 0.30;
        
        
        for (unsigned int hitidx = 0; hitidx < histogram_hits.size(); hitidx++)
        {
          if (hitidx == best_line_index)
            continue;
          
          float hit_width = histogram_hits[hitidx].second -  histogram_hits[hitidx].first;
                    
          float ideal_width;
          if (histogram_hits[hitidx].second <= histogram_hits[best_line_index].first)
            ideal_width = ideal_above_size;
          else if (histogram_hits[hitidx].first >= histogram_hits[best_line_index].second)
            ideal_width = ideal_below_size;
          else
            assert(false);
          
          if (pipeline_data->config->debugCharAnalysis)
            cout << "Hit Width: " << hit_width << " -- ideal width: " << ideal_width << endl;
          
          if ((hit_width >= ideal_width * (1-max_deviation_percent)) &&
              (hit_width <= ideal_width * (1+max_deviation_percent)))
          {
            float diff_from_ideal = hit_width - ideal_width;
            if (diff_from_ideal < lowest_width_diff)
            {
              lowest_width_diff = diff_from_ideal;
              best_secondline_index = hitidx;
              best_secondline_threshold = i;
              best_secondline_top_pixel_offset_from_bestline_top = (histogram_hits[best_line_index].first - histogram_hits[hitidx].first);
              best_secondline_bottom_pixel_offset_from_bestline_top = (histogram_hits[best_line_index].first - histogram_hits[hitidx].second);
            }
          }
 
            
        }
          
        
      }
      
      Mat debugImg(pipeline_data->thresholds[1].size(), pipeline_data->thresholds[1].type());
      pipeline_data->thresholds[1].copyTo(debugImg);
      cvtColor(debugImg, debugImg, CV_GRAY2BGR);
      
      LineSegment orig_top_line(bestLine[0], bestLine[1]);
      LineSegment secondline_top = orig_top_line.getParallelLine(best_secondline_top_pixel_offset_from_bestline_top + 1);
      LineSegment secondline_bottom = orig_top_line.getParallelLine(best_secondline_bottom_pixel_offset_from_bestline_top - 1);
      
      line(debugImg, orig_top_line.p1, orig_top_line.p2, Scalar(0,0,255), 2);
      line(debugImg, secondline_top.p1, secondline_top.p2, Scalar(255,255,0), 2);
      line(debugImg, secondline_bottom.p1, secondline_bottom.p2, Scalar(0,255,0), 2);
      
      if (pipeline_data->config->debugCharAnalysis)
      {
        cout << "Multiline = " << secondline_top.str() << " -- " << secondline_bottom.str() << endl;
        cout << "Multiline winner is: " << best_secondline_index << " on threshold " << best_secondline_threshold << endl;
        //displayImage(pipeline_data->config, "findNextBestLine", tempImg);
      }
            
      vector<cv::Point> response;
      
      if (best_secondline_index >= 0)
      {
        response.push_back(secondline_top.p1);
        response.push_back(secondline_top.p2);
        response.push_back(secondline_bottom.p1);
        response.push_back(secondline_bottom.p2);
      }
      
      return response;
  }


  // Returns a polygon "stripe" across the width of the character region.  The lines are voted and the polygon starts at 0 and extends to image width
  vector<Point> LineFinder::getBestLine(const TextContours contours, vector<CharPointInfo> charPoints)
  {
    vector<Point> bestStripe;

    // Find the best fit line segment that is parallel with the most char segments
    if (charPoints.size() <= 1)
    {
      // Maybe do something about this later, for now let's just ignore
      return bestStripe;
    }


    vector<int> charheights;
    for (unsigned int i = 0; i < charPoints.size(); i++)
      charheights.push_back(charPoints[i].boundingBox.height);
    float medianCharHeight = median(charheights.data(), charheights.size());



    vector<LineSegment> topLines;
    vector<LineSegment> bottomLines;
    // Iterate through each possible char and find all possible lines for the top and bottom of each char segment
    for (unsigned int i = 0; i < charPoints.size() - 1; i++)
    {
      for (unsigned int k = i+1; k < charPoints.size(); k++)
      {

        int leftCPIndex, rightCPIndex;
        if (charPoints[i].top.x < charPoints[k].top.x)
        {
          leftCPIndex = i;
          rightCPIndex = k;
        }
        else
        {
          leftCPIndex = k;
          rightCPIndex = i;
        }


        LineSegment top(charPoints[leftCPIndex].top, charPoints[rightCPIndex].top);
        LineSegment bottom(charPoints[leftCPIndex].bottom, charPoints[rightCPIndex].bottom);



        LineSegment parallelBot = top.getParallelLine(medianCharHeight * -1);
        LineSegment parallelTop = bottom.getParallelLine(medianCharHeight);

        // Only allow lines that have a sane angle
        if (abs(top.angle) <= pipeline_data->config->maxPlateAngleDegrees &&
            abs(parallelBot.angle) <= pipeline_data->config->maxPlateAngleDegrees)
        {
          topLines.push_back(top);
          bottomLines.push_back(parallelBot);
        }

        // Only allow lines that have a sane angle
        if (abs(parallelTop.angle) <= pipeline_data->config->maxPlateAngleDegrees &&
            abs(bottom.angle) <= pipeline_data->config->maxPlateAngleDegrees)
        {
          topLines.push_back(parallelTop);
          bottomLines.push_back(bottom);
        }
      }
    }

    int bestScoreIndex = 0;
    int bestScore = -1;
    int bestScoreDistance = -1; // Line segment distance is used as a tie breaker

    // Now, among all possible lines, find the one that is the best fit
    for (unsigned int i = 0; i < topLines.size(); i++)
    {
      float SCORING_MIN_THRESHOLD = 0.97;
      float SCORING_MAX_THRESHOLD = 1.03;

      int curScore = 0;
      for (unsigned int charidx = 0; charidx < charPoints.size(); charidx++)
      {
        float topYPos = topLines[i].getPointAt(charPoints[charidx].top.x);
        float botYPos = bottomLines[i].getPointAt(charPoints[charidx].bottom.x);

        float minTop = charPoints[charidx].top.y * SCORING_MIN_THRESHOLD;
        float maxTop = charPoints[charidx].top.y * SCORING_MAX_THRESHOLD;
        float minBot = (charPoints[charidx].bottom.y) * SCORING_MIN_THRESHOLD;
        float maxBot = (charPoints[charidx].bottom.y) * SCORING_MAX_THRESHOLD;
        if ( (topYPos >= minTop && topYPos <= maxTop) &&
             (botYPos >= minBot && botYPos <= maxBot))
        {
          curScore++;
        }

      }

      // Tie goes to the one with longer line segments
      if ((curScore > bestScore) ||
          (curScore == bestScore && topLines[i].length > bestScoreDistance))
      {
        bestScore = curScore;
        bestScoreIndex = i;
        // Just use x distance for now
        bestScoreDistance = topLines[i].length;
      }
    }

    if (bestScore < 0)
      return bestStripe;

    if (pipeline_data->config->debugCharAnalysis)
    {
      cout << "The winning score is: " << bestScore << endl;
      // Draw the winning line segment

      Mat tempImg = Mat::zeros(Size(contours.width, contours.height), CV_8U);
      cvtColor(tempImg, tempImg, CV_GRAY2BGR);

      cv::line(tempImg, topLines[bestScoreIndex].p1, topLines[bestScoreIndex].p2, Scalar(0, 0, 255), 2);
      cv::line(tempImg, bottomLines[bestScoreIndex].p1, bottomLines[bestScoreIndex].p2, Scalar(0, 0, 255), 2);

      displayImage(pipeline_data->config, "Winning lines", tempImg);
    }

    bestStripe.push_back(topLines[bestScoreIndex].p1);
    bestStripe.push_back(topLines[bestScoreIndex].p2);
    bestStripe.push_back(bottomLines[bestScoreIndex].p2);
    bestStripe.push_back(bottomLines[bestScoreIndex].p1);

    return bestStripe;
  }

  std::vector<cv::Point> LineFinder::extendToEdgesAndPoints(cv::Size imageSize, std::vector<cv::Point> charArea, Point topPoint, Point bottomPoint) {
    
    vector<Point> extended;
    
            
    if (charArea.size() < 4)
      return extended;
            
    LineSegment top(charArea[0], charArea[1]);
    LineSegment bottom(charArea[3], charArea[2]);
    LineSegment parallelTop;
    LineSegment parallelBottom;

    Point pointOnTopLine(topPoint.x, top.getPointAt(topPoint.x));
    if (pointOnTopLine.y - topPoint.y > 0) {
    	parallelTop = top.getParallelLine(pointOnTopLine.y - topPoint.y);
    	if (pipeline_data->config->debugCharAnalysis)
    	      cout << "LINEFINDER move best TOP line, diff=: " << (pointOnTopLine.y - topPoint.y) << endl;

    } else {
    	parallelTop = LineSegment(charArea[0], charArea[1]);
    	if (pipeline_data->config->debugCharAnalysis)
    	      cout << "LINEFINDER best TOP line should not be changed: topPoint.y=" << topPoint.y << ", pointOnTopLine.y=" << pointOnTopLine.y << endl;
    }

    Point pointOnBottomLine(bottomPoint.x, bottom.getPointAt(bottomPoint.x));
	if (pointOnBottomLine.y - bottomPoint.y < 0) {
		parallelBottom = bottom.getParallelLine(pointOnBottomLine.y - bottomPoint.y);
		if (pipeline_data->config->debugCharAnalysis)
		    cout << "LINEFINDER move best BOTTOM line, diff=: " << (pointOnBottomLine.y - bottomPoint.y) << endl;
	} else {
		parallelBottom = LineSegment(charArea[3], charArea[2]);
		if (pipeline_data->config->debugCharAnalysis)
		    	      cout << "LINEFINDER best BOTTOM line should not be changed: bottomPoint.y=" << bottomPoint.y << ", pointOnBottomLine.y=" << pointOnBottomLine.y << endl;
	}

    Point topLeft 		= Point(0, parallelTop.getPointAt(0));
    Point topRight 		= Point(imageSize.width, parallelTop.getPointAt(imageSize.width));
    Point bottomRight 	= Point(imageSize.width, parallelBottom.getPointAt(imageSize.width));
    Point bottomLeft 	= Point(0, parallelBottom.getPointAt(0));

    extended.push_back(topLeft);
    extended.push_back(topRight);
    extended.push_back(bottomRight);
    extended.push_back(bottomLeft);
    
    return extended;
  }
  
  std::vector<cv::Point> LineFinder::extendToEdgesAndPoints(cv::Size imageSize, std::vector<cv::Point> charArea, const TextContours contours) {
	  vector<Point> extended;


	  if (charArea.size() < 4)
		return extended;

	  LineSegment top(charArea[0], charArea[1]);
	  LineSegment bottom(charArea[3], charArea[2]);
	  LineSegment parallelTop = LineSegment(charArea[0], charArea[1]);
	  LineSegment parallelBottom = LineSegment(charArea[3], charArea[2]);

	  if (pipeline_data->config->debugCharAnalysis)
	  	cout << "LINEFINDER initial parallelTop[(" << parallelTop.p1.x << ", " << parallelTop.p1.y << "), (" << parallelTop.p2.x << ", " << parallelTop.p2.y << ")]" << endl;

	  unsigned int goodIndicesQty = 0;
	  int sumHeight = 0;

	  for (unsigned int i = 0; i < contours.contours.size(); i++)
	  {
		if (contours.goodIndices[i] == false)
		  continue;

		goodIndicesQty++;

		CharPointInfo cpi = CharPointInfo(contours.contours[i], i);
		sumHeight += cpi.boundingBox.height;

		Point pointOnTopLine(cpi.top.x, parallelTop.getPointAt(cpi.top.x));
		  if (pointOnTopLine.y - cpi.top.y > 0) {
			parallelTop = parallelTop.getParallelLine(pointOnTopLine.y - cpi.top.y);
			if (pipeline_data->config->debugCharAnalysis)
				  cout << "LINEFINDER move best TOP line for goodIndices[" << goodIndicesQty << "], diff=: " << (pointOnTopLine.y - cpi.top.y) << endl;

		  } else {
			if (pipeline_data->config->debugCharAnalysis)
				  cout << "LINEFINDER best TOP line should not be changed for goodIndices[" << goodIndicesQty << "]: cpi.top.y=" << cpi.top.y << ", pointOnTopLine.y=" << pointOnTopLine.y << endl;
		  }

		Point pointOnBottomLine(cpi.bottom.x, parallelBottom.getPointAt(cpi.bottom.x));
		if (pointOnBottomLine.y - cpi.bottom.y < 0) {
			parallelBottom = parallelBottom.getParallelLine(pointOnBottomLine.y - cpi.bottom.y);
			if (pipeline_data->config->debugCharAnalysis)
				cout << "LINEFINDER move best BOTTOM line for goodIndices[" << goodIndicesQty << "], diff=: " << (pointOnBottomLine.y - cpi.bottom.y) << endl;
		} else {
			if (pipeline_data->config->debugCharAnalysis)
						  cout << "LINEFINDER best BOTTOM line should not be changed for goodIndices[" << goodIndicesQty << ": cpi.bottom.y=" << cpi.bottom.y << ", pointOnBottomLine.y=" << pointOnBottomLine.y << endl;
		}

	  }
//	  Point topLeft 		= Point(0, parallelTop.getPointAt(0));
//	  Point topRight 		= Point(imageSize.width, parallelTop.getPointAt(imageSize.width));
//	  Point bottomRight 	= Point(imageSize.width, parallelBottom.getPointAt(imageSize.width));
//	  Point bottomLeft 	= Point(0, parallelBottom.getPointAt(0));

	    float avgCharHeight = sumHeight / goodIndicesQty;
	    int expandY = round(0.1*avgCharHeight);
	    LineSegment topLine = parallelTop.getParallelLine(expandY);
		LineSegment bottomLine = parallelBottom.getParallelLine(-1 * expandY);

//		LineSegment leftLine(Point(0,0), Point(0, expandY + h));
//		LineSegment rightLine(Point(verticalRightOffset + w, -1 * expandY), Point(verticalRightOffset + w, expandY + h));
//
//		Point topLeft = topLine.intersection(leftLine);
//		Point topRight = topLine.intersection(rightLine);
//		Point botRight = bottomLine.intersection(rightLine);
//		Point botLeft = bottomLine.intersection(leftLine);

		Point topLeft 		= Point(0, topLine.getPointAt(0));
		Point topRight 		= Point(imageSize.width, topLine.getPointAt(imageSize.width));
		Point bottomRight 	= Point(imageSize.width, bottomLine.getPointAt(imageSize.width));
		Point bottomLeft 	= Point(0, bottomLine.getPointAt(0));

		if (pipeline_data->config->debugCharAnalysis)
			  	  	cout << "LINEFINDER new parallelTop[(" << parallelTop.p1.x << ", " << parallelTop.p1.y << "), (" << parallelTop.p2.x << ", " << parallelTop.p2.y << ")]" << endl;

	  extended.push_back(topLeft);
	  extended.push_back(topRight);
	  extended.push_back(bottomRight);
	  extended.push_back(bottomLeft);

	  return extended;
  }

  std::vector<cv::Point> LineFinder::extendToEdges(cv::Size imageSize, std::vector<cv::Point> charArea) {

      vector<Point> extended;


      if (charArea.size() < 4)
        return extended;

      LineSegment top(charArea[0], charArea[1]);
      LineSegment bottom(charArea[3], charArea[2]);

      Point topLeft 		= Point(0, top.getPointAt(0) );
      Point topRight 		= Point(imageSize.width, top.getPointAt(imageSize.width));
      Point bottomRight 	= Point(imageSize.width, bottom.getPointAt(imageSize.width));
      Point bottomLeft 	= Point(0, bottom.getPointAt(0));

      extended.push_back(topLeft);
      extended.push_back(topRight);
      extended.push_back(bottomRight);
      extended.push_back(bottomLeft);

      return extended;
    }

  CharPointInfo::CharPointInfo(vector<Point> contour, int index) {


    this->contourIndex = index;

    this->boundingBox = cv::boundingRect( Mat(contour) );


    int x = boundingBox.x + (boundingBox.width / 2);
    int y = boundingBox.y;

    this->top = Point(x, y);

    x = boundingBox.x + (boundingBox.width / 2);
    y = boundingBox.y + boundingBox.height;

    this->bottom = Point(x,y);

  }
  
}
