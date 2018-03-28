#include "BuildingSimplification.h"
#include "DPSimplification.h"
#include "../util/ContourUtils.h"
#include "DPSimplification.h"
#include "RightAngleSimplification.h"
#include "CurveSimplification.h"
#include "CurveRightAngleSimplification.h"

namespace simp {

	std::vector<std::shared_ptr<util::BuildingLayer>> BuildingSimplification::simplifyBuildings(const std::vector<std::shared_ptr<util::BuildingLayer>>& raw_buildings, int algorithm, bool record_stats, float alpha, float layering_threshold, float epsilon, int resolution, float curve_threshold, float angle_threshold) {
		std::vector<std::shared_ptr<util::BuildingLayer>> buildings;

		std::vector<std::tuple<float, long long, int>> records;

		time_t start = clock();
		setbuf(stdout, NULL);
		printf("Processing building");
		for (int i = 0; i < raw_buildings.size(); i++) {
			printf("\rProcessing building %d  ", i);

			try {
				
				std::shared_ptr<util::BuildingLayer> building;
				if (algorithm == ALG_ALL) {
					float angle = -1;
					int dx = -1;
					int dy = -1;
					building = simplifyBuildingByAll(i, util::DisjointVoxelData::layering(raw_buildings[i], layering_threshold), alpha, angle, dx, dy, records);
				}
				else if (algorithm == ALG_DP) {
					cv::Point2f hoge = raw_buildings[i]->footprint.contour.getActualPoint(0);
					building = simplifyBuildingByDP(i, util::DisjointVoxelData::layering(raw_buildings[i], layering_threshold), alpha, epsilon);
				}
				else if (algorithm == ALG_RIGHTANGLE) {
					float angle = -1;
					int dx = -1;
					int dy = -1;
					building = simplifyBuildingByRightAngle(i, util::DisjointVoxelData::layering(raw_buildings[i], layering_threshold), alpha, resolution, angle, dx, dy);
				}
				else if (algorithm == ALG_CURVE) {
					building = simplifyBuildingByCurve(i, util::DisjointVoxelData::layering(raw_buildings[i], layering_threshold), alpha, epsilon, curve_threshold);
				}
				else if (algorithm == ALG_CURVE_RIGHTANGLE) {
					building = simplifyBuildingByCurveRightAngle(i, util::DisjointVoxelData::layering(raw_buildings[i], layering_threshold), alpha, epsilon, curve_threshold, angle_threshold);
				}

				buildings.push_back(building);
			}
			catch (...) {}
		}
		printf("\n");
		time_t end = clock();
		std::cout << "Time elapsed " << (double)(end - start) / CLOCKS_PER_SEC << " sec." << std::endl;

		if (record_stats) {
			std::ofstream out("records.txt");
			for (int i = 0; i < records.size(); i++) {
				float error = std::get<0>(records[i]);
				long long num_primitive_shapes = std::get<1>(records[i]);
				int selected_algorithm = std::get<2>(records[i]);
				out << error << " " << num_primitive_shapes << " " << selected_algorithm << std::endl;
			}
			out.close();
		}

		return buildings;
	}

	std::vector<float> BuildingSimplification::sumCost(const std::vector<std::shared_ptr<util::BuildingLayer>>& buildings) {
		std::vector<float> costs(3, 0);

		for (int i = 0; i < buildings.size(); i++) {
			std::vector<float> c = sumCost(buildings[i]->children);
			
			for (int j = 0; j < c.size(); j++) {
				costs[j] += buildings[i]->costs[j] + c[j];
			}
		}

		return costs;
	}

	std::shared_ptr<util::BuildingLayer> BuildingSimplification::simplifyBuildingByAll(int building_id, std::shared_ptr<util::BuildingLayer> layer, float alpha, float angle, int dx, int dy, std::vector<std::tuple<float, long long, int>>& records) {
		float best_cost = std::numeric_limits<float>::max();
		util::Polygon best_simplified_polygon;

		// get baseline cost
		util::Polygon baseline_polygon = DPSimplification::simplify(layer->selectRepresentativeContour(), 0.5);
		std::vector<float> baseline_costs = calculateCost(baseline_polygon, layer);
		
		int best_algorithm = ALG_UNKNOWN;
		float best_error = 0.0f;
		int best_num_primitive_shapes = 0;

		// try Douglas-Peucker
		try {
			float epsilon;
			if (alpha == 0.0) epsilon = 10;
			else if (alpha < 0.2) epsilon = 6;
			else if (alpha < 0.4) epsilon = 4;
			else if (alpha < 0.9) epsilon = 2;
			else epsilon = 0;

			util::Polygon simplified_polygon = DPSimplification::simplify(layer->selectRepresentativeContour(), epsilon);
			std::vector<float> costs = calculateCost(simplified_polygon, layer);
			float cost = alpha * costs[0] / costs[1] + (1 - alpha) * costs[2] / baseline_costs[2];

			if (cost < best_cost) {
				best_algorithm = ALG_DP;
				best_cost = cost;
				best_simplified_polygon = simplified_polygon;

				best_error = costs[0] / costs[1];
				best_num_primitive_shapes = costs[2];
			}
		}
		catch (...) {
		}

		// try right angle
		try {
			int resolution;
			if (alpha == 0.0) resolution = 6;
			else if (alpha < 1.0) resolution = 4;
			else resolution = 1;

			util::Polygon simplified_polygon = RightAngleSimplification::simplify(layer->selectRepresentativeContour(), resolution, angle, dx, dy);
			std::vector<float> costs = calculateCost(simplified_polygon, layer);
			float cost = alpha * costs[0] / costs[1] + (1 - alpha) * costs[2] / baseline_costs[2];
			if (cost < best_cost) {
				best_algorithm = ALG_RIGHTANGLE;
				best_cost = cost;
				best_simplified_polygon = simplified_polygon;

				best_error = costs[0] / costs[1];
				best_num_primitive_shapes = costs[2];
			}
		}
		catch (...) {
		}

		// try curve
		try {
			float epsilon;
			if (alpha == 0.0) epsilon = 10;
			else if (alpha < 0.2) epsilon = 6;
			else if (alpha < 0.4) epsilon = 4;
			else if (alpha < 0.9) epsilon = 2;
			else epsilon = 0;

			float curve_threshold;
			if (alpha == 0.0) curve_threshold = 4.0f;
			else curve_threshold = 1.0f;

			util::Polygon simplified_polygon = CurveSimplification::simplify(layer->selectRepresentativeContour(), epsilon, curve_threshold);
			std::vector<float> costs = calculateCost(simplified_polygon, layer);
			float cost = alpha * costs[0] / costs[1] + (1 - alpha) * costs[2] / baseline_costs[2];

			if (cost < best_cost) {
				best_algorithm = ALG_CURVE;
				best_cost = cost;
				best_simplified_polygon = simplified_polygon;

				best_error = costs[0] / costs[1];
				best_num_primitive_shapes = costs[2];
			}
		}
		catch (...) {
		}

		// try curve + right angle
		try {
			float epsilon;
			if (alpha == 0.0) epsilon = 10;
			else if (alpha < 0.2) epsilon = 6;
			else if (alpha < 0.4) epsilon = 4;
			else if (alpha < 0.9) epsilon = 2;
			else epsilon = 0;

			float curve_threshold;
			if (alpha == 0.0) curve_threshold = 4.0f;
			else curve_threshold = 1.0f;

			float angle_threshold = 15.0f / 180.0f * CV_PI;

			util::Polygon simplified_polygon = CurveRightAngleSimplification::simplify(layer->selectRepresentativeContour(), epsilon, curve_threshold, angle_threshold);
			std::vector<float> costs = calculateCost(simplified_polygon, layer);
			float cost = alpha * costs[0] / costs[1] + (1 - alpha) * costs[2] / baseline_costs[2];

			if (cost < best_cost) {
				best_algorithm = ALG_CURVE_RIGHTANGLE;
				best_cost = cost;
				best_simplified_polygon = simplified_polygon;

				best_error = costs[0] / costs[1];
				best_num_primitive_shapes = costs[2];
			}
		}
		catch (...) {
		}

		if (best_algorithm == ALG_UNKNOWN) throw "No valid simplification is found.";

		records.push_back(std::make_tuple(best_error, best_num_primitive_shapes, best_algorithm));

		std::shared_ptr<util::BuildingLayer> building = std::shared_ptr<util::BuildingLayer>(new util::BuildingLayer(building_id, best_simplified_polygon, layer->bottom_height, layer->top_height));

		for (int i = 0; i < layer->children.size(); i++) {
			try {
				if (best_algorithm != ALG_RIGHTANGLE) {
					angle = -1;
					dx = -1;
					dy = -1;
				}
				std::shared_ptr<util::BuildingLayer> child = simplifyBuildingByAll(building_id, layer->children[i], alpha, angle, dx, dy, records);
				building->children.push_back(child);
			}
			catch (...) {
			}
		}

		return building;
	}

	/**
	 * Simplify the shape of a building.
	 *
	 * @param layer		layer
	 * @param epsilon	simplification level
	 * @return			simplified building shape
	 */
	std::shared_ptr<util::BuildingLayer> BuildingSimplification::simplifyBuildingByDP(int building_id, std::shared_ptr<util::BuildingLayer> layer, float alpha, float epsilon) {
		util::Polygon simplified_polygon = DPSimplification::simplify(layer->selectRepresentativeContour(), epsilon);

		// calculate cost
		std::vector<float> costs = calculateCost(simplified_polygon, layer);

		std::shared_ptr<util::BuildingLayer> building = std::shared_ptr<util::BuildingLayer>(new util::BuildingLayer(building_id, simplified_polygon, layer->bottom_height, layer->top_height));
		building->costs = costs;

		for (int i = 0; i < layer->children.size(); i++) {
			try {
				std::shared_ptr<util::BuildingLayer> child = simplifyBuildingByDP(building_id, layer->children[i], alpha, epsilon);
				DPSimplification::decomposePolygon(child->footprint);
				building->children.push_back(child);
			}
			catch (...) {
			}
		}

		return building;
	}
	
	/**
	 * Simplify the shape of a building.
	 *
	 * @param layer			layer
	 * @param resolution	simplification level
	 * @return				simplified building shape
	 */
	std::shared_ptr<util::BuildingLayer> BuildingSimplification::simplifyBuildingByRightAngle(int building_id, std::shared_ptr<util::BuildingLayer> layer, float alpha, int resolution, float angle, int dx, int dy) {
		util::Polygon simplified_polygon = RightAngleSimplification::simplify(layer->selectRepresentativeContour(), resolution, angle, dx, dy);

		// calculate cost
		std::vector<float> costs = calculateCost(simplified_polygon, layer);

		std::shared_ptr<util::BuildingLayer> building = std::shared_ptr<util::BuildingLayer>(new util::BuildingLayer(building_id, simplified_polygon, layer->bottom_height, layer->top_height));
		building->costs = costs;

		for (int i = 0; i < layer->children.size(); i++) {
			try {
				std::shared_ptr<util::BuildingLayer> child = simplifyBuildingByRightAngle(building_id, layer->children[i], alpha, resolution, angle, dx, dy);
				RightAngleSimplification::decomposePolygon(child->footprint);
				building->children.push_back(child);
			}
			catch (...) {
			}
		}

		return building;
	}

	std::shared_ptr<util::BuildingLayer> BuildingSimplification::simplifyBuildingByCurve(int building_id, std::shared_ptr<util::BuildingLayer> layer, float alpha, float epsilon, float curve_threshold) {
		util::Polygon simplified_polygon = CurveSimplification::simplify(layer->selectRepresentativeContour(), epsilon, curve_threshold);

		// calculate cost
		std::vector<float> costs = calculateCost(simplified_polygon, layer);

		std::shared_ptr<util::BuildingLayer> building = std::shared_ptr<util::BuildingLayer>(new util::BuildingLayer(building_id, simplified_polygon, layer->bottom_height, layer->top_height));
		building->costs = costs;

		for (int i = 0; i < layer->children.size(); i++) {
			try {
				std::shared_ptr<util::BuildingLayer> child = simplifyBuildingByCurve(building_id, layer->children[i], alpha, epsilon, curve_threshold);
				building->children.push_back(child);
			}
			catch (...) {
			}
		}

		return building;
	}

	std::shared_ptr<util::BuildingLayer> BuildingSimplification::simplifyBuildingByCurveRightAngle(int building_id, std::shared_ptr<util::BuildingLayer> layer, float alpha, float epsilon, float curve_threshold, float angle_threshold) {
		util::Polygon simplified_polygon = CurveRightAngleSimplification::simplify(layer->selectRepresentativeContour(), epsilon, curve_threshold, angle_threshold);

		// calculate cost
		std::vector<float> costs = calculateCost(simplified_polygon, layer);

		std::shared_ptr<util::BuildingLayer> building = std::shared_ptr<util::BuildingLayer>(new util::BuildingLayer(building_id, simplified_polygon, layer->bottom_height, layer->top_height));
		building->costs = costs;

		for (int i = 0; i < layer->children.size(); i++) {
			try {
				std::shared_ptr<util::BuildingLayer> child = simplifyBuildingByCurveRightAngle(building_id, layer->children[i], alpha, epsilon, curve_threshold, angle_threshold);
				building->children.push_back(child);
			}
			catch (...) {
			}
		}

		return building;
	}

	/**
	 * Calculate cost for the layer/
	 * 
	 * @param size					XY dimension of the voxel data
	 * @param simplified_polygon	the simplified polygon for which we calculate the cost
	 * @param layer					layer for which we calculate the cost
	 * @param alpha					weight ratio of the error term to the simplicity term in the cost function
	 * @return						three values, (1-IOU) * area, area, and #primitive shapes
	 */
	std::vector<float> BuildingSimplification::calculateCost(const util::Polygon& simplified_polygon, std::shared_ptr<util::BuildingLayer> layer) {
		std::vector<float> ans(3, 0);

		// calculate IOU
		for (int i = 0; i < layer->raw_footprints.size(); i++) {
			float slice_area = util::calculateArea(layer->raw_footprints[i]);

			float iou = util::calculateIOU(simplified_polygon, layer->raw_footprints[i]);
			ans[0] += (1 - iou) * slice_area;
			ans[1] += slice_area;
		}

		ans[2] = simplified_polygon.primitive_shapes.size();
		
		return ans;
	}

}