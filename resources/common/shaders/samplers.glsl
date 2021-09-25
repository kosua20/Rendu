
layout(set = 1, binding =  0) uniform sampler sClampNear; ///< Sampler with no mip map, nearest interpolation, clamped UV
layout(set = 1, binding =  1) uniform sampler sRepeatNear; ///< Sampler with no mip map, nearest interpolation, wrapped UV
layout(set = 1, binding =  2) uniform sampler sClampLinear; ///< Sampler with no mip map, linear interpolation, clamped UV
layout(set = 1, binding =  3) uniform sampler sRepeatLinear; ///< Sampler with no mip map, linear interpolation, wrapped UV
layout(set = 1, binding =  4) uniform sampler sClampNearNear; ///< Sampler with nearest mip map, nearest interpolation, clamped UV
layout(set = 1, binding =  5) uniform sampler sRepeatNearNear; ///< Sampler with nearest mip map, nearest interpolation, wrapped UV
layout(set = 1, binding =  6) uniform sampler sClampLinearNear; ///< Sampler with nearest mip map, linear interpolation, clamped UV;
layout(set = 1, binding =  7) uniform sampler sRepeatLinearNear; ///< Sampler with nearest mip map, linear interpolation, wrapped UVr;
layout(set = 1, binding =  8) uniform sampler sClampNearLinear; ///< Sampler with linear mip map, nearest interpolation, clamped UV
layout(set = 1, binding =  9) uniform sampler sRepeatNearLinear; ///< Sampler with linear mip map, nearest interpolation, wrapped UV
layout(set = 1, binding = 10) uniform sampler sClampLinearLinear; ///< Sampler with linear mip map, linear interpolation, clamped UV
layout(set = 1, binding = 11) uniform sampler sRepeatLinearLinear; ///< Sampler with linear mip map, linear interpolation, wrapped UV
