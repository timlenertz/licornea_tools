#!/usr/local/bin/python
from pylib.dataset import *
from pylib.utility import *
from pylib.temporary import *
from pylib.config import *
import os, subprocess, uuid

import make_vsrs_config
import export_for_vsrs

def try_remove(filename):
	try:
		os.remove(filename)
	except:
		pass


def main(vsrs_binary_filename, datas, left_idx, virtual_idx, right_idx, output_virtual_filename, cameras_filename = None):
	if verbose:
		print "run_vsrs:  left={} virtual={} right={}".format(encode_view_index(left_idx), encode_view_index(virtual_idx), encode_view_index(right_idx))

	# Export left and right texture+disparity to VSRS, if VSRS version does not exist yey
	export_for_vsrs.export_view(datas, left_idx, overwrite_image=False, overwrite_depth=False)
	export_for_vsrs.export_view(datas, right_idx, overwrite_image=False, overwrite_depth=False)

	with temporary_file("yuv") as tmp_virtual_yuv_filename, \
		temporary_file("txt") as tmp_config_filename, \
		temporary_file("txt") as tmp_cameras_filename:
				
		# Make temporary VSRS cameras file, if none is provided
		if cameras_filename is None:
			cameras_filename = tmp_cameras_filename.filename
			call_tool("camera/export_mpeg", [
				datas.cameras_filename(),
				cameras_filename
			]);

		# Make temporary VSRS configuration file
		make_vsrs_config.main(datas, cameras_filename, left_idx, virtual_idx, right_idx, tmp_virtual_yuv_filename.filename, tmp_config_filename.filename)

		# Run VSRS
		if verbose: print "running VSRS"
		try:
			output = subprocess.check_output([vsrs_binary_filename, tmp_config_filename.filename])
		except subprocess.CalledProcessError as err:
			print "VSRS failed on {}. Output:\n{}".format(virtual_idx, err.output)
			raise Exception("VSRS failed")

		if not os.path.isfile(tmp_virtual_yuv_filename.filename):
			print "VSRS failed on {}. Output:\n{}".format(virtual_idx, output)
			raise Exception("VSRS failed")

		# Remove stuff generated by VSRS
		try_remove("5.bmp")
		try_remove("6.bmp")
		try_remove("7.bmp")

		# Convert output YUV to PNG
		if verbose: print "converting output YUV to PNG"
		call_tool("misc/yuv_import", [
			tmp_virtual_yuv_filename.filename,
			output_virtual_filename,
			datas.parameters["width"],
			datas.parameters["height"],
			"ycbcr420"
		]);

		
if __name__ == '__main__':
	def usage_fail():
		print("usage: {} vsrs_binary parameters.json left_idx virtual_idx right_idx output_virtual.png [cameras.txt]\n".format(sys.argv[0]))
		sys.exit(1)

	if len(sys.argv) <= 6: usage_fail()
	vsrs_binary_filename = sys.argv[1]
	parameters_filename = sys.argv[2]
	left_idx = decode_view_index(sys.argv[3])
	virtual_idx = decode_view_index(sys.argv[4])
	right_idx = decode_view_index(sys.argv[5])
	output_virtual_filename = sys.argv[6]
	cameras_filename = None
	if len(sys.argv) > 7:
		cameras_filename = sys.argv[7]

	datas = Dataset(parameters_filename)

	main(vsrs_binary_filename, datas, left_idx, virtual_idx, right_idx, output_virtual_filename, cameras_filename)
