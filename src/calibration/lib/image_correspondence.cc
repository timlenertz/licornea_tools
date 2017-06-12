#include "image_correspondence.h"
#include "../../lib/string.h"
#include "../../lib/assert.h"
#include <fstream>

namespace tlz {

image_correspondence_feature decode_image_correspondence_feature(const json& j_feat) {
	image_correspondence_feature feat;
	
	if(has(j_feat, "reference_view"))
		feat.reference_view = decode_view_index(j_feat["reference_view"]);
	
	const json& j_pts = j_feat["points"];
	for(auto it = j_pts.begin(); it != j_pts.end(); ++it) {
		view_index idx = decode_view_index(it.key());
		feat.points[idx] = decode_feature_point(it.value());
	}

	return feat;
}


json encode_image_correspondence_feature(const image_correspondence_feature& feat) {
	json j_feat = json::object();
	if(feat.reference_view)
		j_feat["reference_view"] = encode_view_index(feat.reference_view);
		
	json j_points = json::object();
	for(const auto& kv : feat.points) {
		const view_index& idx = kv.first;
		const feature_point& pt = kv.second;
		j_points[encode_view_index(idx)] = encode_feature_point(pt);
	}
	j_feat["points"] = j_points;
	return j_feat;
}


image_correspondences decode_image_correspondences(const json& j_cors) {
	image_correspondences cors;
	if(has(j_cors, "dataset_group")) cors.dataset_group = j_cors["dataset_group"];
	const json& j_features = j_cors["features"];
	for(auto it = j_features.begin(); it != j_features.end(); ++it) {
		std::string feature_name = it.key();
		const json& j_feature = it.value();
		cors.features[feature_name] = decode_image_correspondence_feature(j_feature);
	}
	return cors;
}


json encode_image_correspondences(const image_correspondences& cors) {
	json j_cors = json::object();
	if(! cors.dataset_group.empty()) j_cors["dataset_group"] = cors.dataset_group;
	json j_features = json::object();
	for(const auto& kv : cors.features) {
		const std::string& feature_name = kv.first;
		const image_correspondence_feature& feature = kv.second;
		j_features[feature_name] = encode_image_correspondence_feature(feature);
	}
	j_cors["features"] = j_features;
	return j_cors;
}


void export_binary_image_correspondences(const image_correspondences& cors, const std::string& filename) {
	std::ofstream str(filename, std::ios_base::binary);
	
	auto write = [&str](const auto& val) {
		const auto* ptr = reinterpret_cast<const std::ostream::char_type*>(&val);
		str.write(ptr, sizeof(val));
	};
	auto write_buffer = [&str](const void* buf, std::size_t sz) {
		str.write(static_cast<const std::ostream::char_type*>(buf), sz);
	};
	
	auto uint8 = [&write](std::uint8_t val) { write(val); };
	auto int32 = [&write](std::int32_t val) { write(val); };
	auto float64 = [&write](double val) { write(val); };
	
	auto short_string = [&](const std::string& str) {
		Assert(str.length() <= 255);
		uint8(str.length());
		write_buffer(str.data(), str.length());
	};
	auto view_idx = [&](const view_index& idx) {
		int32(idx.x);
		int32(idx.y);
	};
	
	int32(0x2D1111C0);
	short_string(cors.dataset_group);
	int32(cors.features.size());
	for(const auto& kv : cors.features) {
		const std::string& feature_name = kv.first;
		const image_correspondence_feature& feature = kv.second;
		short_string(feature_name);
		view_idx(feature.reference_view);
		int32(feature.points.size());
		for(const auto& kv2 : feature.points) {
			const view_index& idx = kv2.first;
			const feature_point& fpoint = kv2.second;
			view_idx(idx);
			float64(fpoint.position[0]);
			float64(fpoint.position[1]);
			float64(fpoint.depth);
			float64(fpoint.weight);
		}
	}
}


image_correspondences import_binary_image_correspondences(const std::string& filename) {
	std::ifstream str(filename, std::ios_base::binary);
	
	auto read = [&str](auto& val) {
		auto* ptr = reinterpret_cast<std::ostream::char_type*>(&val);
		str.read(ptr, sizeof(val));
	};
	auto read_buffer = [&str](void* buf, std::size_t sz) {
		str.read(static_cast<std::ostream::char_type*>(buf), sz);
	};
	
	auto uint8 = [&read]() { std::uint8_t val; read(val); return val; };
	auto int32 = [&read]() { std::int32_t val; read(val); return val; };
	auto float64 = [&read]() { double val; read(val); return val; };

	auto short_string = [&]() {
		char string[255];
		std::size_t sz = uint8();
		read_buffer(&string, sz);
		return std::string(string, sz);
	};
	auto view_idx = [&]() {
		int x = int32();
		int y = int32();
		return view_index(x, y);
	};
	
	image_correspondences cors;
	int magic = int32();
	if(magic != 0x2D1111C0) throw std::runtime_error("binary cors file does not have magic number");
	cors.dataset_group = short_string();
	std::size_t features_count = int32();
	for(int i = 0; i < features_count; ++i) {
		std::string feature_name = short_string();
		image_correspondence_feature& feature = cors.features[feature_name];
		feature.reference_view = view_idx();
		
		std::size_t points_count = int32();
		for(int j = 0; j < points_count; ++j) {
			view_index idx = view_idx();
			feature_point& fpoint = feature.points[idx];
			fpoint.position[0] = float64();
			fpoint.position[1] = float64();
			fpoint.depth = float64();
			fpoint.weight = float64();
		}
	}
	
	return cors;
}


std::set<view_index> reference_views(const image_correspondences& cors) {
	std::set<view_index> reference_views;
	for(const auto& kv : cors.features) reference_views.insert(kv.second.reference_view);
	return reference_views;
}


std::set<view_index> all_views(const image_correspondences& cors) {
	std::set<view_index> views;
	for(const auto& kv : cors.features)
		for(const auto& kv2 : kv.second.points)
			views.insert(kv2.first);
	return views;
}


std::set<std::string> feature_names(const image_correspondences& cors) {
	std::set<std::string> features;
	for(const auto& kv : cors.features)
		features.insert(kv.first);
	return features;
}


image_correspondences image_correspondences_with_reference(const image_correspondences& cors, const view_index& reference_view) {
	image_correspondences out_cors;
	out_cors.dataset_group = cors.dataset_group;
	for(const auto& kv : cors.features) {
		const image_correspondence_feature& feature = kv.second;
		if(feature.reference_view == reference_view)
			out_cors.features[kv.first] = feature;
	}
	return out_cors;
}


image_correspondences undistort(const image_correspondences& cors, const intrinsics& intr) {
	if(intr.distortion.is_none()) return cors;
	
	std::vector<vec2> dist_points;
	std::vector<vec2*> undist_points_ptrs;
	
	image_correspondences out_cors = cors;
	
	for(const auto& kv : cors.features) {
		const std::string& feature_name = kv.first;
		const image_correspondence_feature& feature = kv.second;
		image_correspondence_feature& out_feature = out_cors.features.at(feature_name);
		for(const auto& kv2 : feature.points) {
			const view_index& idx = kv2.first;
			const feature_point& pt = kv2.second;
			feature_point& out_pt = out_feature.points.at(idx);
			dist_points.push_back(pt.position);
			undist_points_ptrs.push_back(&out_pt.position);
		}
	}
	
	std::vector<vec2> undist_points = undistort_points(intr, dist_points);
	
	for(std::ptrdiff_t i = 0; i < undist_points.size(); ++i)
		*undist_points_ptrs[i] = undist_points[i];
	
	return out_cors;
}


cv::Mat_<cv::Vec3b> visualize_view_points(const image_correspondence_feature& feature, const cv::Mat_<cv::Vec3b>& back_img, const cv::Vec3b& col, int dot_radius, const border& bord) {
	cv::Mat_<cv::Vec3b> img;
	back_img.copyTo(img);

	bool is_2d = feature.points.begin()->first.is_2d();
	if(! is_2d) {
		// draw circle		
		vec2 center_point = feature.points.at(feature.reference_view).position;
		cv::Point center_point_cv(bord.left + center_point[0], bord.top + center_point[1]);
		cv::circle(img, center_point_cv, 10, cv::Scalar(col), 2);
		
		// draw connecting line
		std::vector<cv::Point> trail_points;
		for(const auto& kv : feature.points) {
			vec2 pt = kv.second.position;
			pt[0] += bord.left; pt[1] += bord.top;
			trail_points.emplace_back(pt[0], pt[1]);
		}

		std::vector<std::vector<cv::Point>> polylines = { trail_points };
		cv::polylines(img, polylines, false, cv::Scalar(col), 2);
		
	} else {
		// draw dot for each point
		for(const auto& kv : feature.points) {
			vec2 pt = kv.second.position;
			pt[0] += bord.left; pt[1] += bord.top;
			
			cv::Point pt_cv(pt[0], pt[1]);
			if(dot_radius <= 1) {
				if(cv::Rect(cv::Point(), img.size()).contains(pt_cv)) img(pt_cv) = col;
			} else {
				cv::circle(img, pt_cv, 2, cv::Scalar(col), -1);
			}
		}
	}

	return img;
}



image_correspondences image_correspondences_arg() {
	std::cout << "loading image correspondences" << std::endl;
	return decode_image_correspondences(json_arg());
}



}
