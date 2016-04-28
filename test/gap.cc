#include <apf.h>
#include <apfMDS.h>
#include <apfMesh2.h>
#include <gmi_mesh.h>
#include <gmi_sim.h>
#include <parma.h>
#include <PCU.h>
#include <SimUtil.h>
#include <cassert>
#include <stdlib.h>

namespace {
  apf::MeshTag* setWeights(apf::Mesh* m) {
    apf::MeshIterator* it = m->begin(m->getDimension());
    apf::MeshEntity* e;
    apf::MeshTag* tag = m->createDoubleTag("parma_weight", 1);
    double w = 1.0;
    while ((e = m->iterate(it))) 
      m->setDoubleTag(e, tag, &w);
    m->end(it);
    return tag;
  }
}
int main(int argc, char** argv)
{
  assert(argc == 5);
  MPI_Init(&argc,&argv);
  PCU_Comm_Init();
  SimUtil_start();
  Sim_readLicenseFile(NULL);
  gmi_sim_start();
  if ( argc != 5 ) {
    if ( !PCU_Comm_Self() )
      printf("Usage: %s <model> <mesh> <max elm imb> <out prefix>\n", argv[0]);
    MPI_Finalize();
    exit(EXIT_FAILURE);
  }
  gmi_register_mesh();
  gmi_register_sim();
  //load model and mesh
  apf::Mesh2* m = apf::loadMdsMesh(argv[1],argv[2]);
  Parma_PrintPtnStats(m, "initial");
  apf::MeshTag* weights = setWeights(m);
  int verbose = 2; // set to 1 to silence the 'endStep' stats
  double stepFactor = 0.5;
  apf::Balancer* balancer = Parma_MakeShapeOptimizer(m, stepFactor, verbose);
  balancer->balance(weights, atof(argv[3]));
  delete balancer;
  apf::removeTagFromDimension(m, weights, m->getDimension());
  Parma_PrintPtnStats(m, "final");
  m->destroyTag(weights);
  m->writeNative(argv[4]);
  // destroy mds
  m->destroyNative();
  apf::destroyMesh(m);
  gmi_sim_stop();
  Sim_unregisterAllKeys();
  SimUtil_stop();
  PCU_Comm_Free();
  MPI_Finalize();
}
