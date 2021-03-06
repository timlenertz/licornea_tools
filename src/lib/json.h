#ifndef LICORNEA_JSON_H_
#define LICORNEA_JSON_H_

#include <json.hpp>
#include <string>
#include "common.h"
#include "args.h"

namespace tlz {

using json = nlohmann::json;

void export_json_file(const json&, const std::string& filename, bool compact = false);
json import_json_file(const std::string& filename);

cv::Mat_<real> decode_mat(const json&);

json encode_mat_(const cv::Mat_<real>&);
template<typename Mat> json encode_mat(const Mat& mat) {
	return encode_mat_(cv::Mat_<real>(mat));
}


inline bool has(const json& j, const std::string& key) {
	return (j.count(key) == 1);
}
template<typename T>
T get_or(const json& j, const std::string& key, const T& default_value) {
	if(has(j, key)) return j[key];
	else return default_value;
}


inline json json_arg()
	{ return import_json_file(in_filename_arg()); }

}

#endif
