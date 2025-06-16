#include <pronto/raster/random_raster_parameters.h>
#include <pronto/raster/random_block_generator.h>
#include <cpl_error.h>
#include <ctime>

namespace pronto {
namespace raster {

bool random_raster_parameters::from_json(const nlohmann::json& j) {
    try {
        m_type = j.at("type").get<std::string>();

        // Mandatory fields
        j.at("width").get_to(m_width);
        j.at("height").get_to(m_height);
        j.at("bits").get_to(m_bits);

        std::string data_type_str;
        j.at("data_type").get_to(data_type_str);
        m_data_type = string_to_gdal_data_type(data_type_str);

        // Optional parameters with default values
        if (j.contains("seed") && j["seed"].is_number_integer()) {
            j.at("seed").get_to(m_seed);
        }
        if (j.contains("block_x_size") && j["block_x_size"].is_number_integer()) {
            j.at("block_x_size").get_to(m_block_x_size);
        }
        if (j.contains("block_y_size") && j["block_y_size"].is_number_integer()) {
            j.at("block_y_size").get_to(m_block_y_size);
        }
        
        // Perform internal validation.
        return validate();

    } catch (const nlohmann::json::exception& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "Error parsing JSON parameters: %s. Check for required fields: 'width', 'height', 'bits', 'data_type'.", e.what());
        return false;
    } catch (const std::exception& e) {
        CPLError(CE_Failure, CPLE_AppDefined, "Error processing JSON parameters: %s", e.what());
        return false;
    }
}

nlohmann::json random_raster_parameters::to_json() const {
    nlohmann::json j;
    j["type"] = m_type;
    j["width"] = m_width;
    j["height"] = m_height;
    j["bits"] = m_bits;
    j["data_type"] = gdal_data_type_to_string(m_data_type);
    j["seed"] = m_seed;
    j["block_x_size"] = m_block_x_size;
    j["block_y_size"] = m_block_y_size;
   
    return j;
}

bool random_raster_parameters::validate() const {
    if (m_type != "RANDOM_RASTER") {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid 'type' parameter. Expected 'RANDOM_RASTER', got '%s'.", m_type.c_str());
        return false;
    }
    if (m_width <= 0 || m_height <= 0) {
        CPLError(CE_Failure, CPLE_AppDefined, "Random raster dimensions must be positive (WxH provided: %dx%d).", m_width, m_height);
        return false;
    }
    if (m_data_type == GDT_Unknown) {
        CPLError(CE_Failure, CPLE_AppDefined, "Data type must be specified and valid.");
        return false;
    }
    if (m_bits <= 0 || m_bits > GDALGetDataTypeSizeBits(m_data_type)) {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid number of bits (%d) for data type %s (max: %d).", m_bits, GDALGetDataTypeName(m_data_type), GDALGetDataTypeSizeBits(m_data_type));
        return false;
    }
    if (m_block_x_size <= 0 || m_block_y_size <= 0) {
        CPLError(CE_Failure, CPLE_AppDefined, "Block sizes must be positive (BlockXSize=%d, BlockYSize=%d).", m_block_x_size, m_block_y_size);
        return false;
    }
    return true;
}

bool random_raster_parameters::from_string_json(const std::string& json_string)
{
    *this = random_raster_parameters(); // Reset all members to defaults

    try {
        nlohmann::json j = nlohmann::json::parse(json_string);

        // Check if it's the correct type of JSON for our driver.
        if (!is_random_raster_json(j)) {
            return false; // Quiet failure for incorrect type
        }

        // Proceed with full population and validation.
        return from_json(j);
    } catch (const nlohmann::json::exception&) {
        return false; // Quiet failure for malformed JSON
    }
}

std::unique_ptr<block_generator_interface> random_raster_parameters::create_block_generator() const
{
    switch (m_data_type) {
        case GDT_Byte:
            return std::make_unique<random_block_generator<std::uniform_int_distribution<short>>>(
                m_seed, m_width, m_block_x_size, std::uniform_int_distribution<short>(0, (1 << m_bits) - 1));
        case GDT_UInt16:
            return std::make_unique<random_block_generator<std::uniform_int_distribution<unsigned short>>>(
                m_seed, m_width, m_block_x_size, std::uniform_int_distribution<unsigned short>(0, (1 << m_bits) - 1));
        case GDT_Int16:
            return std::make_unique<random_block_generator<std::uniform_int_distribution<short>>>(
                m_seed, m_width, m_block_x_size, std::uniform_int_distribution<short>(-(1 << (m_bits - 1)), (1 << (m_bits - 1)) - 1));
        case GDT_UInt32:
            return std::make_unique<random_block_generator<std::uniform_int_distribution<unsigned int>>>(
                m_seed, m_width, m_block_x_size, std::uniform_int_distribution<unsigned int>(0, (1 << m_bits) - 1));
        case GDT_Int32:
            return std::make_unique<random_block_generator<std::uniform_int_distribution<int>>>(
                m_seed, m_width, m_block_x_size, std::uniform_int_distribution<int>(-(1 << (m_bits - 1)), (1 << (m_bits - 1)) - 1));
        case GDT_Float32:
            return std::make_unique<random_block_generator<std::uniform_real_distribution<float>>>(
                m_seed, m_width, m_block_x_size, std::uniform_real_distribution<float>(0.0f, 1.0f));
        case GDT_Float64:
            return std::make_unique<random_block_generator<std::uniform_real_distribution<double>>>(
                m_seed, m_width, m_block_x_size, std::uniform_real_distribution<double>(0.0, 1.0));
        default:
            CPLError(CE_Failure, CPLE_AppDefined, "Unsupported data type for block generator creation: %s.", GDALGetDataTypeName(m_data_type));
            return nullptr;
    }
}

} // namespace raster
} // namespace pronto