/* Compare output before/after the fully-masked-cell optimization. */
#include <basalt/utils/keypoints.h>
#include <basalt/image/image.h>
#include <iomanip>
#include <iostream>
#include <random>
int main(){
 basalt::ManagedImage<uint16_t> image(320,240);std::mt19937 rng(42);
 Eigen::MatrixXi cells=Eigen::MatrixXi::Zero(240/25+1,320/25+1);
 for(int run=0;run<12;run++){
  for(int y=0;y<240;y++)for(int x=0;x<320;x++)image(x,y)=uint16_t((rng()%256)<<8);
  basalt::Masks masks;
  for(int y=7;y<220;y+=25)for(int x=10;x<300;x+=25)
   if((x+y+run)%3)masks.masks.emplace_back(x+(run%2),y,25,25);
  basalt::KeypointsData data;
  basalt::detectKeypointsWithCells(basalt::Image<const uint16_t>(image.ptr,image.w,image.h,image.pitch),data,cells,25,1,5,40,0,masks);
  std::cout<<run<<":"<<data.corners.size()<<"\n"<<std::setprecision(17);
  for(size_t i=0;i<data.corners.size();i++)std::cout<<data.corners[i].transpose()<<" "<<data.corner_responses[i]<<"\n";
 }
}
