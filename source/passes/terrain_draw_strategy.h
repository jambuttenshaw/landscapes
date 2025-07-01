#pragma once

#include <donut/render/DrawStrategy.h>


class Terrain;
class TerrainTile;


class TerrainDrawStrategy : public donut::render::IDrawStrategy
{
public:
    virtual void PrepareForView(const std::shared_ptr<donut::engine::SceneGraphNode>& rootNode, const donut::engine::IView& view) override;

    virtual const donut::render::DrawItem* GetNextItem() override;

    void SetData(const Terrain* terrain, donut::math::uint levelToDraw);

private:
    std::vector<donut::render::DrawItem> m_DrawItems;
    std::vector<donut::render::DrawItem>::iterator m_NextItem;
};
