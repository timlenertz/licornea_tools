#ifndef LICORNEA_VIEW_HOMOGRAPHY_H_
#define LICORNEA_VIEW_HOMOGRAPHY_H_

#include "common.h"
#include "dataset.h"
#include "json.h"
#include "args.h"
#include "border.h"
#include <map>

namespace tlz {

struct view_homography {
	mat33 mat;
	real err;
};
using view_homographies = std::map<view_index, view_homography>;

json encode_view_homography(const view_homography&);
view_homography decode_view_homography(const json&);

json encode_view_homographies(const view_homographies&);
view_homographies decode_view_homographies(const json&);

bool is_single_view_homography(const json&);

real view_homographies_error(const view_homographies&);

border maximal_border(const view_homography&, real width, real height);
border maximal_border(const view_homographies&, real width, real height);


inline view_homography homography_arg()
	{ return decode_view_homography(json_arg()); }

inline view_homographies homographies_arg()
	{ return decode_view_homographies(json_arg()); }


}

#endif
