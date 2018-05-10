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

#include "tesseract_ocr.h"
#include "config.h"

#include "segmentation/charactersegmenter.h"

using namespace std;
using namespace cv;
using namespace tesseract;

namespace alpr
{

  TesseractOcr::TesseractOcr(Config* config)
  : OCR(config)
  {
    const string MINIMUM_TESSERACT_VERSION = "3.03";

    this->postProcessor.setConfidenceThreshold(config->postProcessMinConfidence, config->postProcessConfidenceSkipLevel);
    if (config->debugOcr)
       printf("TesseractOcr: config->postProcessMinConfidence=%d", config->postProcessMinConfidence);
    
    if (cmpVersion(tesseract.Version(), MINIMUM_TESSERACT_VERSION.c_str()) < 0)
    {
      std::cerr << "Warning: You are running an unsupported version of Tesseract." << endl;
      std::cerr << "Expecting at least " << MINIMUM_TESSERACT_VERSION << ", your version is: " << tesseract.Version() << endl;
    }

    // Tesseract requires the prefix directory to be set as an env variable
    tesseract.Init(config->getTessdataPrefix().c_str(), config->ocrLanguage.c_str() 	);
    tesseract.SetVariable("save_blob_choices", "T");
    tesseract.SetVariable("debug_file", "/dev/null");
    tesseract.SetPageSegMode(PSM_SINGLE_CHAR);
  }

  TesseractOcr::~TesseractOcr()
  {
    tesseract.End();
  }
  
  std::vector<OcrChar> TesseractOcr::recognize_line_as_text(cv::Mat thresholdParam) {
	  const int SPACE_CHAR_CODE = 32;

    // Make it black text on white background
    Mat threshold(thresholdParam.size(), thresholdParam.type());
    thresholdParam.copyTo(threshold);
    bitwise_not(threshold, threshold);
    tesseract.SetImage((uchar*) threshold.data,
    threshold.size().width, threshold.size().height,
    threshold.channels(), threshold.step1());
    std::vector<OcrChar> ocrChars;

	  tesseract.SetPageSegMode(PSM_SINGLE_LINE);
	  tesseract.Recognize(NULL);
	  char* outText = tesseract.GetUTF8Text();
	  if (this->config->debugOcr)
		  printf("=================================thresholds[...] OCR output: %s\n", outText);
	  tesseract::ResultIterator* ri = tesseract.GetIterator();
	  tesseract::PageIteratorLevel level = tesseract::RIL_SYMBOL;
	  int pos = 0;
    do
     {
       const char* symbol = ri->GetUTF8Text(level);
       //int len = symbol.length();

       float conf = ri->Confidence(level);

       bool dontcare;
      int fontindex = 0;
      int pointsize = 0;
      const char* fontName = ri->WordFontAttributes(&dontcare, &dontcare, &dontcare, &dontcare, &dontcare, &dontcare, &pointsize, &fontindex);

      // Ignore NULL pointers, spaces, and characters that are way too small to be valid
      if(symbol != 0 && symbol[0] != SPACE_CHAR_CODE && pointsize >= config->ocrMinFontSize)
      {
      OcrChar c;
      c.char_index = pos;
      c.confidence = conf;
      c.letter = string(symbol);
      int x1, y1, x2, y2;
        ri->BoundingBox(level, &x1, &y1, &x2, &y2);
        c.rect = Rect(x1, y1, x2-x1, y2-y1);
        ocrChars.push_back(c);
      pos++;
      }

       if (this->config->debugOcr)
         printf("=================================charpos %d symbol %s, conf: %f\n", pos, symbol, conf);

     }
	   while((ri->Next(level)));

	  tesseract.SetPageSegMode(PSM_SINGLE_CHAR);
	  return ocrChars;
  }


  std::vector<OcrChar> TesseractOcr::recognize_line(int line_idx, PipelineData* pipeline_data) {

    const int SPACE_CHAR_CODE = 32;
    tesseract.SetPageSegMode(PSM_SINGLE_CHAR);
    
//    tesseract.SetPageSegMode(PSM_SINGLE_WORD);

//	for (unsigned int j = 0; j < pipeline_data->charRegions[line_idx].size(); j++)
//		{
//		Rect expandedRegion = expandRect( pipeline_data->charRegions[line_idx][j], 2, 2, pipeline_data->thresholds[i].cols, pipeline_data->thresholds[i].rows) ;
//
//		tesseract.SetRectangle(expandedRegion.x, expandedRegion.y, expandedRegion.width, expandedRegion.height);
//		tesseract.Recognize(NULL);
//
//		tesseract::ResultIterator* ri = tesseract.GetIterator();
//		tesseract::PageIteratorLevel level = tesseract::RIL_SYMBOL;
//		do
//		{
//		  const char* symbol = ri->GetUTF8Text(level);
//
//		  //int len = symbol.length();
//
//		  float conf = ri->Confidence(level);
//
//		  bool dontcare;
//		  int fontindex = 0;
//		  int pointsize = 0;
//		  const char* fontName = ri->WordFontAttributes(&dontcare, &dontcare, &dontcare, &dontcare, &dontcare, &dontcare, &pointsize, &fontindex);
//
//		  // Ignore NULL pointers, spaces, and characters that are way too small to be valid
//		  if(symbol != 0 && symbol[0] != SPACE_CHAR_CODE && pointsize >= config->ocrMinFontSize)
//		  {
//			OcrChar c;
//			c.char_index = absolute_charpos;
//			c.confidence = conf;
//			c.letter = string(symbol);
//			recognized_chars.push_back(c);
//		  }
//		} while (true);
//	}






    std::vector<OcrChar> recognized_chars;
    
    for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
    {
      // Make it black text on white background
      bitwise_not(pipeline_data->thresholds[i], pipeline_data->thresholds[i]);
      tesseract.SetImage((uchar*) pipeline_data->thresholds[i].data, 
                          pipeline_data->thresholds[i].size().width, pipeline_data->thresholds[i].size().height, 
                          pipeline_data->thresholds[i].channels(), pipeline_data->thresholds[i].step1());

//      if (this->config->debugOcr) {
//    	  tesseract.SetPageSegMode(PSM_SINGLE_LINE);
//    	  tesseract.Recognize(NULL);
//    	  char* outText = tesseract.GetUTF8Text();
//    	  printf("=================================thresholds[%d] OCR output: %s\n", i, outText);
//    	  tesseract::ResultIterator* ri = tesseract.GetIterator();
//		 tesseract::PageIteratorLevel level = tesseract::RIL_SYMBOL;
//		 int pos = 0;
//		 do
//		 {
//		   const char* symbol = ri->GetUTF8Text(level);
//		   pos++;
//		   //int len = symbol.length();
//
//		   float conf = ri->Confidence(level);
//		   printf("=================================charpos %d symbol %s, conf: %f\n", pos, symbol, conf);
//
//		 }
//		   while((ri->Next(level)));
//
//    	  tesseract.SetPageSegMode(PSM_SINGLE_CHAR);
//      }
 
      int absolute_charpos = 0;
//      tesseract.SetPageSegMode(PSM_SINGLE_WORD);

      for (unsigned int j = 0; j < pipeline_data->charRegions[line_idx].size(); j++)
      {
        //Rect expandedRegion = expandRect( pipeline_data->charRegions[line_idx][j], 2, 2, pipeline_data->thresholds[i].cols, pipeline_data->thresholds[i].rows) ;
    	  Rect expandedRegion = pipeline_data->charRegions[line_idx][j];
        tesseract.SetRectangle(expandedRegion.x, expandedRegion.y, expandedRegion.width, expandedRegion.height);
        tesseract.Recognize(NULL);

        tesseract::ResultIterator* ri = tesseract.GetIterator();
        tesseract::PageIteratorLevel level = tesseract::RIL_SYMBOL;
        do
        {
          const char* symbol = ri->GetUTF8Text(level);
          float conf = ri->Confidence(level);

          bool dontcare;
          int fontindex = 0;
          int pointsize = 0;
          const char* fontName = ri->WordFontAttributes(&dontcare, &dontcare, &dontcare, &dontcare, &dontcare, &dontcare, &pointsize, &fontindex);

          // Ignore NULL pointers, spaces, and characters that are way too small to be valid
          if(symbol != 0 && symbol[0] != SPACE_CHAR_CODE && pointsize >= config->ocrMinFontSize)
          {
            OcrChar c;
            c.char_index = absolute_charpos;
            c.confidence = conf;
            c.letter = string(symbol);
            recognized_chars.push_back(c);

            if (this->config->debugOcr)
              printf("charpos%d line%d: threshold %d:  symbol %s, conf: %f font: %s (index %d) size %dpx", absolute_charpos, line_idx, i, symbol, conf, fontName, fontindex, pointsize);

            bool indent = false;
            tesseract::ChoiceIterator ci(*ri);
            do
            {
              const char* choice = ci.GetUTF8Text();
              
              OcrChar c2;
              c2.char_index = absolute_charpos;
              c2.confidence = ci.Confidence();
              c2.letter = string(choice);
              
              //1/17/2016 adt adding check to avoid double adding same character if ci is same as symbol. Otherwise first choice from ResultsIterator will get added twice when choiceIterator run.
              if (string(symbol) != string(choice))
                recognized_chars.push_back(c2);
              else
              {
                // Explictly double-adding the first character.  This leads to higher accuracy right now, likely because other sections of code
                // have expected it and compensated. 
                // TODO: Figure out how to remove this double-counting of the first letter without impacting accuracy
                recognized_chars.push_back(c2);
              }
              if (this->config->debugOcr)
              {
                if (indent) printf("\t\t ");
                printf("\t- ");
                printf("%s conf: %f\n", choice, ci.Confidence());
              }

              indent = true;
            }
            while(ci.Next());

          } else {
        	  if (this->config->debugOcr)
        	                printf("IGNORE charpos%d line%d: threshold %d:  symbol %s, conf: %f font: %s (index %d) size %dpx, config.ocrMinFontSize %dpx", absolute_charpos, line_idx, i, symbol, conf, fontName, fontindex, pointsize, config->ocrMinFontSize);
          }

          if (this->config->debugOcr)
            printf("---------------------------------------------\n");

          delete[] symbol;
          //absolute_charpos++;
        }
        while((ri->Next(level)));

        delete ri;

        absolute_charpos++;
      }
      
    }
    
    return recognized_chars;
  }
//  void TesseractOcr::segment(PipelineData* pipeline_data) {
//
//    CharacterSegmenter segmenter(pipeline_data);
//    segmenter.segment();
//  }


}
