from pathlib import Path
import shutil
root=Path('quest-pmos-bringup/monado-source')
files=['src/xrt/drivers/monterey/monterey_protocol.h','src/xrt/drivers/monterey/monterey_protocol.c','src/xrt/drivers/monterey/monterey_device.c','tests/tests_monterey_protocol.cpp']
for f in files:
 p=root/f; backup=p.with_name(p.name+'.original');
 if not backup.exists(): shutil.copy2(p,backup)
h=root/files[0];s=h.read_text().replace('#include <stddef.h>','#include <stdbool.h>\n#include <stddef.h>')
s=s.replace('#ifdef __cplusplus\n}\n#endif','/* Initialize world up from gravity; does not determine yaw or sensor mounting. */\nbool\nmonterey_imu_start_orientation(const struct xrt_vec3 *acceleration, struct xrt_quat *orientation);\n\n#ifdef __cplusplus\n}\n#endif')
h.write_text(s)
p=root/files[1];s=p.read_text().replace('#include <string.h>','#include <string.h>\n#include <math.h>\n#include "math/m_api.h"')
s=s.replace('\t\t\tout_sample->metadata =', '\t\t\tif (!math_vec3_validate(&out_sample->acceleration_m_s2) ||\n\t\t\t    !math_vec3_validate(&out_sample->angular_velocity_rad_s)) {\n\t\t\t\treturn MONTEREY_PARSE_MALFORMED;\n\t\t\t}\n\t\t\tout_sample->metadata =')
s+='''
/* Seed gravity alignment once instead of slewing from identity after startup. */
bool
monterey_imu_start_orientation(const struct xrt_vec3 *acceleration, struct xrt_quat *orientation)
{
	if (acceleration == NULL || orientation == NULL || !math_vec3_validate(acceleration)) {
		return false;
	}
	const float norm = sqrtf(acceleration->x * acceleration->x + acceleration->y * acceleration->y +
	                         acceleration->z * acceleration->z);
	/* Reject zero/freefall and large linear acceleration during initialization. */
	if (norm < 8.5f || norm > 11.0f) {
		return false;
	}
	const struct xrt_vec3 up = {0, 1, 0};
	math_quat_from_vec_a_to_vec_b(acceleration, &up, orientation);
	return math_quat_ensure_normalized(orientation);
}
''';p.write_text(s)
p=root/files[2];s=p.read_text();s=s.replace('\tm_imu_3dof_update(&d->fusion, now_ns,', '''	if (d->sample_count == 0 &&
	    !monterey_imu_start_orientation(&sample.acceleration_m_s2, &d->fusion.rot)) {
		os_mutex_unlock(&d->fusion_mutex);
		return false;
	}
	m_imu_3dof_update(&d->fusion, now_ns,''',1)
s=s.replace('if (d->sample_count == 0) {','if (d->sample_count == 0 || os_monotonic_get_ns() - d->last_update_ns > U_TIME_1S_IN_NS / 10) {',1)
p.write_text(s)
p=root/files[3];s=p.read_text().replace('#include "xrt/xrt_device.h"','#include "xrt/xrt_device.h"\n#include "math/m_api.h"\n#include <limits>')
s+='''
TEST_CASE("Monterey initial up is correct for tilted and inverted starts")
{
	const std::array<xrt_vec3, 5> samples{{
	    {0, 9.80665f, 0}, {-9.80665f, 0, 0}, {0, -9.80665f, 0},
	    {0, 0, 9.80665f}, {5.66187f, 5.66187f, 5.66187f}}};
	for (const auto &accel : samples) {
		xrt_quat q{};
		REQUIRE(monterey_imu_start_orientation(&accel, &q));
		xrt_vec3 world{};
		math_quat_rotate_vec3(&q, &accel, &world);
		CHECK(world.x == Catch::Approx(0).margin(0.001f));
		CHECK(world.y == Catch::Approx(9.80665f).margin(0.001f));
		CHECK(world.z == Catch::Approx(0).margin(0.001f));
	}
}

TEST_CASE("Monterey startup rejects invalid gravity")
{
	const std::array<xrt_vec3, 4> samples{{
	    {0, 0, 0}, {0, 20, 0}, {std::numeric_limits<float>::quiet_NaN(), 9.8f, 0},
	    {0, std::numeric_limits<float>::infinity(), 0}}};
	for (const auto &accel : samples) {
		xrt_quat q{};
		CHECK_FALSE(monterey_imu_start_orientation(&accel, &q));
	}
}
'''
p.write_text(s)
