#ifndef QUEST_LENS_MESH_H
#define QUEST_LENS_MESH_H
/* Read-only decoder for the stock Quest 1 32x32 per-eye distortion mesh. */
struct quest_lens_mesh {
 float ray[2][33][33][3][2];
 float center_shift[2];
 unsigned width,height;
};
int quest_lens_load(struct quest_lens_mesh *m,const char *path);
int quest_lens_ray(const struct quest_lens_mesh *m,int eye,int channel,float gx,float gy,float ray[2]);
int quest_lens_inverse(const struct quest_lens_mesh *m,int eye,int channel,float tx,float ty,float *gx,float *gy);
void quest_lens_prepare_fast(const struct quest_lens_mesh *m);
int quest_lens_project_fast(const struct quest_lens_mesh *m,int eye,int channel,float tx,float ty,float *gx,float *gy);
#endif
