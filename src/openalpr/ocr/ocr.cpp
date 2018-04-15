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

#include "ocr.h"

#include "segmentation/charactersegmenter.h"

using namespace cv;

namespace alpr
{
  
  OCR::OCR(Config* config) : postProcessor(config) {
    this->config = config;
  }


  OCR::~OCR() {
  }

  
  void OCR::performOCR(PipelineData* pipeline_data)
  {
    
    timespec startTime;
    getTimeMonotonic(&startTime);

    CharacterSegmenter segmenter(pipeline_data);
    for (unsigned int lineidx = 0; lineidx < pipeline_data->textLines.size(); lineidx++) {
    	segmenter.removeSmallContoursPreprocess(pipeline_data->thresholds, pipeline_data->textLines[lineidx]);
    }

    for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
	{
		std::vector<OcrChar> ocrChars = recognize_line_as_text(pipeline_data->thresholds[i]);
			string str;
			for (unsigned int k = 0; k < ocrChars.size(); k++) {
				str.append(ocrChars[k].letter);

			}
			pipeline_data->thresholdOcrLines.push_back(str);
			if (this->config->debugCharSegmenter)
				std::cout << "===================================== OCR LINE chars in threshols[" << i << "]: " << str << std::endl;
	}

    segmenter.segment();
    
    postProcessor.clear();


//    std::vector<std::vector<cv::Rect> > newCharRegions;
//    std::vector<cv::Rect> newCharRegionsFlat;
    pipeline_data->charRegionsFlat.clear();

    for (unsigned int line_idx = 0; line_idx < pipeline_data->textLines.size(); line_idx++)
    {
	  int sumWidth = 0;
	  int sumHeight = 0;
	  int cntGood = 0;
	  for (unsigned int i = 0; i < pipeline_data->textLines[line_idx].textContours.size(); i++)
	  {
		if (pipeline_data->textLines[line_idx].textContours.goodIndices[i] == false)
		  continue;
		cntGood++;
		Rect mr= boundingRect(pipeline_data->textLines[line_idx].textContours.contours[i]);
		sumWidth += mr.width;
		sumHeight += mr.height;
	  }
      float avgGoodContourWidth = (float)sumWidth / cntGood;
      float avgGoodContourHeight = (float)sumHeight / cntGood;
      std::vector<cv::Mat> imgDbgThresholds;
      //std::vector<std::vector<cv::Rect> > thresholdsInnerSymbRects;
      std::vector<std::vector<OcrChar> > thresholdsOcrChars;
      for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
	  {
    	std::vector<OcrChar> ocrChars = recognize_line_as_text(pipeline_data->thresholds[i]);
    	thresholdsOcrChars.push_back(ocrChars);

    	if (this->config->debugCharSegmenter) {
			Mat debugImg = Mat::zeros(pipeline_data->thresholds[i].size(), pipeline_data->thresholds[i].type());
			pipeline_data->thresholds[i].copyTo(debugImg);
			bitwise_not(debugImg, debugImg);
			for (unsigned int c = 0; c < ocrChars.size(); c++) {
			  rectangle(debugImg, ocrChars[c].rect, Scalar(0, 255, 0), 1);
			}

			for (unsigned int c = 0; c < pipeline_data->charRegions[line_idx].size(); c++) {
				Rect charRegion = pipeline_data->charRegions[line_idx][c];
				rectangle(debugImg, charRegion, Scalar(0, 255, 0), 1);
			}

			imgDbgThresholds.push_back(debugImg);
		  }
	  }

      if (this->config->debugCharSegmenter)
		{
		  Mat imgDash = drawImageDashboard(imgDbgThresholds, CV_8U, 3);
		  displayImage(config, "Segmentation after OCR", imgDash);
		}

      vector<cv::Rect> newLineCharRegions;
      for (unsigned int c = 0; c < pipeline_data->charRegions[line_idx].size(); c++) {
		Rect charRegion = pipeline_data->charRegions[line_idx][c];
		vector<vector<OcrChar>> charRegionInnerOcrCharsForAllThresholds;

		int thresholdWithMaxConfidenceId = -1;
		int thresholdMaxConfidence = 0;

		for (unsigned int n = 0; n < thresholdsOcrChars.size(); n++) {
		  vector<OcrChar> lineOcrChars = thresholdsOcrChars[n];
		  vector<OcrChar> innerOcrCharsInCharRegion;//OCR chars for the current charRegion and current threshold
		  for (unsigned int k = 0; k < lineOcrChars.size(); k++) {
			  Rect r = lineOcrChars[k].rect;
			  if (r.x > charRegion.x + charRegion.width) {
				  break;
			  }
			  int xCenter =  r.x + (r.width / 2);
			  if (charRegion.x <= xCenter && charRegion.x + charRegion.width >= xCenter) {
				  if (lineOcrChars[k].confidence > thresholdMaxConfidence) {
					  thresholdMaxConfidence = lineOcrChars[k].confidence;
					  thresholdWithMaxConfidenceId = n;
				  }
				  innerOcrCharsInCharRegion.push_back(lineOcrChars[k]);
			  }
		  }
		  charRegionInnerOcrCharsForAllThresholds.push_back(innerOcrCharsInCharRegion);
		  if (this->config->debugCharSegmenter)
		  		std::cout << "OCR find " << innerOcrCharsInCharRegion.size() << " symbols in char region, id = " << c
				<< ", charRegionInnerOcrCharsForAllThresholds[" << n << "].size()=" << charRegionInnerOcrCharsForAllThresholds[n].size() << std::endl;
		}
		unsigned int ocrCharsSize = 0;
		if (thresholdWithMaxConfidenceId > -1)
			ocrCharsSize = charRegionInnerOcrCharsForAllThresholds[thresholdWithMaxConfidenceId].size();
		if (this->config->debugCharSegmenter)
			std::cout << "OCR char region, id = " << c << ": thresholdWithMaxConfidenceId=" << thresholdWithMaxConfidenceId
				<< ", ocrCharsSize=" << ocrCharsSize << ", charRegion.width=" << charRegion.width << ", avgGoodContourWidth=" << avgGoodContourWidth << std::endl;

		bool newRegion = false;

		if (thresholdWithMaxConfidenceId > -1 && charRegionInnerOcrCharsForAllThresholds[thresholdWithMaxConfidenceId].size() > 1) {
			vector<OcrChar> innerOcrCharsInCharRegion = charRegionInnerOcrCharsForAllThresholds[thresholdWithMaxConfidenceId];
			if (charRegion.width > 2.5*avgGoodContourWidth && charRegionInnerOcrCharsForAllThresholds[thresholdWithMaxConfidenceId].size() > 2) {
				int firstRightX = innerOcrCharsInCharRegion[0].rect.x + innerOcrCharsInCharRegion[0].rect.width;
				int secondLeftX = innerOcrCharsInCharRegion[1].rect.x;
				int newX1 = (int)(firstRightX + secondLeftX)/2;

				int secondRightX = innerOcrCharsInCharRegion[1].rect.x + innerOcrCharsInCharRegion[1].rect.width;
				int thirdLeftX = innerOcrCharsInCharRegion[2].rect.x;
				int newX2 = (int)(secondRightX + thirdLeftX)/2;

				Rect rA(charRegion.x, charRegion.y,	newX1 - charRegion.x, charRegion.height);
				Rect rB(newX1, charRegion.y, newX2 - newX1, charRegion.height);
				Rect rC(newX2, charRegion.y, charRegion.x + charRegion.width - newX2, charRegion.height);

				newLineCharRegions.push_back(rA);
				newLineCharRegions.push_back(rB);
				newLineCharRegions.push_back(rC);
				newRegion = true;
				if (this->config->debugCharSegmenter)
					std::cout << "OCR find NEW char region, id = " << c << ", rect A: " << rA << ", rect B:" << rB << ", rect С:" << rC << ", old rect: " << charRegion << std::endl;
			} else if (charRegion.width > 1.5*avgGoodContourWidth) {
				int firstRightX = innerOcrCharsInCharRegion[0].rect.x + innerOcrCharsInCharRegion[0].rect.width;
				int secondLeftX = innerOcrCharsInCharRegion[1].rect.x;
				int newRegionX = (int)(firstRightX + secondLeftX)/2;
				Rect rA(charRegion.x, charRegion.y,	newRegionX - charRegion.x, charRegion.height);
				Rect rB(newRegionX, charRegion.y, charRegion.x + charRegion.width - newRegionX, charRegion.height);
				newLineCharRegions.push_back(rA);
				newLineCharRegions.push_back(rB);
				newRegion = true;
				if (this->config->debugCharSegmenter)
					std::cout << "OCR find NEW char region, id = " << c << ", rect A: " << rA << ", rect B:" << rB << ", old rect: " << charRegion << std::endl;
			}
		}
		if (!newRegion) {
			newLineCharRegions.push_back(charRegion);
			if (this->config->debugCharSegmenter)
				std::cout << "OCR OLD char region, id = " << c << ", rect: " << charRegion << std::endl;
		}

	  }
      //newCharRegions.push_back(newLineCharRegions);
      pipeline_data->charRegions[line_idx].clear();
      for (unsigned int t = 0; t < newLineCharRegions.size(); t++) {
    	  pipeline_data->charRegions[line_idx].push_back(newLineCharRegions[t]);
    	  pipeline_data->charRegionsFlat.push_back(newLineCharRegions[t]);
      }

    }



    int absolute_charpos = 0;

    for (unsigned int line_idx = 0; line_idx < pipeline_data->textLines.size(); line_idx++)
        {

      std::vector<OcrChar> chars = recognize_line(line_idx, pipeline_data);
      
      for (uint32_t i = 0; i < chars.size(); i++)
      {
        // For multi-line plates, set the character indexes to sequential values based on the line number
        int line_ordered_index = (line_idx * config->postProcessMaxCharacters) + chars[i].char_index;
        postProcessor.addLetter(chars[i].letter, line_idx, line_ordered_index, chars[i].confidence);
//        if (config->debugOcr)
//            {
//              std::cout << "OCR postProcessor.addLetter[" << chars[i].char_index << "]: " << chars[i].letter << ", confidence=" << chars[i].confidence << std::endl;
//            }
        absolute_charpos++;
      }
    }
    

    if (config->debugTiming)
    {
      timespec endTime;
      getTimeMonotonic(&endTime);
      std::cout << "OCR Time: " << diffclock(startTime, endTime) << "ms." << std::endl;
    }
  }

//  std::vector<cv::Rect> getCharRects(PipelineData* pipeline_data) {
//
//  }
}
