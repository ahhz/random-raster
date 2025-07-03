import numpy as np
from osgeo import gdal

def test_driver_is_present():
    """Check if the RANDOM_RASTER driver is available in GDAL."""
    driver = gdal.GetDriverByName("RANDOM_RASTER")
    assert driver is not None, "RANDOM_RASTER driver was not found by GDAL."

def test_driver_long_name():
    """Verify the long name metadata of the driver."""
    driver = gdal.GetDriverByName("RANDOM_RASTER")
    assert driver is not None
    long_name = driver.GetMetadataItem("DMD_LONGNAME")
    assert long_name == "Random Distribution Raster"