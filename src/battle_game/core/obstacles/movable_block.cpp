#include "battle_game/core/obstacles/movable_block.h"

namespace battle_game::obstacle {

MovableBlock::MovableBlock(GameCore *game_core,
                           uint32_t id,
                           glm::vec2 position,
                           float rotation,
                           glm::vec2 scale)
    : Obstacle(game_core, id, position, rotation) {
  SetMovable();
}

bool MovableBlock::IsBlocked(glm::vec2 p) const {
  p = WorldToLocal(p);
  return p.x <= scale_.x && p.x >= -scale_.x && p.y <= scale_.y &&
         p.y >= -scale_.y;
}

void MovableBlock::Render() {
  battle_game::SetColor(glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
  battle_game::SetTexture(0);
  battle_game::SetTransformation(position_, rotation_, scale_);
  battle_game::DrawModel(0);
}
}  // namespace battle_game::obstacle
