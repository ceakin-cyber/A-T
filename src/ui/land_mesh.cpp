#include "ui/land_mesh.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>

namespace ui {

namespace {

// The area enclosed by a ring of points (shoelace formula), whichever way round it runs.
double RingArea(const std::vector<ImVec2>& points) {
    double twiceArea = 0.0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const ImVec2& a = points[i];
        const ImVec2& b = points[(i + 1) % points.size()];
        twiceArea += static_cast<double>(a.x) * b.y - static_cast<double>(b.x) * a.y;
    }
    return std::abs(twiceArea) / 2.0;
}

// The total area of a piece's triangles.
double TriangleArea(const LandMesh::Piece& piece) {
    double total = 0.0;
    for (std::size_t i = 0; i + 2 < piece.indices.size(); i += 3) {
        const ImVec2& a = piece.points[static_cast<std::size_t>(piece.indices[i])];
        const ImVec2& b = piece.points[static_cast<std::size_t>(piece.indices[i + 1])];
        const ImVec2& c = piece.points[static_cast<std::size_t>(piece.indices[i + 2])];
        total += std::abs(static_cast<double>(b.x - a.x) * (c.y - a.y) -
                          static_cast<double>(c.x - a.x) * (b.y - a.y)) /
                 2.0;
    }
    return total;
}

} // namespace

LandMesh BuildLandMesh(const std::vector<core::LandRing>& rings) {
    constexpr double kRadToDeg = 180.0 / std::numbers::pi;
    LandMesh mesh;

    // A scratch draw list with anti-aliasing off, so filling a polygon into it leaves exactly its
    // triangles -- no extra fringe of feathered edge -- to read back out.
    ImDrawList scratch(ImGui::GetDrawListSharedData());
    std::vector<ImVec2> outline;
    for (const core::LandRing& ring : rings) {
        // Latitude is negated so the points run the way screen coordinates do (y growing
        // downward), which is what ImGui's polygon filling expects; given the other way round, it
        // cuts many coastlines into the wrong triangles.
        outline.clear();
        for (const core::Geodetic& point : ring.points) {
            outline.push_back({static_cast<float>(point.longitude * kRadToDeg),
                               static_cast<float>(-point.latitude * kRadToDeg)});
        }
        const double expected = RingArea(outline);

        // A correct triangulation covers exactly the ring's own area; anything else means some
        // triangles spill outside the coastline, which would smear across the map. A few rings
        // only come out right with their points taken in the opposite order, so that is tried
        // next; one that still fails is left out (its land just goes unfilled).
        bool filled = false;
        for (int attempt = 0; attempt < 2 && !filled; ++attempt) {
            if (attempt == 1) {
                std::reverse(outline.begin(), outline.end());
            }
            scratch._ResetForNewFrame();
            scratch.Flags = ImDrawListFlags_None;
            scratch.PushClipRectFullScreen();
            scratch.AddConcavePolyFilled(outline.data(), static_cast<int>(outline.size()),
                                         IM_COL32_WHITE);

            LandMesh::Piece piece;
            piece.hole = ring.hole;
            piece.points.reserve(static_cast<std::size_t>(scratch.VtxBuffer.Size));
            for (const ImDrawVert& vertex : scratch.VtxBuffer) {
                piece.points.push_back(vertex.pos);
            }
            piece.indices.reserve(static_cast<std::size_t>(scratch.IdxBuffer.Size));
            for (const ImDrawIdx index : scratch.IdxBuffer) {
                piece.indices.push_back(static_cast<int>(index));
            }
            if (!piece.indices.empty() &&
                std::abs(TriangleArea(piece) - expected) <= 1e-3 * expected) {
                mesh.pieces.push_back(std::move(piece));
                filled = true;
            }
        }
        if (!filled) {
            std::cerr << "Land outline with " << ring.points.size()
                      << " points could not be filled; leaving it out\n";
        }
    }
    return mesh;
}

void DrawLandMesh(ImDrawList* drawList, const LandMesh& mesh, ImVec2 origin, ImVec2 size,
                  ImU32 landColor, ImU32 waterColor) {
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    // Land first, then holes over it, so a lake inside a continent is painted back as water.
    for (const bool holes : {false, true}) {
        for (const LandMesh::Piece& piece : mesh.pieces) {
            if (piece.hole != holes) {
                continue;
            }
            const ImU32 color = holes ? waterColor : landColor;
            drawList->PrimReserve(static_cast<int>(piece.indices.size()),
                                  static_cast<int>(piece.points.size()));
            const auto base = static_cast<ImDrawIdx>(drawList->_VtxCurrentIdx);
            for (const ImVec2& point : piece.points) {
                // Points are (longitude, -latitude) in degrees; see BuildLandMesh.
                drawList->PrimWriteVtx({origin.x + (point.x + 180.0F) / 360.0F * size.x,
                                        origin.y + (90.0F + point.y) / 180.0F * size.y},
                                       uv, color);
            }
            for (const int index : piece.indices) {
                drawList->PrimWriteIdx(static_cast<ImDrawIdx>(base + index));
            }
        }
    }
}

} // namespace ui
