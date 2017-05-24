#include <opencv2/opencv.hpp>
#include "../lib/point.h"
#include "../lib/ply_exporter.h"
#include "../lib/image_io.h"
#include "lib/kinect_reprojection.h"
#include "lib/densify/depth_densify.h"
#include "lib/kinect_internal_parameters.h"
#include "lib/common.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <string>

using namespace tlz;


void do_depth_reprojection(const cv::Mat_<ushort>& in, cv::Mat_<ushort>& out, cv::Mat_<uchar>& out_mask, const kinect_reprojection& reproj, const std::string& method) {	
	std::vector<vec3> samples;
	for(int dy = 0; dy < depth_height; ++dy) for(int dx = 0; dx < depth_width; ++dx) {
		ushort dz = in(dy, dx);
		if(dz == 0) continue;
		
		vec2 distorted_depth_position(dx, dy);
		vec2 undistorted_depth_position = reproj.undistort_ir(distorted_depth_position); // TODO improve
		
		real cz;
		vec2 color_position = reproj.reproject_ir_to_color(undistorted_depth_position, dz, cz);
									
		samples.emplace_back(color_position[0], color_position[1], cz);
	}
	
	cv::Mat_<real> densify_out(texture_height, texture_width);
	make_depth_densify(method)->densify(samples, densify_out, out_mask);
	out = densify_out;
}


int main(int argc, const char* argv[]) {
	if(argc <= 5) {
		std::cout << "usage: " << argv[0] << " input.png output.png output_mask.png reprojection_parameters.json method" << std::endl;
		return EXIT_FAILURE;
	}
	std::string input_filename = argv[1];
	std::string output_filename = argv[2];
	std::string output_mask_filename = argv[3];
	std::string reprojection_parameters_filename = argv[5];
	std::string method = argv[5];
	
	std::cout << "reading parameters" << std::endl;
	kinect_reprojection_parameters reprojection_parameters = decode_kinect_reprojection_parameters(import_json_file(reprojection_parameters_filename));
	kinect_reprojection reproj(internal_parameters, reprojection_parameters);
	
	std::cout << "reading input depth map" << std::endl;
	cv::Mat_<ushort> in_depth = load_depth(input_filename.c_str());
	cv::flip(in_depth, in_depth, 1);
		
	std::cout << "doing depth densification" << std::endl;
	cv::Mat_<ushort> out_depth(texture_height, texture_width);
	cv::Mat_<uchar> out_mask(texture_height, texture_width);
	do_depth_reprojection(in_depth, out_depth, out_mask, reproj, method);
	
	std::cout << "saving output depth map+mask" << std::endl;
	cv::flip(out_depth, out_depth, 1);
	cv::flip(out_mask, out_mask, 1);
	if(output_filename != "-") save_depth(output_filename.c_str(), out_depth);
	if(output_mask_filename != "-") cv::imwrite(output_mask_filename.c_str(), out_mask);
	
	std::cout << "done" << std::endl;
}
