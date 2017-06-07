#ifndef LICORNEA_FEATURE_POINT_H_
#define LICORNEA_FEATURE_POINT_H_

#include "../../lib/common.h"
#include "../../lib/json.h"

namespace tlz {

struct feature_point {
	vec2 position;
	real depth = 0.0;
	real weight = 1.0;
};

feature_point decode_feature_point(const json&);
json encode_feature_point(const feature_point&);

}

#endif