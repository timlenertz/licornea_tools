#!/usr/local/bin/python
from pylib import *
import string, sys

class FormatPlaceholder:
    def __init__(self, key):
        self.key = key
    def __format__(self, spec):
    	result = self.key
    	if spec:
    		result += ":" + spec
    	return "{" + result + "}"
 
class FormatDict(dict):
    def __missing__(self, key):
        return FormatPlaceholder(key)

def filter_name_template(tpl, y):
	formatter = string.Formatter()
	mapping = FormatDict(y=y)
	return formatter.vformat(tpl, (), mapping)
	
def process_clause(name, datas):
	view = datas.view(datas.x_min(), y).local_view(name)
	par = view.local_parameters()
	local_y = view.local_view(name).local_y()
	
	if "y_index_factor" in par: del par["y_index_factor"]
	if "y_index_offset" in par: del par["y_index_offset"]
	if "image_filename_format" in par: par["image_filename_format"] = filter_name_template(par["image_filename_format"], local_y)
	if "depth_filename_format" in par: par["depth_filename_format"] = filter_name_template(par["depth_filename_format"], local_y)
		
if __name__ == '__main__':
	def usage_fail():
		print("usage: {} in_parameters.json y out_parameters.json\n".format(sys.argv[0]))
		sys.exit(1)

	if len(sys.argv) <= 3: usage_fail()
	in_parameters_filename = sys.argv[1]
	y = int(sys.argv[2])
	out_parameters_filename = sys.argv[3]
	
	datas = Dataset(in_parameters_filename)
	if not datas.is_2d(): raise Exception("dataset must be 2d")
	if not datas.y_valid(y): raise Exception("y slice out of range")

	new_parameters = datas.parameters.copy()
	del new_parameters["y_index_range"]
	
	if "camera_name_format" in new_parameters:
		new_parameters["camera_name_format"] = filter_name_template(new_parameters["camera_name_format"], y)
	
	if "image_filename_format" in new_parameters:
		new_parameters["image_filename_format"] = filter_name_template(new_parameters["image_filename_format"], y)
	if "depth_filename_format" in new_parameters:
		new_parameters["depth_filename_format"] = filter_name_template(new_parameters["depth_filename_format"], y)
	if "vsrs" in new_parameters:
		process_clause("vsrs", datas)
	if "kinect_raw" in new_parameters:
		process_clause("kinect_raw", datas)
	
	with open(out_parameters_filename, 'w') as f:
		print >>f, json.dumps(new_parameters, indent=4, sort_keys=True)
