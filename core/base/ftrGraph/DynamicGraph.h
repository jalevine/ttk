/// \ingroup base
/// \class ttk::ftr::DynamicGraph
/// \author Gueunet Charles <charles.gueunet+ttk@gmail.com>
/// \date 2018-01-15
///
/// \brief TTK %fTRGraph dynamic graph tracking the evolution of level sets
///
/// This class deal with dynamic graph part of the algorithm, thracking the number of contour
/// on each vertex to deduce the Reeb graph.
/// This is done using an ST-tree.
///
/// \sa ttk::FTRGraph

#ifndef DYNAMICGRAPH_H
#define DYNAMICGRAPH_H

#include "FTRCommon.h"

#include <set>
#include <vector>

namespace ttk
{
   namespace ftr
   {
      template<typename Type>
      struct DynGraphNode;

      using idRoot = int;

      template <typename Type>
      class DynamicGraph : public Allocable
      {
        private:
         std::vector<DynGraphNode<Type>> nodes_;

        public:
         DynamicGraph();
         virtual ~DynamicGraph();

         // Initialize functions
         // --------------------

         void setNumberOfNodes(const std::size_t nbNodes)
         {
            // we use the nbVerts_ as the number
            // of nodes to allocate
            setNumberOfVertices(nbNodes);
         }

         /// preallocate the array of tree
         /// representing the forset,
         /// \pre needs nbVerts_ to be set.
         void alloc() override;

         void init() override;

         // Dyn Graph functions
         // -------------------

         /// \brief get the node with the id: nid
         DynGraphNode<Type>* getNode(const std::size_t nid)
         {
            return &nodes_[nid];
         }

         const DynGraphNode<Type>* getNode(const std::size_t nid) const
         {
            return &nodes_[nid];
         }

         /// \brief get the id of the node: node
         std::size_t getNodeId(DynGraphNode<Type>* node)
         {
            return std::distance(nodes_, node);
         }

         void setSubtreeArc(const std::size_t nid, const idSuperArc arc)
         {
            getNode(nid)->findRoot()->setRootArc(arc);
         }

         idSuperArc getSubtreeArc(const std::size_t nid) const
         {
            return getNode(nid)->findRootArc();
         }

         // check wether or not this node is connected to others
         bool isDisconnected(const DynGraphNode<Type>* const node) const
         {
            return !node->hasParent();
         }

         // check wether or not this node is connected to others
         bool isDisconnected(const std::size_t nid)
         {
            return !getNode(nid)->hasParent();
         }

         /// \brief recover the root of a node
         DynGraphNode<Type>* findRoot(const DynGraphNode<Type>* const node)
         {
            return node->findRoot();
         }

         /// \brief recover the root of a node using its id
         DynGraphNode<Type>* findRoot(const std::size_t nid)
         {
            return findRoot(getNode(nid));
         }

         /// \brief recover the root of several nodes once, using
         /// brace initializers style: findRoot({n1,n2})
         std::vector<DynGraphNode<Type>*> findRoot(
             std::initializer_list<DynGraphNode<Type>*> nodes)
         {
            std::vector<DynGraphNode<Type>*> roots;
            roots.reserve(nodes.size());
            for(auto* n : nodes) {
               roots.emplace_back(findRoot(n));
            }
            std::sort(roots.begin(), roots.end());
            const auto it = std::unique(roots.begin(), roots.end());
            roots.erase(it, roots.end());
            return roots;
         }

         /// \brief findRoot but using ids of the nodes
         std::vector<DynGraphNode<Type>*> findRoot(std::initializer_list<std::size_t> nodesIds)
         {
            std::vector<DynGraphNode<Type>*> roots;
            roots.reserve(nodesIds.size());
            for(auto n : nodesIds) {
               roots.emplace_back(findRoot(n));
            }
            std::sort(roots.begin(), roots.end());
            const auto it = std::unique(roots.begin(), roots.end());
            roots.erase(it, roots.end());
            return roots;
         }

         /// \brief findRoot but using ids of the nodes in a vector
         template<typename type>
         std::vector<DynGraphNode<Type>*> findRoot(const std::vector<type>& nodesIds)
         {
            std::vector<DynGraphNode<Type>*> roots;
            roots.reserve(nodesIds.size());
            for(auto n : nodesIds) {
               roots.emplace_back(findRoot(n));
            }
            std::sort(roots.begin(), roots.end());
            const auto it = std::unique(roots.begin(), roots.end());
            roots.erase(it, roots.end());
            return roots;
         }


         /// \brief count the number of connected component in a list of nodes
         std::size_t getNbCC(std::initializer_list<DynGraphNode<Type>*> nodes)
         {
            return findRoot(nodes).size();
         }

         /// \brief count the number of connected component in a list of nodes using ids
         std::size_t getNbCC(std::initializer_list<std::size_t> nodesIds)
         {
            return findRoot(nodesIds).size();
         }


         /// \ret true if we have merged two tree, false if it was just an intern operation
         bool insertEdge(DynGraphNode<Type>* const n1, DynGraphNode<Type>* const n2,
                         const Type w)
         {
           return n1->insertEdge(n2, w);
         }

         /// inert or replace existing edge between n1 and n2
         bool insertEdge(const std::size_t n1, const std::size_t n2, const Type w)
         {
            return insertEdge(getNode(n1), getNode(n2), w);
         }

         /// remove the link btwn n and its parent
         void removeEdge(DynGraphNode<Type>* const n)
         {
            n->removeEdge();
         }

         /// remove the link btwn n and its parent
         void removeEdge(const std::size_t nid)
         {
            removeEdge(getNode(nid));
         }

         /// remove the edge btwn n1 and n2
         /// \ret 0 if not an edge
         int removeEdge(DynGraphNode<Type>* const n1, DynGraphNode<Type>* const n2);

         /// remove the edge btwn n1 and n2
         /// \ret 0 if not an edge
         int removeEdge(const std::size_t nid1, const std::size_t nid2)
         {
            return removeEdge(getNode(nid1), getNode(nid2));
         }

         // Debug

         std::string print(void);

         std::string print(std::function<std::string(std::size_t)>);

         void test(void);
      };

      /// \brief class representing a node
      /// of a tree and the link to its parent if not the root
      template<typename Type>
      struct DynGraphNode {
         DynGraphNode* parent_;
         Type          weight_;
         idNode        nbChilds_;

         idSuperArc    corArc_;

         explicit DynGraphNode() : parent_(nullptr), weight_(0), nbChilds_(0), corArc_(nullSuperArc)
         {
         }

         DynGraphNode(const DynGraphNode& other)
         {
            operator=(other);
         }

         DynGraphNode& operator=(const DynGraphNode& other)
         {
            parent_   = other.parent_;
            weight_   = other.weight_;
            nbChilds_ = other.nbChilds_;
            return *this;
         }

         // Accessor functions
         // ------------------

         Type getWeight(void) const
         {
            return weight_;
         }

         idNode getNbChilds(void) const
         {
            return nbChilds_;
         }

         bool hasParent(void) const
         {
            return parent_;
         }

         // Graph functions
         // ---------------

         /// Make this node the root of its tree
         // Various way to do that, test perfs ?
         void evert(void);

         /// Get representative node
         DynGraphNode* findRoot(void) const;

         /// Get the arcs corresponding to this subtree
         idSuperArc findRootArc(void) const;

         void setRootArc(const idSuperArc arcId);

         /// Get representative node and kepp track of
         /// the node with the min weight on the path
         /// \ret tuple<root, node with min weight>
         std::tuple<DynGraphNode<Type>*, DynGraphNode<Type>*> findMinWeightRoot() const;

         /// Create a new edge between this node and the node n
         /// \ret true if we have merged two tree, false if it was just an intern operation
         bool insertEdge(DynGraphNode* const n, const Type weight);

         /// Remove the link between this node and its parent, thus makeing a new root
         void removeEdge(void);

      };
   }
}

#include "DynamicGraph_Template.h"

#endif /* end of include guard: DYNAMICGRAPH_H */
