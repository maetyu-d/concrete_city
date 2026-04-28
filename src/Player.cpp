#include "Player.hpp"

#include <algorithm>
#include <cmath>

namespace {
float lengthOf(Vector2 value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

Vector2 add(Vector2 a, Vector2 b) {
    return {a.x + b.x, a.y + b.y};
}

Vector2 subtract(Vector2 a, Vector2 b) {
    return {a.x - b.x, a.y - b.y};
}

Vector2 multiply(Vector2 value, float scalar) {
    return {value.x * scalar, value.y * scalar};
}

Vector2 clampLength(Vector2 value, float maxLength) {
    const float length = lengthOf(value);
    if (length <= maxLength || length <= 0.0f) {
        return value;
    }
    return multiply(value, maxLength / length);
}
}

Player::Player(Vector2 position)
    : m_position(position) {
}

void Player::setMovementInput(Vector2 movement) {
    m_movementInput = movement;
}

void Player::setMovementTuning(float maxSpeed, float acceleration, float damping) {
    m_maxSpeed = maxSpeed;
    m_acceleration = acceleration;
    m_damping = damping;
}

void Player::setPosition(Vector2 position) {
    m_position = position;
    m_velocity = {0.0f, 0.0f};
}

void Player::update(float dt) {
    Vector2 desiredDirection = m_movementInput;
    const float desiredLength = lengthOf(desiredDirection);
    Vector2 desiredVelocity{0.0f, 0.0f};

    if (desiredLength > 0.0f) {
        desiredDirection = multiply(desiredDirection, 1.0f / desiredLength);
        desiredVelocity = multiply(desiredDirection, m_maxSpeed);
    }

    const Vector2 velocityDelta = subtract(desiredVelocity, m_velocity);
    const float response = (desiredLength > 0.0f) ? m_acceleration : m_acceleration * m_damping;
    m_velocity = add(m_velocity, clampLength(velocityDelta, response * dt));

    if (desiredLength <= 0.0f && lengthOf(m_velocity) < 8.0f) {
        m_velocity = {0.0f, 0.0f};
    }

    m_position = add(m_position, multiply(m_velocity, dt));
}
