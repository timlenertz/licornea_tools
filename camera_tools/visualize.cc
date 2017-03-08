#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "lib/eigen.h"
#include "lib/camera.h"

[[noreturn]] void usage_fail() {
	std::cout << "usage: visualize in_cameras.json out_view.ply\n";
	std::cout << std::endl;
	std::exit(1);
}

int main(int argc, const char* argv[]) {
	if(argc <= 2) usage_fail();
	std::string in_cameras = argv[1];
	std::string out_ply = argv[2];
	
	std::ifstream input(in_cameras.c_str());
	input.exceptions(std::ios_base::badbit);	
	
	auto cameras = read_cameras_file(in_cameras);
	if(cameras.size() == 0) {
		std::cout << "no cameras in file" << std::endl;
		usage_fail();
	} else {
		std::cout << cameras.size() << " cameras..." << std::endl;
	}
			
	double scale = 1.0;
	Eigen_mat4 transform; transform <<
		1.0*scale, 0.0, 0.0, 0.0,
		0.0, 1.0*scale, 0.0, 0.0,
		0.0, 0.0, 2.0*scale, 0.0,
		0.0, 0.0, 0.0, 1.0;
	
	constexpr std::size_t vertex_count = 5;
	constexpr std::size_t face_count = 6;
	Eigen_vec3 vertices[vertex_count] = {
		Eigen_vec3(0, 0, 0),    // 0: base
		Eigen_vec3(+1, -1, 1),  // 1: top left
		Eigen_vec3(+1, +1, 1),  // 2: top right
		Eigen_vec3(-1, +1, 1),  // 3: bottom left
		Eigen_vec3(-1, -1, 1)   // 4: bottom right
	};
	std::ptrdiff_t faces[face_count][3] = {
		{0, 2, 1}, // top
		{0, 3, 2}, // right
		{0, 4, 3}, // bottom,
		{0, 1, 4}, // left,
		{1, 2, 4}, // back top left
		{3, 4, 2}, // back bottom right
	};
	
	std::ofstream output(out_ply.c_str());
	output << "ply\n";
	output << "format ascii 1.0\n";
	output << "element vertex " << cameras.size() * vertex_count << '\n';
	output << "property float x\n";
	output << "property float y\n";
	output << "property float z\n";
	output << "element face " << cameras.size() * face_count << '\n';
	output << "property list uchar int vertex_indices\n";
	output << "end_header\n";
	
	for(const camera& cam : cameras) {		
		Eigen_mat4 backproj = (transform.inverse() * cam.extrinsic).inverse();
		
		for(const Eigen_vec3& model_vertex : vertices) {
			Eigen_vec3 world_vertex = (backproj * model_vertex.homogeneous()).eval().hnormalized();
			output << world_vertex[0] << ' ' << world_vertex[1] << ' ' << world_vertex[2] << '\n';
		}
	}
	
	int idx = 0;
	for(std::ptrdiff_t camera_index = 0; camera_index < cameras.size(); ++camera_index) {
		for(const std::ptrdiff_t* face : faces) {
			output << "3 " << (idx + face[0]) << ' ' << (idx + face[1]) << ' ' << (idx + face[2]) << '\n';
		}
		idx += vertex_count;
	}
	
	
	std::cout << "done" << std::endl;
}
