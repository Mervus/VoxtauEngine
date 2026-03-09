#ifndef VOXTAU_REPFIELD_INL
#define VOXTAU_REPFIELD_INL
#pragma once

#include "Core/Network/Protocol/BitStream.h"

template<typename T, Scope S>
void Rep<T, S>::Write(BitWriter& writer) const {
    writer.Write(_value);
}

template<typename T, Scope S>
void Rep<T, S>::Read(BitReader& reader) {
    reader.Read(_value);
}

#endif // VOXTAU_REPFIELD_INL