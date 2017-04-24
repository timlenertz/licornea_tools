#include "image_io.h"

namespace tlz {

cv::Mat_<ushort> load_depth(const std::string& filename) {
	cv::Mat mat = cv::imread(filename, CV_LOAD_IMAGE_ANYDEPTH);
	if(mat.depth() != CV_16U) throw std::runtime_error("input depth map: must be 16 bit");
	cv::Mat_<ushort> mat_ = mat;
	return mat_;
}

void save_depth(const std::string& filename, const cv::Mat_<ushort>& depth) {
	std::vector<int> params = { CV_IMWRITE_PNG_COMPRESSION, 0 };
	cv::imwrite(filename, depth, params);
}

cv::Mat_<cv::Vec3b> load_texture(const std::string& filename) {
	cv::Mat mat = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
	cv::Mat_<cv::Vec3b> mat_ = mat;
	return mat_;
}

void save_texture(const std::string& filename, const cv::Mat_<cv::Vec3b>& texture) {
	std::vector<int> params = { CV_IMWRITE_PNG_COMPRESSION, 0 };
	cv::imwrite(filename, texture, params);
}

}