#include "pentagon_attacker.h"

#include "battle_game/core/bullets/bullets.h"
#include "battle_game/core/game_core.h"
#include "battle_game/graphics/graphics.h"

namespace battle_game::unit {

namespace {
uint32_t penta_body_model_index = 0xffffffffu;
uint32_t penta_turret_model_index = 0xffffffffu;
}  // namespace

Pentagon::Pentagon(GameCore *game_core, uint32_t id, uint32_t player_id)
    : Unit(game_core, id, player_id) {
  if (!~penta_body_model_index) {
    auto mgr = AssetsManager::GetInstance();
    {
      /* Pentagon Body */
      penta_body_model_index = mgr->RegisterModel(
          {{{-0.8f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
           {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
           {{0.8f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
           {{0.8f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
           {{-0.8f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}},
          {0, 1, 2, 0, 2, 3, 0, 3, 4});
    }

    {
      /* Penta Turret */
      std::vector<ObjectVertex> turret_vertices;
      std::vector<uint32_t> turret_indices;
      const int precision = 60;
      const float inv_precision = 1.0f / float(precision);
      for (int i = 0; i < precision; i++) {
        auto theta = (float(i) + 0.5f) * inv_precision;
        theta *= glm::pi<float>() * 2.0f;
        auto sin_theta = std::sin(theta);
        auto cos_theta = std::cos(theta);
        turret_vertices.push_back({{sin_theta * 0.5f, cos_theta * 0.5f},
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_indices.push_back(i);
        turret_indices.push_back((i + 1) % precision);
        turret_indices.push_back(precision);
      }
      turret_vertices.push_back(
          {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});

      for (int i = 0; i < 5; ++i) {
        auto theta = float(i) * 0.2f;
        theta *= glm::pi<float>() * 2.0f;
        turret_vertices.push_back({Rotate({-0.1f, 0.0f}, theta),
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_vertices.push_back({Rotate({0.1f, 0.0f}, theta),
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_vertices.push_back({Rotate({-0.1f, 1.2f}, theta),
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_vertices.push_back({Rotate({0.1f, 1.2f}, theta),
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_indices.push_back(precision + 1 + 4 * i + 0);
        turret_indices.push_back(precision + 1 + 4 * i + 1);
        turret_indices.push_back(precision + 1 + 4 * i + 2);
        turret_indices.push_back(precision + 1 + 4 * i + 1);
        turret_indices.push_back(precision + 1 + 4 * i + 2);
        turret_indices.push_back(precision + 1 + 4 * i + 3);
      }
      penta_turret_model_index =
          mgr->RegisterModel(turret_vertices, turret_indices);
    }
  }
  Skill temp;
  temp.name = "FFFire!";
  temp.description = "More bullets";
  temp.time_remain = 0;
  temp.time_total = 300;
  temp.type = E;
  temp.function = SKILL_ADD_FUNCTION(Pentagon::FFFireClick);
  skills_.push_back(temp);
  temp.name = "WeakDefense";
  temp.description = "Generate movable Block";
  temp.time_remain = 0;
  temp.time_total = 600;
  temp.type = Q;
  temp.function = SKILL_ADD_FUNCTION(Pentagon::WeakDefenseClick);
  skills_.push_back(temp);
  temp.name = "StrongDefense";
  temp.description = "Generate destructible defensive Wall";
  temp.time_remain = 0;
  temp.time_total = 1200;
  temp.type = R;
  temp.function = SKILL_ADD_FUNCTION(Pentagon::StrongDefenseClick);
  skills_.push_back(temp);
}

void Pentagon::Render() {
  battle_game::SetTransformation(position_, rotation_);
  battle_game::SetTexture(0);
  battle_game::SetColor(game_core_->GetPlayerColor(player_id_));
  battle_game::DrawModel(penta_body_model_index);
  battle_game::SetRotation(turret_rotation_);
  battle_game::DrawModel(penta_turret_model_index);
}

void Pentagon::Update() {
  PentaMove(3.0f, glm::radians(180.0f));
  TurretRotate();
  Fire();
  FFFire();
  WeakDefense();
  StrongDefense();
}

void Pentagon::PentaMove(float move_speed, float rotate_angular_speed) {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    glm::vec2 offset{0.0f};
    if (input_data.key_down[GLFW_KEY_W]) {
      offset.y += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_S]) {
      offset.y -= 1.0f;
    }
    float speed = move_speed * GetSpeedScale();
    offset *= kSecondPerTick * speed;
    auto new_position =
        position_ + glm::vec2{glm::rotate(glm::mat4{1.0f}, rotation_,
                                          glm::vec3{0.0f, 0.0f, 1.0f}) *
                              glm::vec4{offset, 0.0f, 0.0f}};
    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }
    float rotation_offset = 0.0f;
    if (input_data.key_down[GLFW_KEY_A]) {
      rotation_offset += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_D]) {
      rotation_offset -= 1.0f;
    }
    rotation_offset *= kSecondPerTick * rotate_angular_speed * GetSpeedScale();
    game_core_->PushEventRotateUnit(id_, rotation_ + rotation_offset);
  }
}

void Pentagon::TurretRotate() {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    auto diff = input_data.mouse_cursor_position - position_;
    if (glm::length(diff) < 1e-4) {
      turret_rotation_ = rotation_;
    }
    turret_rotation_ = std::atan2(diff.y, diff.x) - glm::radians(90.0f);
  }
}

void Pentagon::Fire() {
  if (is_fffire_ > 0) {
    if (fire_count_down_ > 10)
      fire_count_down_ = 10;
    is_fffire_--;
  }
  if (fire_count_down_) {
    fire_count_down_--;
  } else {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.mouse_button_down[GLFW_MOUSE_BUTTON_LEFT]) {
        if (is_fffire_ > 0) {
          for (int i = 0; i < 20; ++i) {
            auto theta = float(i) * 0.05f;
            theta *= glm::pi<float>() * 2.0f;
            auto velocity =
                Rotate(glm::vec2{0.0f, 20.0f}, turret_rotation_ + theta);
            GenerateBullet<bullet::RepellingBall>(
                position_ + Rotate({0.0f, 1.2f}, turret_rotation_ + theta),
                turret_rotation_, GetDamageScale(), velocity);
          }
          fire_count_down_ = 10;  // FFFire interval 1/6 second .
        } else {
          for (int i = 0; i < 5; ++i) {
            auto theta = float(i) * 0.2f;
            theta *= glm::pi<float>() * 2.0f;
            auto velocity =
                Rotate(glm::vec2{0.0f, 20.0f}, turret_rotation_ + theta);
            GenerateBullet<bullet::RepellingBall>(
                position_ + Rotate({0.0f, 1.2f}, turret_rotation_ + theta),
                turret_rotation_, GetDamageScale(), velocity);
          }
          fire_count_down_ = kTickPerSecond;  // Fire interval 1 second.
        }
      }
    }
  }
}

void Pentagon::FFFireClick() {
  is_fffire_ = 2 * kTickPerSecond;
  fffire_count_down_ = 5 * kTickPerSecond;
}

void Pentagon::FFFire() {
  skills_[0].time_remain = fffire_count_down_;
  if (fffire_count_down_) {
    fffire_count_down_--;
  } else {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.key_down[GLFW_KEY_E]) {
        FFFireClick();
      }
    }
  }
}

void Pentagon::WeakDefenseClick() {
  auto theta = rotation_ + glm::pi<float>() * 0.5f;
  game_core_->AddObstacle<battle_game::obstacle::MovableBlock>(
      this->GetPosition() + glm::vec2{2 * cos(theta), 2 * sin(theta)});
  weak_defense_count_down_ = 10 * kTickPerSecond;
}

void Pentagon::WeakDefense() {
  skills_[1].time_remain = weak_defense_count_down_;
  if (weak_defense_count_down_) {
    weak_defense_count_down_--;
  } else {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.key_down[GLFW_KEY_Q]) {
        WeakDefenseClick();
      }
    }
  }
}

void Pentagon::StrongDefenseClick() {
  auto theta = rotation_ + glm::pi<float>() * 0.5f;
  game_core_->AddObstacle<battle_game::obstacle::StrongWall>(
      this->GetPosition() + glm::vec2{2 * cos(theta), 2 * sin(theta)},
      rotation_);
  strong_defense_count_down_ = 20 * kTickPerSecond;
}

void Pentagon::StrongDefense() {
  skills_[2].time_remain = strong_defense_count_down_;
  if (strong_defense_count_down_) {
    strong_defense_count_down_--;
  } else {
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      auto &input_data = player->GetInputData();
      if (input_data.key_down[GLFW_KEY_R]) {
        StrongDefenseClick();
      }
    }
  }
}

bool Pentagon::IsHit(glm::vec2 position) const {
  position = WorldToLocal(position);
  if (position.y > 0.0f)
    return (4.0f * position.y - 5.0f * position.x < 4.0f) &&
           (4.0f * position.y + 5.0f * position.x < 4.0f);
  else
    return position.y > -1.0f && position.x > -0.8f && position.x < 0.8f;
}

const char *Pentagon::UnitName() const {
  return "Pentagon_attacker_ver_2.0";
}

const char *Pentagon::Author() const {
  return "ZigZagKmp";
}
}  // namespace battle_game::unit
