//
// RepField.inl
//
// Template implementations for Rep<T, S>::Write / ::Read.
// Separated from RepField.h because these need the full
// BitWriter/BitReader definitions.

#ifndef VOXTAU_REPFIELD_INL
#define VOXTAU_REPFIELD_INL
#include "Core/Network/Protocol/BitStream.h"

// RepSerializer<T> per-type serialization strategy
//
// The default uses BitWriter::Write / BitReader::Read directly
// (raw, full precision). Specialize this for types that benefit
// from compressed encoding.
//
// One specialization covers ALL scopes  no need to repeat
// for Scope::All, Scope::Owner, Scope::Server separately.

template<typename T>
struct RepSerializer {
    static void Write(BitWriter& writer, const T& value) {
        writer.Write(value);
    }
    static void Read(BitReader& reader, T& value) {
        reader.Read(value);
    }
};


/**
 *  Quaternion smallest-three compression
 *
 *  16 bytes (4 floats) -> 7 bytes (index + 3x uint16).
 *  Precision: ±0.00002 per component (~0.001° angular error).
 *  More than sufficient for character rotation replication.
 */
template<>
struct RepSerializer<Math::Quaternion> {
    static void Write(BitWriter& writer, const Math::Quaternion& value) {
        writer.WriteCompressedQuat(value);
    }
    static void Read(BitReader& reader, Math::Quaternion& value) {
        reader.ReadCompressedQuat(value);
    }
};

/**
 * Rep<T, S> default implementations
 * Route through RepSerializer<T> so specializations
 * are picked up automatically.
 * @param writer
 */
template<typename T, Scope S>
void Rep<T, S>::Write(BitWriter& writer) const {
    RepSerializer<T>::Write(writer, _value);
}

/**
 * Rep<T, S> default implementations
 * @param reader
 */
template<typename T, Scope S>
void Rep<T, S>::Read(BitReader& reader) {
    RepSerializer<T>::Read(reader, _value);
}

#endif // VOXTAU_REPFIELD_INL