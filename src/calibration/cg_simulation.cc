#include <string>
#include <map>
#include <cmath>
#include <random>
#include "lib/image_correspondence.h"
#include "../lib/utility/misc.h"
#include "../lib/json.h"
#include "../lib/dataset.h"
#include "../lib/opencv.h"
#include "../lib/camera.h"
#include "../lib/image_io.h"
#include "../lib/random_color.h"

using namespace tlz;

[[noreturn]] void usage_fail() {
	std::cout << "usage: cg_simulation rotation.json intrinsics.json out_image_correspondences.json out_datas_dir/ [features_count=100] [num_x=30] [num_y=10] [step_x=1.0] [step_y=1.0]\n";
	std::cout << std::endl;
	std::exit(1);
}
int main(int argc, const char* argv[]) {
	if(argc <= 4) usage_fail();
	std::string rotation_filename = argv[1];
	std::string intrinsics_filename = argv[2];
	std::string out_cors_filename = argv[3];
	std::string out_datas_dir = argv[4];
	int features_count = 100;
	int num_x = 30;
	int num_y = 10;
	real step_x = 1;
	real step_y = 1;
	int i = 4;
	if(argc > ++i) features_count = std::atoi(argv[i]);
	if(argc > ++i) num_x = std::atoi(argv[i]);
	if(argc > ++i) num_y = std::atoi(argv[i]);
	if(argc > ++i) step_x = std::atof(argv[i]);
	if(argc > ++i) step_y = std::atof(argv[i]);
	
	mat33 R = decode_mat(import_json_file(rotation_filename));
	
	json j_intrinsics = import_json_file(intrinsics_filename);
	mat33 K = decode_mat(j_intrinsics["K"]);
	real fx = K(0, 0), fy = K(1, 1), cx = K(0, 2), cy = K(1, 2);	
	
	int width = j_intrinsics["width"];
	int height = j_intrinsics["height"];
	
	std::cout << "generating " << features_count << " random 3D features" << std::endl;
	std::vector<vec3> features;
	{
		std::uniform_real_distribution<real> ix_dist(0, width);
		std::uniform_real_distribution<real> iy_dist(0, height);
		std::uniform_real_distribution<real> vz_dist(500.0, 2000.0);	
		std::mt19937 gen;

		for(int i = 0; i < features_count; ++i) {
			real ix = ix_dist(gen);
			real iy = iy_dist(gen);
			real vz = vz_dist(gen);
			
			real vx = (ix - cx)*vz/fx;
			real vy = (iy - cy)*vz/fy;
			
			vec3 feature(vx, vy, vz);
			features.push_back(feature);
		}
	}
	
	
	std::cout << "generating dataset parameters" << std::endl;
	{
		json j_dataset = json::object();
		j_dataset["x_index_range"] = json::array({0, num_x-1});
		j_dataset["y_index_range"] = json::array({0, num_y-1});
		j_dataset["width"] = width;
		j_dataset["height"] = height;
		j_dataset["image_filename_format"] = "image_y{y}_x{x}.png";
		j_dataset["depth_filename_format"] = "depth_y{y}_x{x}.png";
		j_dataset["cameras_filename"] = "cameras.json";
		j_dataset["camera_name_format"] = "camera_y{y}_x{x}";
		export_json_file(j_dataset, out_datas_dir + "/parameters.json");
	}
	
	
	std::cout << "projecting features for each view (-> correspondences, image, depth)" << std::endl;
	image_correspondences cors;
	cors.reference = view_index(num_x/2, num_y/2);
	std::map<view_index, vec3> view_camera_centers;
	for(int y = 0; y < num_y; ++y) for(int x = 0; x < num_x; ++x) {
		std::cout << '.' << std::flush;
		
		view_index idx(x, y);
		
		// camera center position
		real px = step_x*(x - num_x/2);
		real py = step_y*(y - num_y/2);
		vec3 p(px, py, 0.0);
		view_camera_centers[idx] = p;
				
		// images
		cv::Mat_<cv::Vec3b> texture_image(height, width);
		texture_image.setTo(cv::Vec3b(0, 0, 0));
		
		cv::Mat_<ushort> depth_image(height, width);
		depth_image.setTo(0);
		
		const int blob_radius = 4;
		
		for(int feature = 0; feature < features_count; ++feature) {
			// feature image position in this view
			const vec3& w = features[feature];
			vec3 v = R*(w + p);
			vec3 i_ = K*v;
			vec2 i(i_[0]/i_[2], i_[1]/i_[2]);
			cv::Point i_pt(i[0], i[1]);
		
			// add image correspondence
			std::string feature_name = "feat" + std::to_string(feature);
			cors.features[feature_name].points[idx] = i;
			cors.features[feature_name].depth = v[2];

			cv::Vec3b col = random_color(feature);

			// draw blob in texture image
			cv::circle(texture_image, i_pt, blob_radius, cv::Scalar(col), -1);
			
			// draw blob in depth image
			ushort depth = v[2];
			cv::circle(depth_image, i_pt, blob_radius, depth, -1);
		}
		
		// save images
		std::string texture_image_filename = out_datas_dir + "/image_y" + std::to_string(idx.y) + "_x" + std::to_string(idx.x) + ".png";
		std::string depth_image_filename = out_datas_dir + "/depth_y" + std::to_string(idx.y) + "_x" + std::to_string(idx.x) + ".png";
		save_texture(texture_image_filename, texture_image);
		save_depth(depth_image_filename, depth_image);
	}
	std::cout << std::endl;
	
	std::cout << "saving correspondences" << std::endl;
	export_image_correspondences_file(out_cors_filename, cors);
	
	
	std::cout << "saving cameras" << std::endl;
	camera_array cams;
	for(const auto& kv : view_camera_centers) {
		view_index idx = kv.first;
		const vec3& p = kv.second;
		vec3 t = R * p;
			
		camera cam;
		cam.name = "camera_y" + std::to_string(idx.y) + "_x" + std::to_string(idx.x);
		cam.intrinsic = K;
		cam.rotation = R;
		cam.translation = t;
		cams.push_back(cam);
	}
	std::string cameras_filename = out_datas_dir + "/cameras.json";
	write_cameras_file(cameras_filename, cams);
	
	
	std::cout << "done" << std::endl;
}
