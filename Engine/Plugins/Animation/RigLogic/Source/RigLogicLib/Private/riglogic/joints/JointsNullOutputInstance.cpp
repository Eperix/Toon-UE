// Copyright Epic Games, Inc. All Rights Reserved.

#include "riglogic/joints/JointsNullOutputInstance.h"

namespace rl4 {

ArrayView<float> JointsNullOutputInstance::getOutputBuffer() {
    return {};
}

void JointsNullOutputInstance::resetOutputBuffer() {
}

}  // namespace rl4
