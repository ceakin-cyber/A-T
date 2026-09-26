#pragma once

#include "core/land.h"

#include <imgui.h>
#include <vector>

namespace ui {

// Land outlines (see core::LandRing) cut into triangles once, ahead of time, so the ground track
// map can fill in the continents every frame cheaply: triangulating a few thousand points of
// concave coastline is far too slow to redo every frame, but drawing the finished triangles is
// not. Points are kept in degrees -- x longitude, y latitude negated (so y grows southward, as
// on screen) -- and projected onto the map when drawn, so the same mesh fits any map size.
struct LandMesh {
    struct Piece {
        std::vector<ImVec2> points; // (longitude, -latitude), in degrees
        std::vector<int> indices;   // three per triangle, into `points`
        bool hole = false;          // water inside the land around it (see core::LandRing)
    };
    std::vector<Piece> pieces; // one per ring, kept apart so no one draw call grows too large
};

// Triangulates every ring. Needs a current ImGui context (it borrows ImGui's own polygon
// filling to do the triangulation), so call it after ImGui::CreateContext. A ring that cannot be
// cut into triangles cleanly is left out, with a note on stderr, rather than drawn wrongly.
LandMesh BuildLandMesh(const std::vector<core::LandRing>& rings);

// Fills the land onto an equirectangular map whose top-left corner is `origin` and whose size is
// `size` (longitude -180 to 180 across, latitude 90 to -90 down), in `landColor`, then paints its
// holes back over it in `waterColor`, which should match whatever is behind the map.
void DrawLandMesh(ImDrawList* drawList, const LandMesh& mesh, ImVec2 origin, ImVec2 size,
                  ImU32 landColor, ImU32 waterColor);

} // namespace ui
