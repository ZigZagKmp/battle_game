#include "battle_game/core/bullets/repelling_ball.h"

#include "battle_game/core/game_core.h"
#include "battle_game/core/particles/particles.h"

namespace battle_game::bullet {
RepellingBall::RepellingBall(GameCore *core,
                             uint32_t id,
                             uint32_t unit_id,
                             uint32_t player_id,
                             glm::vec2 position,
                             float rotation,
                             float damage_scale,
                             glm::vec2 velocity)
    : Bullet(core, id, unit_id, player_id, position, rotation, damage_scale),
      velocity_(velocity) {
}

void RepellingBall::Render() {
  SetTransformation(position_, rotation_, glm::vec2{0.1f});
  SetColor(game_core_->GetPlayerColor(player_id_));
  SetTexture("../../textures/particle3.png");
  DrawModel(0);
}

void RepellingBall::Update() {
  position_ += velocity_ * kSecondPerTick;
  bool should_die = false;
  if (game_core_->IsBlockedByObstacles(position_)) {
    uint32_t obstacle_id = game_core_->GetBlockedObstacleId(position_);
    auto obstacle = game_core_->GetBlockedObstacle(position_);
    if (obstacle_id) {
      game_core_->PushEventDealDamageObstacle(obstacle_id, id_,
                                              damage_scale_ * 5.0f);
      if (obstacle->IsMovable()) {
        game_core_->PushEventMoveObstacle(
            obstacle_id,
            obstacle->GetPosition() + velocity_ * (kSecondPerTick * 0.5f));
      }
    }
    should_die = true;
  }

  auto &units = game_core_->GetUnits();
  for (auto &unit : units) {
    if (unit.first == unit_id_) {
      continue;
    }
    if (unit.second->IsHit(position_)) {
      game_core_->PushEventDealDamage(unit.first, id_, damage_scale_ * 10.0f);
      should_die = true;
    }
  }

  if (should_die) {
    game_core_->PushEventRemoveBullet(id_);
  }
}

RepellingBall::~RepellingBall() {
  for (int i = 0; i < 5; i++) {
    game_core_->PushEventGenerateParticle<particle::Smoke>(
        position_, rotation_, game_core_->RandomInCircle() * 2.0f, 0.2f,
        glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, 3.0f);
  }
}
}  // namespace battle_game::bullet
