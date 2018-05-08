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

#include "charactersegmenter.h"

using namespace cv;
using namespace std;

namespace alpr
{

  CharacterSegmenter::CharacterSegmenter(PipelineData* pipeline_data)
  {
    this->pipeline_data = pipeline_data;
    this->config = pipeline_data->config;

    this->confidence = 0;

    if (this->config->debugCharSegmenter)
      cout << "Starting CharacterSegmenter" << endl;

    //CharacterRegion charRegion(img, debug);


    if (pipeline_data->plate_inverted)
      bitwise_not(pipeline_data->crop_gray, pipeline_data->crop_gray);
    pipeline_data->clearThresholds();
    pipeline_data->thresholds = produceThresholds(pipeline_data->crop_gray, config);

    // TODO: Perhaps a bilateral filter would be better here.
    medianBlur(pipeline_data->crop_gray, pipeline_data->crop_gray, 3);
    //bilateralFilter(pipeline_data->crop_gray, pipeline_data->crop_gray, 3, 45, 45);


    /*bilateralFilter(inputImage, smoothed, 3, 45, 45);
    Mat edges(inputImage.size(), inputImage.type());
    Canny(smoothed, edges, 66, 133);*/

    if (this->config->debugCharSegmenter) {
      cout << "Segmenter: inverted: " << pipeline_data->plate_inverted << endl;
      displayImage(config, "CharacterSegmenter  Thresholds (Init)", drawImageDashboard(pipeline_data->thresholds, CV_8U, 3));
    }

  }
  
  void CharacterSegmenter::removeSmallContoursPreprocess(TextLine textLine) {
	  for (unsigned int lineidx = 0; lineidx < pipeline_data->textLines.size(); lineidx++)
	      {
	        //orig
	        //todo TEST1
	        float avgCharHeight = pipeline_data->textLines[lineidx].lineHeight;
	        float height_to_width_ratio = pipeline_data->config->charHeightMM[lineidx] / pipeline_data->config->charWidthMM[lineidx];
	        float avgCharWidth = avgCharHeight / height_to_width_ratio;
	        int sumWidth = 0;
	        int sumHeight = 0;
	        int cntGood = 0;
	        for (unsigned int i = 0; i < pipeline_data->textLines[lineidx].textContours.size(); i++)
	            {
	              if (pipeline_data->textLines[lineidx].textContours.goodIndices[i] == false)
	                continue;
	              cntGood++;
	              Rect mr= boundingRect(pipeline_data->textLines[lineidx].textContours.contours[i]);
	              sumWidth += mr.width;
	              sumHeight += mr.height;
	            }

	        //140+3
		    //float avgCharWidth = sumWidth / cntGood;
	        //float avgCharHeight = min((float)sumHeight / cntGood, pipeline_data->textLines[lineidx].lineHeight);

	        if (config->debugCharSegmenter)
	        {
	          cout << "LINE " << lineidx << ": avgCharHeight: " << avgCharHeight << endl;
	          cout << "LINE " << lineidx << ": avgCharWidth: " << avgCharWidth << endl;
	        }


	        removeSmallContours2(avgCharHeight, avgCharWidth, pipeline_data->textLines[lineidx]);
	        //removeSmallContours(pipeline_data->thresholds, avgCharHeight, pipeline_data->textLines[lineidx]);

	        if (this->config->debugCharSegmenter)
	  		{
	  		  Mat imgDash = drawImageDashboard(pipeline_data->thresholds, CV_8U, 3);
	  		  line(imgDash, top.p1, top.p2, Scalar(100, 100, 100), 2, CV_AA);
	  		  line(imgDash, bottom.p1, bottom.p2, Scalar(100, 100, 100), 1, CV_AA);
	  		  displayImage(config, "Segmentation after removeSmallContours", imgDash);
	  		}
	      }
  }

  void CharacterSegmenter::segment() {

    timespec startTime;
    getTimeMonotonic(&startTime);
    
    if (this->config->debugCharSegmenter)
    {
      displayImage(config, "CharacterSegmenter  Thresholds", drawImageDashboard(pipeline_data->thresholds, CV_8U, 3));
    }

    Mat edge_filter_mask = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8U);
    bitwise_not(edge_filter_mask, edge_filter_mask);

    for (unsigned int lineidx = 0; lineidx < pipeline_data->textLines.size(); lineidx++)
    {
      this->top = pipeline_data->textLines[lineidx].topLine;
      this->bottom = pipeline_data->textLines[lineidx].bottomLine;

      //orig
      float avgCharHeight = pipeline_data->textLines[lineidx].lineHeight;
      float height_to_width_ratio = pipeline_data->config->charHeightMM[lineidx] / pipeline_data->config->charWidthMM[lineidx];
      float avgCharWidth = avgCharHeight / height_to_width_ratio;


      /*int sumWidth = 0;
      int sumHeight = 0;
      int cntGood = 0;
      for (unsigned int i = 0; i < pipeline_data->textLines[lineidx].textContours.size(); i++)
          {
            if (pipeline_data->textLines[lineidx].textContours.goodIndices[i] == false)
              continue;
            cntGood++;
            Rect mr= boundingRect(pipeline_data->textLines[lineidx].textContours.contours[i]);
            sumWidth += mr.width;
            sumHeight += mr.height;
          }

      float avgCharWidth = 1.2 * (sumWidth / cntGood);
      float avgCharHeight = min((float)1.05*sumHeight / cntGood, pipeline_data->textLines[lineidx].lineHeight);*/

      if (config->debugCharSegmenter)
      {
        //cout << "LINE " << lineidx << ": avgCharHeight: " << avgCharHeight << " - height_to_width_ratio: " << height_to_width_ratio << endl;
    	  cout << "LINE " << lineidx << ": avgCharHeight: " << avgCharHeight << endl;
          cout << "LINE " << lineidx << ": avgCharWidth: " << avgCharWidth << endl;
      }
      
      
      //removeSmallContours(pipeline_data->thresholds, avgCharHeight, avgCharWidth, pipeline_data->textLines[lineidx]);
      //removeSmallContoursPreprocess(pipeline_data->thresholds, pipeline_data->textLines[lineidx]);

      if (this->config->debugCharSegmenter)
      		{
      		  Mat imgDash = drawImageDashboard(pipeline_data->thresholds, CV_8U, 3);
      		  line(imgDash, top.p1, top.p2, Scalar(255, 0, 0), 1, CV_AA);
      		  line(imgDash, bottom.p1, bottom.p2, Scalar(0, 0, 255), 1, CV_AA);
      		  displayImage(config, "Segmentation after removeSmallContours", imgDash);
      		}

      // Do the histogram analysis to figure out char regions

      timespec startTime;
      getTimeMonotonic(&startTime);

      vector<Mat> allHistograms;

      vector<Rect> lineBoxes;
      for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
      {
        Mat histogramMask = Mat::zeros(pipeline_data->thresholds[i].size(), CV_8U);

        fillConvexPoly(histogramMask, pipeline_data->textLines[lineidx].linePolygon.data(), pipeline_data->textLines[lineidx].linePolygon.size(), Scalar(255,255,255));
        HistogramVertical vertHistogram(pipeline_data->thresholds[i], histogramMask);

        if (this->config->debugCharSegmenter)
        {
          Mat histoCopy(vertHistogram.histoImg.size(), vertHistogram.histoImg.type());

          vertHistogram.histoImg.copyTo(histoCopy);
          cvtColor(vertHistogram.histoImg, histoCopy, CV_GRAY2RGB);

          string label = "threshold: " + toString(i);
          allHistograms.push_back(addLabel(histoCopy, label));

          std::cout << histoCopy.cols << " x " << histoCopy.rows << std::endl;
        }

        float score = 0;
        vector<Rect> charBoxes = getHistogramBoxes(vertHistogram, avgCharWidth, avgCharHeight, &score);
//        if (this->config->debugCharSegmenter)
//        {
//          for (unsigned int cboxIdx = 0; cboxIdx < charBoxes.size(); cboxIdx++)
//          {
//            rectangle(allHistograms[i], charBoxes[cboxIdx], Scalar(0, 255, 0));
//          }
//          Mat histDashboard = drawImageDashboard(allHistograms, allHistograms[0].type(), 1);
//          displayImage(this->config, "Char seg histograms", histDashboard);
//        }

        for (unsigned int z = 0; z < charBoxes.size(); z++)
          lineBoxes.push_back(charBoxes[z]);
        //drawAndWait(&histogramMask);
      }


      if (config->debugTiming)
      {
        timespec endTime;
        getTimeMonotonic(&endTime);
        cout << "  -- Character Segmentation Create and Score Histograms Time: " << diffclock(startTime, endTime) << "ms." << endl;
      }

      vector<Rect> candidateBoxes = getBestCharBoxes(pipeline_data->thresholds[0], lineBoxes, avgCharWidth);

        // Setup the dashboard images to show the cleaning filters
        for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
        {
        	int cntGood = 0;

        	for (unsigned int n = 0; n < candidateBoxes.size(); n++) {
				  Rect rHist = candidateBoxes[n];
				  int bottomY = 0;
				  int topY = rHist.y + rHist.height;
				  bool setTop = false;
				  bool setBottom = false;
				  for (unsigned int c = 0; c < pipeline_data->textLines[0].textContours.contours.size(); c++) {
					  if (pipeline_data->textLines[0].textContours.goodIndices[c] == false)
							continue;
					  cntGood++;
					  Rect r = boundingRect(pipeline_data->textLines[0].textContours.contours[c]);
					  if (r.x > rHist.x + rHist.width) {
						  break;
					  }

					  int xCenter =  r.x + (r.width / 2);
					  if (rHist.x <= xCenter && rHist.x + rHist.width >= xCenter) {
						  if (r.y + r.height > bottomY) {
							  bottomY = r.y + r.height;
							  setBottom = true;
						  }
						  if (r.y < topY) {
							  topY = r.y;
							  setTop = true;
						  }
						  if (rHist.y < r.y - 2) {
							  if (config->debugCharSegmenter)
								  cout << "thresholds[" << i << "],  cntGood: " << cntGood << ", candidateBoxes[" << n << "]" << " TOP" << endl;
							  //Rect topRect(rHist.x, rHist.y, rHist.width, r.y - 2 - rHist.y);
							  Rect topRect(r.x, rHist.y, r.width, r.y - 2 - rHist.y);
							  rectangle(pipeline_data->thresholds[i], topRect, Scalar(0,255,0), CV_FILLED);
						  }
						  if (rHist.y + rHist.height > r.y + r.height + 2) {
							  if (config->debugCharSegmenter)
								  cout << "thresholds[" << i << "],  cntGood: " << cntGood << ", candidateBoxes[" << n << "]" << " BOTTOM" << endl;
							  //Rect bottomRect(rHist.x, r.y + r.height + 2, rHist.width, rHist.y + rHist.height - (r.y + r.height + 2));
							  Rect bottomRect(r.x, r.y + r.height + 2, r.width, rHist.y + rHist.height - (r.y + r.height + 2));
							  rectangle(pipeline_data->thresholds[i], bottomRect, Scalar(0,255,0), CV_FILLED);
						  }
						  /*if (r.x - 2 > rHist.x) {
							  if (config->debugCharSegmenter)
								cout << "thresholds[" << i << "],  cntGood: " << cntGood << ", candidateBoxes[" << n << "]" << " LEFT" << endl;
							  Rect leftRect(rHist.x, rHist.y, r.x - 2 - rHist.x, rHist.height);
							  rectangle(pipeline_data->thresholds[i], leftRect, Scalar(0,255,0), CV_FILLED);
						  }
						  if (rHist.x + rHist.width > r.x + r.width + 2) {
							  if (config->debugCharSegmenter)
								cout << "thresholds[" << i << "],  cntGood: " << cntGood << ", candidateBoxes[" << n << "]" << " RIGHT" << endl;
							  Rect rightRect(r.x + r.width + 2, rHist.y, rHist.x + rHist.width - (r.x + r.width + 2), rHist.height);
							  rectangle(pipeline_data->thresholds[i], rightRect, Scalar(0,255,0), CV_FILLED);
						  } else {
							  if (config->debugCharSegmenter)
								cout << "thresholds[" << i << "],  cntGood: " << cntGood << ", candidateBoxes[" << n << "]" << " NOT RIGHT: rHist.x=" << rHist.x
								<< ", r.x=" << r.x << ", rHist.width=" << rHist.width << ", r.width=" << r.width << endl;
						  }*/
					  }

			  }
			  if (setTop && topY - 2 > rHist.y) {
				  Rect topRect(rHist.x, rHist.y, rHist.width, topY - 2 - rHist.y);
				  rectangle(pipeline_data->thresholds[i], topRect, Scalar(0,255,0), CV_FILLED);
			  }
			  if (setBottom && bottomY + 2 < rHist.y + rHist.height) {
				  Rect bottomRect(rHist.x, bottomY + 2, rHist.width, rHist.y + rHist.height - (bottomY + 2));
				  rectangle(pipeline_data->thresholds[i], bottomRect, Scalar(0,255,0), CV_FILLED);
			  }
		  }

          if (this->config->debugCharSegmenter)
		  {
			Mat imgDash = drawImageDashboard(pipeline_data->thresholds, CV_8U, 3);
			displayImage(config, "Segmentation after clean with contours rectangles", imgDash);
		  }
		  
		  Mat cleanImg = Mat::zeros(pipeline_data->thresholds[i].size(), pipeline_data->thresholds[i].type());
          Mat boxMask = getCharBoxMask(pipeline_data->thresholds[i], candidateBoxes);
          pipeline_data->thresholds[i].copyTo(cleanImg);
          bitwise_and(cleanImg, boxMask, cleanImg);
          cvtColor(cleanImg, cleanImg, CV_GRAY2BGR);

          for (unsigned int c = 0; c < candidateBoxes.size(); c++)
            rectangle(cleanImg, candidateBoxes[c], Scalar(0, 255, 0), 1);

          for (unsigned int c = 0; c < pipeline_data->textLines[0].textContours.contours.size(); c++) {
			  if (pipeline_data->textLines[0].textContours.goodIndices[c] == true) {
				  Rect r = boundingRect(pipeline_data->textLines[0].textContours.contours[c]);
				  rectangle(cleanImg, r, Scalar(255, 255, 0), 1);
			  }
		  }

          imgDbgCleanStages.push_back(cleanImg);
          //string name = "imgDbgCleanStages_" + i;
          //displayImage(config, name, cleanImg);
        }

      getTimeMonotonic(&startTime);

      Mat edge_mask = filterEdgeBoxes(pipeline_data->thresholds, candidateBoxes, avgCharWidth, avgCharHeight);
      bitwise_and(edge_filter_mask, edge_mask, edge_filter_mask);
      
      candidateBoxes = combineCloseBoxes(candidateBoxes);

      candidateBoxes = filterMostlyEmptyBoxes(pipeline_data->thresholds, candidateBoxes);

      pipeline_data->charRegions.push_back(candidateBoxes);
      for (unsigned int cboxidx = 0; cboxidx < candidateBoxes.size(); cboxidx++)
        pipeline_data->charRegionsFlat.push_back(candidateBoxes[cboxidx]);

      if (config->debugTiming)
      {
        timespec endTime;
        getTimeMonotonic(&endTime);
        cout << "  -- Character Segmentation Box cleaning/filtering Time: " << diffclock(startTime, endTime) << "ms." << endl;
      }

      if (this->config->debugCharSegmenter)
      {
        Mat imgDash = drawImageDashboard(pipeline_data->thresholds, CV_8U, 3);
        displayImage(config, "Segmentation after cleaning", imgDash);

        Mat generalDash = drawImageDashboard(this->imgDbgGeneral, this->imgDbgGeneral[0].type(), 2);
        displayImage(config, "Segmentation General", generalDash);

        Mat cleanImgDash = drawImageDashboard(this->imgDbgCleanStages, this->imgDbgCleanStages[0].type(), 3);
        displayImage(config, "Segmentation Clean Filters", cleanImgDash);


      }
    }
    
    if (this->config->debugCharSegmenter) {
    	for (unsigned int cboxidx = 0; cboxidx < pipeline_data->charRegionsFlat.size(); cboxidx++)
    		cout << "FINAL pipeline_data->charRegionsFlat[" << cboxidx << "]: " << pipeline_data->charRegionsFlat[cboxidx] << endl;
    }


//    CharacterAnalysis* textAnalysis = new alpr::CharacterAnalysis();
//    textAnalysis->findTextContours(pipeline_data);

    // Apply the edge mask (left and right ends) after all lines have been processed.
    for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
    {
      bitwise_and(pipeline_data->thresholds[i], edge_filter_mask, pipeline_data->thresholds[i]);
    }

    vector<Rect> all_regions_combined;
    for (unsigned int lidx = 0; lidx < pipeline_data->charRegions.size(); lidx++)
    {
      for (unsigned int boxidx = 0; boxidx < pipeline_data->charRegions[lidx].size(); boxidx++)
        all_regions_combined.push_back(pipeline_data->charRegions[lidx][boxidx]);
    }
    cleanCharRegions(pipeline_data->thresholds, all_regions_combined);

    if (config->debugTiming)
    {
      timespec endTime;
      getTimeMonotonic(&endTime);
      cout << "Character Segmenter Time: " << diffclock(startTime, endTime) << "ms." << endl;
      Mat imgDash = drawImageDashboard(pipeline_data->thresholds, CV_8U, 3);
      displayImage(config, "Segmentation after SECOND cleaning", imgDash);
    }
  }

  

  CharacterSegmenter::~CharacterSegmenter()
  {

  }

  // Given a histogram and the horizontal line boundaries, respond with an array of boxes where the characters are
  // Scores the histogram quality as well based on num chars, char volume, and even separation
  vector<Rect> CharacterSegmenter::getHistogramBoxes(HistogramVertical histogram, float avgCharWidth, float avgCharHeight, float* score)
  {
    float MIN_HISTOGRAM_HEIGHT = avgCharHeight * config->segmentationMinCharHeightPercent;

    float MAX_SEGMENT_WIDTH = avgCharWidth * config->segmentationMaxCharWidthvsAverage;

    //float MIN_BOX_AREA = (avgCharWidth * avgCharHeight) * 0.25;

    int pxLeniency = 2;

    vector<Rect> charBoxes;
    vector<Rect> allBoxes = convert1DHitsToRect(histogram.get1DHits(pxLeniency), top, bottom);

    for (unsigned int i = 0; i < allBoxes.size(); i++)
    {
      if (allBoxes[i].width >= config->segmentationMinBoxWidthPx && allBoxes[i].width <= MAX_SEGMENT_WIDTH &&
          allBoxes[i].height > MIN_HISTOGRAM_HEIGHT )
      {
        charBoxes.push_back(allBoxes[i]);
      }
      else if (allBoxes[i].width > avgCharWidth * 2 && allBoxes[i].width < MAX_SEGMENT_WIDTH * 2 && allBoxes[i].height > MIN_HISTOGRAM_HEIGHT)
      {
        //    rectangle(histogram.histoImg, allBoxes[i], Scalar(255, 0, 0) );
        //    drawAndWait(&histogram.histoImg);
        // Try to split up doubles into two good char regions, check for a break between 40% and 60%
        int leftEdge = allBoxes[i].x + (int) (((float) allBoxes[i].width) * 0.4f);
        int rightEdge = allBoxes[i].x + (int) (((float) allBoxes[i].width) * 0.6f);

        int minX = histogram.getLocalMinimum(leftEdge, rightEdge);
        int maxXChar1 = histogram.getLocalMaximum(allBoxes[i].x, minX);
        int maxXChar2 = histogram.getLocalMaximum(minX, allBoxes[i].x + allBoxes[i].width);
        int minHeight = histogram.getHeightAt(minX);

        int maxHeightChar1 = histogram.getHeightAt(maxXChar1);
        int maxHeightChar2 = histogram.getHeightAt(maxXChar2);

        if (maxHeightChar1 > MIN_HISTOGRAM_HEIGHT && minHeight < (0.25 * ((float) maxHeightChar1)))
        {
          // Add a box for Char1
          Point botRight = Point(minX - 1, allBoxes[i].y + allBoxes[i].height);
          charBoxes.push_back(Rect(allBoxes[i].tl(), botRight) );
        }
        if (maxHeightChar2 > MIN_HISTOGRAM_HEIGHT && minHeight < (0.25 * ((float) maxHeightChar2)))
        {
          // Add a box for Char2
          Point topLeft = Point(minX + 1, allBoxes[i].y);
          charBoxes.push_back(Rect(topLeft, allBoxes[i].br()) );
        }
      }
    }

    return charBoxes;
  }

  vector<Rect> CharacterSegmenter::getBestCharBoxes(Mat img, vector<Rect> charBoxes, float avgCharWidth)
  {
    float MAX_SEGMENT_WIDTH = avgCharWidth * config->segmentationMaxCharWidthvsAverage;

    // This histogram is based on how many char boxes (from ALL of the many thresholded images) are covering each column
    // Makes a sort of histogram from all the previous char boxes.  Figures out the best fit from that.

    Mat histoImg = Mat::zeros(Size(img.cols, img.rows), CV_8U);

    int columnCount;

    for (int col = 0; col < img.cols; col++)
    {
      columnCount = 0;

      for (unsigned int i = 0; i < charBoxes.size(); i++)
      {
        if (col >= charBoxes[i].x && col < (charBoxes[i].x + charBoxes[i].width))
          columnCount++;
      }

      // Fill the line of the histogram
      for (; columnCount > 0; columnCount--)
        histoImg.at<uchar>(histoImg.rows -  columnCount, col) = 255;
    }

    HistogramVertical histogram(histoImg, Mat::ones(histoImg.size(), CV_8U));

    // Go through each row in the histoImg and score it.  Try to find the single line that gives me the most right-sized character regions (based on avgCharWidth)

    int bestRowIndex = 0;
    float bestRowScore = 0;
    vector<Rect> bestBoxes;

    for (int row = 0; row < histogram.histoImg.rows; row++)
    {
      vector<Rect> validBoxes;
      
      vector<Rect> allBoxes = convert1DHitsToRect(histogram.get1DHits(row), top, bottom);

      if (this->config->debugCharSegmenter)
        cout << "All Boxes size " << allBoxes.size() << endl;


      float rowScore = 0;

      for (unsigned int boxidx = 0; boxidx < allBoxes.size(); boxidx++)
      {
        int w = allBoxes[boxidx].width;
        if (w >= config->segmentationMinBoxWidthPx && w <= MAX_SEGMENT_WIDTH)
        {
          float widthDiffPixels = abs(w - avgCharWidth);
          float widthDiffPercent = widthDiffPixels / avgCharWidth;
          rowScore += 10 * (1 - widthDiffPercent);

          if (widthDiffPercent < 0.25)	// Bonus points when it's close to the average character width
            rowScore += 8;

          validBoxes.push_back(allBoxes[boxidx]);
        }
        else if (w > avgCharWidth * 2  && w <= MAX_SEGMENT_WIDTH * 2 )
        {
		if (this->config->debugCharSegmenter)
			cout << "SEGMENTATION not valid char box [" << boxidx << "] by width, check 2*width: w=" << w << ", segmentationMinBoxWidthPx=" << config->segmentationMinBoxWidthPx
						<< ", MAX_SEGMENT_WIDTH=" << MAX_SEGMENT_WIDTH << endl;
          // Try to split up doubles into two good char regions, check for a break between 40% and 60%
          int leftEdge = allBoxes[boxidx].x + (int) (((float) allBoxes[boxidx].width) * 0.4f);
          int rightEdge = allBoxes[boxidx].x + (int) (((float) allBoxes[boxidx].width) * 0.6f);

          int minX = histogram.getLocalMinimum(leftEdge, rightEdge);
          int maxXChar1 = histogram.getLocalMaximum(allBoxes[boxidx].x, minX);
          int maxXChar2 = histogram.getLocalMaximum(minX, allBoxes[boxidx].x + allBoxes[boxidx].width);
          int minHeight = histogram.getHeightAt(minX);

          int maxHeightChar1 = histogram.getHeightAt(maxXChar1);
          int maxHeightChar2 = histogram.getHeightAt(maxXChar2);

          if (  minHeight < (0.25 * ((float) maxHeightChar1)))
          {
            // Add a box for Char1
            Point botRight = Point(minX - 1, allBoxes[boxidx].y + allBoxes[boxidx].height);
            validBoxes.push_back(Rect(allBoxes[boxidx].tl(), botRight) );
          }
          if (  minHeight < (0.25 * ((float) maxHeightChar2)))
          {
            // Add a box for Char2
            Point topLeft = Point(minX + 1, allBoxes[boxidx].y);
            validBoxes.push_back(Rect(topLeft, allBoxes[boxidx].br()) );
          }
        } else if (this->config->debugCharSegmenter)
			cout << "SEGMENTATION not valid char box [" << boxidx << "] by width and 2*width: w=" << w << ", segmentationMinBoxWidthPx=" << config->segmentationMinBoxWidthPx
						<< ", MAX_SEGMENT_WIDTH=" << MAX_SEGMENT_WIDTH << endl;
      }

      if (rowScore > bestRowScore)
      {
        bestRowScore = rowScore;
        bestRowIndex = row;
        bestBoxes = validBoxes;
      }
    }

    if (this->config->debugCharSegmenter)
    {
      cvtColor(histoImg, histoImg, CV_GRAY2BGR);
      line(histoImg, Point(0, histoImg.rows - 1 - bestRowIndex), Point(histoImg.cols, histoImg.rows - 1 - bestRowIndex), Scalar(0, 255, 0));

      Mat imgBestBoxes(img.size(), img.type());
      img.copyTo(imgBestBoxes);
      cvtColor(imgBestBoxes, imgBestBoxes, CV_GRAY2BGR);
      for (unsigned int i = 0; i < bestBoxes.size(); i++)
        rectangle(imgBestBoxes, bestBoxes[i], Scalar(0, 255, 0));

      this->imgDbgGeneral.push_back(addLabel(histoImg, "All Histograms"));
      this->imgDbgGeneral.push_back(addLabel(imgBestBoxes, "Best Boxes"));

      displayImage(config, "Seg Best Boxes", imgBestBoxes);
    }

    return bestBoxes;
  }


  void CharacterSegmenter::removeSmallContours(vector<Mat> thresholds, float avgCharHeight,  TextLine textLine)
  {
    //const float MIN_CHAR_AREA = 0.02 * avgCharWidth * avgCharHeight;	// To clear out the tiny specks
    const float MIN_CONTOUR_HEIGHT = config->segmentationMinSpeckleHeightPercent * avgCharHeight;

    Mat textLineMask = Mat::zeros(thresholds[0].size(), CV_8U);
    fillConvexPoly(textLineMask, textLine.linePolygon.data(), textLine.linePolygon.size(), Scalar(255,255,255));

    for (unsigned int i = 0; i < thresholds.size(); i++)
    {
      vector<vector<Point> > contours;
      vector<Vec4i> hierarchy;
      Mat thresholdsCopy = Mat::zeros(thresholds[i].size(), thresholds[i].type());

      thresholds[i].copyTo(thresholdsCopy, textLineMask);
      findContours(thresholdsCopy, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

      for (unsigned int c = 0; c < contours.size(); c++)
      {
        if (contours[c].size() == 0)
          continue;

        Rect mr = boundingRect(contours[c]);
        if (mr.height < MIN_CONTOUR_HEIGHT)
        {
          // Erase it
          drawContours(thresholds[i], contours, c, Scalar(0, 0, 0), -1);
          continue;
        }
      }
    }
  }


  void CharacterSegmenter::removeSmallContours2(float avgCharHeight,  float avgCharWidth, TextLine textLine)
      {

//        const float MIN_CONTOUR_HEIGHT = config->segmentationMinSpeckleHeightPercent * avgCharHeight;
//        const float MIN_CONTOUR_WIDTH = 0.2 * avgCharWidth;
//
//        const float MIN_CHAR_AREA = 0.06 * avgCharWidth * avgCharHeight;	// To clear out the tiny specks
//
//        const float MAX_CONTOUR_WIDTH = 3 * avgCharWidth;
//
//        const float MIN_PLATE_HEIGHT = avgCharHeight;
//        const float MIN_PLATE_WIDTH = 9 * 1.2 * avgCharWidth;



        const float MIN_CHAR_AREA = 0.05 * avgCharWidth * avgCharHeight;	// To clear out the tiny specks
        const float MIN_CONTOUR_HEIGHT = config->segmentationMinSpeckleHeightPercent * avgCharHeight;
        const float MIN_CONTOUR_WIDTH = 0.1 * avgCharWidth;
        const float MAX_CONTOUR_WIDTH = 3 * avgCharWidth;//todo sol
        const float MAX_CONTOUR_HEIGHT = 1.2 * avgCharHeight;//todo sol

        const float MIN_PLATE_HEIGHT = 1.1 * avgCharHeight;
        const float MIN_PLATE_WIDTH = 8 * 1.15 * avgCharWidth;

        //orig - removes everything outside the polygon
    //    Mat textLineMask = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8U);
    //    fillConvexPoly(textLineMask, textLine.linePolygon.data(), textLine.linePolygon.size(), Scalar(255,255,255));

        int biggestContourIdx = -1;
        float biggestContourWidth = 0;
        vector<vector<Point> > plateContAll;

        if (this->config->debugCharSegmenter) {
    		cout << "CHARACTERSEGMENTER: pipeline_data->regionOfInterest.x=" << pipeline_data->regionOfInterest.x << ", pipeline_data->regionOfInterest.y=" << pipeline_data->regionOfInterest.y << endl;
    		cout << "CHARACTERSEGMENTER: pipeline_data->regionOfInterest.width=" << pipeline_data->regionOfInterest.width << ", pipeline_data->regionOfInterest.height=" << pipeline_data->regionOfInterest.height << endl;
    		cout << "CHARACTERSEGMENTER: pipeline_data->plate_corners: ";
    		for (unsigned int c = 0; c < pipeline_data->plate_corners.size(); c++)
    			cout << "[" << pipeline_data->plate_corners[c].x << ", " << pipeline_data->plate_corners[c].y << "]  ";
    		cout << endl;
    		cout << "CHARACTERSEGMENTER: pipeline_data->plate_corners: width=" << (pipeline_data->plate_corners[1].x - pipeline_data->plate_corners[0].x);
    		cout << ", height=" << (pipeline_data->plate_corners[3].y - pipeline_data->plate_corners[0].y) << endl;
    	}

        Mat resultMask;// = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8UC1);

        /*if (pipeline_data->plate_corners[1].x - pipeline_data->plate_corners[0].x < pipeline_data->config->templateWidthPx / 2) {
      	  if (this->config->debugCharSegmenter)
      	   		cout << "CHARACTERSEGMENTER: perform morphologyEx" << endl;
    		//for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
    		//{
    			//todo test1
    			Mat element = getStructuringElement( 1,
    								Size( 3, 3 ),
    								Point( 1, 1 ) );
    				  //dilate(pipeline_data->thresholds[i], tempImg, element);
    				  //erode(pipeline_data->thresholds[i], pipeline_data->thresholds[i], element);
    				  morphologyEx(pipeline_data->thresholds[0], pipeline_data->thresholds[0], MORPH_OPEN, element);
    				  //drawAndWait(&tempImg);

    		//}
        }*/


        /*Mat element1 = getStructuringElement( 1,
								Size( 2, 2 ),
								Point( 1, 1 ) );
				  //dilate(pipeline_data->thresholds[i], tempImg, element);
				  erode(pipeline_data->thresholds[0], pipeline_data->thresholds[0], element1);
				  //morphologyEx(pipeline_data->thresholds[0], pipeline_data->thresholds[0], MORPH_OPEN, element);
				  //drawAndWait(&tempImg);
	    vector<vector<Point> > erContours;
		vector<Vec4i> erHierarchy;

		Mat erThreshold(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
		pipeline_data->thresholds[0].copyTo(erThreshold);
		findContours(erThreshold, erContours, erHierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

		for (unsigned int c = 0; c < erContours.size(); c++)
		  {
			  if (erContours[c].size() == 0)
				continue;

			  Rect mr = boundingRect(erContours[c]);
			  float ctArea= contourArea(erContours[c]);

			  if (mr.height < avgCharHeight*0.05 && mr.width < avgCharWidth*0.05)
			  {
				// Erase it
				drawContours(pipeline_data->thresholds[0], erContours, c, Scalar(0, 0, 0), -1);
			  }
		  }

		dilate(pipeline_data->thresholds[0], pipeline_data->thresholds[0], element1);

		if (this->config->debugCharSegmenter) {
			displayImage(config, "Remove small contours: erode/remove/dilate", pipeline_data->thresholds[0]);
		}*/

        //Mat dbgImage(pipeline_data->thresholds[0].size(), CV_8U);
          /*Mat hFilterImg = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8U);
        	Mat elementH = getStructuringElement(MORPH_RECT,
        					Size((int)(avgCharWidth*3), 1),
        					Point(-1, -1) );
        	morphologyEx(pipeline_data->thresholds[0], hFilterImg, MORPH_OPEN, elementH);

        	Mat vFilterImg = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8U);
        	int height = std::min((int)(avgCharHeight*1.3), (int)pipeline_data->thresholds[0].rows);
        	Mat elementV = getStructuringElement(MORPH_RECT,
        					Size(1, height),
        					Point(-1, -1) );
        	morphologyEx(pipeline_data->thresholds[0], vFilterImg, MORPH_OPEN, elementV);

        	Mat hvFilterImg = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8U);
        	bitwise_or(hFilterImg, vFilterImg, hvFilterImg);
        	bitwise_not(hvFilterImg, hvFilterImg);


            if (this->config->debugCharSegmenter)
        	  {
            	vector<Mat> filterList;
        		filterList.push_back(hFilterImg);
        		filterList.push_back(vFilterImg);
        		filterList.push_back(hvFilterImg);

        		Mat dbgImg = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8U);
        		bitwise_and(pipeline_data->thresholds[0], hvFilterImg, dbgImg);

        		filterList.push_back(dbgImg);
        		displayImage(config, "Remove small contours: HV-filters", drawImageDashboard(filterList, pipeline_data->thresholds[0].type(), 4));
        	  }
        	  */


          Mat hvFilterImg = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8U);
          bitwise_not(hvFilterImg, hvFilterImg);

          vector<vector<Point> > contours;
          vector<Vec4i> hierarchy;

          //orig
    //      Mat thresholdsCopy = Mat::zeros(pipeline_data->thresholds[i].size(), pipeline_data->thresholds[i].type());
    //      pipeline_data->thresholds[i].copyTo(thresholdsCopy, textLineMask);
    //      findContours(thresholdsCopy, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

          //sol
          //first version
          Mat tempThreshold(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
          pipeline_data->thresholds[0].copyTo(tempThreshold);
          findContours(tempThreshold, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

          //Mat mask = Mat::zeros(pipeline_data->thresholds[i].size(), CV_8UC1);
    	  //////////////////////fillPoly(mask, plateContAll, cv::Scalar(255,255,255));
    	  //fillConvexPoly(mask, textLine.linePolygon.data(), textLine.linePolygon.size(), Scalar(255,255,255));
    	  //bitwise_and(pipeline_data->thresholds[i], mask, pipeline_data->thresholds[i]);
          //findContours(pipeline_data->thresholds[i], contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);
          vector<vector<Point> > contours_poly;
          vector<int> wideContoursIdx;
          for (unsigned int c = 0; c < contours.size(); c++)
          {
    		  if (contours[c].size() == 0)
    			continue;

    		  Rect mr = boundingRect(contours[c]);
    		  float ctArea= contourArea(contours[c]);

    		  if (mr.height < MIN_CONTOUR_HEIGHT || mr.width < MIN_CONTOUR_WIDTH || ctArea < MIN_CHAR_AREA)
    		  {
    			// Erase it
    			drawContours(pipeline_data->thresholds[0], contours, c, Scalar(0, 0, 0), -1, 8, hierarchy, 1);

    			if (this->config->debugCharSegmenter) {
    			  cout << "CHARACTERSEGMENTER remove(1) threshold 0 ";
    			  if (mr.height < MIN_CONTOUR_HEIGHT) {
    				  cout << "SHORT ";
    			  }
    			  if (mr.width < MIN_CONTOUR_WIDTH) {
    				  cout << "TIGHT ";
    			  }
    			  if (ctArea < MIN_CHAR_AREA) {
    				  cout << "SMALL ";
    			  }
    				cout << "contour[" << c << "] height=" << mr.height << " (min=" << MIN_CONTOUR_HEIGHT << "), width=" << mr.width
    			  << " (min=" << MIN_CONTOUR_WIDTH << "), ctArea=" << ctArea << " (min=" << MIN_CHAR_AREA << ")" << endl;
    			}
    		  } else if (mr.width > MIN_PLATE_WIDTH && mr.height > MIN_PLATE_HEIGHT) {
            	  if (contourArea(contours[c],true) > 0) {// > 0 inner
            		  vector<Point> approx;
            		  double perimeter = arcLength(contours[c], true);
            		  Mat img = Mat(contours[c]);
            		  approxPolyDP(img, approx, perimeter * 0.05, true);
            		  if (approx.size() >= 4) {
            			  if (!isContourConvex(approx)) {
            				  /*vector<Point> hull;
            				  convexHull(approx, hull, CV_CLOCKWISE, true);
            				  contours_poly.push_back(hull);*/
            				  convexHull(approx, approx, CV_CLOCKWISE, true);
            			  }
            			  /*
            			  check the case:
            			     +++++++++++++++++++++++
            			    +                  +
            			   +               +
            			   +           +
            			   +++++++++
            			   */
            			  Mat tmpImg = Mat::zeros(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
            			  RotatedRect rect  = minAreaRect(approx);//not contour[c]
						  Point2f pointsFloat[4];
						  rect.points(pointsFloat);
						  vector<Point> rectPoint;
						  for (int h = 0; h < 4; h++) {
							  rectPoint.push_back(Point(floor(pointsFloat[h].x), floor(pointsFloat[h].y)));
						  }

						  //find "diff" areas
						  fillConvexPoly(tmpImg, rectPoint, Scalar(255, 255, 255));
						  fillConvexPoly(tmpImg, approx, Scalar(0, 0, 0));

						  Mat tempFindImg(pipeline_data->thresholds[0].size(), CV_8U);
						  tmpImg.copyTo(tempFindImg);

						  vector<vector<Point> > tmpContours;
						  vector<Vec4i> tmpHierarchy;
						  findContours(tempFindImg, tmpContours, tmpHierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

						  float rectArea= contourArea(rectPoint);

						  if (this->config->debugCharSegmenter) {
								Mat debugImg = Mat::zeros(tmpImg.size(), tmpImg.type());
								tmpImg.copyTo(debugImg);
								cvtColor(debugImg, debugImg, CV_GRAY2BGR);
								//fillPoly(tmpImg, contours_poly, Scalar(150, 100, 50));
								for (unsigned int z = 0; z < tmpContours.size(); z++) {
									drawContours(debugImg, tmpContours, z, Scalar(0, 0, 255), 2);
								}
								displayImage(config, "Remove small contours: approx&rect", debugImg);
								cout << "CHARACTERSEGMENTER approx=" << approx << endl;
							}

						  //vector<vector<Point>> bigContours;
						  bool performConvexHull = false;
						  for (int h = 0; h < tmpContours.size(); h++)
							{
							  if (tmpContours[h].size() == 0)
								continue;
							  float tmpArea= contourArea(tmpContours[h]);
							  if (this->config->debugCharSegmenter) {
								  cout << "CHARACTERSEGMENTER rectArea=" << rectArea << ", rectArea*0.125=" << (rectArea*0.125) << ", tmpArea=" << tmpArea << endl;
							  }

							  if (tmpArea > rectArea*0.125) {
								  //bigContours.push_back(tmpContours[h]);
								  approx.insert(approx.end(), tmpContours[h].begin(), tmpContours[h].end());
								  performConvexHull = true;
							  }

							}
						  if (performConvexHull) {
							  convexHull(approx, approx, CV_CLOCKWISE, true);
							  if (this->config->debugCharSegmenter) {
									Mat debugImg = Mat::zeros(tmpImg.size(), tmpImg.type());
									tmpImg.copyTo(debugImg);
									cvtColor(debugImg, debugImg, CV_GRAY2BGR);
									for (int t = 0; t < approx.size() - 1; t++) {
										line(debugImg, approx[t], approx[t+1], Scalar(255, 0, 0), 3, CV_AA);
									}
									line(debugImg, approx[0], approx[approx.size() - 1], Scalar(255, 0, 0), 3, CV_AA);
									displayImage(config, "Remove small contours: approx", debugImg);

								}
						  }

            			  contours_poly.push_back(approx);

						  if (this->config->debugCharSegmenter)
								cout << "CHARACTERSEGMENTER collect(1) threshold 0 WIDE contour[" << c << "] height=" << mr.height << " (MIN_PLATE_HEIGHT=" << MIN_PLATE_HEIGHT << "), width=" << mr.width
									 << " (MIN_PLATE_WIDTH=" << MIN_PLATE_WIDTH << ")" << endl;
            		  } else {
            			  RotatedRect rect  = minAreaRect(contours[c]);
            			  Point2f pointsFloat[4];
            			  rect.points(pointsFloat);
            			  vector<Point> rectPoint;
            			  for (int h = 0; h < 4; h++) {
            				  rectPoint.push_back(Point(floor(pointsFloat[h].x), floor(pointsFloat[h].y)));
            			  }
            			  contours_poly.push_back(rectPoint);
            			  if (this->config->debugCharSegmenter)
            				  cout << "CHARACTERSEGMENTER collect_minAreaRect(1) threshold 0 WIDE contour[" << c << "] height=" << mr.height << " (MIN_PLATE_HEIGHT=" << MIN_PLATE_HEIGHT << "), width=" << mr.width
            				  								 << " (MIN_PLATE_WIDTH=" << MIN_PLATE_WIDTH << "), approxPolyDP.size()=" << approx.size() << endl;

            		  }
            		  wideContoursIdx.push_back(c);
            	  }
              }

    		  //calculate for first threshold only
    		  if(mr.width > biggestContourWidth && mr.width > MIN_PLATE_WIDTH && mr.height > MIN_PLATE_HEIGHT)
    		  {
    			  //biggestContourWidth = mr.width;
    			  biggestContourIdx = c;
    		  }
          }
          if (contours_poly.size() > 0) {
			Mat tmpImg = Mat::zeros(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
			//fillPoly(tmpImg, contours_poly, Scalar(255, 255, 255));
			for (int h = 0; h < contours_poly.size(); h++) {
				fillConvexPoly(tmpImg, contours_poly[h], Scalar(255, 255, 255));
			}
			bitwise_and(hvFilterImg, tmpImg, hvFilterImg);

			if (this->config->debugCharSegmenter) {
				Mat debugImg = Mat::zeros(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
				pipeline_data->thresholds[0].copyTo(debugImg);
				cvtColor(debugImg, debugImg, CV_GRAY2BGR);
				//fillPoly(tmpImg, contours_poly, Scalar(150, 100, 50));
				drawContours(debugImg, contours_poly, 0, Scalar(0, 0, 255), 2);

				for (unsigned int j = 0; j < wideContoursIdx.size(); j++)
				  {
					  drawContours(debugImg, contours, wideContoursIdx[j], cv::Scalar(0,255,0), 1);
				  }

				displayImage(config, "Remove small contours: fillPoly", debugImg);

			}
		  }

          if (this->config->debugCharSegmenter)
          	displayImage(config, "Remove small contours: hvFilterImg(FINAL)", hvFilterImg);
          //test3
    	  Mat blackFilterImg = Mat::zeros(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
    	  Mat blackArea = Mat::zeros(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
    	  Mat elementBA = getStructuringElement(MORPH_RECT,
    						Size((int)avgCharWidth, (int)avgCharHeight),
    						Point(-1, -1) );
    	  bitwise_not(pipeline_data->thresholds[0], blackArea);
    	  morphologyEx(blackArea, blackArea, MORPH_OPEN, elementBA);
    	  bitwise_not(blackArea, blackFilterImg);

    	  vector< vector <Point> > contoursBA; // Vector for storing contour
    	  vector< Vec4i > hierarchyBA;
    //	  int largestContourIdx = 0;
    //	  int largestArea=0;


    	  //Mat debugImg(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());
    	  Mat tmpImg = Mat::zeros(pipeline_data->thresholds[0].size(), pipeline_data->thresholds[0].type());

    	  blackArea.copyTo(tmpImg);
    	  findContours(tmpImg, contoursBA, hierarchyBA, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

    	  for (unsigned int j = 0; j < contoursBA.size(); j++)
    	  {
    		  if (contoursBA[j].size() == 0)
    			continue;
    		  Rect mr = boundingRect(contoursBA[j]);
    		  if (mr.x < 0.5*MIN_PLATE_WIDTH) {
    			  rectangle(blackFilterImg, Rect(0, 0, mr.x + mr.width, blackFilterImg.rows), Scalar(0, 0, 0), -1);
    		  } else if (blackFilterImg.cols - (mr.x + mr.width) < 0.5*MIN_PLATE_WIDTH) {
    			  rectangle(blackFilterImg, Rect(mr.x, 0, blackFilterImg.cols - mr.x, blackFilterImg.rows), Scalar(0, 0, 0), -1);
    		  } else {
    			  rectangle(blackFilterImg, Rect(mr.x, 0, mr.width, blackFilterImg.rows), Scalar(0, 0, 0), -1);
    		  }
    	  }

    	bitwise_and(hvFilterImg, blackFilterImg, resultMask);

    	  if (this->config->debugCharSegmenter) {
    		Mat debugImg = Mat::zeros(blackArea.size(), blackArea.type());
    		blackArea.copyTo(debugImg);
    		cvtColor(debugImg, debugImg, CV_GRAY2BGR);

    		for (unsigned int j = 0; j < contoursBA.size(); j++)
    		  {
    			  if (contoursBA[j].size() == 0)
    				continue;
    			  drawContours(debugImg, contoursBA, j, cv::Scalar(255,0,0), 2);
    		  }

    		displayImage(config, "Remove small contours: blackArea", debugImg);
    		displayImage(config, "Remove small contours: blackFilterImg", blackFilterImg);
    		displayImage(config, "Remove small contours: thresholds[0]", pipeline_data->thresholds[0]);
    	  }



          Point leftTop = Point(textLine.topLine.p1.x, textLine.topLine.p1.y);
          Point rightTop = Point(textLine.topLine.p2.x, textLine.topLine.p2.y);
          Point leftBottom = Point(textLine.bottomLine.p1.x, textLine.bottomLine.p1.y);
          Point rightBottom = Point(textLine.bottomLine.p2.x, textLine.bottomLine.p2.y);

          if (biggestContourIdx > -1) {



            RotatedRect plateBoundingBox = minAreaRect(contours[biggestContourIdx]);

  		  Point2f corners[4];
  		  plateBoundingBox.points(corners);

  		  vector<Point> fillContSingle;
  		  fillContSingle.push_back(corners[0]);
  		  fillContSingle.push_back(corners[1]);
  		  fillContSingle.push_back(corners[2]);
  		  fillContSingle.push_back(corners[3]);

  		  vector<vector<Point> > plateContAll;
  		  plateContAll.push_back(fillContSingle);

  		  Mat mask = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8UC1);
  		  fillConvexPoly(mask, fillContSingle, cv::Scalar(255,255,255));
  		  //fillPoly(mask, plateContAll, cv::Scalar(255,255,255));
  		  bitwise_and(resultMask, mask, resultMask);




        	  Mat maskTextLine = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8UC1);
        	  //fillConvexPoly(maskTextLine, textLine.linePolygon.data(), textLine.linePolygon.size(), Scalar(255,255,255));
        	  line(maskTextLine, textLine.topLine.p1, textLine.topLine.p2, cv::Scalar(255,255,255), 2);

        	  Mat maskContour = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8UC1);
        	  drawContours(maskContour, contours,biggestContourIdx, cv::Scalar(255,255,255), 1); // with a thickness of 1
        	  //drawContours(maskContour, contours, biggestContourIdx, Scalar(255, 255, 255), -1, 8, hierarchy, 1);
        	  Mat lineMask;
        	  bitwise_and(maskTextLine, maskContour, lineMask);
        	  vector<Point> locations;   // output, locations of non-zero pixels

        	  unsigned count = cv::countNonZero(lineMask);
        	  if (count > 0) {
    			  cv::findNonZero(lineMask, locations);

    			  Point min = Point(0, 0);
    			  Point max = Point(0, 0);
    			  //int topSegmentLength = textLine.topLine.p2.x -  textLine.topLine.p1.x;
    			  for (unsigned int h = 0; h < locations.size(); h++) {
    				  Point pnt = locations[h];
    				  if (h == 0) {
    					  min = Point(pnt.x, pnt.y);
    					  max = Point(pnt.x, pnt.y);
    				  } else {
    					  if (pnt.x < min.x) {
    						  min = Point(pnt.x, pnt.y);
    					  }
    					  if (pnt.x > max.x) {
    						  max = Point(pnt.x, pnt.y);
    					  }
    				  }

    			  }

    			  if (min.x + 5 != 0 && min.x < pipeline_data->thresholds[0].cols / 2) {
    				  leftTop = Point(min.x + 5, textLine.topLine.getPointAt(min.x + 5));
    			  }
    			  if (max.x - 5 != 0 && max.x > pipeline_data->thresholds[0].cols / 2) {
    				  rightTop = Point(max.x - 5, textLine.topLine.getPointAt(max.x - 5));
    			  }

    			  //cv::Scalar intersectionArea = cv::sum(intersectionMat);
        	  }


           }

          if (biggestContourIdx > -1) {
    		  Mat maskTextLine = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8UC1);
    		  line(maskTextLine, textLine.bottomLine.p1, textLine.bottomLine.p2, cv::Scalar(255,255,255), 2);

    		  Mat maskContour = Mat::zeros(pipeline_data->thresholds[0].size(), CV_8UC1);
    		  drawContours(maskContour, contours,biggestContourIdx, cv::Scalar(255,255,255),1); // with a thickness of 1
    		  //drawContours(maskContour, contours, biggestContourIdx, Scalar(255, 255, 255), -1, 8, hierarchy, 1);
    		  Mat lineMask;
    		  bitwise_and(maskTextLine, maskContour, lineMask);
    		  vector<Point> locations;   // output, locations of non-zero pixels
    		  unsigned count = cv::countNonZero(lineMask);
    		  if (count > 0) {
    			  cv::findNonZero(lineMask, locations);

    			  Point min = Point(0, 0);
    			  Point max = Point(0, 0);
    			  //int topSegmentLength = textLine.topLine.p2.x -  textLine.topLine.p1.x;
    			  for (unsigned int h = 0; h < locations.size(); h++) {
    				  Point pnt = locations[h];
    				  if (h == 0) {
    					  min = Point(pnt.x, pnt.y);
    					  max = Point(pnt.x, pnt.y);
    				  } else {
    					  if (pnt.x < min.x) {
    						  min = Point(pnt.x, pnt.y);
    					  }
    					  if (pnt.x > max.x) {
    						  max = Point(pnt.x, pnt.y);
    					  }
    				  }
    				  if (this->config->debugCharSegmenter)
    					  cout << "locations[" << h << "]=" << pnt << endl;

    			  }

    			  if (min.x + 5 != 0 && min.x < pipeline_data->thresholds[0].cols / 2) {
    				  leftBottom = Point(min.x + 5, textLine.bottomLine.getPointAt(min.x + 5));
    			  }
    			  if (max.x - 5 != 0 && max.x > pipeline_data->thresholds[0].cols / 2) {
    				  rightBottom = Point(max.x - 5, textLine.bottomLine.getPointAt(max.x - 5));
    			  }

    			  //cv::Scalar intersectionArea = cv::sum(intersectionMat);
    			  }


    		 }

          line(resultMask, leftTop, rightTop, cv::Scalar(0,0,0));
          line(resultMask, leftBottom, rightBottom, cv::Scalar(0,0,0));


          if (this->config->debugCharSegmenter) {
          	cout << "leftTop=" << leftTop << ", rightTop=" << rightTop << ", leftBottom=" << leftBottom << ", rightBottom=" << rightBottom << endl;
          }

                //if (biggestContourIdx > -1) {

                	if (this->config->debugCharSegmenter)
    				  {
                		Mat dbgImage(pipeline_data->thresholds[0].size(), CV_8U);
                		pipeline_data->thresholds[0].copyTo(dbgImage);
    				    cvtColor(dbgImage, dbgImage, CV_GRAY2RGB);
    				    //line(dbgImage, left, right, cv::Scalar(0,255,0));
    				    //drawContours(dbgImage, contours, biggestContourIdx, Scalar(255,105,180), 3);

    				    line(dbgImage, leftTop, rightTop, cv::Scalar(0,255,0));
    				    line(dbgImage, leftBottom, rightBottom, cv::Scalar(0,255,0));

    				    //line(dbgImage, textLine.bottomLine.p1, textLine.bottomLine.p2, cv::Scalar(0,255,0), 2);
    				    //line(dbgImage, textLine.topLine.p1, textLine.topLine.p2, cv::Scalar(0,255,0), 2);

    				    //drawContours(dbgImage, contours, biggestContourIdx, Scalar(255,105,180), -1, 8, hierarchy, 1);
    				    if (biggestContourIdx > -1) {
    				    	drawContours(dbgImage, contours, biggestContourIdx, Scalar(255,0,0), 1);
    				    }
    					displayImage(config, "Remove small contours: with top/bottom lines", dbgImage);
    					displayImage(config, "Remove small contours (RESULT filter): with top/bottom lines", resultMask);
    				  }
                //}


      for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
          {

        	vector<vector<Point> > contours;
    		vector<Vec4i> hierarchy;

    		//line(pipeline_data->thresholds[i], leftTop, rightTop, cv::Scalar(0,0,0));
    		//line(pipeline_data->thresholds[i], leftBottom, rightBottom, cv::Scalar(0,0,0));

    		bitwise_and(pipeline_data->thresholds[i], resultMask, pipeline_data->thresholds[i]);

    		Mat tempThreshold(pipeline_data->thresholds[i].size(), CV_8U);
    		pipeline_data->thresholds[i].copyTo(tempThreshold);
    		findContours(tempThreshold, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);//todo test CV_CHAIN_APPROX_NONE


    		bool first = true;
          for (unsigned int c = 0; c < contours.size(); c++)
          {
            if (contours[c].size() == 0)
              continue;

            Rect mr = boundingRect(contours[c]);
            float ctArea= contourArea(contours[c]);

            if (mr.width > MAX_CONTOUR_WIDTH /*|| mr.height > MAX_CONTOUR_HEIGHT*/ //todo sol
            		|| mr.height < MIN_CONTOUR_HEIGHT || mr.width < MIN_CONTOUR_WIDTH || ctArea < MIN_CHAR_AREA)
            {
              // Erase it
              if (mr.width > MAX_CONTOUR_WIDTH && contourArea(contours[c], true) < 0) { // < 0 - outer
    //        	  int childIndex = hierarchy[c][2];
    //        	  if (childIndex != -1) {
    //        		  drawContours(pipeline_data->thresholds[i], contours, childIndex, Scalar(255, 255, 255));
              	drawContours(pipeline_data->thresholds[i], contours, c, Scalar(0, 0, 0), -1, 8, hierarchy, 1);

              } else {
            	  drawContours(pipeline_data->thresholds[i], contours, c, Scalar(0, 0, 0), -1, 8, hierarchy, 1);
              }
              if (this->config->debugCharSegmenter) {
            	  cout << "CHARACTERSEGMENTER remove(2) ";
            	  if (mr.width > MAX_CONTOUR_WIDTH) {
            		  cout << "WIDE ";
            	  }
            	  if (mr.height < MIN_CONTOUR_HEIGHT) {
    				  cout << "SHORT ";
    			  }
            	  if (mr.width < MIN_CONTOUR_WIDTH) {
    				  cout << "TIGHT ";
    			  }
            	  if (ctArea < MIN_CHAR_AREA) {
    				  cout << "SMALL ";
    			  }
                  cout << " contours[" << c << "] in threshold " << i << " -- height=" << mr.height << " (min=" << MIN_CONTOUR_HEIGHT << "), width=" << mr.width
    			  << " (min=" << MIN_CONTOUR_WIDTH << ", max=" << MAX_CONTOUR_WIDTH << "), ctArea=" << ctArea << " (min=" << MIN_CHAR_AREA << ")" << endl;
              }
            }

          }


    //      for (unsigned int i = 0; i < contours.size(); i++)
    //	  {
    //		if (bestContours.goodIndices[i] == false)
    //		  continue;
    //
    //		for (unsigned int z = 0; z < bestContours.contours[i].size(); z++)
    //		{
    //		  if (bestContours.contours[i][z].x < leftX)
    //			leftX = bestContours.contours[i][z].x;
    //		  if (bestContours.contours[i][z].x > rightX)
    //			rightX = bestContours.contours[i][z].x;
    //		}
    //	  }
        }
          displayImage(config, "Remove small contours(FINAL): thresholds[0]", pipeline_data->thresholds[0]);

      }














  int CharacterSegmenter::getCharGap(cv::Rect leftBox, cv::Rect rightBox) {
      int right_midpoint = (rightBox.x + (rightBox.width / 2));
      int left_midpoint = (leftBox.x + (leftBox.width / 2));
      return  right_midpoint - left_midpoint;
  }

  vector<Rect> CharacterSegmenter::combineCloseBoxes( vector<Rect> charBoxes)
  {
    // Don't bother combining if there are fewer than the min number of characters
    if (charBoxes.size() < config->postProcessMinCharacters)
      return charBoxes;
    
    // First find the median char gap (the space from midpoint to midpoint of chars)
    
    vector<int> char_gaps;
    for (unsigned int i = 0; i < charBoxes.size(); i++)
    {
      if (i == charBoxes.size() - 1)
        break;
      
      // the space between charbox i and i+1
      char_gaps.push_back(getCharGap(charBoxes[i], charBoxes[i+1]));
    }
        
    int median_char_gap = median(char_gaps.data(), char_gaps.size());
    
    
    // Find the second largest char box.  Should help ignore a big outlier
    vector<int> char_sizes;
    float biggestCharWidth;
    for (unsigned int i = 0; i < charBoxes.size(); i++)
    {
      char_sizes.push_back(charBoxes[i].width);
    }

    std::sort(char_sizes.begin(), char_sizes.end());
    biggestCharWidth = char_sizes[char_sizes.size() - 2];
    
    
    vector<Rect> newCharBoxes;

    for (unsigned int i = 0; i < charBoxes.size(); i++)
    {
      if (i == charBoxes.size() - 1)
      {
        newCharBoxes.push_back(charBoxes[i]);
        break;
      }
      float bigWidth = (charBoxes[i + 1].x + charBoxes[i + 1].width - charBoxes[i].x);

      float left_gap;
      float right_gap;
      
      if (i == 0)
        left_gap = 999999999999;
      else
        left_gap = getCharGap(charBoxes[i-1], charBoxes[i]);
      
      right_gap = getCharGap(charBoxes[i], charBoxes[i+1]);
      
      int min_gap = (int) ((float)median_char_gap) * 0.75;
      int max_gap = (int) ((float)median_char_gap) * 1.25;
      
      int max_width = (int) ((float)biggestCharWidth) * 1.2;
      
      bool has_good_gap = (left_gap >= min_gap && left_gap <= max_gap) || (right_gap >= min_gap && right_gap <= max_gap);
      

      
      if (has_good_gap && bigWidth <= max_width)
      {
        Rect bigRect(charBoxes[i].x, charBoxes[i].y, bigWidth, charBoxes[i].height);
        newCharBoxes.push_back(bigRect);
        if (this->config->debugCharSegmenter)
        {
          for (unsigned int z = 0; z < pipeline_data->thresholds.size(); z++)
          {
            Point center(bigRect.x + bigRect.width / 2, bigRect.y + bigRect.height / 2);
            RotatedRect rrect(center, Size2f(bigRect.width, bigRect.height + (bigRect.height / 2)), 0);
            ellipse(imgDbgCleanStages[z], rrect, Scalar(0,255,0), 1);
          }
          cout << "Merging 2 boxes -- " << i << " and " << i + 1 << endl;
        }

        i++;
      }
      else
      {
        newCharBoxes.push_back(charBoxes[i]);
        if (this->config->debugCharSegmenter)
        {
          cout << "Not Merging 2 boxes -- " << i << " and " << i + 1 << " -- has_good_gap: " << has_good_gap <<
                  " bigWidth (" << bigWidth << ") > max_width ("  << max_width << ")" << endl;
        }
      }
    }

    return newCharBoxes;
  }

  void CharacterSegmenter::cleanCharRegions(vector<Mat> thresholds, vector<Rect> charRegions)
  {
    const float MIN_SPECKLE_HEIGHT_PERCENT = 0.13;
    const float MIN_SPECKLE_WIDTH_PX = 3;
    const float MIN_CONTOUR_AREA_PERCENT = 0.05;
    const float MIN_CONTOUR_HEIGHT_PERCENT = config->segmentationMinCharHeightPercent;

    //const float MIN_CHAR_AREA_PERCENT = 0.1;

    Mat mask = getCharBoxMask(thresholds[0], charRegions);

    for (unsigned int i = 0; i < thresholds.size(); i++)
    {
      bitwise_and(thresholds[i], mask, thresholds[i]);
      vector<vector<Point> > contours;

      Mat tempImg(thresholds[i].size(), thresholds[i].type());
      thresholds[i].copyTo(tempImg);

//      Mat element = getStructuringElement( 1,
//  				    Size( 2 + 1, 2+1 ),
//    		  //Size( 4, 4 ),
//  				    Point( 1, 1 ) );
//      //dilate(thresholds[i], tempImg, element);
//      morphologyEx(thresholds[i], tempImg, MORPH_OPEN, element);
//      drawAndWait(&tempImg);

      findContours(tempImg, contours, RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

      for (unsigned int j = 0; j < charRegions.size(); j++)
      {
        const float MIN_SPECKLE_HEIGHT = ((float)charRegions[j].height) * MIN_SPECKLE_HEIGHT_PERCENT;
        const float MIN_CONTOUR_AREA = ((float)charRegions[j].area()) * MIN_CONTOUR_AREA_PERCENT;

        int tallestContourHeight = 0;
        float totalArea = 0;
        for (unsigned int c = 0; c < contours.size(); c++)
        {
          if (contours[c].size() == 0)
            continue;
          if (charRegions[j].contains(contours[c][0]) == false)
            continue;

          Rect r = boundingRect(contours[c]);

          if (r.height <= MIN_SPECKLE_HEIGHT || r.width <= MIN_SPECKLE_WIDTH_PX)// || r.height*r.width < MIN_CONTOUR_AREA)
          {
            // Erase this speckle
            drawContours(thresholds[i], contours, c, Scalar(0,0,0), CV_FILLED);

            if (this->config->debugCharSegmenter)
            {
              drawContours(imgDbgCleanStages[i], contours, c, COLOR_DEBUG_SPECKLES, CV_FILLED);
            }
          }
          else
          {
            if (r.height > tallestContourHeight)
              tallestContourHeight = r.height;

            totalArea += contourArea(contours[c]);
          }
          //else if (r.height > tallestContourHeight)
          //{
          //  tallestContourIndex = c;
          //  tallestContourHeight = h;
          //}
        }

        if (totalArea < MIN_CONTOUR_AREA)
        {
          // Character is not voluminous enough.  Erase it.
          if (this->config->debugCharSegmenter)
          {
            cout << "Character CLEAN: (area) removing box " << j << " in threshold " << i << " -- Area " << totalArea << " < " << MIN_CONTOUR_AREA << endl;

            Rect boxTop(charRegions[j].x, charRegions[j].y - 10, charRegions[j].width, 10);
            rectangle(imgDbgCleanStages[i], boxTop, COLOR_DEBUG_MIN_AREA, -1);
          }

          rectangle(thresholds[i], charRegions[j], Scalar(0, 0, 0), -1);
        }
        else if (tallestContourHeight < ((float) charRegions[j].height * MIN_CONTOUR_HEIGHT_PERCENT))
        {
          // This character is too short.  Black the whole thing out
          if (this->config->debugCharSegmenter)
          {
            cout << "Character CLEAN: (height) removing box " << j << " in threshold " << i << " -- Height " << tallestContourHeight << " < " << ((float) charRegions[j].height * MIN_CONTOUR_HEIGHT_PERCENT) << ", charRegions[" << j << "].height=" << charRegions[j].height << endl;

            Rect boxBottom(charRegions[j].x, charRegions[j].y + charRegions[j].height, charRegions[j].width, 10);
            rectangle(imgDbgCleanStages[i], boxBottom, COLOR_DEBUG_MIN_HEIGHT, -1);
          }
          rectangle(thresholds[i], charRegions[j], Scalar(0, 0, 0), -1);
        }
      }

      //orig
      int morph_size = 1;
      Mat closureElement = getStructuringElement( 2, // 0 Rect, 1 cross, 2 ellipse
                           Size( 2 * morph_size + 1, 2* morph_size + 1 ),
                           Point( morph_size, morph_size ) );

      //morphologyEx(thresholds[i], thresholds[i], MORPH_OPEN, element);

      //dilate(thresholds[i], thresholds[i], element);
      //erode(thresholds[i], thresholds[i], element);

      morphologyEx(thresholds[i], thresholds[i], MORPH_CLOSE, closureElement);

      // Lastly, draw a clipping line between each character boxes
      for (unsigned int j = 0; j < charRegions.size(); j++)
      {
        line(thresholds[i], Point(charRegions[j].x - 1, charRegions[j].y), Point(charRegions[j].x - 1, charRegions[j].y + charRegions[j].height), Scalar(0, 0, 0));
        line(thresholds[i], Point(charRegions[j].x + charRegions[j].width + 1, charRegions[j].y), Point(charRegions[j].x + charRegions[j].width + 1, charRegions[j].y + charRegions[j].height), Scalar(0, 0, 0));
      }
    }
  }

  void CharacterSegmenter::cleanBasedOnColor(vector<Mat> thresholds, Mat colorMask, vector<Rect> charRegions)
  {
    // If I knock out x% of the contour area from this thing (after applying the color filter)
    // Consider it a bad news bear.  REmove the whole area.
    const float MIN_PERCENT_CHUNK_REMOVED = 0.6;

    for (unsigned int i = 0; i < thresholds.size(); i++)
    {
      for (unsigned int j = 0; j < charRegions.size(); j++)
      {
        Mat boxChar = Mat::zeros(thresholds[i].size(), CV_8U);
        rectangle(boxChar, charRegions[j], Scalar(255,255,255), CV_FILLED);

        bitwise_and(thresholds[i], boxChar, boxChar);

        float meanBefore = mean(boxChar, boxChar)[0];

        Mat thresholdCopy(thresholds[i].size(), CV_8U);
        bitwise_and(colorMask, boxChar, thresholdCopy);

        float meanAfter = mean(thresholdCopy, boxChar)[0];

        if (meanAfter < meanBefore * (1-MIN_PERCENT_CHUNK_REMOVED))
        {
          rectangle(thresholds[i], charRegions[j], Scalar(0,0,0), CV_FILLED);

          if (this->config->debugCharSegmenter)
          {
            //vector<Mat> tmpDebug;
            //Mat thresholdCopy2 = Mat::zeros(thresholds[i].size(), CV_8U);
            //rectangle(thresholdCopy2, charRegions[j], Scalar(255,255,255), CV_FILLED);
            //tmpDebug.push_back(addLabel(thresholdCopy2, "box Mask"));
            //bitwise_and(thresholds[i], thresholdCopy2, thresholdCopy2);
            //tmpDebug.push_back(addLabel(thresholdCopy2, "box Mask + Thresh"));
            //bitwise_and(colorMask, thresholdCopy2, thresholdCopy2);
            //tmpDebug.push_back(addLabel(thresholdCopy2, "box Mask + Thresh + Color"));
  //
  //		Mat tmpytmp = addLabel(thresholdCopy2, "box Mask + Thresh + Color");
  //		Mat tmpx = drawImageDashboard(tmpDebug, tmpytmp.type(), 1);
            //drawAndWait( &tmpx );

            cout << "Segmentation Filter Clean by Color: Removed Threshold " << i << " charregion " << j << endl;
            cout << "Segmentation Filter Clean by Color: before=" << meanBefore << " after=" << meanAfter  << endl;

            Point topLeft(charRegions[j].x, charRegions[j].y);
            circle(imgDbgCleanStages[i], topLeft, 5, COLOR_DEBUG_COLORFILTER, CV_FILLED);
          }
        }
      }
    }
  }


  vector<Rect> CharacterSegmenter::filterMostlyEmptyBoxes(vector<Mat> thresholds, const vector<Rect> charRegions)
  {
    // Of the n thresholded images, if box 3 (for example) is empty in half (for example) of the thresholded images,
    // clear all data for every box #3.

    //const float MIN_AREA_PERCENT = 0.1;
    const float MIN_CONTOUR_HEIGHT_PERCENT = config->segmentationMinCharHeightPercent;

    
    vector<int> boxScores(charRegions.size());

    for (unsigned int i = 0; i < charRegions.size(); i++)
      boxScores[i] = 0;

    for (unsigned int i = 0; i < thresholds.size(); i++)
    {
      for (unsigned int j = 0; j < charRegions.size(); j++)
      {
        //float minArea = charRegions[j].area() * MIN_AREA_PERCENT;

        Mat tempImg = Mat::zeros(thresholds[i].size(), thresholds[i].type());
        rectangle(tempImg, charRegions[j], Scalar(255,255,255), CV_FILLED);
        bitwise_and(thresholds[i], tempImg, tempImg);

        vector<vector<Point> > contours;
        findContours(tempImg, contours, RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

        vector<Point> allPointsInBox;
        for (unsigned int c = 0; c < contours.size(); c++)
        {
          if (contours[c].size() == 0)
            continue;

          for (unsigned int z = 0; z < contours[c].size(); z++)
            allPointsInBox.push_back(contours[c][z]);
        }

        float height = 0;
        if (allPointsInBox.size() > 0)
        {
          height = boundingRect(allPointsInBox).height;
        }

        if (height >= ((float) charRegions[j].height * MIN_CONTOUR_HEIGHT_PERCENT))
        {
          boxScores[j] = boxScores[j] + 1;
        }
        else if (this->config->debugCharSegmenter)
        {
          drawX(imgDbgCleanStages[i], charRegions[j], COLOR_DEBUG_EMPTYFILTER, 3);
        }
      }
    }

    vector<Rect> newCharRegions;

    int maxBoxScore = 0;
    for (unsigned int i = 0; i < charRegions.size(); i++)
    {
      if (boxScores[i] > maxBoxScore)
        maxBoxScore = boxScores[i];
    }

    // Need a good char sample in at least 50% of the boxes for it to be valid.
    //int MIN_FULL_BOXES = maxBoxScore * 0.49;
    int MIN_FULL_BOXES = maxBoxScore * 0.49;

    // Now check each score.  If it's below the minimum, remove the charRegion
    for (unsigned int i = 0; i < charRegions.size(); i++)
    {
      //if (boxScores[i] > MIN_FULL_BOXES)
      if (boxScores[i] >= MIN_FULL_BOXES)
        newCharRegions.push_back(charRegions[i]);
      else
      {
        // Erase the box from the Mat... mainly for debug purposes
        if (this->config->debugCharSegmenter)
        {
          cout << "Mostly Empty Filter: box index: " << i;
          cout << " this box had a score of : " << boxScores[i];;
          cout << " MIN_FULL_BOXES: " << MIN_FULL_BOXES;
          cout << " maxBoxScore: " << maxBoxScore << endl;;

          for (unsigned int z = 0; z < thresholds.size(); z++)
          {
            rectangle(thresholds[z], charRegions[i], Scalar(0,0,0), -1);//DEBUG??????????????

            drawX(imgDbgCleanStages[z], charRegions[i], COLOR_DEBUG_EMPTYFILTER, 1);
          }
        }
      }

      if (this->config->debugCharSegmenter)
        cout << " Box Score: " << boxScores[i] << endl;
    }

    return newCharRegions;
  }

  Mat CharacterSegmenter::filterEdgeBoxes(vector<Mat> thresholds, const vector<Rect> charRegions, float avgCharWidth, float avgCharHeight)
  {
    const float MIN_ANGLE_FOR_ROTATION = 0.4;
    int MIN_CONNECTED_EDGE_PIXELS = (avgCharHeight * 1.5);

    // Sometimes the rectangle won't be very tall, making it impossible to detect an edge
    // Adjust for this here.
    int alternate = thresholds[0].rows * 0.92;
    if (alternate < MIN_CONNECTED_EDGE_PIXELS && alternate > avgCharHeight)
      MIN_CONNECTED_EDGE_PIXELS = alternate;

    Mat empty_mask = Mat::zeros(thresholds[0].size(), CV_8U);
    bitwise_not(empty_mask, empty_mask);
    
    //
    // Pay special attention to the edge boxes.  If it's a skinny box, and the vertical height extends above our bounds... remove it.
    //while (charBoxes.size() > 0 && charBoxes[charBoxes.size() - 1].width < MIN_SEGMENT_WIDTH_EDGES)
    //  charBoxes.erase(charBoxes.begin() + charBoxes.size() - 1);
    // Now filter the "edge" boxes.  We don't want to include skinny boxes on the edges, since these could be plate boundaries
    //while (charBoxes.size() > 0 && charBoxes[0].width < MIN_SEGMENT_WIDTH_EDGES)
    //  charBoxes.erase(charBoxes.begin() + 0);

    // TECHNIQUE #1
    // Check for long vertical lines.  Once the line is too long, mask the whole region

    if (charRegions.size() <= 1)
      return empty_mask;

    // Check both sides to see where the edges are
    // The first starts at the right edge of the leftmost char region and works its way left
    // The second starts at the left edge of the rightmost char region and works its way right.
    // We start by rotating the threshold image to the correct angle
    // then check each column 1 by 1.

    vector<int> leftEdges;
    vector<int> rightEdges;

    if (this->config->debugCharSegmenter)
            cout << "CHARACTERSEGMENTER filterEdgeBoxes: top.angle=" << top.angle << endl;

    vector<Mat> rotatedList;
    for (unsigned int i = 0; i < thresholds.size(); i++)
    {
      Mat rotated;

      if (abs(top.angle) > MIN_ANGLE_FOR_ROTATION)
      {
        // Rotate image:
        rotated = Mat(thresholds[i].size(), thresholds[i].type());
        Mat rot_mat( 2, 3, CV_32FC1 );
        Point center = Point( thresholds[i].cols/2, thresholds[i].rows/2 );

        rot_mat = getRotationMatrix2D( center, top.angle, 1.0 );
        warpAffine( thresholds[i], rotated, rot_mat, thresholds[i].size() );

        if (this->config->debugCharSegmenter)
                    cout << "CHARACTERSEGMENTER warpAffine thresholds[" << i << "], angle=" << top.angle << endl;
      }
      else
      {
        rotated = thresholds[i];
      }
      rotatedList.push_back(rotated);

      int leftEdgeX = 0;
      int rightEdgeX = rotated.cols;
      // Do the left side
      int col = charRegions[0].x + charRegions[0].width;
      while (col >= 0)
      {
        int rowLength = getLongestBlobLengthBetweenLines(rotated, col);

        if (rowLength > MIN_CONNECTED_EDGE_PIXELS)
        {
          leftEdgeX = col;
          break;
        }

        col--;
      }

      col = charRegions[charRegions.size() - 1].x;
      while (col < rotated.cols)
      {
        int rowLength = getLongestBlobLengthBetweenLines(rotated, col);

        if (rowLength > MIN_CONNECTED_EDGE_PIXELS)
        {
          rightEdgeX = col;
          break;
        }
        col++;
      }

      if (leftEdgeX != 0)
        leftEdges.push_back(leftEdgeX);
      if (rightEdgeX != thresholds[i].cols)
        rightEdges.push_back(rightEdgeX);
    }

    if (this->config->debugCharSegmenter) {
		Mat imgDash = drawImageDashboard(rotatedList, CV_8U, 3);
		displayImage(config, "thresholds after rotation", imgDash);
    }

    int leftEdge = 0;
    int rightEdge = thresholds[0].cols;

    // Assign the edge values to the SECOND closest value
    if (leftEdges.size() > 1)
    {
      sort (leftEdges.begin(), leftEdges.begin()+leftEdges.size());
      leftEdge = leftEdges[leftEdges.size() - 2] + 1;
    }
    if (rightEdges.size() > 1)
    {
      sort (rightEdges.begin(), rightEdges.begin()+rightEdges.size());
      rightEdge = rightEdges[1] - 1;
    }

    if (this->config->debugCharSegmenter) {
    	cout << "Edge Filter: leftEdge=" << leftEdge << ", rightEdge=" << rightEdge << ", thresholds[0].cols=" << thresholds[0].cols
    			<< ", calculate mask=" << (leftEdge != 0 || rightEdge != thresholds[0].cols) << endl;
	}

    if (leftEdge != 0 || rightEdge != thresholds[0].cols)
    {
      Mat mask = Mat::zeros(thresholds[0].size(), CV_8U);
      bitwise_not(mask, mask);
      
      rectangle(mask, Point(0, charRegions[0].y), Point(leftEdge, charRegions[0].y+charRegions[0].height), Scalar(0,0,0), -1);
      rectangle(mask, Point(rightEdge, charRegions[0].y), Point(mask.cols, charRegions[0].y+charRegions[0].height), Scalar(0,0,0), -1);

      if (abs(top.angle) > MIN_ANGLE_FOR_ROTATION)
      {
        // Rotate mask:
        Mat rot_mat( 2, 3, CV_32FC1 );
        Point center = Point( mask.cols/2, mask.rows/2 );

        rot_mat = getRotationMatrix2D( center, top.angle * -1, 1.0 );
        warpAffine( mask, mask, rot_mat, mask.size() );
      }

      // If our edge mask covers more than x% of the char region, mask the whole thing...
      const float MAX_COVERAGE_PERCENT = 0.6;
      int leftCoveragePx = leftEdge - charRegions[0].x;
      float leftCoveragePercent = ((float) leftCoveragePx) / ((float) charRegions[0].width);
      float rightCoveragePx = (charRegions[charRegions.size() -1].x + charRegions[charRegions.size() -1].width) - rightEdge;
      float rightCoveragePercent = ((float) rightCoveragePx) / ((float) charRegions[charRegions.size() -1].width);
      if ((leftCoveragePercent > MAX_COVERAGE_PERCENT) ||
          (charRegions[0].width - leftCoveragePx < config->segmentationMinBoxWidthPx))
      {
        rectangle(mask, charRegions[0], Scalar(0,0,0), -1);	// Mask the whole region
        if (this->config->debugCharSegmenter)
          cout << "Edge Filter: Entire left region is erased" << endl;
      }
      if ((rightCoveragePercent > MAX_COVERAGE_PERCENT) ||
          (charRegions[charRegions.size() -1].width - rightCoveragePx < config->segmentationMinBoxWidthPx))
      {
        rectangle(mask, charRegions[charRegions.size() -1], Scalar(0,0,0), -1);
        if (this->config->debugCharSegmenter)
          cout << "Edge Filter: Entire right region is erased" << endl;
      }



      if (this->config->debugCharSegmenter)
      {
        cout << "Edge Filter: left=" << leftEdge << " right=" << rightEdge << endl;
        Mat bordered = addLabel(mask, "Edge Filter #1");
        imgDbgGeneral.push_back(bordered);

        Mat invertedMask(mask.size(), mask.type());
        bitwise_not(mask, invertedMask);
        for (unsigned int z = 0; z < imgDbgCleanStages.size(); z++)
          fillMask(imgDbgCleanStages[z], invertedMask, Scalar(0,0,255));
      }
      
      return mask;
    }

    return empty_mask;
  }

  int CharacterSegmenter::getLongestBlobLengthBetweenLines(Mat img, int col)
  {
    int longestBlobLength = 0;

    bool onSegment = false;
    bool wasbetweenLines = false;
    float curSegmentLength = 0;
    for (int row = 0; row < img.rows; row++)
    {
      bool isbetweenLines = false;

      bool isOn = img.at<uchar>(row, col);

      if (isOn)
      {
        // We're on a segment.  Increment the length
        isbetweenLines = top.isPointBelowLine(Point(col, row)) && !bottom.isPointBelowLine(Point(col, row));
        float incrementBy = 1;

        // Add a little extra to the score if this is outside of the lines
        if (!isbetweenLines)
          incrementBy = 1.1;

        onSegment = true;
        curSegmentLength += incrementBy;
      }
      if (isOn && isbetweenLines)
      {
        wasbetweenLines = true;
      }

      if (onSegment && (isOn == false || (row == img.rows - 1)))
      {
        if (wasbetweenLines && curSegmentLength > longestBlobLength)
          longestBlobLength = curSegmentLength;

        onSegment = false;
        isbetweenLines = false;
        curSegmentLength = 0;
      }
    }

    return longestBlobLength;
  }

  // Checks to see if a skinny, tall line (extending above or below the char Height) is inside the given box.
  // Returns the contour index if true.  -1 otherwise
  int CharacterSegmenter::isSkinnyLineInsideBox(Mat threshold, Rect box, vector<vector<Point> > contours, vector<Vec4i> hierarchy, float avgCharWidth, float avgCharHeight)
  {
    float MIN_EDGE_CONTOUR_HEIGHT = avgCharHeight * 1.25;

    // Sometimes the threshold is smaller than the MIN_EDGE_CONTOUR_HEIGHT.
    // In that case, adjust to be smaller
    int alternate = threshold.rows * 0.92;
    if (alternate < MIN_EDGE_CONTOUR_HEIGHT && alternate > avgCharHeight)
      MIN_EDGE_CONTOUR_HEIGHT = alternate;

    Rect slightlySmallerBox(box.x, box.y, box.width, box.height);
    Mat boxMask = Mat::zeros(threshold.size(), CV_8U);
    rectangle(boxMask, slightlySmallerBox, Scalar(255, 255, 255), -1);

    for (unsigned int i = 0; i < contours.size(); i++)
    {
      // Only bother with the big boxes
      if (boundingRect(contours[i]).height < MIN_EDGE_CONTOUR_HEIGHT)
        continue;

      Mat tempImg = Mat::zeros(threshold.size(), CV_8U);
      drawContours(tempImg, contours, i, Scalar(255,255,255), -1, 8, hierarchy, 1);
      bitwise_and(tempImg, boxMask, tempImg);

      vector<vector<Point> > subContours;
      findContours(tempImg, subContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
      int tallestContourIdx = -1;
      int tallestContourHeight = 0;
      int tallestContourWidth = 0;
      float tallestContourArea = 0;
      for (unsigned int s = 0; s < subContours.size(); s++)
      {
        Rect r = boundingRect(subContours[s]);
        if (r.height > tallestContourHeight)
        {
          tallestContourIdx = s;
          tallestContourHeight = r.height;
          tallestContourWidth = r.width;
          tallestContourArea = contourArea(subContours[s]);
        }
      }

      if (tallestContourIdx != -1)
      {
        //cout << "Edge Filter: " << tallestContourHeight << " -- " << avgCharHeight << endl;
        if (tallestContourHeight >= avgCharHeight * 0.9 &&
            ((tallestContourWidth < config->segmentationMinBoxWidthPx) || (tallestContourArea < avgCharWidth * avgCharHeight * 0.1)))
        {
          cout << "Edge Filter: Avg contour width: " << avgCharWidth << " This guy is: " << tallestContourWidth << endl;
          cout << "Edge Filter: tallestContourArea: " << tallestContourArea << " Minimum: " << avgCharWidth * avgCharHeight * 0.1 << endl;
          return i;
        }
      }
    }

    return -1;
  }

  Mat CharacterSegmenter::getCharBoxMask(Mat img_threshold, vector<Rect> charBoxes)
  {
    Mat mask = Mat::zeros(img_threshold.size(), CV_8U);
    for (unsigned int i = 0; i < charBoxes.size(); i++)
      rectangle(mask, charBoxes[i], Scalar(255, 255, 255), -1);

    return mask;
  }
  std::vector<cv::Rect> CharacterSegmenter::convert1DHitsToRect(vector<pair<int, int> > hits, LineSegment top, LineSegment bottom) {

    vector<Rect> boxes;
    
    for (unsigned int i = 0; i < hits.size(); i++)
    {
      Point topLeft = Point(hits[i].first, top.getPointAt(hits[i].first) - 1);
      Point botRight = Point(hits[i].second, bottom.getPointAt(hits[i].second) + 1);
      
      boxes.push_back(Rect(topLeft, botRight));
    }
    
    return boxes;
  }

}
