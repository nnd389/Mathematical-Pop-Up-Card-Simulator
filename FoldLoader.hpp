#ifndef FOLDLOADER_HPP
#define FOLDLOADER_HPP

#include "Mechanism.hpp"
#include <string>
#include <vector>

// Reads a FOLD-style pop-up card file (as exported by the Origami Simulator, plus the extra
// assignments PM/PV/GS) and builds the same std::vector<Mechanism> you would otherwise
// write by hand in main.cpp. Mechanism 0 is the base card.
//
// File -> card mapping:
//   * file coords (x, y, z) become card coords (x, -z, 0) * scale   (z is flipped to match the SVG-style y)
//   * mechanisms = connected components of the edge graph, ignoring GS edges. The component holding
//     the lowest vertex index is the base card; the rest are ordered by BFS across glue springs.
//   * a mechanism's flatPos is its OUTLINE only: the vertices met walking its cyclic loop of B edges,
//     starting at the loop's lowest-numbered vertex and heading toward its lower-numbered neighbour. A glue
//     target lying mid-edge on a straight stretch of the loop (e.g. a tower top) is walked past, not stored.
//     Interior vertices (e.g. the base card's glue-target points) are not stored; their positions are
//     only used as GlueDot::gluePosition.
//   * M/PM and V/PV edges become creases; collinear chains of fold edges merge into one Crease.
//   * each GS edge becomes a GlueDot (index = the child's loop position, position = the parent's vertex) on the higher-numbered mechanism, targeting the lower one.
// object_faces / patch_faces / faces_vertices are not used.
std::vector<Mechanism> loadCard(const std::string& path, float scale = 1.0f);

#endif
