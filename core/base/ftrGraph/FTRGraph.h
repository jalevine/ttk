/// \ingroup base
/// \class ttk::ftr::FTRGraph
/// \author charles gueunet charles.gueunet+ttk@gmail.com
/// \date 2018-01-15
///
/// \brief TTK %FTRGraph processing package.
///
/// %FTRGraph is a TTK processing package that takes a scalar field on the input
/// and produces a scalar field on the output.
///
/// \sa ttk::Triangulation
/// \sa vtkFTRGraph.cpp %for a usage example.

#ifndef _FTRGRAPH_H
#define _FTRGRAPH_H

// base code includes
#include <Triangulation.h>

// local includes
#include "DataTypesFTR.h"
#include "DynamicGraph.h"
#include "FTRCommon.h"
#include "Graph.h"
#include "Propagation.h"
#include "Scalars.h"

// c++ includes
#include<set>

namespace ttk
{
   namespace ftr
   {
      template<typename ScalarType>
      class FTRGraph : virtual public Debug, public Allocable
      {
        private:
         // Exernal fields
         Params* const              params_;
         Scalars<ScalarType>* const scalars_;

         const bool needDelete_;

         // Internal fields
         Triangulation*         mesh_;
         Graph                  graph_;
         DynamicGraph<idVertex> dynGraph_;

         AtomicVector<Propagation*> propagations_;
         std::vector<UnionFind*>    toVisit_;

        public:
         FTRGraph(Params* const params, Triangulation* mesh, Scalars<ScalarType>* const scalars);
         FTRGraph();
         virtual ~FTRGraph();

         /// build the Reeb Graph
         /// \pre If this TTK package uses ttk::Triangulation for fast mesh
         /// traversals, the function setupTriangulation() must be called on this
         /// object prior to this function, in a clearly distinct pre-processing
         /// steps. An error will be returned otherwise.
         /// \note In such a case, it is recommended to exclude
         /// setupTriangulation() from any time performance measurement.
         void build();

         // General documentation info:
         //
         /// Setup a (valid) triangulation object for this TTK base object.
         ///
         /// \pre This function should be called prior to any usage of this TTK
         /// object, in a clearly distinct pre-processing step that involves no
         /// traversal or computation at all. An error will be returned otherwise.
         ///
         /// \note It is recommended to exclude this pre-processing function from
         /// any time performance measurement. Therefore, it is recommended to
         /// call this function ONLY in the pre-processing steps of your program.
         /// Note however, that your triangulation object must be valid when
         /// calling this function (i.e. you should have filled it at this point,
         /// see the setInput*() functions of ttk::Triangulation). See vtkFTRGraph
         /// for further examples.
         ///
         /// \param triangulation Pointer to a valid triangulation.
         /// \return Returns 0 upon success, negative values otherwise.
         /// \sa ttk::Triangulation
         //
         //
         // Developer info:
         // ttk::Triangulation is a generic triangulation representation that
         // enables fast mesh traversal, either on explicit triangulations (i.e.
         // tet-meshes) or implicit triangulations (i.e. low-memory footprint
         // implicit triangulations obtained from regular grids).
         //
         // Not all TTK packages need such mesh traversal features. If your
         // TTK package needs any mesh traversal procedure, we recommend to use
         // ttk::Triangulation as described here.
         //
         // Each call to a traversal procedure of ttk::Triangulation
         // must satisfy some pre-condition (see ttk::Triangulation for more
         // details). Such pre-condition functions are typically called from this
         // function.
         inline int setupTriangulation(Triangulation *triangulation)
         {
            mesh_ = triangulation;

            if (mesh_) {
               mesh_->preprocessVertexNeighbors();
               mesh_->preprocessVertexTriangles();
               mesh_->preprocessTriangleEdges();
               mesh_->preprocessEdgeTriangles();
            }

            return 0;
         }

         // Accessor on the graph
         // ---------------------

         Graph&& extractOutputGraph(void)
         {
            return std::move(graph_);
         }

         // Parameters
         // ----------

         /// The nuber of threads to be used during the computation
         /// of the reeb graph
         void setThreadNumber(const idThread nb)
         {
            params_->threadNumber = nb;
         }

         /// Control the verbosity of the base code
         virtual int setDebugLevel(const int& lvl) override
         {
            Debug::setDebugLevel(lvl);
            params_->debugLevel = lvl;
            return 0;
         }

         /// Scalar field used to compute the Reeb Graph
         void setScalars(const void* scalars)
         {
            scalars_->setScalars((ScalarType*)scalars);
         }

         /// When several points have the same scalar value,
         /// we use simulation of simplicity to distingish between
         /// them in a morse discret geometry compliant way.
         /// This is explained in the TTK report.
         /// Set the array to use here
         void setVertexSoSoffsets(idVertex* sos)
         {
            scalars_->setOffsets(sos);
         }

        protected:

         // Build functions

         /// Find the extrema from which the local propagations will start
         void leafSearch();

         /// Launch the sweep algorithm, but adapted to growth
         /// locally from each seed (min and/or max)
         /// See: grwothFromSeed.
         void sweepFrowSeeds();

         // Print function (FTRGraphPrint)

         std::string printEdge(const orderedEdge& oEdge, const Propagation* const localPropagation) const;

         std::string printEdge(const idEdge edgeId, const Propagation* const localPropagation) const;

         std::string printTriangle(const orderedTriangle&   oTriangle,
                              const Propagation* const localPropagation) const;

         std::string printTriangle(const idCell cellId, const Propagation* const localPropagation) const;

         void printGraph(const int verbosity) const;

         void printTime(DebugTimer& timer, const std::string& msg, const int lvl) const;

         // Initialize functions (virtual inherit from Allocable)
         // called automatically by the build

         void alloc() override;

         void init() override;

        private: // FTRGraphPrivate
         /// Local propagation for the vertex seed, using BFS with a priority queue
         /// localPropagation.
         /// This will process all the area corresponding to one connected component
         /// of level set.
         /// When a 2 Saddle is met, this function wait it has been completely visited
         /// and then continue.
         /// When a 1 Saddle is met, we split the local propagation with a BFS
         /// to continue locally.
         void growthFromSeed(const idVertex seed, Propagation* localPropagation);

         /// visit the star of v and create two vector,
         /// first one contains edges finishing at v (lower star)
         /// second one contains edges starting at v (upper star)
         std::pair<std::vector<idEdge>, std::vector<idEdge>> visitStar(
             const Propagation* const localPropagation) const;

         /// Consider edges ending at the vertex v, one by one,
         /// and find their corresponding components in the current
         /// preimage graph, each representing a component.
         /// \ret the set of uniques representing components
         std::set<DynGraphNode<idVertex>*> lowerComps(const std::vector<idEdge>& finishingEdges);

         /// Symetric to lowerComps
         /// \ref lowerComps
         std::set<DynGraphNode<idVertex>*> upperComps(const std::vector<idEdge>& startingEdges);

         /// update (locally) the preimage graph (dynGraph) from that
         /// of immediately before f(v) to that of immediately after f(v).
         /// (v is localGrowth->getCurVertex())
         void updatePreimage(const Propagation* const localPropagation);

         /// update the dynamicGraph by adding (if needed) a new edge corresponding to the
         /// starting cell cellId
         void updatePreimageStartCell(const orderedTriangle&   oTriangle,
                                      const Propagation* const localPropagation);

         /// update the dynamicGraph by moving (if needed) the corresponding to the
         /// current visited cell cellId
         void updatePreimageMiddleCell(const orderedTriangle&   oTriangle,
                                       const Propagation* const localPropagation);

         /// update the dynamicGraph by removing (if needed) the edge corresponding to the
         /// last visit of the cell cellId
         void updatePreimageEndCell(const orderedTriangle&   oTriangle,
                                    const Propagation* const localPropagation);

         /// update the skeleton structure
         /// \ret the nodeId of the current saddle/max
         idNode updateReebGraph(const idSuperArc         currentArc,
                                const Propagation* const localPropagation);

         /// local growth replacing the global sort
         void localGrowth(Propagation* const localPropagation);

         // Check if the current vertex which is on a Join saddle come from the
         // last growth touching this saddle
         bool checkLast(const idSuperArc currentArc, const Propagation* const localPropagation,
                        const std::vector<idEdge>& lowerStarEdges);

         // At a join saddle, merge local propagations coming here
         // and close remiang opened arcs.
         void mergeAtSaddle(const idNode saddleId);

         // At a split saddle, break the localProagation into pieces
         // corresponding to each upper CC (use BFS)
         std::vector<Propagation*> splitAtSaddle(const Propagation* const localProp);

         // Retrun one triangle by upper CC of the vertex v
         std::set<idCell> upCCtriangleSeeds(const idVertex v, const Propagation* const localProp);

         // bfs on triangles/edges in the neighborhood of the saddle to mark
         // each cc with a distinct identifier.
         void bfsSeed(const std::size_t idt, const valence idcc, std::vector<idCell>& triangles,
                      std::vector<valence>& cc, const Propagation* const localProp);

         // bfs on triangles/edges crossing the level set at saddle, starting
         // at seed. upper vertices encountred are added to newLocalProp
         void bfsPropagation(const idVertex saddle, const idCell seed,
                             Propagation* const newLocalProp, std::set<idCell>& visitedCells,
                             std::set<idVertex>& addedVertices);

         // Tools

         // Create a new propagation starting at leaf
         // use rpz as representant for new uf if not nullptr
         Propagation* newPropagation(const idVertex leaf, UnionFind* rpz = nullptr);

         // Compute the wieght of the edge in the dyngraph between e1 and e2.
         // This weight is the min value of the two endpoints, we use the mirror array (int)
         idVertex getWeight(const orderedEdge& e1, const orderedEdge& e2,
                            const Propagation* const localPropagation);

         /// get an edge with the "start" vertex in first position
         /// The start vertex is the lowest according to localPropagation comparison
         orderedEdge getOrderedEdge(const idEdge edgeId, const Propagation* const localPropagation) const;

         /// get a triangle defined by its edges sorted and its id
         orderedTriangle getOrderedTriangle(const idCell             cellId,
                                            const Propagation* const localPropagation) const;

         /// On a triangle, recover the position of the current vertex to classify the triangle
         vertPosInTriangle getVertPosInTriangle(const orderedTriangle&   oTriangle,
                                                const Propagation* const localPropagation) const;

      };
   }  // namespace ftr
}  // namespace ttk

// Implementation
#include "FTRGraph_Template.h"
#include "FTRGraphBFS_Template.h"
#include "FTRGraphPrint_Template.h"
#include "FTRGraphPrivate_Template.h"

#endif  // FTRGRAPH_H
