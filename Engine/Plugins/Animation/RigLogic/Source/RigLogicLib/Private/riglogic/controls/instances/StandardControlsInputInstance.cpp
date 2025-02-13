// Copyright Epic Games, Inc. All Rights Reserved.

#include "riglogic/controls/instances/StandardControlsInputInstance.h"

#include <cstdint>
#include <cstddef>

namespace rl4 {

StandardControlsInputInstance::StandardControlsInputInstance(std::uint16_t guiControlCount_,
                                                             std::uint16_t rawControlCount_,
                                                             std::uint16_t psdControlCount_,
                                                             std::uint16_t mlControlCount_,
                                                             std::uint16_t rbfControlCount_,
                                                             MemoryResource* memRes) :
    guiControlBuffer{guiControlCount_, {}, memRes},
    inputBuffer{static_cast<std::size_t>(rawControlCount_ + psdControlCount_ + mlControlCount_ + rbfControlCount_), {}, memRes},
    guiControlCount{guiControlCount_},
    rawControlCount{rawControlCount_},
    psdControlCount{psdControlCount_},
    mlControlCount{mlControlCount_},
    rbfControlCount{rbfControlCount_} {
}

ArrayView<float> StandardControlsInputInstance::getGUIControlBuffer() {
    return ArrayView<float>{guiControlBuffer};
}

ArrayView<float> StandardControlsInputInstance::getInputBuffer() {
    return ArrayView<float>{inputBuffer};
}

ConstArrayView<float> StandardControlsInputInstance::getGUIControlBuffer() const {
    return ConstArrayView<float>{guiControlBuffer};
}

ConstArrayView<float> StandardControlsInputInstance::getInputBuffer() const {
    return ConstArrayView<float>{inputBuffer};
}

std::uint16_t StandardControlsInputInstance::getGUIControlCount() const {
    return guiControlCount;
}

std::uint16_t StandardControlsInputInstance::getRawControlCount() const {
    return rawControlCount;
}

std::uint16_t StandardControlsInputInstance::getPSDControlCount() const {
    return psdControlCount;
}

std::uint16_t StandardControlsInputInstance::getMLControlCount() const {
    return mlControlCount;
}

std::uint16_t StandardControlsInputInstance::getRBFControlCount() const {
    return rbfControlCount;
}

}  // namespace rl4
