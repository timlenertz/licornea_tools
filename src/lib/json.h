#ifndef LICORNEA_JSON_H_
#define LICORNEA_JSON_H_

#include <json.hpp>
#include <string>
#include "opencv.h"
#include "eigen.h"

namespace tlz {

using json = nlohmann::json;

void export_json_file(const json&, const std::string& filename);
json import_json_file(const std::string& filename);

cv::Mat_<double> decode_mat_cv(const json&);
Eigen::MatrixXd decode_mat(const json&);

json encode_mat(const cv::Mat_<double>&);

template<typename Scalar, std::size_t Rows, std::size_t Cols>
json encode_mat(const Eigen::Matrix<Scalar, Rows, Cols>& mat) {
	json j = json::array();
	for(int y = 0; y < Cols; ++y) {
		json j_row = json::array();
		for(int x = 0; x < Rows; ++x) j_row.push_back(mat(y, x));
		j.push_back(j_row);
	}
	return j;
}

}

#endif