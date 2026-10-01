// Compile against the patched Basalt headers and src/utils/vio_config.cpp.
// Exercise the actual constructor with a non-collinear four-camera rig.
#include <basalt/optical_flow/optical_flow.h>
#include <basalt/optical_flow/patterns.h>
#include <cassert>
#include <iostream>
struct Probe : basalt::OpticalFlowTyped<double, basalt::Pattern51> {
  Probe(const basalt::VioConfig& config, const basalt::Calibration<double>& cal)
      : OpticalFlowTyped(config, cal) { processing_thread.reset(new std::thread([] {})); }
  void processingLoop() override {}
};
int main() {
  basalt::Calibration<double> cal;
  cal.intrinsics.resize(4);
  cal.T_i_c.resize(4);
  cal.T_i_c[1] = Sophus::SE3d(Sophus::SO3d(), Eigen::Vector3d(.12, 0, 0));
  cal.T_i_c[2] = Sophus::SE3d(Sophus::SO3d::exp(Eigen::Vector3d(.2, 0, 0)), Eigen::Vector3d(0, .08, 0));
  cal.T_i_c[3] = Sophus::SE3d(Sophus::SO3d::exp(Eigen::Vector3d(.15, -.1, 0)), Eigen::Vector3d(.12, .08, .02));
  basalt::VioConfig config;
  Probe probe(config, cal);
  for (int cam = 1; cam < 4; ++cam) {
    for (int n = 0; n < 20; ++n) {
      Eigen::Vector3d p(.03*n-.2, .01*n-.1, 1.+.05*n);
      Eigen::Vector4d a, b;
      a << p.normalized(), 0;
      b << (cal.T_i_c[cam].inverse()*p).normalized(), 0;
      double error = std::abs((a.transpose()*probe.E[cam]*b)(0));
      if (error > 1e-10) { std::cerr << "camera " << cam << " error " << error << '\n'; return 1; }
    }
  }
  std::cout << "All camera-pair epipolar constraints PASS\n";
}
