from pathlib import Path
p=Path('quest-pmos-bringup/monado-source/tests/tests_monterey_protocol.cpp');s=p.read_text().replace('#include <unistd.h>','#include <unistd.h>\n#include <sys/stat.h>\n#include <sys/wait.h>\n#include "os/os_time.h"\n#include "math/m_imu_3dof.h"')
s=s.replace('REQUIRE(write(stream_fd, record.data(), record.size()) == static_cast<ssize_t>(record.size()));\n\tclose(stream_fd);\n\tclose(command_fd);', '''close(stream_fd);
	close(command_fd);
	REQUIRE(unlink(stream_path) == 0);
	REQUIRE(mkfifo(stream_path, 0600) == 0);
	pid_t producer = fork();
	REQUIRE(producer >= 0);
	if (producer == 0) {
		int fd = open(stream_path, O_WRONLY);
		if (fd < 0) _exit(1);
		for (int i = 0; i < 1000; i++) {
			if (write(fd, record.data(), record.size()) != static_cast<ssize_t>(record.size())) _exit(2);
			usleep(1000);
		}
		close(fd); _exit(0);
	}''')
s=s.replace('\txrt_device_destroy(&hmd);','''	xrt_space_relation pose{};
	hmd->get_tracked_pose(hmd, XRT_INPUT_GENERIC_HEAD_POSE, os_monotonic_get_ns(), &pose);
	CHECK((pose.relation_flags & XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT) != 0);
	int producer_status = 0;
	REQUIRE(waitpid(producer, &producer_status, 0) == producer);
	CHECK(WIFEXITED(producer_status));
	CHECK(WEXITSTATUS(producer_status) == 0);
	usleep(150000);
	hmd->get_tracked_pose(hmd, XRT_INPUT_GENERIC_HEAD_POSE, os_monotonic_get_ns(), &pose);
	CHECK(pose.relation_flags == 0);
	xrt_device_destroy(&hmd);''',1)
s+='''
TEST_CASE("Monterey measured sensor mounting maps pitch yaw and roll")
{
	const auto pitch = monterey_sensor_to_head({0, -1, 0});
	const auto yaw = monterey_sensor_to_head({-1, 0, 0});
	const auto roll = monterey_sensor_to_head({0, 0, -1});
	CHECK(pitch.x == 1); CHECK(pitch.y == 0); CHECK(pitch.z == 0);
	CHECK(yaw.x == 0); CHECK(yaw.y == 1); CHECK(yaw.z == 0);
	CHECK(roll.x == 0); CHECK(roll.y == 0); CHECK(roll.z == 1);
	const auto flat = monterey_sensor_to_head({-9.851389f, 0.022999f, 0.744476f});
	CHECK(flat.y > 9.8f);
	const auto front_up = monterey_sensor_to_head({-7.74027f, 0.18207f, 6.07913f});
	CHECK(front_up.z < -6.0f);
}

TEST_CASE("Monterey stationary startup estimates bias before publishing orientation")
{
	monterey_imu_calibration cal{};
	const xrt_vec3 a{-0.022999f, 9.851389f, -0.744476f};
	const xrt_vec3 g{0.006936f, -0.012934f, -0.003589f};
	for (int i = 0; i <= 750; i++) {
		bool ready = monterey_imu_calibrate(&cal, i*1000000LL, &a, &g);
		CHECK(ready == (i == 750));
	}
	CHECK(cal.gyro_bias.x == Catch::Approx(g.x));
	CHECK(cal.gyro_bias.y == Catch::Approx(g.y));
	CHECK(cal.gyro_bias.z == Catch::Approx(g.z));
	m_imu_3dof fusion{};
	m_imu_3dof_init(&fusion, M_IMU_3DOF_USE_GRAVITY_DUR_20MS);
	fusion.rot = cal.orientation;
	fusion.gyro_bias.value = cal.gyro_bias;
	for (int i = 0; i < 5000; i++) m_imu_3dof_update(&fusion, i*1000000LL, &a, &g);
	float dot = fusion.rot.x*cal.orientation.x + fusion.rot.y*cal.orientation.y +
	            fusion.rot.z*cal.orientation.z + fusion.rot.w*cal.orientation.w;
	CHECK(dot == Catch::Approx(1).margin(0.00001f));
	m_imu_3dof_close(&fusion);
}

TEST_CASE("Monterey startup waits through motion and rejects noisy calibration")
{
	monterey_imu_calibration cal{};
	const xrt_vec3 a{0, 9.80665f, 0}, moving{0, 0.3f, 0}, still{0, 0, 0};
	for (int i = 0; i < 1000; i++) CHECK_FALSE(monterey_imu_calibrate(&cal, i*1000000LL, &a, &moving));
	CHECK(cal.count == 0);
	for (int i = 1000; i < 1750; i++) CHECK_FALSE(monterey_imu_calibrate(&cal, i*1000000LL, &a, &still));
	CHECK(monterey_imu_calibrate(&cal, 1750000000LL, &a, &still));
	cal = {};
	for (int i = 0; i <= 750; i++) {
		xrt_vec3 noisy{(i%2)?0.04f:-0.04f, 0, 0};
		CHECK_FALSE(monterey_imu_calibrate(&cal, i*1000000LL, &a, &noisy));
	}
	CHECK_FALSE(cal.ready);
}
''';p.write_text(s)
