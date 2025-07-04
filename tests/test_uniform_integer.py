import json
import numpy as np
import pytest
from osgeo import gdal

@pytest.fixture
def uniform_integer_json():
    """Provides a standard JSON configuration for a uniform integer raster."""
    return {
        "type": "RANDOM_RASTER",
        "rows": 64,
        "cols": 64,
        "data_type": "Int16",
        "seed": 123,
        "distribution": "uniform_integer",
        "distribution_parameters": {
            "a": -100,
            "b": 100
        }
    }

def test_uniform_integer_virtual_file(uniform_integer_json):
    """Verify uniform integer distribution with a virtual file."""
    json_str = json.dumps(uniform_integer_json)
    vsi_filename = "/vsimem/uniform_integer_params.json"
    
    # Create an in-memory file from the JSON string
    gdal.FileFromMemBuffer(vsi_filename, json_str.encode('utf-8'))

    ds = gdal.Open(vsi_filename)
    assert ds is not None, "GDAL could not open the dataset from the virtual file."

    band = ds.GetRasterBand(1)
    assert band is not None
    assert ds.RasterXSize == uniform_integer_json["cols"]
    assert ds.RasterYSize == uniform_integer_json["rows"]

    gdal_type = gdal.GetDataTypeByName(uniform_integer_json["data_type"])
    assert band.DataType == gdal_type

    # Verify that the pixel values are within the specified range [a, b]
    data = band.ReadAsArray()
    params = uniform_integer_json["distribution_parameters"]
    assert np.all(data >= params["a"])
    assert np.all(data <= params["b"])

    # Print a 4x6 subraster for visual inspection
    sub_raster = data[0:4, 0:6]
    print("\nSample 4x6 subraster (virtual file):")
    print(sub_raster)

    # Re-open and check for reproducibility
    ds2 = gdal.Open(vsi_filename)
    data2 = ds2.GetRasterBand(1).ReadAsArray()
    assert np.array_equal(data, data2), "Data should be reproducible with the same seed."

    # Clean up the in-memory file
    gdal.Unlink(vsi_filename)

def test_uniform_integer_physical_file(uniform_integer_json, tmp_path):
    """Verify uniform integer distribution with a physical file."""
    json_path = tmp_path / "uniform_integer_params.json"
    with open(json_path, "w") as f:
        json.dump(uniform_integer_json, f)

    ds = gdal.Open(str(json_path))
    assert ds is not None, "GDAL could not open the dataset from the physical file."

    band = ds.GetRasterBand(1)
    assert band is not None
    assert ds.RasterXSize == uniform_integer_json["cols"]
    assert ds.RasterYSize == uniform_integer_json["rows"]

    gdal_type = gdal.GetDataTypeByName(uniform_integer_json["data_type"])
    assert band.DataType == gdal_type

    # Verify that the pixel values are within the specified range [a, b]
    data = band.ReadAsArray()
    params = uniform_integer_json["distribution_parameters"]
    assert np.all(data >= params["a"])
    assert np.all(data <= params["b"])

    # Print a 4x6 subraster for visual inspection
    sub_raster = data[0:4, 0:6]
    print("\nSample 4x6 subraster (physical file):")
    print(sub_raster)

    # Re-open and check for reproducibility
    ds2 = gdal.Open(str(json_path))
    data2 = ds2.GetRasterBand(1).ReadAsArray()
    assert np.array_equal(data, data2), "Data should be reproducible with the same seed."

def test_uniform_integer_invalid_a_greater_b(uniform_integer_json):
    """Verify that an invalid range (a > b) is handled correctly."""
    invalid_json_config = uniform_integer_json.copy()
    invalid_json_config["distribution_parameters"] = {"a": 100, "b": -100}

    json_str = json.dumps(invalid_json_config)
    vsi_filename = "/vsimem/invalid_uniform_integer.json"
    
    gdal.FileFromMemBuffer(vsi_filename, json_str.encode('utf-8'))
    
    # Clear any previous errors
    gdal.ErrorReset()
    
    # Suppress error messages from appearing in the console during the test
    with gdal.quiet_errors():
        ds = gdal.Open(vsi_filename)

    assert ds is None, "Dataset should not be created with invalid parameters (a > b)."
    
    error_msg = gdal.GetLastErrorMsg()
    assert "Invalid range" in error_msg or "'a' must not be greater than 'b'" in error_msg, \
        f"Expected an error message about an invalid range, but got: {error_msg}"

    gdal.Unlink(vsi_filename)

def test_uniform_integer_invalid_out_of_range(uniform_integer_json):
    """Verify that a value out of range is handled correctly: Unsigned int < 0"""
    invalid_json_config = uniform_integer_json.copy()
    invalid_json_config["data_type"] = "UInt16"
    invalid_json_config["distribution_parameters"] = {"a":"hello", "b": "world"}

    json_str = json.dumps(invalid_json_config)
    vsi_filename = "/vsimem/invalid_uniform_integer.json"
    
    gdal.FileFromMemBuffer(vsi_filename, json_str.encode('utf-8'))
    
    # Clear any previous errors
    gdal.ErrorReset()
    
    # Suppress error messages from appearing in the console during the test
    with gdal.quiet_errors():
        ds = gdal.Open(vsi_filename)

    assert ds is None, "Dataset should not be created with invalid type (string instead of int)."
    print(gdal.GetLastErrorMsg())
    gdal.Unlink(vsi_filename)