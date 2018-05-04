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

#include "result_aggregator.h"

#include <iomanip>

using namespace std;
using namespace cv;

namespace alpr
{

  ResultAggregator::ResultAggregator(ResultMergeStrategy merge_strategy, int topn, Config* config)
  {
    this->prewarp = new PreWarp(config);
    this->merge_strategy = merge_strategy;
    this->topn = topn;
    this->config = config;
  }

  ResultAggregator::~ResultAggregator() {
    delete prewarp;
  }


  void ResultAggregator::addResults(AlprFullDetails full_results)
  {
    all_results.push_back(full_results);
  }
  

  cv::Mat ResultAggregator::applyImperceptibleChange(cv::Mat image, int index) {
    
    const float WIDTH_HEIGHT = 600;
    const float NO_MOVE_WIDTH_DIST = 1.0;
    const float NO_PAN_VAL = 0;
    float step = 0.000035;

    // Don't warp the first indexed image
    if (index == 0)
      return image;
    
    // Use 3 bits to figure out which one is on.  Multiply by the modulus of 8
    // 000, 001, 010, 011, 100, 101, 110, 111
    // if 101, then x_rotation and z_rotation are on.
    if (index % 8 == 0)
    {
      // Do something special for the 0s, so they don't repeat
      index--;
      step = step / 1.5;
    }
    int multiplier = (index / 8) + 1;
    int bitwise_on = index % 8;
    float x_rotation = ((bitwise_on & 1) == 1) * multiplier * step;
    float y_rotation = ((bitwise_on & 2) == 2) * multiplier * step;
    float z_rotation = ((bitwise_on & 4) == 4) * multiplier * step;
    
    //cout << "Iteration: " << index << ": " << x_rotation << ", " << y_rotation << ", " << z_rotation << endl;
    
    prewarp->setTransform(WIDTH_HEIGHT, WIDTH_HEIGHT, x_rotation, y_rotation, z_rotation, 
            NO_PAN_VAL, NO_PAN_VAL, NO_MOVE_WIDTH_DIST, NO_MOVE_WIDTH_DIST);
    
    return prewarp->warpImage(image);
  }

  bool compareScore(const std::pair<float, ResultPlateScore>& firstElem, const std::pair<float, ResultPlateScore>& secondElem) {
    return firstElem.first > secondElem.first;
  }
  
  bool compareScoreAndRuTemplate(const AlprPlate &first, const AlprPlate &second) {
  	  if (first.matches_template && second.matches_template) {
  		  if ((first.characters.size() == 9 && second.characters.size() == 9) ||
  				  (first.characters.size() == 8 && second.characters.size() == 8)) {
  			  return first.overall_confidence > second.overall_confidence;
  		  } else if (first.characters.size() == 9) {
  			  if (first.overall_confidence > second.overall_confidence || second.overall_confidence - first.overall_confidence < 5) {
  				  return true;
  			  } else {
  				  return false;
  			  }
  		  } else {//second.characters.size() == 9
  			if (second.overall_confidence > first.overall_confidence || first.overall_confidence - second.overall_confidence < 5) {
				  return false;
			  } else {
				  return true;
			  }
  		  }
  	  } else if (first.matches_template) {
  		  return true;
  	  } else if (second.matches_template) {
  		  return false;
  	  }
      return first.overall_confidence > second.overall_confidence;
    }

  bool plateCompareByConfidence(const AlprPlate &first, const AlprPlate &second)
	{
	  return first.overall_confidence > second.overall_confidence;
	}

  bool compareAlprPlateResults(const AlprPlateResult &first, const AlprPlateResult &second)
  	{
	  float firstMaxOverallConfidence = -1;
	  for (int k = 0; k < first.topNPlates.size(); k++) {
		  if (k == 0 || (first.topNPlates[k].overall_confidence > firstMaxOverallConfidence)) {
			  firstMaxOverallConfidence = first.topNPlates[k].overall_confidence;
		  }
	  }

	  float secondMaxOverallConfidence = -1;
	  for (int k = 0; k < second.topNPlates.size(); k++) {
		  if (k == 0 || (second.topNPlates[k].overall_confidence > secondMaxOverallConfidence)) {
			  secondMaxOverallConfidence = second.topNPlates[k].overall_confidence;
		  }
	  }

	  return firstMaxOverallConfidence > secondMaxOverallConfidence;
  	}

  AlprFullDetails ResultAggregator::getAggregateResults()
  {
    assert(all_results.size() > 0);

    if (all_results.size() == 1)
      return all_results[0];


    AlprFullDetails response;

    // Plate regions are needed for benchmarking
    // Copy all detected boxes across all results
    for (unsigned int i = 0; i < all_results.size(); i++)
    {
      for (unsigned int k = 0; k < all_results[i].plateRegions.size(); k++)
        response.plateRegions.push_back(all_results[i].plateRegions[k]);
    }


    response.results.epoch_time = all_results[0].results.epoch_time;
    response.results.img_height = all_results[0].results.img_height;
    response.results.img_width = all_results[0].results.img_width;
    response.results.total_processing_time_ms = all_results[0].results.total_processing_time_ms;
    response.results.regionsOfInterest = all_results[0].results.regionsOfInterest;


    vector<vector<AlprPlateResult> > clusters = findClusters();

    if (merge_strategy == MERGE_PICK_BEST)
    {
      // Assume we have multiple results, one cluster for each unique train data (e.g., eu, eu2)

      // Now for each cluster of plates, pick the best one
      for (unsigned int i = 0; i < clusters.size(); i++)
      {
        float best_confidence = 0;
        int best_index = 0;
        for (unsigned int k = 0; k < clusters[i].size(); k++)
        {
          if (clusters[i][k].bestPlate.overall_confidence > best_confidence)
          {
            best_confidence = clusters[i][k].bestPlate.overall_confidence;
            best_index = k;
          }
        }

        response.results.plates.push_back(clusters[i][best_index]);
      }
    } else if (merge_strategy == MERGE_ON_MATCH_TEMPLATE) {

//    	if (letters[i].size() > 0)
//    	        std::stable_sort(letters[i].begin(), letters[i].end(), letterCompare);
//
//    	 std::sort(tempTextLines.begin(), tempTextLines.end(), sort_text_line);
//
//
//    	 std::sort(contours.begin(), contours.end(),
//				[](const vector<Point>& contour1, const vector<Point>& contour2){
//					Rect ra(boundingRect(contour1));
//					  Rect rb(boundingRect(contour2));
//					  return (ra.x < rb.x);
//				});

    	vector<AlprPlate> matchesTemplatePlates;
    	vector<AlprPlate> notMatchesTemplatePlates;
    	for (unsigned int i = 0; i < clusters.size(); i++)//for each country
		  {
			for (unsigned int k = 0; k < clusters[i].size(); k++)//for each detected plate
			{
				for (unsigned int m = 0; m < clusters[i][k].topNPlates.size(); m++) {
					if (clusters[i][k].topNPlates[m].matches_template) {
						matchesTemplatePlates.push_back(clusters[i][k].topNPlates[m]);
					} else {
						notMatchesTemplatePlates.push_back(clusters[i][k].topNPlates[m]);
					}
				}
			}
		  }
    	std::sort(matchesTemplatePlates.begin(), matchesTemplatePlates.end(), plateCompareByConfidence);
    	std::sort(notMatchesTemplatePlates.begin(), notMatchesTemplatePlates.end(), plateCompareByConfidence);

//    	for (unsigned int i = 0; i < matchesTemplatePlates.size(); i++) {
//    		response.results.plates.push_back(matchesTemplatePlates[i]);
//    	}
//    	for (unsigned int i = 0; i < notMatchesTemplatePlates.size(); i++) {
//			response.results.plates.push_back(notMatchesTemplatePlates[i]);
//		}

    } else if (merge_strategy == MERGE_COMBINE)
    {
      // Each cluster is the same plate, just analyzed from a slightly different 
      // perspective.  Merge them together and score them as if they are one

      const float MIN_CONFIDENCE = 50; //config->resultAggregatorOverallConfidence;//50;
      

      // Factor in the position of the plate in the topN list, the confidence, and the template match status
      // First loop is for clusters of possible plates.  If they're in separate clusters, they don't get combined, 
      // since they are likely separate plates in the same image
      for (unsigned int unique_plate_idx = 0; unique_plate_idx < clusters.size(); unique_plate_idx++)
      {
        std::map<string, ResultPlateScore> score_hash;
        
        // Second loop is for separate plate results for the same plate
        for (unsigned int i = 0; i < clusters[unique_plate_idx].size(); i++)
        {
          // Third loop is the individual topN results for a single plate result
          for (unsigned int j = 0; j < clusters[unique_plate_idx][i].topNPlates.size() && j < topn; j++)
          {
            AlprPlate plateCandidate = clusters[unique_plate_idx][i].topNPlates[j];
            
            if (plateCandidate.overall_confidence < MIN_CONFIDENCE) {
            	if (config->debugGeneral)
            	          cout << "Result Aggregator:: skip one of cluster plateCandidates: cluster[" << unique_plate_idx
						  << "], plate[" << i << "], topNPlates[" << j << "]=" << plateCandidate.characters
						  << ", overall_confidence=" << ", plateCandidate.overall_confidence=" << plateCandidate.overall_confidence
						  << ", MIN_CONFIDENCE=" << MIN_CONFIDENCE << endl;
            	continue;
            }

            float score = (plateCandidate.overall_confidence - 60) * 4;

            // Add a bonus for matching the template
            if (plateCandidate.matches_template)
              score += 150;//1000;//150;

            // Add a bonus the higher the plate is to the #1 position
            // and how frequently it appears there
            float position_score_max_bonus = 65;
            float frequency_modifier = ((float) position_score_max_bonus) / topn;
            score += position_score_max_bonus - (j * frequency_modifier);
            

            if (score_hash.find(plateCandidate.characters) == score_hash.end())
            {
              ResultPlateScore newentry;
              newentry.plate = plateCandidate;
              newentry.score_total = 0;
              newentry.count = 0;
              score_hash[plateCandidate.characters] = newentry;
            }

            score_hash[plateCandidate.characters].score_total += score;
            score_hash[plateCandidate.characters].count += 1;
            // Use the best confidence value for a particular candidate
            if (plateCandidate.overall_confidence > score_hash[plateCandidate.characters].plate.overall_confidence)
              score_hash[plateCandidate.characters].plate.overall_confidence = plateCandidate.overall_confidence;
          }
        }

        // There is a big list of results that have scores.  Sort them by top score
        std::vector<std::pair<float, ResultPlateScore> > sorted_results;
        std::map<string, ResultPlateScore>::iterator iter;
        for (iter = score_hash.begin(); iter != score_hash.end(); iter++) {
          std::pair<float,ResultPlateScore> r;
          r.second = iter->second;
          r.first = iter->second.score_total;
          sorted_results.push_back(r);
        }

        std::sort(sorted_results.begin(), sorted_results.end(), compareScore);
        
        // output the sorted list for debugging:
        if (config->debugGeneral)
        {
          cout << "Result Aggregator Scores: " << endl;
          cout << "  " << std::setw(14) << "Plate Num"
              << std::setw(15) << "Score"
              << std::setw(10) << "Count"
              << std::setw(10) << "Best conf (%)"
              << endl;
          
          for (int r_idx = 0; r_idx < sorted_results.size(); r_idx++)
          {
            cout << "  " << std::setw(14) << sorted_results[r_idx].second.plate.characters
                    << std::setw(15) << sorted_results[r_idx].second.score_total
                    << std::setw(10) << sorted_results[r_idx].second.count
                    << std::setw(10) << sorted_results[r_idx].second.plate.overall_confidence 
                    << endl;

          }
        }
        
        //for the one cluster (one plate)
        if (sorted_results.size() > 0)
        {
          // Figure out the best region for this cluster
          ResultRegionScore regionResults = findBestRegion(clusters[unique_plate_idx]);

          AlprPlateResult firstResult = clusters[unique_plate_idx][0];
          AlprPlateResult copyResult;
          copyResult.bestPlate = sorted_results[0].second.plate;
          copyResult.plate_index = firstResult.plate_index;
          copyResult.region = regionResults.region;
          copyResult.regionConfidence = regionResults.confidence;
          copyResult.processing_time_ms = firstResult.processing_time_ms;
          copyResult.requested_topn = firstResult.requested_topn;
          for (int p_idx = 0; p_idx < 4; p_idx++)
            copyResult.plate_points[p_idx] = firstResult.plate_points[p_idx];

          for (int i = 0; i < sorted_results.size(); i++)
          {
            if (i >= topn)
              break;

            copyResult.topNPlates.push_back(sorted_results[i].second.plate);
          }
          
          std::sort(copyResult.topNPlates.begin(), copyResult.topNPlates.end(), compareScoreAndRuTemplate);

          for (int i = 0; i < firstResult.thresholdOcrLines.size(); i++) {
        	  copyResult.thresholdOcrLines.push_back(firstResult.thresholdOcrLines[i]);
			}

          response.results.plates.push_back(copyResult);
        }

      }

      std::sort(response.results.plates.begin(), response.results.plates.end(), compareAlprPlateResults);
    }

    return response;
  }
  
  ResultRegionScore ResultAggregator::findBestRegion(std::vector<AlprPlateResult> clusterPlates) {

    const float MIN_REGION_CONFIDENCE = 60;//config->resultAggregatorOverallConfidence;//60;
    
    std::map<std::string, float> score_hash;
    std::map<std::string, float> score_count;
    int max_topn = 10;
    
    ResultRegionScore response;
    response.confidence = 0;
    response.region = "";
    
    for (unsigned int i = 0; i < clusterPlates.size(); i++)
    {
      AlprPlateResult plate = clusterPlates[i];
      //plate.bestPlate - either the first entry, or the first entry with a postprocessor template match
      if (plate.bestPlate.overall_confidence < MIN_REGION_CONFIDENCE ) {
    	  if (config->debugGeneral)
			  cout << "Result Aggregator:: skip entire cluster due of low overall_confidence of the bestPlate. bestPlate: plate["
			  	  << i << "], plate.bestPlate.characters="
			  	  << plate.bestPlate.characters << ", plate.bestPlate.overall_confidence=" << plate.bestPlate.overall_confidence
			  	  << ", MIN_REGION_CONFIDENCE=" << MIN_REGION_CONFIDENCE << endl;
    	  continue;
      }
      
      float score = (float) plate.regionConfidence;
      
      if (score_hash.count(plate.region) == 0)
      {
        score_hash[plate.region] = 0;
        score_count[plate.region] = 0;
      }

      score_hash[plate.region] = score_hash[plate.region] + score;
      score_count[plate.region] = score_count[plate.region] + 1;
    }
    
    float best_score = -1;
    std::string best_score_val = "";
    // Now we have a hash that contains all the scores.  Iterate and find the best one and return it.
    for(std::map<std::string, float >::iterator hash_iter=score_hash.begin(); hash_iter!=score_hash.end(); ++hash_iter) {
      if (hash_iter->second > best_score)
      {
        best_score = hash_iter->second;
        best_score_val = hash_iter->first;
      }
    }
    
    if (best_score > 0)
    {
      response.confidence = best_score / score_count[best_score_val];
      response.region = best_score_val;
    }
    
    return response;
  
  }

  
  // Searches all_plates to find overlapping plates
  // Returns an array containing "clusters" (overlapping plates)
  std::vector<std::vector<AlprPlateResult> > ResultAggregator::findClusters()
  {
    std::vector<std::vector<AlprPlateResult> > clusters;

    for (unsigned int i = 0; i < all_results.size(); i++)
    {
      for (unsigned int plate_id = 0; plate_id < all_results[i].results.plates.size(); plate_id++)
      {
        AlprPlateResult plate = all_results[i].results.plates[plate_id];

        int cluster_index = overlaps(plate, clusters);
        if (cluster_index < 0)
        {
          vector<AlprPlateResult> new_cluster;
          new_cluster.push_back(plate);
          clusters.push_back(new_cluster);
        }
        else
        {
          clusters[cluster_index].push_back(plate);
        }
      }
    }

    return clusters;
  }

  PlateShapeInfo ResultAggregator::getShapeInfo(AlprPlateResult plate)
  {
    int NUM_POINTS = 4;
    Moments mu;

    PlateShapeInfo response;

    vector<Point> points;
    for (int i = 0; i < NUM_POINTS; i++ )
    {
      cv::Point p(plate.plate_points[i].x, plate.plate_points[i].y);
      points.push_back(p);
    }

    mu = moments( points, false );
    response.center = cv::Point2f( mu.m10/mu.m00 , mu.m01/mu.m00 );
    response.area = mu.m00;

    Rect r = cv::boundingRect(points);
    response.max_width = r.width;
    response.max_height = r.height;

    return response;
  }

  // Returns the cluster ID if the plate overlaps.  Otherwise returns -1
  int ResultAggregator::overlaps(AlprPlateResult plate,
                                 std::vector<std::vector<AlprPlateResult> > clusters)
  {
    // Check the center positions to see how close they are to each other
    // Also compare the size.  If it's much much larger/smaller, treat it as a separate cluster

    PlateShapeInfo psi = getShapeInfo(plate);

    for (unsigned int i = 0; i < clusters.size(); i++)
    {
      for (unsigned int k = 0; k < clusters[i].size(); k++)
      {
        PlateShapeInfo cluster_shapeinfo = getShapeInfo(clusters[i][k]);

        int diffx = abs(psi.center.x - cluster_shapeinfo.center.x);
        int diffy = abs(psi.center.y - cluster_shapeinfo.center.y);

        // divide the larger plate area by the smaller plate area to determine a match
        float area_diff;
        if (psi.area > cluster_shapeinfo.area)
          area_diff = psi.area / cluster_shapeinfo.area;
        else
          area_diff = cluster_shapeinfo.area / psi.area;

        int max_x_diff = (psi.max_width + cluster_shapeinfo.max_width) / 2;
        int max_y_diff = (psi.max_height + cluster_shapeinfo.max_height) / 2;

        float max_area_diff = 4.0;
        // Consider it a match if center diffx/diffy are less than the average height
        // the area is not more than 4x different

        if (diffx <= max_x_diff && diffy <= max_y_diff && area_diff <= max_area_diff)
        {
          return i;
        }
      }

    }


    return -1;
  }
}
