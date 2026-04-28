#include "Object.hpp"

const char* objectTypeName(ObjectType type) {
    switch (type) {
        case ObjectType::Tree: return "tree";
        case ObjectType::Stone: return "stone";
        case ObjectType::Ruin: return "ruin";
        case ObjectType::Marker: return "marker";
        case ObjectType::Anomaly: return "anomaly";
    }
    return "unknown";
}
