#include "../lib/args.h"
#include "../lib/opencv.h"
#include "../lib/dataset.h"
#include "../lib/image_io.h"
#include "../lib/border.h"
#include <functional>

using namespace tlz;


std::function<void()> update_function;
void update_callback(int = 0, void* = nullptr) {	
	update_function();
}

int main(int argc, const char* argv[]) {
	get_args(argc, argv, "dataset_parameters.json [dataset_group]");
	dataset datas = dataset_arg();
	std::string dataset_group = string_opt_arg("");
			
	std::cout << "running viewer" << std::endl;

	const std::string window_name = "Dataset Viewer";
	
	view_index shown_idx(datas.x_mid(),datas.y_mid());
	int slider_x = datas.x_mid() - datas.x_min();
	int slider_y = datas.y_mid() - datas.y_min();

	auto update = [&]() {
		int nx = slider_x + datas.x_min();
		int ny = slider_y + datas.y_min();
		
		if(datas.x_valid(nx)) shown_idx.x = nx;
		if(datas.y_valid(ny)) shown_idx.y = ny;
				
		cv::Mat_<cv::Vec3b> img = load_texture(datas.view(shown_idx).group_view(dataset_group).image_filename());

		cv::Mat_<cv::Vec3b> shown_img = img;
	
		cv::imshow(window_name, shown_img);
	};
	update_function = update;

	cv::namedWindow(window_name, CV_WINDOW_NORMAL);

	cv::createTrackbar("x", window_name, &slider_x, datas.x_max() - datas.x_min(), &update_callback);
	if(datas.is_2d() && datas.y_count() > 1) cv::createTrackbar("y", window_name, &slider_y, datas.y_max() - datas.y_min(), &update_callback);

	update();
	
	while(cv::waitKey(0) != escape_keycode);
}