#!/usr/local/bin/python
from pylib import *
import sys, os, json, shutil

simulate = True
parallel = True
parallel_jobs = 10
intrinsics_filename = "../data/kinect_internal_intrinsics.json"

if parallel:
	from joblib import Parallel, delayed

def usage_fail():
	print("usage: {} parameters.json densify_method\n".format(sys.argv[0]))
	sys.exit(1)

progress = None
dataset = None
densify_method = None

def process_view(x, y):	
	if verbose: print "view x={}, y={}".format(x, y)
	
	view = dataset.view(x, y)
	raw_view = view.kinect_raw()
	
	image_filename = view.image_filename()
	depth_filename = view.depth_filename()
	in_image_filename = raw_view.image_filename()
	in_depth_filename = raw_view.depth_filename()
	
	os.path.isfile(in_image_filename)
	os.path.isfile(in_depth_filename)
	
	if not os.path.isfile(image_filename):
		if verbose: print "copying image {} -> {}".format(in_image_filename, image_filename)
		if not simulate:
			shutil.copyfile(in_image_filename, image_filename)
	
	if not os.path.isfile(depth_filename):
		if verbose: print "reprojecting depth {} -> {}".format(in_depth_filename, depth_filename)
		if not simulate:
			call_tool("kinect/depth_reprojection", [
				in_depth_filename,
				depth_filename,
				mask_filename,
				intrinsics_filename,
				densify_method
			])



if __name__ == '__main__':
	if len(sys.argv) <= 2: usage_fail()
	parameters_filename = sys.argv[1]
	densify_method = sys.argv[2];

	dataset = Dataset(parameters_filename)
	
	total_count = dataset.y_count() * dataset.x_count()
	#indices = [(x, y) for y in dataset.y_indices() for x in dataset.x_indices()]
	indices = [(x, 0) for x in dataset.x_indices()]
	print indices
	
	progress = Progress(total_count)
	if not parallel:
		for xy in indices:
			process_view(*xy)
	else:
		Parallel(n_jobs=parallel_jobs, backend="threading")(delayed(process_view)(*xy) for xy in indices)
		# need threading backend because of shared 'done_count' variable

	print "done."

