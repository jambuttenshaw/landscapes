#pragma once

#include <donut/render/DrawStrategy.h>
#include <donut/render/GeometryPasses.h>


class TerrainMeshView;

struct TerrainDrawItem : public donut::render::DrawItem
{
    const TerrainMeshView* terrainView;
};


class TerrainDrawStrategy : public donut::render::IDrawStrategy
{
public:
    virtual void PrepareForView(const std::shared_ptr<donut::engine::SceneGraphNode>& rootNode, const donut::engine::IView& view) override;

    virtual const donut::render::DrawItem* GetNextItem() override;

private:
    donut::engine::SceneGraphWalker m_Walker{};

    std::vector<TerrainDrawItem> m_DrawItems;
    std::vector<TerrainDrawItem>::iterator m_NextItem;
};
