#pragma once

#include <raylib.h>

class Player {
public:
    explicit Player(Vector2 position);

    void setMovementInput(Vector2 movement);
    void setMovementTuning(float maxSpeed, float acceleration, float damping);
    void setPosition(Vector2 position);
    void update(float dt);
    Vector2 position() const { return m_position; }

private:
    Vector2 m_position{0.0f, 0.0f};
    Vector2 m_velocity{0.0f, 0.0f};
    Vector2 m_movementInput{0.0f, 0.0f};
    float m_maxSpeed = 230.0f;
    float m_acceleration = 1400.0f;
    float m_damping = 11.0f;
};
