#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

template <std::uint32_t capacity = 1024>
class DataBuffer
{
private:
    struct DataChunk
    {
        std::uint32_t m_id             = { 0 };
        std::uint32_t m_count          = { 0 };
        std::uint8_t  m_data[capacity] = { 0 };
    };

    fs::path      m_name;
    std::fstream  m_stream;
    std::uint32_t m_size;
    std::uint32_t m_total_chunks;
    DataChunk     m_chunks[2];
    std::uint8_t  m_front_id;
    std::uint8_t  m_back_id;

public:
    DataBuffer() : m_size(0),
                   m_total_chunks(0),
                   m_front_id(1),
                   m_back_id(0)
    {
    }

    const fs::path& name() const { return m_name; }
    std::uint32_t   size() const { return m_size; }

    bool open_file(const fs::path& file_name)
    {
        if (!fs::exists(file_name))
            return false;

        m_name = fs::canonical(file_name);
        m_stream.open(m_name.string(), std::ios::in | std::ios::binary);
        if (!m_stream)
            return false;

        m_size         = fs::file_size(m_name);
        m_total_chunks = m_size / capacity;
        if (m_size % capacity)
            m_total_chunks++;

        // TODO<marios>: maybe make this public so that we can
        // support the variable begining offset
        load_chunk(0);

        return true;
    }

    // No bounds checking is performed by the operator
    std::uint8_t operator[](std::uint32_t byte_id)
    {
        std::uint32_t chunk_id    = byte_id / capacity;
        std::uint32_t relative_id = byte_id - capacity * chunk_id;

        if (m_chunks[m_front_id].m_id == chunk_id)
            return m_chunks[m_front_id].m_data[relative_id];
        else if (m_chunks[m_back_id].m_id == chunk_id)
            return m_chunks[m_back_id].m_data[relative_id];

        // chunk miss, load from disk...
        load_chunk(chunk_id);

        return m_chunks[m_front_id].m_data[relative_id];
    }

    bool load_chunk(std::uint32_t chunk_id)
    {
        m_stream.seekg(chunk_id * capacity);
        if (!m_stream)
            return false;

        std::uint32_t bytes_to_read = capacity;
        if (chunk_id == m_total_chunks - 1)
            bytes_to_read = m_size % capacity;

        auto& target_cache = m_chunks[m_back_id];
        m_stream.read(reinterpret_cast<char*>(target_cache.m_data), bytes_to_read);

        target_cache.m_id    = chunk_id;
        target_cache.m_count = bytes_to_read;
        std::swap(m_back_id, m_front_id);

        return true;
    }
};

#endif // DATA_BUFFER_H
