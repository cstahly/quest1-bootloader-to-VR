from pathlib import Path
r=Path('quest-pmos-bringup/monado-source/src/xrt/drivers/monterey')
p=r/'monterey_protocol.h';s=p.read_text();before='/* Initialize world up from gravity; does not determine yaw or sensor mounting. */'
s=s.replace(before,'''/* Right-handed sensor-to-head mounting transform, observed on Quest 1. */
struct xrt_vec3 monterey_sensor_to_head(struct xrt_vec3 sensor);

struct monterey_imu_calibration {
	int64_t start_ns;
	uint32_t count;
	double acceleration_sum[3], acceleration_square_sum[3];
	double gyro_sum[3], gyro_square_sum[3];
	struct xrt_quat orientation;
	struct xrt_vec3 gyro_bias;
	bool ready;
};
/* Requires a continuous 0.75-second stationary window; never changes firmware. */
bool monterey_imu_calibrate(struct monterey_imu_calibration *cal, int64_t now_ns,
                           const struct xrt_vec3 *acceleration, const struct xrt_vec3 *gyro);

'''+before);p.write_text(s)
p=r/'monterey_protocol.c';s=p.read_text();s+='''
struct xrt_vec3
monterey_sensor_to_head(struct xrt_vec3 sensor)
{
	/* Level: sensor -X is up. Front-edge-up tilt: sensor +Z grows, so head Z=-Z.
	 * Right-handed completion gives head X=-Y; determinant is +1. */
	return (struct xrt_vec3){-sensor.y, -sensor.x, -sensor.z};
}

bool
monterey_imu_calibrate(struct monterey_imu_calibration *cal, int64_t now_ns,
                      const struct xrt_vec3 *acceleration, const struct xrt_vec3 *gyro)
{
	if (cal->ready) return true;
	const float a[3] = {acceleration->x, acceleration->y, acceleration->z};
	const float g[3] = {gyro->x, gyro->y, gyro->z};
	float a2 = 0, g2 = 0;
	for (int i = 0; i < 3; i++) { a2 += a[i]*a[i]; g2 += g[i]*g[i]; }
	if (!math_vec3_validate(acceleration) || !math_vec3_validate(gyro) ||
	    a2 < 8.5f*8.5f || a2 > 11.0f*11.0f || g2 > 0.08f*0.08f ||
	    (cal->count && now_ns < cal->start_ns)) {
		memset(cal, 0, sizeof(*cal)); return false;
	}
	if (!cal->count) cal->start_ns = now_ns;
	cal->count++;
	for (int i = 0; i < 3; i++) {
		cal->acceleration_sum[i] += a[i]; cal->acceleration_square_sum[i] += a[i]*a[i];
		cal->gyro_sum[i] += g[i]; cal->gyro_square_sum[i] += g[i]*g[i];
	}
	if (now_ns-cal->start_ns < 750000000 || cal->count < 100) return false;
	float average_a[3], average_g[3];
	for (int i = 0; i < 3; i++) {
		double am = cal->acceleration_sum[i]/cal->count, gm = cal->gyro_sum[i]/cal->count;
		double av = cal->acceleration_square_sum[i]/cal->count-am*am;
		double gv = cal->gyro_square_sum[i]/cal->count-gm*gm;
		if (av > 0.01 || gv > 0.008*0.008) {
			memset(cal, 0, sizeof(*cal)); return false;
		}
		average_a[i] = am; average_g[i] = gm;
	}
	const struct xrt_vec3 average = {average_a[0], average_a[1], average_a[2]};
	if (!monterey_imu_start_orientation(&average, &cal->orientation)) {
		memset(cal, 0, sizeof(*cal)); return false;
	}
	cal->gyro_bias = (struct xrt_vec3){average_g[0], average_g[1], average_g[2]};
	cal->ready = true;
	return true;
}
''';p.write_text(s)
p=r/'monterey_device.c';s=p.read_text().replace('#define MONTEREY_STARTUP_TIMEOUT_NS (2 * U_TIME_1S_IN_NS)','#define MONTEREY_STARTUP_TIMEOUT_NS (10 * U_TIME_1S_IN_NS)')
s=s.replace('struct m_imu_3dof fusion;', 'struct m_imu_3dof fusion;\n\tstruct monterey_imu_calibration calibration;\n\tint64_t last_received_ns;')
a=s.index('\t/* Device timestamp units still need capture validation.')
b=s.index('\tU_ZERO(&d->last_relation);',a)
s=s[:a]+'''	/* Keep host monotonic time until device timestamp units are verified. */
	const int64_t now_ns = os_monotonic_get_ns();
	sample.acceleration_m_s2 = monterey_sensor_to_head(sample.acceleration_m_s2);
	sample.angular_velocity_rad_s = monterey_sensor_to_head(sample.angular_velocity_rad_s);
	os_mutex_lock(&d->fusion_mutex);
	if (d->last_received_ns && now_ns-d->last_received_ns > U_TIME_1S_IN_NS/10) {
		/* Do not integrate a new sample over a long sensor outage. */
		memset(&d->calibration, 0, sizeof(d->calibration));
		d->sample_count = 0;
		m_imu_3dof_close(&d->fusion);
		m_imu_3dof_init(&d->fusion, M_IMU_3DOF_USE_GRAVITY_DUR_20MS);
	}
	d->last_received_ns = now_ns;
	if (!d->calibration.ready) {
		if (!monterey_imu_calibrate(&d->calibration, now_ns, &sample.acceleration_m_s2,
		                          &sample.angular_velocity_rad_s)) {
			os_mutex_unlock(&d->fusion_mutex);
			return false;
		}
		d->fusion.rot = d->calibration.orientation;
		d->fusion.gyro_bias.value = d->calibration.gyro_bias;
		MONTEREY_INFO(d, "Gravity initialized; stationary gyro bias %.6f %.6f %.6f rad/s",
		              d->calibration.gyro_bias.x, d->calibration.gyro_bias.y, d->calibration.gyro_bias.z);
	}
	m_imu_3dof_update(&d->fusion, now_ns, &sample.acceleration_m_s2, &sample.angular_velocity_rad_s);
''' +s[b:]
s=s.replace('d->last_relation.angular_velocity = sample.angular_velocity_rad_s;', '''d->last_relation.angular_velocity = (struct xrt_vec3){
	    sample.angular_velocity_rad_s.x - d->calibration.gyro_bias.x,
	    sample.angular_velocity_rad_s.y - d->calibration.gyro_bias.y,
	    sample.angular_velocity_rad_s.z - d->calibration.gyro_bias.z};''')
s=s.replace('No valid v50 HMD IMU packet arrived during startup','No stable HMD IMU window during startup; hold headset still and retry')
p.write_text(s)
