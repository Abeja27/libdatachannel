/**
 * Copyright (c) 2024 Shigemasa Watanabe (Wandbox)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RTC_DEPENDENCY_DESCRIPTOR_H
#define RTC_DEPENDENCY_DESCRIPTOR_H

#include <bitset>
#include <cassert>
#include <optional>
#include <vector>

namespace rtc {

struct BitWriter {
	static BitWriter fromSizeBits(std::byte *buf, size_t offsetBits, size_t sizeBits);
	static BitWriter fromNull();

	size_t getWrittenBits() const;

	bool write(uint64_t v, size_t bits);
	// Write non-symmetric unsigned encoded integer
	// ref: https://aomediacodec.github.io/av1-rtp-spec/#a82-syntax
	bool writeNonSymmetric(uint64_t v, uint64_t n);

private:
	size_t writePartialByte(uint8_t *p, size_t offset, uint64_t v, size_t bits);

private:
	std::byte *mBuf = nullptr;
	size_t mInitialOffset = 0;
	size_t mOffset = 0;
	size_t mSize = 0;
};

enum class DecodeTargetIndication {
	NotPresent = 0,
	Discardable = 1,
	Switch = 2,
	Required = 3,
};

struct RenderResolution {
	int width = 0;
	int height = 0;
};

struct FrameDependencyTemplate {
	int spatialId = 0;
	int temporalId = 0;
	std::vector<DecodeTargetIndication> decodeTargetIndications;
	std::vector<int> frameDiffs;
	std::vector<int> chainDiffs;
};

struct FrameDependencyStructure {
	int templateIdOffset = 0;
	int decodeTargetCount = 0;
	int chainCount = 0;
	std::vector<int> decodeTargetProtectedBy;
	std::vector<RenderResolution> resolutions;
	std::vector<FrameDependencyTemplate> templates;
};

struct DependencyDescriptor {
	bool startOfFrame = true;
	bool endOfFrame = true;
	int frameNumber = 0;
	FrameDependencyTemplate dependencyTemplate;
	std::optional<RenderResolution> resolution;
	std::optional<uint32_t> activeDecodeTargetsBitmask;
	bool structureAttached;
};

// Write dependency descriptor to RTP Header Extension
// Dependency descriptor specification is here:
// https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension
class DependencyDescriptorWriter {
public:
	DependencyDescriptorWriter(const FrameDependencyStructure &structure,
	                           std::bitset<32> activeChains,
	                           const DependencyDescriptor &descriptor);
	size_t getSizeBits() const;
	void writeTo(std::byte *buf, size_t sizeBits) const;

private:
	void doWriteTo(BitWriter &writer) const;
	void writeBits(BitWriter &writer, uint64_t v, size_t bits) const;
	void writeNonSymmetric(BitWriter &writer, uint64_t v, uint64_t n) const;

private:
	const FrameDependencyStructure &mStructure;
	std::bitset<32> mActiveChains;
	const DependencyDescriptor &mDescriptor;
};

} // namespace rtc

#endif
