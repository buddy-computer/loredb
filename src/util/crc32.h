/// \file crc32.h
/// \brief CRC32 checksum calculation utilities.
/// \author wiki-graph contributors
/// \ingroup util
#pragma once

#include <cstdint>
#include <span>

namespace graphdb::util {

/**
 * @class CRC32
 * @brief Static utility for CRC32 checksum calculation (single-shot and streaming).
 */
class CRC32 {
public:
    /**
     * @brief Calculate CRC32 checksum for a data buffer.
     * @param data Input data buffer.
     * @return CRC32 checksum.
     */
    static uint32_t calculate(std::span<const uint8_t> data);
    /**
     * @brief Calculate CRC32 checksum with an initial CRC value.
     * @param data Input data buffer.
     * @param initial_crc Initial CRC value.
     * @return CRC32 checksum.
     */
    static uint32_t calculate(std::span<const uint8_t> data, uint32_t initial_crc);
    
    // For streaming CRC calculation
    /**
     * @brief Update CRC32 checksum with additional data (for streaming).
     * @param crc Current CRC value.
     * @param data Input data buffer.
     * @return Updated CRC value.
     */
    static uint32_t update(uint32_t crc, std::span<const uint8_t> data);
    /**
     * @brief Finalize CRC32 checksum calculation.
     * @param crc Current CRC value.
     * @return Final CRC32 checksum.
     */
    static uint32_t finalize(uint32_t crc);
    
private:
    static const uint32_t CRC_TABLE[256];
    static void initialize_table();
};

}  // namespace graphdb::util