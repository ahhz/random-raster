#include <pronto/raster/random_dataset.h>
#include <pronto/raster/random_raster_band.h>
#include <pronto/raster/block_generator_interface.h> 

#define PRONTO_RASTER_MAX_JSON_FILE_SIZE (10 * 1024 * 1024) // 10 MB limit

namespace pronto {
namespace raster {

// --- random_raster_band Implementation ---

// Constructor.
// Private constructor.
random_dataset::random_dataset(int raster_x_size, int raster_y_size, GDALDataType data_type,
    int block_x_size, int block_y_size, std::unique_ptr<block_generator_interface>&& block_generator)
    : m_block_generator(std::move(block_generator))
{
    nRasterXSize = raster_x_size;
    nRasterYSize = raster_y_size;
    SetBand(1, new random_raster_band(this, 1, m_block_generator.get(),
        data_type, block_x_size, block_y_size));
}

// Creates a random_dataset instance.
random_dataset* random_dataset::create(const random_raster_parameters& params)
{
    // Assumes 'params' is already validated.
    std::unique_ptr<block_generator_interface> generator = params.create_block_generator();

    if (generator == nullptr) {
        return nullptr;
    }

    return new random_dataset(params.m_width, params.m_height, params.m_data_type,
        params.m_block_x_size, params.m_block_y_size, std::move(generator));
}


// GDAL driver entry point for opening datasets.
GDALDataset* random_dataset::Open(GDALOpenInfo* openInfo)
{
    std::string content_to_parse;

    // 1. Check if the input is a file path and if it matches our criteria
    VSIStatBufL sStatBuf;
    if (VSIStatL(openInfo->pszFilename, &sStatBuf) == 0 && VSI_ISREG(sStatBuf.st_mode)) {
        // It's a regular file. Check extension and size.
        std::string filename_str = openInfo->pszFilename;
        if (filename_str.length() < 5 || !EQUAL(filename_str.c_str() + filename_str.length() - 5, ".json"))
        {
            return nullptr; // Not a .json file.
        }

        if (sStatBuf.st_size > PRONTO_RASTER_MAX_JSON_FILE_SIZE) {
            CPLError(CE_Debug, CPLE_AppDefined, "JSON parameter file too large (%lld bytes): %s",
                static_cast<long long>(sStatBuf.st_size), openInfo->pszFilename);
            return nullptr; // File too large.
        }

        // Read file content.
        VSILFILE* fp = VSIFOpenL(openInfo->pszFilename, "rb");
        if (fp == nullptr) {
            return nullptr; // File can't be opened.
        }

        content_to_parse.resize(sStatBuf.st_size);
        size_t bytes_read = VSIFReadL(&content_to_parse[0], 1, sStatBuf.st_size, fp);
        VSIFCloseL(fp);

        if (bytes_read != (size_t)sStatBuf.st_size) {
            CPLError(CE_Debug, CPLE_AppDefined, "Incomplete read of JSON parameter file: %s", openInfo->pszFilename);
            return nullptr; // Incomplete read.
        }
    }
    else {
        // Not a regular file, assume the filename string itself is the JSON content.
        content_to_parse = openInfo->pszFilename;
    }

    // 2. Attempt to parse the content as JSON.
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(content_to_parse);
    }
    catch (const nlohmann::json::exception&) {
        return nullptr; // Malformed JSON.
    }

    // 3. Check if the JSON is specifically for RANDOM_RASTER.
    if (!random_raster_parameters::is_random_raster_json(j)) {
        return nullptr; // Valid JSON, but not our 'type'.
    }

    // 4. Populate parameters and perform full validation.
    random_raster_parameters params;
    if (!params.from_json(j)) {
        CPLError(CE_Debug, CPLE_AppDefined, "JSON for RANDOM_RASTER but content validation failed. Message: %s", CPLGetLastErrorMsg());
        CPLErrorReset();
        return nullptr;
    }

    return random_dataset::create(params);
}

} // namespace raster 
} // namespace pronto